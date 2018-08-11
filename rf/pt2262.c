#include "arctech.h"
#include "receive.h"
#include "message.h"
#include <htc.h>
#include <stdio.h>

/**
 * Decoding of the PT2262/CS2262 IC. The chip encodes a
 * 12 bit register of tri-state address pins. 
 */

#define LONG(x) (x > 20 && x < 30) // Typically between 27-29
#define SHORT(x) (x > 5 && x < 15) // Typically between 9-10


unsigned char pt2262Bit(UCHAR8 b1, UCHAR8 b2, UCHAR8 b3, UCHAR8 b4) {
	if (LONG(b1) && SHORT(b2) && LONG(b3) && SHORT(b4)){
		return 0;
	}
	if (SHORT(b1) && LONG(b2) && SHORT(b3) && LONG(b4)){
		return 1;
	}
	if (SHORT(b1) && LONG(b2) && LONG(b3) && SHORT(b4)){
		return 2;
	}
	return 3;
}

char parsePt2262(unsigned short scanP, unsigned char scanBit) {
	unsigned long data = 0;
	long power = 1;

	rfRetreatBit(&scanP, &scanBit);  //retreat one bit
	rfCountSimilar(&scanP, &scanBit);  //skip last

	for (int i=0; i<12; ++i) {
		unsigned char b1 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b2 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b3 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b4 = rfCountSimilar(&scanP, &scanBit);
		unsigned char ternary = pt2262Bit(b1,b2,b3,b4);

		if(ternary == 3){

			return 0;  
		}

		data += (ternary * power);
		power *= 3;
	}

	rfMessageBeginRaw();
	rfMessageAddString("protocol", "pt2262");
	rfMessageAddLong("data", data);
	rfMessageEnd(2);

	return 1;
}
