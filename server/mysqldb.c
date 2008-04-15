#include "conf.h"
#include "array.h"
#include "config.h"
#include "nastdio.h"
#include "log.h"
#include "mysqldb.h"
#include "thread.h"

#include <errno.h>
#include <errmsg.h>
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern fieldent *db_fields;
extern char db_key[1024];
extern char db_dbn[1024];
extern char db_tbl[1024];
extern short db_fieldcount;

static char cols[512];

static int sec_c;
static float onemin_c, fivemin_c, fifteenmin_c;

int
mysqldb_new()
{
	int i;

	sec_c = 0;
	onemin_c = fivemin_c = fifteenmin_c = 0;

	cols[0] = '\0';
	for (i = 0; i < db_fieldcount; i++) {
		strncat(cols, db_fields[i].name, sizeof(cols));
		if (i < db_fieldcount-1)
			strncat(cols, ",", sizeof(cols));
	}

	return 0;
}

void *
mysqldb_connect_new()
{
	MYSQL *dbh;

	log_info("Initialising MySQL database.");

	dbh = mysql_init(NULL);
	if (dbh == NULL) {
		log_err("Couldn't allocate mysql handle: %s.",
			strerror(errno));
		return NULL;
	}

	if (!mysql_real_connect(dbh, config.mysql_host,
                                config.mysql_user, config.mysql_pass,
                                db_dbn, 0, NULL, 0)) {
		log_err("Couldn't open connection to database: %s.",
			mysql_error(dbh));
		mysqldb_connect_close(dbh);
		return NULL;
	}

	log_info("MySQL database interface initialised.");
	return (void *)dbh;
}

void
mysqldb_connect_close(void *dbh)
{
	if (dbh != NULL) {
		log_info("MySQL connection shutting down.");
		mysql_close(dbh);
		free(dbh);
	}
}

int
mysqldb_get(reqthread_t *self, const char *key, int keylen, array_t *aa)
{
	MYSQL *dbh;
	MYSQL_RES *result;
	char buffer[1024];
	MYSQL_ROW row;
	int i, rc;

	snprintf(buffer, sizeof(buffer), DBSELECT, cols, db_tbl, db_key, key);

	if (self->arg == NULL) {
		self->arg = mysqldb_connect_new();
		if (self->arg == NULL)
			return -1;
	}
	dbh = (MYSQL *)self->arg;

	rc = mysql_query(dbh, buffer);
	if (rc) {
		log_err("Error performing query: %s.", mysql_error(dbh));
		mysqldb_connect_close(dbh);
		self->arg = NULL;
		return -1;
	}

	result = mysql_use_result(dbh);
	row = mysql_fetch_row(result);
	if (row == NULL) {
		log_info("Couldn't find %s in MySQL database.",
			 key);
		mysql_free_result(result);
		sec_c++;
		return 1;
	}

	if (mysql_num_fields(result) < db_fieldcount) {
		log_err("MySQL server didn't return all fields.");
		mysql_free_result(result);
		return 0;
	}

	for (i = 0; i < db_fieldcount; i++) {
		if (array_add(aa, strlen(row[i]), row[i], ARRTERM) == -1) {
			mysql_free_result(result);
			return -1;
		}
	}
	while (mysql_fetch_row(result));
	mysql_free_result(result);

	sec_c++;

	return 0;
}

int
mysqldb_stats(array_t *statarr)
{
	char buffer[512];

	snprintf(buffer, sizeof(buffer), "MySQL: %.2f, %.2f, %.2f",
		 onemin_c, fivemin_c, fifteenmin_c);
	return array_add(statarr, strlen(buffer), buffer, ARRTERM);
}

void
mysqldb_collate()
{
	onemin_c = ((onemin_c * 59) + sec_c) / 60;
	fivemin_c = ((fivemin_c * 299) + sec_c) / 300;
	fifteenmin_c = ((fifteenmin_c * 899) + sec_c) / 900;
	sec_c = 0;
}
