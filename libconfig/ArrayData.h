#ifndef ARRAYDATA_H
#	define ARRAYDATA_H

#include "Data.h"

@interface ArrayData: DataObject
{
	id *data;
	int numChildren;
	int curObj;
}
-free;
-(int)numObjects;
-addChild: child;
-objectAt: (int)index;
@end
#endif
