#include "transmit.h"
#include "config.h"
#include <htc.h>

volatile unsigned char rfNextTime = 0;
volatile unsigned char rfCurrentTime = 0;
volatile bit rfNewTime, rfNextIsZero;

void rfTransmitUpdate() {

	if (rfCurrentTime == 0) {
		rfNewTime = 1;
		if (!rfNextTime) {
			//next time haven't been calculated yet, try again next time
			return;
		}
		//Load next time
		rfCurrentTime = rfNextTime;
		rfNextTime = 0;
		if (!rfNextIsZero) {
			SENDER ^= 1;
		} else {
			rfNextIsZero = 0;
		}
	}
	if (rfCurrentTime) {
		--rfCurrentTime;
	}
}

void rfSend(const char *string) {
	rfNextIsZero = 0;
	rfCurrentTime = 0;
	rfNewTime = 0;
	rfNextTime = string[0];
	TMR3ON=1;
	while(!rfNewTime);

	TMR2IE=0; //disable this timer (pvm) to avoid these interrupts during send
	for(unsigned short i = 1; string[i] != 0; ++i) {
		if (string[i] == 1) {
			rfNextIsZero = 1;
			continue;
		}
		rfNewTime = 0;
		rfNextTime = string[i];
		while(!rfNewTime);
	}
	TMR2IE=1;

	//End
	rfNewTime = 0;
	while(!rfNewTime);

	TMR3ON=0;
	SENDER = 0;
}

void rfSendExtended(const char *string) {
	unsigned char times[4];
	unsigned char pulses, time;
	unsigned char *j;

	for(char i=0; i<4; ++i) {
		times[i] = string[i];
	}

	j = &string[5];

	rfNextIsZero = 0;
	rfCurrentTime = 0;
	rfNewTime = 0;

	pulses = string[4];
	if (pulses == 0) {
		//Invalid data
		return;
	}

	for(; pulses > 0;++j) {
		time = (*j);

		for(char i=0; i<4; ++i) {
			unsigned char timePointer = (time & 0b11000000) >> 6;
			if (times[timePointer] == 1) {
				rfNextIsZero = 1;
			} else {
				rfNextTime = times[timePointer];
				TMR3ON=1;
				while(!rfNewTime);
				rfNewTime = 0;
			}

			time <<= 2;

			--pulses;
			if (pulses == 0) {
				break;
			}
		}
	}

	//End
	rfNewTime = 0;
	while(!rfNewTime);

	TMR3ON=0;
	SENDER = 0;

}
