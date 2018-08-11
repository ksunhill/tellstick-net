#include "oregon.h"
#include "receive.h"
#include "message.h"
#include <htc.h>
#include <stdio.h>
#include <string.h>

enum {
	SM_WAIT = 0,
	SM_PARSE_ID,
	SM_PARSE_DATA
} static state = SM_WAIT;

#define BIG_PULSE(x) (x>=12 && x<=22)
#define SMALL_PULSE(x) (x>=4 && x<=13)

#define MORE_DATA_NEEDED -1
#define INVALID_DATA -2

static unsigned char byteCnt = 0;
static unsigned char bitCnt = 0;
static unsigned char totByteCnt = 0;
signed int byteLength = -1;

void clearOregon() {
	byteCnt = 0;
	bitCnt = 0;
	totByteCnt = 0;
	state = SM_WAIT;
	byteLength = -1;
}

signed char oregonBit(unsigned char level, unsigned char count) {
	static bit b1 = 0;

	if (bitCnt == 0) {
		//First pulse must be small
		if (!SMALL_PULSE(count)) {
			return INVALID_DATA;
		}
		bitCnt = 1;

	} else if (bitCnt == 1) {
		//Second pulse must be long
		if (!BIG_PULSE(count) && totByteCnt!=byteLength){ //special check - last byte might have strange values
			bitCnt = 0;
			return INVALID_DATA;
		}

		b1 = level;
		bitCnt = 2;
		return b1;

	} else if (bitCnt == 2) {
		//Prepare for next bit
		if (level && SMALL_PULSE(count)) {
			//Clean start
			bitCnt = 0;
		} else if (BIG_PULSE(count)) {
			//Combined bit
			bitCnt = 1;
		} else if (SMALL_PULSE(count)) {
			//Clean start
			bitCnt = 0;
		}
		return MORE_DATA_NEEDED;
	}

	return MORE_DATA_NEEDED;
}

signed int oregonByte(unsigned char level, unsigned char count) {
	signed char b1 = oregonBit(level, count);
	static unsigned char byte = 0;
	
	if (b1 == INVALID_DATA) {
		return INVALID_DATA;
	} else if (b1 == MORE_DATA_NEEDED) {
		return MORE_DATA_NEEDED;
	}
	byte >>= 1;
	if (b1) {
		byte |= (1<<7);
	}
	++totByteCnt;
	++byteCnt;
	if (byteCnt < 8) {
		return MORE_DATA_NEEDED;
	}
	byteCnt=0;
	return byte;
}

void streamOregon(unsigned char level, unsigned char count) {
	static unsigned char cnt = 0;
	static unsigned int id = 0;
	static signed int b1;
	static unsigned char length = 0;
	static unsigned char buffer[8];

	if (level) {
		count+=3;
	} else {
		count-=3;
	}

	switch(state) {
		case SM_WAIT:
			if (BIG_PULSE(count)) {
				++cnt;
				break;
			} else {
			}
			if (SMALL_PULSE(count)) {
				if (cnt > 25) {
					state=SM_PARSE_ID;
					id = 0;
				}
				cnt = 0;
			}
			break;

		case SM_PARSE_ID:
			b1 = oregonByte(level, count);
			if (b1 == INVALID_DATA) {
				clearOregon();
				break;
			} else if (b1 == MORE_DATA_NEEDED) {
				break;
			} else {
				if (id == 0) {
					id = b1 << 8;
				} else {
					id |= b1;
					switch (id) {
						case 0xEA4C:
							length = 6;
							byteLength = 63;
							break;
						case 0x0A4D:
						case 0x1A2D:
							length = 8;
							byteLength = 79;
							break;
						default:
							clearOregon();
							return;
					}
					state = SM_PARSE_DATA;
					cnt = 0;
				}
			}
			break;

		case SM_PARSE_DATA:
			b1 = oregonByte(level, count);
			if (b1 == INVALID_DATA) {
				clearOregon();
				break;
			} else if (b1 == MORE_DATA_NEEDED) {
				break;
			}
			buffer[cnt] = b1;
			++cnt;
			--length;
			if (length == 0) {
				clearOregon();
				rfMessageBeginRaw();
					rfMessageAddString("class", "sensor");
					rfMessageAddString("protocol", "oregon");
					rfMessageAddLong("model", id);
					rfMessageAddHexString("data", buffer, cnt);
				rfMessageEnd(1);
			}

			break;
	}
}
