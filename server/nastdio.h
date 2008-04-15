#ifndef NASTDIO_H
#define NASTDIO_H

struct _fieldent {
	char *name;
	short index;
};
typedef struct _fieldent fieldent;

int io_new(int sock);

#endif
