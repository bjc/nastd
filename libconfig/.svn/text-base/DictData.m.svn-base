/*
 * Copyright (c) 1998, Brian Cully <shmit@kublai.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of contributers to this software may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "DictData.h"
#include "Parser.h"
#include "String.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

@implementation DictData
-init
{
	self = [super init];

	numChildren = 0;
	curObj = 0;
	data = malloc(MALLOC_GRANULARITY * sizeof(DataObject *));
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

	fprintf(stderr, "DEBUG: freeing dict\n");
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
				     sizeof(DictEntry *));
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
	if (offset[0] == '{') {
		if (buffer[len] != '}') {
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
		DictEntry *dictEntry;
		String *key;
		char *sep;
		int strLen;

		/* Do the whitespace shuffle. */
		for (; offset <= buffer+len && isspace((int)*offset); offset++);

		/* Get the key element. */
		sep = strchr(offset, '=');
		if (!sep) {
			fprintf(stderr, "ERROR: No equals sign!\n");
			return nil;
		}
		for (sep--; sep > offset && isspace((int)*sep); sep--);
		key = [[[String new] init] setStr: offset
					   withLength: (sep-offset+1)];

		/* Get the data element. */
		offset = strchr(offset, '=');
		for(offset++; offset < buffer+len && isspace((int)*offset);
		    offset++);
		strLen = [parser getLineLenFrom: offset to: buffer+len];
		[string setStr: offset withLength: strLen];
		offset += strLen + 1;

		/* Parse the data element, then add it to our list. */
		dataObj = [parser getDataFrom: string];
		if (dataObj == nil) {
			fprintf(stderr,
				"ERROR: Couldn't parse data for node `%s'.\n",
				[key getStr]);
			[parser free]; [string free];
			return nil;
		}

		dictEntry = [[DictEntry new] setKey: key];
		if (dictEntry == nil) {
			fprintf(stderr,
				"ERROR: Couldn't create node `%s'.\n",
				[key getStr]);
			[parser free]; [string free];
			return nil;
		}
		[dictEntry setData: dataObj];
		[self addChild: dictEntry];
	}

	[parser free]; [string free];
	return self;
}

-delChild: (const char *)keyName
{
	return self;
}

-findChild: (const char *)keyName
{
	int i;

	for (i = 0; i < numChildren; i++)
		if (!strcmp([[data[i] getKey] getStr], keyName))
			return data[i];
	return nil;
}

-firstObject
{
	curObj = 0;
	return [self nextObject];
}

-nextObject
{
	DictEntry *obj;

	if (curObj >= numChildren)
		return nil;
	obj = data[curObj];
	curObj++;
	return obj;
}
@end
