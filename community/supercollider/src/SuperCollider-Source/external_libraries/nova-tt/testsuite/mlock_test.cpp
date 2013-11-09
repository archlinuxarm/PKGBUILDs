#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include "mlock.hpp"

BOOST_AUTO_TEST_CASE( mlock_test )
{
    long size = 1<<16;
    char * data = new char[size];
    mlock((void*)data, size);
    munlock(data, size);
    delete[] data;
}