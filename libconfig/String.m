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
