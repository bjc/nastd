#ifndef DICTDATA_H
#	define DICTDATA_H

#include "Data.h"

@interface DictData: DataObject
{
	id *data;
	int numChildren;
	int curObj;
}

-free;
-addChild: child;
-delChild: (const char *)keyName;
-findChild: (const char *)keyName;
-firstObject;
-nextObject;
@end
#endif
