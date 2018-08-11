typedef unsigned char UCHAR8;

unsigned char rfCountSimilar(unsigned short *scanP, unsigned char *scanBit);

void rfReceiveUpdate(unsigned char antenna);
void rfReceiveTask();

void rfRetreatBit(unsigned short *scanP, unsigned char *scanBit);

void rfDebugPrintPulse(unsigned short scanP, unsigned char scanBit);
void rfDebugPrintLen(unsigned short scanP, unsigned char scanBit, unsigned char len);
