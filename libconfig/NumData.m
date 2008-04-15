#include "NumData.h"
#include "Parser.h"
#include "String.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

@implementation NumData
-init
{
	self = [super init];
	return self;
}

-(String *)
createStr
{
	char buffer[1024];

	snprintf(buffer, sizeof(buffer), "%ld", data);
	return [[String new] setStr: buffer];
}

-setFromBuffer: (const char *)buffer withLength: (int)len
{
	char *endPtr;

	data = strtol(buffer, &endPtr, 0);
	if (*endPtr) {
		fprintf(stderr, "ERROR: `%s' is not a number.\n", buffer);
		return nil;
	}

	return self;
}

-(long)
getNum
{
	return data;
}
@end
