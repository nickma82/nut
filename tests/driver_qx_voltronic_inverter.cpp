#include <cppunit/extensions/HelperMacros.h>

#include "nutdrv_qx.h"
#include "nutdrv_qx_voltronic_preprocessing.h"

class Voltronic_inverter_preprocessor : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( Voltronic_inverter_preprocessor );
    CPPUNIT_TEST( test_inverter_sign );
    CPPUNIT_TEST( test_voltronic_inverter_qe );
    CPPUNIT_TEST( test_voltronic_inverter_cmd_boolinput );
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {};
  void tearDown() {};

  void test_inverter_sign();
  void test_voltronic_inverter_qe();
  void test_voltronic_inverter_cmd_boolinput();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( Voltronic_inverter_preprocessor );




void Voltronic_inverter_preprocessor::test_inverter_sign()
{
	char value[SMALLBUF] = "\0";
	item_t item = { NULL,		0,	NULL,	NULL,	"",	0,	0,	"000",	0,	0,	"%s",	0,	NULL,	NULL };

	int rc = 0;
	memset(value, '\0', SMALLBUF);
	memcpy(item.value, "10", 8);
	rc = voltronic_inverter_sign(&item, value, SMALLBUF );
	CPPUNIT_ASSERT_EQUAL(std::string("-0"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(item.value, "00000000", 8);
	rc = voltronic_inverter_sign(&item, value, SMALLBUF );
	CPPUNIT_ASSERT_EQUAL(std::string("0"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(item.value, "01234567", 8);
	rc = voltronic_inverter_sign(&item, value, SMALLBUF );
	CPPUNIT_ASSERT_EQUAL(std::string("1234567"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(item.value, "11234567", 8);
	rc = voltronic_inverter_sign(&item, value, SMALLBUF );
	CPPUNIT_ASSERT_EQUAL(std::string("-1234567"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(item.value, "10000567", 8);
	rc = voltronic_inverter_sign(&item, value, SMALLBUF );
	CPPUNIT_ASSERT_EQUAL(std::string("-567"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memset(item.value, '\0', 8);
	rc = voltronic_inverter_sign(&item, value, SMALLBUF );
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_FAILED, rc);

	const size_t limitedValueLen = 2;
	memset(value, '\0', SMALLBUF);
	memcpy(item.value, "11234567", 8);
	rc = voltronic_inverter_sign(&item, value, limitedValueLen );
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_FAILED, rc);
}

void Voltronic_inverter_preprocessor::test_voltronic_inverter_qe() {

	char value[SMALLBUF]="\0";
	item_t item = { NULL,		0,	NULL,	"QED%s\r",	"",	0,	0,	"000",	0,	0,	"%s",	0,	NULL,	NULL };

	memcpy(value, "20150322", 8);
	int rc = voltronic_inverter_qe(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(std::string("QED20150322105\r"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(value, "20150323", 8);
	rc = voltronic_inverter_qe(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(std::string("QED20150323106\r"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	const size_t wrongSizeValue = 9;
	memset(value, '\0', SMALLBUF);
	memcpy(value, "123456789", wrongSizeValue);
	rc = voltronic_inverter_qe(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_FAILED, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(value, "20150101", 8);
	rc = voltronic_inverter_qe(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(std::string("QED20150101100\r"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);
}

void Voltronic_inverter_preprocessor::test_voltronic_inverter_cmd_boolinput() {
	char value[SMALLBUF] = "\0";
	item_t item = { NULL,		0,	NULL,	"FOO%d\r",	"",	0,	0,	"000",	0,	0,	"%s",	0,	NULL,	NULL };

	memcpy(value, "enable", 6);
	int rc = voltronic_inverter_cmd_boolinput(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(std::string("FOO1\r"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(value, "1", 1);
	rc = voltronic_inverter_cmd_boolinput(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(std::string("FOO1\r"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(value, "disable", 7);
	rc = voltronic_inverter_cmd_boolinput(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(std::string("FOO0\r"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(value, "0", 1);
	rc = voltronic_inverter_cmd_boolinput(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(std::string("FOO0\r"), std::string(value));
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_SUCCESSFUL, rc);

	memset(value, '\0', SMALLBUF);
	memcpy(value, "bar", 3);
	rc = voltronic_inverter_cmd_boolinput(&item, value, SMALLBUF);
	CPPUNIT_ASSERT_EQUAL(RC_PREPROC_FAILED, rc);
}
