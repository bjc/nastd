#ifndef STRING_H
#	define STRING_H

#include <Object.h>

@interface String: Object
{
	char *data;
	int len;
}

-init;
-free;
-setStr: (const char *)stringData withLength: (int)length;
-setStr: (const char *)stringData;
-(int)getLen;
-(char *)getStr;
@end
#endif
