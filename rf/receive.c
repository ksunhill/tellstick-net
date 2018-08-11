#include "receive.h"
#include "arctech.h"
#include "everflourish.h"
#include "fineoffset.h"
#include "hasta.h"
#include "mandolyn.h"
#include "oregon.h"
#include "oregonv3.h"
#include "x10.h"
#include "pt2262.h"
#include "config.h"
#include <stdio.h>
#include <htc.h>

#define DATA_BYTES 512
typedef unsigned char UCHAR8;
#define TRUE 1
#define FALSE 0

unsigned char volatile data[DATA_BYTES];
static volatile unsigned short dataP = 0;

#define TRIGGER_LEVEL 3
#define TRIGGER_MASK 0b00000111;

void rfReceiveUpdate(unsigned char antenna) {
	static unsigned char buffer = 0, cnt = 0;
	static unsigned char preFilterBuffer = 0, preFilterCnt=0;
	
	//Add bit to our filter buffer.
	preFilterBuffer <<= 1;
	if (antenna) {
		++preFilterCnt;
		if (preFilterCnt == TRIGGER_LEVEL) {
			preFilterBuffer |= TRIGGER_MASK;
		} else if (preFilterCnt > TRIGGER_LEVEL) {
			preFilterBuffer |= 1;
		}
	} else {
		preFilterCnt = 0;
	}
	
	//Copy a value from our filter buffer to our real buffer
	buffer <<= 1;
	++cnt;
	if (preFilterBuffer & (1<<7)) {
		buffer |= 1;
// 		LATC0 = 1;
	} else {
// 		LATC0 = 0;
	}
	if (cnt == 8) {
		//Buffer full, copy to our data array
		cnt=0;
		data[dataP] = buffer;
		++dataP;
		if (dataP == DATA_BYTES) {
// 			LATD2 ^= 1;
			dataP = 0;
		}
	}
}
/*

void advanceBit(unsigned short *scanP, unsigned char *scanBit) {
// 	scanBit >>= 1;
// 	if (scanBit == 0) {
// 		scanBit = (1<<7);
// 		++scanP;
// 		if (scanP == DATA_BYTES) {
// 			scanP = 0;
// 		}
// 	}

}*/

void rfRetreatBit(unsigned short *scanP, unsigned char *scanBit) {
	(*scanBit) <<= 1;
		if ((*scanBit) == 0) {
			(*scanBit) = 1;
			if ((*scanP) == 0) {
				(*scanP) = DATA_BYTES;
			}
			--(*scanP);
		}
	
}

bit getBit(signed short scanP, signed char scanBit) {
	unsigned char buf = data[scanP];
	if (scanBit & buf) {
		return 1;
	}
	return 0;
}

unsigned char rfCountSimilar(unsigned short *scanP, unsigned char *scanBit) {
	unsigned char count = 0;
	unsigned char test = getBit(*scanP, *scanBit);
	while(1) {
		++count;
		rfRetreatBit(scanP, scanBit);
		if (getBit(*scanP, *scanBit) != test) {
			return count;
		}
		if (count == 255) {
			//Overflow
			return 0;
		}
	}
	return 0;
}

signed int calculateDistance(signed short start, signed short stop) {
	if (start == -1) {
		return -1;
	}
	if (stop >= start) {
		return stop-start;
	}
	return (DATA_BYTES-start+stop);
}

#define REQUIRED_SILENCE 150 //TODO: calibrate


void rfReceiveTask() {
	static unsigned short scanP = 0;
	static unsigned char scanBit = (1<<7);
	static signed short startSilenceP = -1, stopSilenceP = -13;
	static unsigned char startSilenceBit = 0, stopSilenceBit = -1;
	static unsigned unsigned char count0 = 0, count1 = 0;
	UCHAR8 found = FALSE;
	static UCHAR8 parsed = FALSE;
	
	while(scanP != dataP) {
		//Count silence and restart on high
		if (getBit(scanP, scanBit)) {
			if (count0 > REQUIRED_SILENCE) {
				stopSilenceP = scanP;
				stopSilenceBit = scanBit;
			}
			if (count0 > 0) {
				streamOregon(0, count0);
				streamOregonV3(0, count0);
				streamHasta(0, count0);
			}
			++count1;
			count0=0;
			parsed = FALSE;
		} else {
			if (count0 == 0) {
				startSilenceP = scanP;
				startSilenceBit = scanBit;
			}
			if (count1 > 0) {
				streamOregon(1, count1);
				streamOregonV3(1, count1);
				streamHasta(1, count1);
			}
			++count0;
			count1=0;
		}

		//Advance to next bit
		scanBit >>= 1;
		if (scanBit == 0) {
			scanBit = (1<<7);
			++scanP;
			if (scanP == DATA_BYTES) {
				scanP = 0;
			}
		}
		
		if (stopSilenceP == scanP && stopSilenceBit == scanBit) {
			//We have traveled a complete lap around the buffer
			stopSilenceP = -1;
// 			LATD6^=1;
		}
		
		//Start parsing if enough silence detected
		if (count0 == REQUIRED_SILENCE) {
// 			LATD4 = 0;
			found = TRUE;
			break;
		}
	}

	if (!found) {
		return;
	}
	if (parsed) {
		return;
	}

	//We calculate stopSilence (start of data) until startSilence (end of data)
	signed int dist = calculateDistance(stopSilenceP, startSilenceP);
// 	printf("%u\r\n", dist);
	if (dist >= 0 && dist < 50) {
		return;
	}
	if (dist < 158 || dist > 162) { //TODO: For now, remove later!
// 		printf("%i\r\n", dist );
// 		return 0;
	}
	char matched = 0;
	matched += parseArcTechSelflearning(startSilenceP, startSilenceBit);
	matched += parseArcTechCodeSwitch(startSilenceP, startSilenceBit);
	matched += parseFineOffset(startSilenceP, startSilenceBit);
	//matches += parseSartano(startSilenceP, startSilenceBit);  //will be detected by arctech
	matched += parseEverFlourish(startSilenceP, startSilenceBit);
	matched += parseMandolyn(startSilenceP, startSilenceBit);
	matched += parseX10(startSilenceP, startSilenceBit);
	matched += parsePt2262(startSilenceP, startSilenceBit);
	parsed = TRUE;
	if (matched) {
		scanP = 0;
		dataP = 0;
	}
	return;
}

#ifdef DEBUG

void rfDebugPrintPulse(unsigned short scanP, unsigned char scanBit) {
	unsigned char b = '0';
	if (getBit(scanP, scanBit)) {
		b = '1';
	}
	unsigned char len = rfCountSimilar(&scanP, &scanBit);
	for(unsigned char i = 0; i < len; ++i) {
		putch(b);
	}
}

void rfDebugPrintLen(unsigned short P, unsigned char B, unsigned char len) {
	unsigned short scanP = P, scanBit = B;
	for(unsigned char i = 0; i < len; ++i) {
		if (getBit(scanP, scanBit)) {
			printf("1");
		} else {
			printf("0");
		}
		rfRetreatBit(&scanP, &scanBit);
	}
	printf("\r\n");
}

#endif
