#include "x10.h"
#include "receive.h"
#include "message.h"
#include <htc.h>
#include <stdio.h>

#define LOWERX10 12
#define HIGHERX10 25

unsigned char x10Bit(UCHAR8 b1, UCHAR8 b2) {
	if (b1 <= LOWERX10 &&
	    b2 <= LOWERX10) {
		return 0;
	}
	if (b1 >= HIGHERX10 &&
	    b2 <= LOWERX10) {
		return 1;
	}
	return 2;
}

char parseX10(unsigned short scanP, unsigned char scanBit) {
	unsigned long code = 0;
	
	rfRetreatBit(&scanP, &scanBit);  //retreat one bit
	rfCountSimilar(&scanP, &scanBit); //skip last

	unsigned long mask = 1;
	for(int i = 0; i<32; ++i) {
		UCHAR8 b1 = rfCountSimilar(&scanP, &scanBit);
		UCHAR8 b2 = rfCountSimilar(&scanP, &scanBit);
		switch (x10Bit(b1, b2)) {
			case 0:
				break;
			case 1:
				code |= mask;
				break;
			default:
				return 0; // Not X10
		}
		mask <<= 1;
	}
	
	//We check the compliment-bytes
	if ( ((code>>24) & 0xFF) != (((~code)>>16) & 0xFF) ) {
		return 0;
	}
	if (((code>>8) & 0xFF) != ((~code) & 0xFF)) {
		return 0;
	}

	rfMessageBeginRaw();
	rfMessageAddString("protocol", "x10");
	rfMessageAddLong("data", code);
	rfMessageEnd(2);
	
	return 1;
}
    
