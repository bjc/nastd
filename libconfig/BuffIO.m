#include "BuffIO.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

@implementation BuffIO
-init: (const char *)fileName
{
	struct stat sb;
	int confFd;

	confFd = open(fileName, O_RDONLY, 0);
	if (confFd == -1) {
		fprintf(stderr, "ERROR: couldn't open config file %s: %s.\n",
			fileName, strerror(errno));
		return nil;
	}
	if (fstat(confFd, &sb) == -1) {
		fprintf(stderr, "ERROR: couldn't stat config file %s: %s.\n",
			fileName, strerror(errno));
		return nil;
	}
	fileLen = sb.st_size;
	file = mmap(NULL, fileLen, PROT_READ, MAP_PRIVATE, confFd, 0);
	if (file == MAP_FAILED) {
		fprintf(stderr, "ERROR: couldn't mmap config file %s: %s.\n",
			fileName, strerror(errno));
		return nil;
	}
	close(confFd);

	curOff = file;
	EOL = '\n';

	return self;
}

-free
{
	munmap(file, fileLen);
	return [super free];
}

-(char *)
getCurOff
{
	return curOff;
}

-(off_t)
getLength
{
	return fileLen;
}

-setEOL: (char)delim
{
	EOL = delim;
	return self;
}
@end
