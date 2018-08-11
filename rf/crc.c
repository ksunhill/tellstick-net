#include "crc.h"

unsigned char calculateCrc8(unsigned char crc, const unsigned char c, unsigned char polynom)
{
	unsigned int i;
	int onebit;

	for (i = 0x80; i > 0; i >>= 1) {
		onebit = !!(crc & 0x80);
		if (c & i) {
			onebit = !onebit;
		}
		crc <<= 1;
		if (onebit) {
			crc ^= polynom;
		}
	}
	crc &= 0xff;
	return crc & 0xff;
}
