/* $Id: config.h,v 1.3 2001/11/09 15:54:37 shmit Exp $ */

#ifndef CONFIG_H
#	define CONFIG_H

enum data_type { NUMBER, STRING, ARRAY, DICTIONARY };

struct conf_entry {
	char *name;
	void *data;
	int namelen, datalen;
	enum data_type type;
	int num_entries;
};
typedef struct conf_entry conf_entry_t;

struct nast_config {
	char *description;
	char *nast_dir;
	char *nast_sock;
	char *nast_cdb_file;
	char *mysql_host;
	char *mysql_user;
	char *mysql_pass;
	int nofork_flag;
	int null_cache_timeout;
	int tcp_port;
};

extern struct nast_config config;

int config_init();
void config_delete();
void config_setdefaults();
void *config_find(void *root_node, const char *name, enum data_type *type);
void *config_arrayitemat(void *array, int index, enum data_type *type);
void *config_dictitemat(void *dict, const char *name, enum data_type *type);
void *getdata(void *itemref, void *dst, enum data_type want_type);

#endif
