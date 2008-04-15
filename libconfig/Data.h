#ifndef DATA_H
#	define DATA_H

#include "String.h"

#include <Object.h>

#include <sys/types.h>

#define MALLOC_GRANULARITY 10		/* Malloc for every 10 entries. */

/*
 * Generic Entry and Data objects.
 */
@interface DataObject: Object
-(char *)createCStr;
-(String *)createStr;
-setFromBuffer: (const char *)buffer withLength: (int)len;
@end

@interface DictEntry: Object
{
	String *key;
	DataObject *data;
	DictEntry *super;
}

-init;
-setKey: (String *)keyName;
-setData: (DataObject *)dataPtr;
-getKey;
-getData;
@end
#endif
