#include "ArrayData.h"
#include "Parser.h"
#include "String.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

@implementation ArrayData
-init
{
	self = [super init];

	numChildren = 0;
	curObj = 0;
	data = malloc(MALLOC_GRANULARITY * sizeof(DictEntry *));
	if (!data) {
		fprintf(stderr, "ERROR: couldn't allocate memory: %s.\n",
			strerror(errno));
		return nil;
	}

	return self;
}

-free
{
	int i;

	fprintf(stderr, "DEBUG: freeing array\n");
	for (i = 0; i < numChildren; i++)
		[data[i] free];
	free(data);
	[super free];

	return nil;
}

-addChild: child
{
	/* Allocate additional space for children pointers, if required. */
	if (numChildren % MALLOC_GRANULARITY == 0) {
		/* TODO: fail gracefully. */
		data = realloc(data, (numChildren+MALLOC_GRANULARITY) *
				     sizeof(DataObject *));
		if (!data) {
			fprintf(stderr,
				"ERROR: couldn't allocate memory: %s.\n",
				strerror(errno));
			return nil;
		}
	}

	data[numChildren] = child;

	numChildren++;
	return self;
}

-setFromBuffer: (const char *)buffer withLength: (int)len
{
	Parser *parser;
	String *string;
	const char *offset;

	self = [super init];
	parser = [Parser new];
	string = [[String new] init];
	if (parser == nil || string == nil)
		return nil;

	if (buffer[len] == '\0')
		len--;

	/* Skip any leading white space. */
	for (offset = buffer; offset <= buffer+len && isspace((int)*offset);
	     offset++);
	/* Kill any ending white space. */
	for (; len >= 0 && isspace((int)buffer[len]); len--);

	/* If there's a leading brace, kill it and the ending one. */
	if (offset[0] == '[') {
		if (buffer[len] != ']') {
			fprintf(stderr, "ERROR: Unbalanced braces.\n");
			[parser free]; [string free];
			return nil;
		}
		offset++; len--;

		/* Do the whitespace shuffle again. */
		for (; len >= 0 && isspace((int)buffer[len]); len--);
	}

	while (offset < buffer+len) {
		DataObject *dataObj;
		int strLen;

		/* Do the whitespace shuffle. */
		for (; offset <= buffer+len && isspace((int)*offset); offset++);

		strLen = [parser getLineLenFrom: offset to: buffer+len];
		[string setStr: offset withLength: strLen];
		offset += strLen + 1;

		/* Parse the data element, then add it to our list. */
		dataObj = [parser getDataFrom: string];
		if (dataObj == nil) {
			[parser free]; [string free];
			return nil;
		}
		[self addChild: dataObj];
	}

	[parser free]; [string free];
	return self;
}

-(int)
numObjects
{
	return numChildren;
}

-objectAt: (int)index
{
	if (index >= numChildren)
		return nil;
	return data[index];
}
@end
