# Network UPS Tools: tests

if HAVE_CPPUNIT

TESTS = cppunittest

check_PROGRAMS = $(TESTS)

AM_CPPFLAGS = -I../drivers
cppunittest_CXXFLAGS = $(CPPUNIT_CFLAGS)
cppunittest_LDFLAGS = $(CPPUNIT_LIBS) 
cppunittest_LDADD = ../common/common.o 

# List of src files for CppUnit tests
CPPUNITTESTSRC = example.cpp \
		../drivers/nutdrv_qx_voltronic_preprocessing.c driver_qx_voltronic_inverter.cpp

cppunittest_SOURCES = $(CPPUNITTESTSRC) cpputest.cpp

else !HAVE_CPPUNIT

#EXTRA_DIST = example.cpp cpputest.cpp

endif !HAVE_CPPUNIT
