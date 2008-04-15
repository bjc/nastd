#include "conf.h"

#include "StringData.h"
#include "Parser.h"
#include "String.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if NEED_STRINGS
#include <strings.h>
#endif

@implementation StringData
-init
{
	self = [super init];
	data = [[String new] init];
	return self;
}

-free
{
	[data free];
	[super free];
	return self;
}

-(char *)
createCStr
{
	char *buffer;

	buffer = malloc(([data getLen]+1) * sizeof(char));
	if (!buffer) {
		fprintf(stderr, "ERROR: Couldn't allocate buffer: %s.\n",
			strerror(errno));
		return NULL;
	}

	strcpy(buffer, [data getStr]);
	return buffer;
}

-(String *)
createStr
{
	String *str;
	char *buffer;

	buffer = [self createCStr];
	if (buffer == NULL)
		return nil;

	str = [[[String new] init] setStr: buffer];
	free(buffer);

	return str;
}

-setFromBuffer: (const char *)buffer withLength: (int)len
{
	const char *startPtr, *endPtr;

	startPtr = index(buffer, '"');
	if (!startPtr) {
		fprintf(stderr, "ERROR: No opening quote for string.\n");
		return nil;
	}
	startPtr++;

	for (endPtr = buffer+len; endPtr > startPtr; endPtr--)
		if (*endPtr == '"')
			break;

	if (endPtr == startPtr) {
		fprintf(stderr, "ERROR: No ending quote for string.\n");
		return nil;
	}
	endPtr--;

	[data setStr: startPtr withLength: endPtr-startPtr+1];
	return self;
}
@end
