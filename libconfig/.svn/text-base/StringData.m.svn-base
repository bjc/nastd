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
