#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <boost/tuple/tuple.hpp>

#include <iostream>

#include "thread_priority.hpp"

using namespace nova;
using namespace std;

BOOST_AUTO_TEST_CASE( priority_test )
{
    int current_priority = thread_priority();
    int low, high;
    boost::tie(low, high) = thread_priority_interval();

#ifdef NOVA_TT_PRIORITY_RT
    int low_rt, high_rt;
    boost::tie(low_rt, high_rt) = thread_priority_interval_rt();
    BOOST_CHECK(thread_set_priority_rt(low_rt));
#endif


#ifdef NOVA_TT_PRIORITY_PERIOD_COMPUTATION_CONSTRAINT
    int ns_per_tick = 1e9 / 44100 * 64;

    BOOST_CHECK(thread_set_priority_rt(ns_per_tick, ns_per_tick - 2, ns_per_tick - 1, false));
#endif

}
