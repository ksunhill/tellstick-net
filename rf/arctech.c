#include "arctech.h"
#include "receive.h"
#include "message.h"
#include <htc.h>
#include <stdio.h>

#define SIGNAL_LENGTH 31

unsigned char arctechCodeSwitchBit(UCHAR8 b1, UCHAR8 b2, UCHAR8 b3, UCHAR8 b4) {
	if (b1 > 15 && b2 < 10 && b3 > 15 && b4 < 10) {
		return 0;
	}
	if (b1 < 10 && b2 > 15 && b3 > 15 && b4 < 10) {
		return 1;
	}
	return 2;
}

unsigned char arctechSelflearningBit(UCHAR8 b1, UCHAR8 b2, UCHAR8 b3, UCHAR8 b4) {
	if (b1 > 15 && b2 < 10 && b3 < 10 && b4 < 10) {
		return 0;
	}
	if (b1 < 10 && b2 < 10 && b3 > 15 && b4 < 10) {
		return 1;
	}
	return 2;
}

char parseArcTechCodeSwitch(unsigned short scanP, unsigned char scanBit) {

	unsigned long data = 0;
	unsigned long mask = 1;
	
	rfRetreatBit(&scanP, &scanBit);  //retreat one bit
	rfCountSimilar(&scanP, &scanBit);  //skip last
	
	for(int i=0; i < 12; ++i) {
		unsigned char b1 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b2 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b3 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b4 = rfCountSimilar(&scanP, &scanBit);
		data <<= 1;
		switch (arctechCodeSwitchBit(b1,b2,b3,b4)) {
			case 0:
				break;
			case 1:
				data |= 1;
				break;
			case 2:
				return 0;
		}

	}

	if(data == 0){
		//avoid common invalid signals
		return 0;
	}

	rfMessageBeginRaw();
	rfMessageAddString("protocol", "arctech");
	rfMessageAddString("model", "codeswitch");
	rfMessageAddLong("data", data);
	rfMessageEnd(2);

	return 1;
}

char parseArcTechSelflearning(unsigned short scanP, unsigned char scanBit) {
	unsigned long data = 0;
	unsigned long mask = 1;
	unsigned int finished = 0;
	int i = 0;

	rfRetreatBit(&scanP, &scanBit);
	rfCountSimilar(&scanP, &scanBit);

	for(;i < 36; ++i) {
		unsigned char b1 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b2 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b3 = rfCountSimilar(&scanP, &scanBit);
		unsigned char b4 = rfCountSimilar(&scanP, &scanBit);

		switch(arctechSelflearningBit(b1,b2,b3,b4)) {
			case 0:
				if (i > SIGNAL_LENGTH) {
					//only when we have scanned more than 32
					data = data >> 1;
				}
				break;
			case 1:
				if (i > SIGNAL_LENGTH) {
					//only when we have scanned more than 32
					data = data >> 1;
				}
				data |= mask;
				break;
			default:
				if (i >= SIGNAL_LENGTH) {
					finished = 1;
					break;
				}
				else{
					return 0; //not arctech
				}
		}

		if (finished) {
			break;
		}
		if (i < SIGNAL_LENGTH) {
			mask <<= 1;
		}

	}

	rfMessageBeginRaw();
	rfMessageAddString("protocol", "arctech");
	rfMessageAddString("model", "selflearning");
	rfMessageAddLong("data", data);
	rfMessageEnd(2);

	return 1;
}
    
