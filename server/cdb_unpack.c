#include "conf.h"
#include "cdbpriv.h"

uint32_t
cdb_unpack(unsigned char *buf)
{
	uint32_t num;

	num = buf[3]; num <<= 8;
	num += buf[2]; num <<= 8;
	num += buf[1]; num <<= 8;
	num += buf[0];
	return num;
}
