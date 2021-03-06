#include "conf.h"
#include "cdbpriv.h"

uint32_t
cdb_hash(const unsigned char *buf, unsigned int len)
{
	uint32_t h;

	h = 5381;
	while (len) {
		--len;
		h += (h << 5);
		h ^= (uint32_t)*buf++;
	}
	return h;
}
