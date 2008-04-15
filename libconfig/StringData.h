#ifndef STRINGDATA_H
#	define STRINGDATA_H

#include "Data.h"
#include "String.h"

@interface StringData: DataObject
{
	String *data;
}

-free;
@end
#endif
