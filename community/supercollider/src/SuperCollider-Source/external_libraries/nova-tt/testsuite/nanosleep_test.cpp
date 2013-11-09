#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include "nanosleep.hpp"

using namespace nova;

BOOST_AUTO_TEST_CASE( nanosleep_test )
{
    nanosleep(100000);
    nanosleep(2000000000);
}
