#include "oregonv3.h"
#include "receive.h"
#include "message.h"
#include <htc.h>
#include <stdio.h>
#include <string.h>

enum {
	SM_PREAMBLE = 0,
	SM_SYNC,
	SM_PARSE_V3ID,
	SM_PARSE_PAYLOAD
} static state = SM_PREAMBLE;

#define SMALL_PULSEV3(x) (x>=6 && x<=11) //around 7-10 in our case
#define BIG_PULSEV3(x) (x>=14 && x<=21) //around 16-19 in our case
#define END_SILENCE(x) (x > 30) //66 really, but everything greater than max pulse length should be ok, right?

#define MORE_DATA_NEEDED -1
#define INVALID_DATA -2

static unsigned int halfTime = 0;
static unsigned char preambleSyncCount = 0;
static unsigned char byteCntV3 = 0;
static unsigned char nibblePositionV3 = 0;

void clearOregonV3() {
	state = SM_PREAMBLE;
	preambleSyncCount = 0;
	halfTime = 0;
	byteCntV3 = 0;
	nibblePositionV3 = 0;
}

signed char oregonBitV3(unsigned char level, unsigned char count) {

	if (BIG_PULSEV3(count)) {
		if (halfTime % 2 == 1) {
			//odd, this sequency is not allowed in this protocol
			return INVALID_DATA;
		}
		halfTime = halfTime + 2;
		//is even
		return level;
	}
	if (SMALL_PULSEV3(count)) {
		halfTime++;
		if (halfTime % 2 == 0) {
			//is even
			return level;
		}
		return MORE_DATA_NEEDED;
	}
	return INVALID_DATA;
}

signed int oregonByteV3(unsigned char level, unsigned char count) {
	signed char b1 = oregonBitV3(level, count);
	static unsigned char byte = 0;
	if (byteCntV3 == 0) {
		byte = 0; //this need to be reset
	}

	if (b1 == INVALID_DATA) {
		return INVALID_DATA;
	} else if (b1 == MORE_DATA_NEEDED) {
		return MORE_DATA_NEEDED;
	}
	if (nibblePositionV3 == 4){
		byte <<= 4;
		nibblePositionV3 = 0;
	}
	if (b1) {
		byte |= (1<<nibblePositionV3);
	}

	++byteCntV3;
	++nibblePositionV3;
	if (byteCntV3 < 8) {
		return MORE_DATA_NEEDED;
	}
	byteCntV3=0;
	nibblePositionV3 = 0;
	return byte;
}

void streamOregonV3(unsigned char level, unsigned char count) {
	//this protocol starts out with 24 bits (6 nibbles) only 1:s
	//then 4 sync bits (0101)
	static unsigned int id = 0;
	static signed int b1;
	static signed char expectedPayloadLengthV3 = -1;
	static unsigned char bufferV3[9]; //21 nibbles, -4 for sensor id, stored as bytes
	static unsigned char bufferCntV3 = 0;

	if (level) {
		count+=1;
	} else {
		count-=1;
	}

	switch(state) {
		case SM_PREAMBLE:
			if (SMALL_PULSEV3(count)) {
				//in preamble
				++preambleSyncCount;
				break;
			}

			if (BIG_PULSEV3(count) && preambleSyncCount > 20) {
				state=SM_SYNC;
				preambleSyncCount = 3;
				break;
			}

			clearOregonV3(); //something is wrong
			break;

		case SM_SYNC:
			if (BIG_PULSEV3(count)){
				--preambleSyncCount;
				if (preambleSyncCount == 0){
					//sync successful (long pulse = opposite vs the previous, i.e. 1010)
					id = 0;
					state=SM_PARSE_V3ID;
				}
				break;
			}
			clearOregonV3();
			break;

		case SM_PARSE_V3ID:
			b1 = oregonByteV3(level, count);
			if (b1 == INVALID_DATA) {
				clearOregonV3();
				break;
			} else if (b1 == MORE_DATA_NEEDED) {
				break;
			} else {
				if (id == 0) {
					id = b1 << 8;
				} else {
					id |= b1;
					switch (id) {
						case 0xF824:
						case 0xF8B4:
							expectedPayloadLengthV3 = 7; //13 -> 17, but -4 and /2 since id is already parsed, and we are counting bytes, not nibbles (goes for all below too)
							break;
						case 0xD874:
						case 0xC844:
							expectedPayloadLengthV3 = 5; //10, probably 14, sketchy documentation
							break;
						case 0x1984:
						case 0x1994:
							expectedPayloadLengthV3 = 8; //15
							break;
						case 0x5D60:
							expectedPayloadLengthV3 = 9; //17 unknown protocol version
							break;
						case 0x2914:
							expectedPayloadLengthV3 = 8; //16
							break;
						case 0x2D10:
							expectedPayloadLengthV3 = 7; //14 probably 18 (sketchy documentation), unknown protocol version
							break;
						default:
							expectedPayloadLengthV3 = -1; //unknown length, may be decoded server side anyways
							break;
					}
					state = SM_PARSE_PAYLOAD;
					bufferCntV3 = 0;
				}
			}
			break;

		case SM_PARSE_PAYLOAD:

			b1 = oregonByteV3(level, count);
			if (b1 == INVALID_DATA) {
				if (expectedPayloadLengthV3 < 0 && !level && (halfTime/2) > 55 && END_SILENCE(count)) {
					//invalid data, but it may be the end silence if using default length
					expectedPayloadLengthV3 = 0;
					b1 = 0; //this last part will be ignored in the data parsing anyways
				}
				else{
					clearOregonV3();
					break;
				}
			} else if (b1 == MORE_DATA_NEEDED) {
				break;
			}
			else{
				--expectedPayloadLengthV3;
			}
			bufferV3[bufferCntV3] = b1;
			++bufferCntV3;

			if (expectedPayloadLengthV3 == 0){
				//complete packet
				//whole payload is parsed (if length is known), or unknown length followed by silence
				clearOregonV3();
				rfMessageBeginRaw();
					rfMessageAddString("class", "sensor");
					rfMessageAddString("protocol", "oregon");
					rfMessageAddLong("model", id);
					rfMessageAddHexString("data", bufferV3, bufferCntV3);
				rfMessageEnd(1);
				break;
			}
			if (halfTime > 400){
				//error, message is crazy too long
				clearOregonV3();
				break;
			}
			break;
	}
}
