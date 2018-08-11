#ifndef OREGONTEST_H
#define OREGONTEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class OregonTest : public CPPUNIT_NS :: TestFixture
{
	CPPUNIT_TEST_SUITE (OregonTest);
	CPPUNIT_TEST (decodeTest1);
	CPPUNIT_TEST (decodeTest2);
	CPPUNIT_TEST_SUITE_END ();

public:
	void setUp (void);
	void tearDown (void);

protected:
	void decodeTest1(void);
	void decodeTest2(void);

private:
	class PrivateData;
	PrivateData *d;

	void testData(unsigned char *data, int length, const std::string &expected);
};

#endif // OREGONTEST_H
