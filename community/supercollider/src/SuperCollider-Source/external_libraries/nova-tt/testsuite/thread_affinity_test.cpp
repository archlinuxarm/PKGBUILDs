#define BOOST_TEST_MAIN
#include "thread_affinity.hpp"
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_CASE( affinity_test )
{
    BOOST_REQUIRE(nova::thread_set_affinity(0));
}
