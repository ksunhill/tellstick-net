#include "hasta.h"
#include "receive.h"
#include "message.h"
#include <htc.h>
#include <stdio.h>
#include <string.h>

enum {
	SM_NONE = 0,
	SM_FIRST,
	SM_OLD_BEFOREPARSE,
	SM_OLD_PARSE,
	SM_SECOND_NEW,
	SM_THIRD_NEW,
	SM_NEW_BEFOREPARSE,
	SM_NEW_PARSE
} static hastastate = SM_NONE;

#define BIG_PULSE_NEW(x) (x>=9 && x<=15)
#define BIG_PULSE_OLD(x) (x>=5 && x<=7)
#define SMALL_PULSE_NEW(x) (x>=4 && x<=8)
#define SMALL_PULSE_OLD(x) (x>=2 && x<=4)
#ifdef TELLSTICK_DUO
//duo
#define FIRST_PULSE(x) (x>=77 && x<=81) //typically 78-79
#define SECOND_PULSE_NEW(x) (x>=40 && x<=43)	//typically 41-42
#define SECOND_PULSE_OLD(x) (x>=26 && x<=29)  //typically 27 (28)
#define THIRD_PULSE_NEW(x) (x>=25 && x<=28)  //typically 26 (27)
#else
//net
#define FIRST_PULSE(x) (x>=86 && x<=91) //typically 88
#define SECOND_PULSE_NEW(x) (x>=43 && x<=46)	//typically 44-45
#define SECOND_PULSE_OLD(x) (x>=28 && x<=31)  //typically 29-30
#define THIRD_PULSE_NEW(x) (x>=27 && x<=31)  //typically 28-29
#endif


#define FOURTH_PULSE_NEW(x) (x>=4 && x<=8)  //typically 6-7 (5 for Duo)

static unsigned char bytecount = 0;
static unsigned char hastabyte = 0;
unsigned long hastadata = 0;
unsigned long mask = 1;
static unsigned char prepulse = 0; //0=none, 1=short, 2=long
static unsigned int pulsecount = 0;
unsigned long totalhastadata = 0;

void clearHasta() {
	prepulse = 0;
	hastadata = 0;
	totalhastadata = 0;
	mask = 1;
	hastastate = SM_NONE;
	pulsecount = 0;
	bytecount = 0;
	hastabyte = 0;
}

void streamHasta(unsigned char level, unsigned char count) {
	switch(hastastate) {
		case SM_NONE:
			if (level && FIRST_PULSE(count)) {
				hastastate = SM_FIRST;
			}
			break;
		case SM_FIRST:
			if (SECOND_PULSE_OLD(count)) {
				hastastate = SM_OLD_PARSE;
				break;
			}
			if (SECOND_PULSE_NEW(count)) {
				hastastate = SM_SECOND_NEW;
				break;
			}
			//else
			clearHasta();
			break;
		case SM_SECOND_NEW:
			if (THIRD_PULSE_NEW(count)){
				hastastate = SM_THIRD_NEW;
				break;
			}
			clearHasta();
			break;
		case SM_THIRD_NEW:
			if (FOURTH_PULSE_NEW(count)){
				hastastate = SM_NEW_PARSE;
				break;
			}
			clearHasta();
			break;

		case SM_OLD_PARSE:
			if (prepulse == 0){
				//first pulse
				if (SMALL_PULSE_OLD(count)){
					prepulse = 1;
					break;
				}
				else if (BIG_PULSE_OLD(count)){
					prepulse = 2;
					break;
				}
				//error
				clearHasta();
				break;
			}
			else if (prepulse == 1){
				if (BIG_PULSE_OLD(count)){
					prepulse = 0;
					pulsecount++;
					mask <<= 1;
					break;
				}
				//error, wrong kind of pulse or invalid pulse
				clearHasta();
				break;
			}
			else if(prepulse == 2){
				if(SMALL_PULSE_OLD(count)){
					prepulse = 0;
					pulsecount++;
					hastabyte |= mask;
					mask <<= 1;
					break;
				}
				//error, wrong kind of pulse or invalid pulse
				clearHasta();
				break;
			}
			clearHasta();
			break;

		case SM_NEW_PARSE:
			if (prepulse == 0){
				//first pulse
				if (SMALL_PULSE_NEW(count)){
					prepulse = 1;
					break;
				}
				else if (BIG_PULSE_NEW(count)){
					prepulse = 2;
					break;
				}
				clearHasta();
				break;
			}
			else if (prepulse == 1){
				if (BIG_PULSE_NEW(count)){
					prepulse = 0;
					pulsecount++;
					mask <<= 1;
					break;
				}
				//error, wrong kind of pulse or invalid pulse
				clearHasta();
				break;
			}
			else if(prepulse == 2){
				if(SMALL_PULSE_NEW(count)){
					prepulse = 0;
					pulsecount++;
					hastabyte |= mask;
					mask <<= 1;
					break;
				}
				//error, wrong kind of pulse or invalid pulse
				clearHasta();
				break;
			}
			clearHasta();
			break;
	}

	if (pulsecount == 8){
		bytecount++;
		if (bytecount < 5){
			totalhastadata = totalhastadata + hastabyte;
			if (bytecount < 4){
				totalhastadata = totalhastadata << 8;
			}
			hastadata = hastadata + hastabyte;
			hastabyte = 0;
		}
		pulsecount = 0;
		mask = 1;
	}

	if (hastastate == SM_OLD_PARSE && bytecount == 4){
		unsigned long temptotalhastadata = totalhastadata;

		clearHasta();

		rfMessageBeginRaw();
			rfMessageAddString("protocol", "hasta");
			rfMessageAddString("model", "selflearning");
			rfMessageAddLong("data", temptotalhastadata);
		rfMessageEnd(1);
		return;
	}

	if (hastastate == SM_NEW_PARSE && bytecount == 5){
		unsigned long hastachecksum = hastabyte;
		unsigned long temphastadata = hastadata;
		unsigned long temptotalhastadata = totalhastadata;

		clearHasta();
		if(((temphastadata + hastachecksum) % 256) != 1){
			//printf("checksum ERROR");
			return;
		}

		rfMessageBeginRaw();
			rfMessageAddString("protocol", "hasta");
			rfMessageAddString("model", "selflearningv2");
			rfMessageAddLong("data", temptotalhastadata);
		rfMessageEnd(1);
	}
	return;
}
