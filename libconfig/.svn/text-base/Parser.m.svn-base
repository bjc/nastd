#include "Parser.h"
#include "ArrayData.h"
#include "DictData.h"
#include "NumData.h"
#include "StringData.h"
#include "String.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

@implementation Parser
-(int)
getLineLenFrom: (const char *)startAddr to: (const char *)endAddr
{
	int len;
	char quote = '\0';
	int frenchBraces = 0, squareBraces = 0;

	for (len = 0; startAddr+len < endAddr; len++) {
		if (!quote && !frenchBraces && !squareBraces &&
		    *(startAddr+len) == ';')
			break;

		switch (*(startAddr+len)) {
		case '{':
			frenchBraces++;
			break;
		case '}':
			if (!frenchBraces)
				fprintf(stderr,
					"WARNING: found `}' without `{'.\n");
			else
				frenchBraces--;
			break;
		case '[':
			squareBraces++;
			break;
		case ']':
			if (!squareBraces)
				fprintf(stderr,
					"WARNING: found ']' without '['.\n");
			else
				squareBraces--;
			break;
		case '"':
			if (!quote)
				quote = *(startAddr+len);
			else
				quote = '\0';
			break;
		}
	}

	return len;
}

-init
{
	return self;
}

-free
{
	return [super free];
}

-(DataObject *)
getDataFrom: (String *)dataStr
{
	const char *dataBuff;
	id dataElem;

	dataBuff = [dataStr getStr];

	/* Figure out what type of node to allocate. */
	switch (dataBuff[0]) {
	case '{':
		dataElem = [[DictData new] init];
		break;
	case '[':
		dataElem = [[ArrayData new] init];
		break;
	case '"':
		dataElem = [[StringData new] init];
		break;
	default:
		dataElem = [[NumData new] init];
		break;
	}

	return [dataElem setFromBuffer: dataBuff withLength: [dataStr getLen]];
}
@end
