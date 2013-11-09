#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include "nanosleep.hpp"


BOOST_AUTO_TEST_CASE( nanosleep )
{
    nanosleep(0, 100000);
    nanosleep(3, 200);
}
