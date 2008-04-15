#include "conf.h"
#include "config.h"
#include "array.h"
#include "nastdio.h"
#include "cdbpriv.h"
#include "log.h"
#include "memdb.h"
#include "thread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

RCSID("$Id: cdb.c,v 1.64 2001/10/19 22:28:07 shmit Exp $");

time_t cdb_mtime;

static char *memcdb = NULL;
static rw_mutex_t *cdb_lk = NULL;
static uint32_t cdb_len = 0;

/*
 * Exportable symbols - the CDB file is authoritative for them, so they
 * come from here, but nastdio.c is their consumer.
 */
char db_key[1024];
char db_dbn[1024];
char db_tbl[1024];
fieldent *db_fields = NULL;
char db_sep;
short db_fieldcount = 0;

static int sec_c;
static float onemin_c, fivemin_c, fifteenmin_c;

static int
cdb_findbykey(char *mcdb, int len, const char *key, int keylen,
	      char **data, int *dlen)
{
	if (cdb_find(mcdb, len, key, keylen, data, dlen) != 1)
		return -1;

	return 0;
}

int
cdb_new()
{
	char *newcdb;
	char *p, *s_p, *e_p;
	fieldent *newfields;
	char buffer[1024];
	char cdbfields_s[1024], newkey[1024], newtbl[1024], newdbn[1024];
	char newsep;
	struct stat sb;
	int i, fieldslen, newfieldcount;
	int fd;
	size_t len;

	log_info("Initialising CDB interface.");

	sec_c = 0;
	onemin_c = fivemin_c = fifteenmin_c = 0;

	if (!cdb_lk) {
		cdb_lk = malloc(sizeof(rw_mutex_t));
		if (!cdb_lk) {
			log_err("Couldn't allocate CDB mutex: %s.",
				strerror(errno));
			return -1;
		}
		if (rw_mutex_new(cdb_lk)) {
			log_err("Couldn't initialse CDB mutex: %s.",
				strerror(errno));
			return -1;
		}
	}

	snprintf(buffer, sizeof(buffer), "%s/%s",
		 config.nast_dir, config.nast_cdb_file);
	fd = open(buffer, O_RDONLY);
	if (fd == -1) {
		log_err("Couldn't open %s: %s.", buffer, strerror(errno));
		return -1;
	}

	if (fstat(fd, &sb) == -1) {
		log_err("Couldn't stat %s: %s.", buffer, strerror(errno));
		return -1;
	}
	len = sb.st_size;

	newcdb = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if (newcdb == MAP_FAILED) {
		log_err("Couldn't mmap %s: %s.", buffer, strerror(errno));
		return -1;
	}

	(void)close(fd);

	/* Get the key out of the CDB, so clients know how to search. */
	if (cdb_findbykey(newcdb, len, "_KEY_", strlen("_KEY_"),
			  &p, &i) == -1) {
		log_err("Couldn't find `_KEY_' in the CDB.");
		munmap(newcdb, len);
		return -1;
	}

	if (i > sizeof(newkey)) {
		log_err("Couldn't copy key information from the CDB.");
		munmap(newcdb, len);
		return -1;
	}
	memcpy(newkey, p, i);
	newkey[i] = '\0';

	/* Get the dbname out of the CDB, so mysql knows which db to use. */
	if (cdb_findbykey(newcdb, len, "_DB_", strlen("_DB_"), &p, &i) == -1) {
		log_err("Warning: Couldn't find `_DB_' in the CDB.");
		strncpy(newdbn, DBNAME, sizeof(newdbn));
	} else {
		if (i > sizeof(newdbn)) {
			log_err("Couldn't copy database information "
				"from the CDB.");
			munmap(newcdb, len);
			return -1;
		}
		memcpy(newdbn, p, i);
		newdbn[i] = '\0';
	}

	/* Get the table out of the CDB, so mysql knows which to use. */
	if (cdb_findbykey(newcdb, len, "_TABLE_", strlen("_TABLE_"),
			  &p, &i) == -1) {
		log_err("Warning: Couldn't find `_TABLE_' in the CDB.");
		strncpy(newtbl, DBTBL, sizeof(newtbl));
	} else {
		if (i > sizeof(newtbl)) {
			log_err("Couldn't copy table information "
			        "from the CDB.");
			munmap(newcdb, len);
			return -1;
		}
		memcpy(newtbl, p, i);
		newtbl[i] = '\0';
	}

	/* Get the delimiter out of the CDB file. */
	if (cdb_findbykey(newcdb, len, "_DELIM_", strlen("_DELIM_"),
			  &p, &i) == -1) {
		log_info("Couldn't find `_DELIM_' in the CDB. Using default.");
		newsep = ':';
	} else
		newsep = *p;

	/* Now get the column names. */
	if (cdb_findbykey(newcdb, len, "_VALUES_", strlen("_VALUES_"),
			  &p, &i) == -1) {
		log_err("Couldn't find `_VALUES_' in the CDB.");
		munmap(newcdb, len);
		return -1;
	}
	if (i >= sizeof(cdbfields_s)) {
		log_err("Couldn't copy value information from the CDB.");
		munmap(newcdb, len);
		return -1;
	}
	memcpy(cdbfields_s, p, i);
	cdbfields_s[i] = '\0';
	fieldslen = strlen(cdbfields_s);

	/* Fill out cdbfields with an array of field names. */
	newfieldcount = 0;
	for (i = 0; i <= fieldslen; i++) {
		if (cdbfields_s[i] == newsep || i == fieldslen)
			newfieldcount++;
	}

	newfields = malloc(sizeof(fieldent) * newfieldcount);
	if (newfields == NULL) {
		log_err("Couldn't allocate space for field array: %s.",
			strerror(errno));
		munmap(newcdb, len);
		return -1;
	}

	s_p = cdbfields_s;
	for (i = 0; i < newfieldcount; i++) {
		for (e_p = s_p; *e_p != newsep && *e_p != '\0'; e_p++);

		newfields[i].index = i;
		newfields[i].name = malloc(e_p - s_p + 1*sizeof(char));
		if (newfields[i].name == NULL) {
			/* XXX: clean up. */
			log_err("Couldn't allocate space for field: %s.",
				strerror(errno));
			munmap(newcdb, len);
			return -1;
		}
		memcpy(newfields[i].name, s_p, e_p - s_p);
		newfields[i].name[e_p - s_p] = '\0';
		s_p = e_p + 1;
	}

	rw_mutex_write_lock(cdb_lk);
	if (memcdb)
		munmap(memcdb, cdb_len);
	memcdb = newcdb;
	cdb_len = (uint32_t)len;
	cdb_mtime = sb.st_mtime;

	memcpy(db_dbn, newdbn, sizeof(db_dbn));
	memcpy(db_tbl, newtbl, sizeof(db_tbl));
	memcpy(db_key, newkey, sizeof(db_key));
	if (db_fields)
		free(db_fields);
	db_fields = newfields;
	db_sep = newsep;
	db_fieldcount = newfieldcount;
	rw_mutex_unlock(cdb_lk);

	log_info("CDB interface initialised.");
	return 0;
}

int
cdb_get(const char *key, int keylen, array_t *aa)
{
	char *s_p, *e_p;
	char *data;
	int dlen;

	rw_mutex_read_lock(cdb_lk);
	if (cdb_findbykey(memcdb, cdb_len, key, keylen, &data, &dlen) == -1) {
		rw_mutex_unlock(cdb_lk);
		return -1;
	}

	sec_c++;

	s_p = data; e_p = data;
	while (e_p <= data+dlen) {
		if (*e_p == db_sep || e_p == data+dlen) {
			if (array_add(aa, e_p-s_p, s_p, ARRTERM) == -1) {
				rw_mutex_unlock(cdb_lk);
				return -1;
			}
			s_p = e_p + 1;
		}
		e_p++;
	}

	rw_mutex_unlock(cdb_lk);
	return 0;
}

void
cdb_periodic()
{
	char buffer[1024];
	struct stat sb;

	snprintf(buffer, sizeof(buffer), "%s/%s",
		 config.nast_dir, config.nast_cdb_file);
	if (stat(buffer, &sb) == -1) {
		log_err("PERIODIC: Couldn't stat %s: %s.\n", buffer,
			strerror(errno));
		return;
	}

	if (sb.st_size == 0) {
		log_err("PERIODIC: WARNING! CDB file is empty!");
		return;
	}

	if (cdb_mtime < sb.st_mtime) {
		log_info("PERIODIC: CDB file changed, reloading.\n");
		(void)cdb_new();

		/*
		 * Turned off, as now entries are time stamped and
		 * auto-expire when the cdb file is updated.
		 */
		/* (void)memdb_new(); */
		return;
	}
}

int
cdb_stats(array_t *statarr)
{
	char buffer[512];
	char tbuff[512];
	struct tm res;

	snprintf(buffer, sizeof(buffer), "CDB: %.2f, %.2f, %.2f",
		 onemin_c, fivemin_c, fifteenmin_c);
	if (array_add(statarr, strlen(buffer), buffer, ARRTERM) == -1)
		return -1;

	/* Convert CDB mtime into a string. */
	(void)localtime_r(&cdb_mtime, &res);
	strftime(tbuff, sizeof(tbuff), "%Y%m%d %H:%M:%S", &res);

	snprintf(buffer, sizeof(buffer), "CDB: last updated: %s",
		 tbuff);
	return array_add(statarr, strlen(buffer), buffer, ARRTERM);
}

void
cdb_collate()
{
	onemin_c = ((onemin_c * 59) + sec_c) / 60;
	fivemin_c = ((fivemin_c * 299) + sec_c) / 300;
	fifteenmin_c = ((fifteenmin_c * 899) + sec_c) / 900;
	sec_c = 0;
}
