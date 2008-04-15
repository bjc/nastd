#include "String.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

@implementation String
-init
{
	self = [super init];
	data = NULL;
	return self;
}

-free
{
	free(data);
	return [super free];
}

-setStr: (const char *)stringData withLength: (int)length
{
	if (data)
		free(data);

	data = malloc((length+1)*sizeof(char));
	if (!data) {
		fprintf(stderr, "ERROR: Couldn't allocate memory: %s.\n",
			strerror(errno));
		return nil;
	}

	memcpy(data, stringData, length);
	data[length] = '\0';
	len = length;
	return self;
}

-setStr: (const char *)stringData
{
	return [self setStr: stringData withLength: strlen(stringData)];
}

-(int)
getLen
{
	return len;
}

-(char *)
getStr
{
	return data;
}
@end
