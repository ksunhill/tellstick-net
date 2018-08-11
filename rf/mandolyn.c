#include "mandolyn.h"
#include "receive.h"
#include "message.h"
#include <stdio.h>

#define ZERO(x) (x>=30 && x<=40)
#define SHORT(x) (x>=10 && x<=20)
#define ONE(x,y) (SHORT(x) && SHORT(y))

#define INVALID_DATA 2

// For some reason this must be a global variable (non const) or else printf() seems to behave strange!
char MODEL[] = "temperaturehumidity";

unsigned char mandolynBit(unsigned short *scanP, unsigned char *scanBit) {
	UCHAR8 b1 = rfCountSimilar(scanP, scanBit);
	if (ZERO(b1)) {
		return 0;
	}
	UCHAR8 b2 = rfCountSimilar(scanP, scanBit);
	if (ONE(b1,b2)) {
		return 1;
	}
	return INVALID_DATA;
}

char parseMandolyn(unsigned short scanP, unsigned char scanBit) {
	unsigned short P, B;
	unsigned char preamble = 0;
	unsigned long data = 0;
	unsigned long mask = 1;

	rfRetreatBit(&scanP, &scanBit); //skip last bit
	rfCountSimilar(&scanP, &scanBit); //skip last pulse

	P = scanP; B = scanBit;

	for(int i=0;i<32;++i){
		UCHAR8 b = mandolynBit(&scanP, &scanBit);
		if (b == INVALID_DATA) {
			return 0;
		}
		if (b) {
			data |= mask;
		}
		mask <<= 1;
	}

	for(int i=0;i<4;++i){
		UCHAR8 b = mandolynBit(&scanP, &scanBit);
		if (b == INVALID_DATA) {
			return 0;
		}
		if (b) {
			preamble |= (1<<i);
		}
	}
	if (preamble != 0xC) {
		return 0;
	}

	rfMessageBeginRaw();
		rfMessageAddString("class", "sensor");
		rfMessageAddString("protocol", "mandolyn");
		rfMessageAddString("model", MODEL);
		rfMessageAddLong("data", data);
	rfMessageEnd(1);
	return 1;
}
