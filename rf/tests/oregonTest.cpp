#include "oregonTest.h"

extern "C" {
	#include "oregon.h"
};
#include "message.h"

CPPUNIT_TEST_SUITE_REGISTRATION (OregonTest);

class OregonTest::PrivateData {
public:
};

void OregonTest :: setUp (void) {
	d = new PrivateData;
}

void OregonTest :: tearDown (void) {
	delete d;
}

void OregonTest :: decodeTest1 (void) {
	unsigned char data[] = {
		13, 7, 19, 8, 9, 17, 10, 7, 19, 8, 10, 16, 10, 7, 19, 8, 9, 17, 18, 17, 18, 17, 10, 7, 19, 16, 19, 7, 10, 17, 18, 17, 10, 7, 19, 16, 19, 7, 10, 17, 10, 7, 19, 7, 10, 17, 10, 7, 19, 16, 19, 16, 18, 17, 19, 7, 10, 16, 10, 8, 18, 17, 18, 8, 10, 16, 19, 16, 19, 16, 10, 7, 19, 8, 9, 17, 18, 17, 18, 17, 10, 7, 19, 16, 19, 16, 19, 16, 19, 16, 19, 8, 9, 17, 18, 17, 18, 17, 9, 8, 19, 7, 10, 17, 18, 17, 18, 16, 11, 7, 19, 16, 18, 8, 10, 16, 10, 8, 18, 17, 18, 8, 10, 16, 10, 7, 19, 16, 19, 16, 19, 16, 19, 16, 19, 16, 19, 16, 19, 16, 19, 16, 19, 8, 9, 17, 10, 7, 19, 8, 9, 17, 10, 7, 19, 8, 9, 17, 9, 8, 19
	};
	testData(data, sizeof(data), "+RAWclass:sensor;protocol:oregon;model:EA4C;data:217770270154;");
}

void OregonTest :: decodeTest2 (void) {
	unsigned char data[] = {
		13, 7, 19, 7, 10, 17, 10, 7, 19, 7, 10, 17, 10, 7, 19, 7, 10, 17, 18, 17, 18, 16, 10, 8, 19, 16, 19, 7, 10, 16, 19, 16, 10, 8, 18, 17, 19, 7, 10, 16, 10, 7, 19, 8, 10, 16, 10, 8, 18, 16, 19, 16, 19, 16, 19, 8, 10, 16, 10, 7, 19, 16, 19, 8, 9, 17, 18, 17, 18, 17, 10, 7, 19, 7, 10, 17, 18, 17, 18, 17, 10, 7, 19, 16, 19, 16, 19, 16, 19, 16, 19, 7, 10, 16, 11, 7, 18, 8, 10, 16, 10, 8, 18, 17, 18, 8, 10, 16, 10, 8, 18, 16, 19, 8, 9, 17, 10, 7, 19, 8, 9, 17, 10, 7, 19, 8, 10, 16, 10, 7, 19, 16, 19, 16, 19, 16, 19, 16, 19, 8, 9, 17, 18, 17, 18, 17, 18, 16, 11, 7, 19, 16, 19, 7, 10, 16, 10, 8, 19, 7, 10, 16, 19, 16
	};
	testData(data, sizeof(data), "+RAWclass:sensor;protocol:oregon;model:EA4C;data:21775052C1D3;");
}

void OregonTest::testData(unsigned char *data, int length, const std::string &expected) {
	clearOregon();
	for(int i = 0; i < 30; ++i) {
		streamOregon(1, 15);
	}
	for(int i = 0; i < length; ++i) {
		streamOregon(i%2, data[i]);
	}
	CPPUNIT_ASSERT_EQUAL_MESSAGE("Oregon", expected, getBuffer());
}
