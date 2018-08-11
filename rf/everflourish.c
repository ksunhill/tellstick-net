#include "everflourish.h"
#include "receive.h"
#include "message.h"
#include <stdio.h>

#define LOWEREVERFLOURISH 13
#define HIGHEREVERFLOURISH 16

UCHAR8 everflourishBit(UCHAR8 b1, UCHAR8 b2, UCHAR8 b3, UCHAR8 b4) {
  
	if (b1 < LOWEREVERFLOURISH && b2 < LOWEREVERFLOURISH && b3 < LOWEREVERFLOURISH && b4 > HIGHEREVERFLOURISH) {
		return 1;
	}
	if (b1 < LOWEREVERFLOURISH && b2 > HIGHEREVERFLOURISH && b3 < LOWEREVERFLOURISH && b4 < LOWEREVERFLOURISH) {
		return 0;
	}
	return 2;
}

char parseEverFlourish(unsigned short scanP, unsigned char scanBit) {
	
	unsigned long data = 0;
	unsigned long mask = 1;
	rfRetreatBit(&scanP, &scanBit); //retreat one bit
	
	for(int i=0;i<24;++i){
		UCHAR8 b1 = rfCountSimilar(&scanP, &scanBit);
		UCHAR8 b2 = rfCountSimilar(&scanP, &scanBit);
		UCHAR8 b3 = rfCountSimilar(&scanP, &scanBit);
		UCHAR8 b4 = rfCountSimilar(&scanP, &scanBit);
		switch(everflourishBit(b1,b2,b3,b4)){
			case 0:
				break;
			case 1:
				data |= mask;
				break;
			default:
				return 0; //not everflourish
		}	
		mask <<= 1;
	}
	
	rfMessageBeginRaw();
	rfMessageAddString("protocol", "everflourish");
	rfMessageAddLong("data", data);
	rfMessageEnd(2);
			
	return 1;
}
