#include "conf.h"
#include "config.h"
#include "nastipc.h"
#include "thread.h"

#include "BuffIO.h"
#include "ArrayData.h"
#include "DictData.h"
#include "NumData.h"
#include "StringData.h"

#include <netdb.h>
#include <stdlib.h>

RCSID("$Id: config.m,v 1.5 2001/11/09 15:54:38 shmit Exp $");

/*
 * Global variables.
 */
static DictData *root;
char config_filen[MAXPATHLEN];
struct nast_config config;

static int
config_read(const char *filename)
{
	BuffIO *confFile;

	confFile = [[[BuffIO new] init: filename] setEOL: ';'];
	if (confFile == nil) {
		fprintf(stderr, "ERROR: Couldn't load config file.");
		return -1;
	}

	root = [[DictData new] setFromBuffer: [confFile getCurOff]
			       withLength: [confFile getLength]];

	/* We've duplicated all the data in here into private storage. */
	[confFile free];

	if (root == nil) {
		fprintf(stderr, "ERROR: Couldn't parse config file.");
		return -1;
	}
	return 0;
}

static int
get_val(const char *name, void *dst, enum data_type type)
{
	id node;

	node = [[root findChild: name] getData];
	if (node == nil)
		return 0;

	switch (type) {
	case NUMBER:
		if (![node isKindOf: [NumData class]]) {
			fprintf(stderr, "Node `%s' in `%s' should be a number.",
				name, config_filen);
			return -1;
		}
		*(int *)dst = [node getNum];
		break;
	case STRING:
		if (![node isKindOf: [StringData class]]) {
			fprintf(stderr, "Node `%s' in `%s' should be a string.",
				name, config_filen);
			return -1;
		}
		*(char **)dst = [node createCStr];
		break;
	case ARRAY:
		fprintf(stderr, "ERROR: Can't load array into C yet.");
		return -1;
	case DICTIONARY:
		fprintf(stderr, "ERROR: Can't load dictionary into C yet.");
		return -1;
	default:
		fprintf(stderr, "ERROR: Don't know what to do with type `%d'.",
			type);
		return -1;
	}

	return 0;
}

/*
 * Sets configuration variables to defaults, in case they're not set
 * elsewhere.
 */
void
config_setdefaults()
{
	/* Set the defaults. */
	config.description = "Description not set in config file.";
	config.nast_dir = DATADIR;
	config.nast_sock = NASTHOLE;
	config.nast_cdb_file = "nast.cdb";
	config.mysql_host = DBHOST;
	config.mysql_user = DBUSER;
	config.mysql_pass = DBPASS;
	config.nofork_flag = 0;
	config.null_cache_timeout = 60;
	config.tcp_port = 31337;

	return;
}

/*
 * Initializes configuration variables from config file.
 */
int
config_init()
{
	snprintf(config_filen, sizeof(config_filen),
		 "%s/nastd.conf", config.nast_dir);
	if (config_read(config_filen) == -1)
		return -1;

	/*
	 * Now grab all the top-level config information.
	 */
	if (get_val("description", &config.description, STRING) == -1)
		return -1;

	if (get_val("nast_dir", &config.nast_dir, STRING) == -1)
		return -1;

	if (get_val("nast_sock", &config.nast_sock, STRING) == -1)
		return -1;

	if (get_val("nast_cdb_file", &config.nast_cdb_file, STRING) == -1)
		return -1;

	if (get_val("mysql_host", &config.mysql_host, STRING) == -1)
		return -1;

	if (get_val("mysql_user", &config.mysql_user, STRING) == -1)
		return -1;

	if (get_val("mysql_pass", &config.mysql_pass, STRING) == -1)
		return -1;

	if (get_val("null_cache_timeout", &config.null_cache_timeout,
		    NUMBER) == -1)
		return -1;

	if (get_val("tcp_port", &config.tcp_port, NUMBER) == -1)
		return -1;

	return 0;
}

void
config_delete()
{
	[root free];
}

static enum data_type
getctype(id node)
{
	if ([node isKindOf: [StringData class]])
		return STRING;
	else if ([node isKindOf: [NumData class]])
		return NUMBER;
	else if ([node isKindOf: [ArrayData class]])
		return ARRAY;
	else if ([node isKindOf: [DictData class]])
		return DICTIONARY;
	else {
		return -1;
	}
}

void *
config_find(void *root_node, const char *name, enum data_type *type)
{
	id rootNode, dstNode;

	if (root_node == NULL)
		root_node = root;

	rootNode = (id)root_node;
	if ([rootNode isKindOf: [DictData class]] == NO) {
		fprintf(stderr, "Trying to find `%s' in non-dictionary.", name);
		return NULL;
	}

	dstNode = [[rootNode findChild: name] getData];
	if (dstNode == nil)
		return NULL;
	*type = getctype(dstNode);
	return dstNode;
}

void *
config_arrayitemat(void *array, int index, enum data_type *type)
{
	id arrayNode, dstNode;

	if (array == NULL)
		return NULL;

	arrayNode = (id)array;
	if ([arrayNode isKindOf: [ArrayData class]] == NO) {
		fprintf(stderr, "Trying to index in non-array.");
		return NULL;
	}

	if (index >= [arrayNode numObjects])
		return NULL;

	dstNode = [arrayNode objectAt: index];
	if (dstNode == nil)
		return NULL;
	*type = getctype(dstNode);
	return dstNode;
}

void *
config_dictitemat(void *dict, const char *name, enum data_type *type)
{
	id dictNode, dstNode;

	if (dict == NULL)
		return NULL;

	dictNode = (id)dict;
	if ([dictNode isKindOf: [DictData class]] == NO) {
		fprintf(stderr, "Trying to find `%s' in non-dictionary.", name);
		return NULL;
	}

	dstNode = [[dictNode findChild: name] getData];
	if (dstNode == nil)
		return NULL;
	*type = getctype(dstNode);
	return dstNode;
}

void *
getdata(void *itemref, void *dst, enum data_type want_type)
{
	id item;

	if (itemref == NULL)
		return NULL;

	item = (id)itemref;
	if ([item isKindOf: [DataObject class]] == NO) {
		fprintf(stderr, "Can only deref DataObject types "
				"or sub-classes.");
		return NULL;
	}

	switch (want_type) {
	case NUMBER:
		if ([item isKindOf: [NumData class]] == NO)
			return NULL;
		*(int *)dst = [item getNum];
		break;
	case STRING:
		if ([item isKindOf: [StringData class]] == NO)
			return NULL;
		*(char **)dst = [item createCStr];
		break;
	case ARRAY:
		if ([item isKindOf: [ArrayData class]] == NO)
			return NULL;
		dst = [item getData];
		break;
	case DICTIONARY:
		if ([item isKindOf: [DictData class]] == NO)
			return NULL;
		dst = [item getData];
		break;
	default:
		fprintf(stderr,
			"Can't deref type `%d': unknown type.", want_type);
		return NULL;
	}

	return dst;
}
