#ifndef BUFFIO_H
#	define BUFFIO_H

#undef __OBJC2__                        /* Turn off OBJC2 on Darwin. */

#include <Object.h>

#include <sys/types.h>

@interface BuffIO: Object
{
	char *file;
	char *curOff;
	off_t fileLen;
	char EOL;
}

-init: (const char *)fileName;
-free;
-(char *)getCurOff;
-(off_t)getLength;
-setEOL: (char)delim;
@end
#endif
