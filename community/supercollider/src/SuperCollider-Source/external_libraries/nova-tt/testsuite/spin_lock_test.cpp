#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include "spin_lock.hpp"

#include <boost/thread/thread.hpp>

using namespace nova;
using namespace boost;

namespace
{
const int thread_count = 8;
const int total_count = 5000;

int count = 0;

spin_lock sl;

void test_fn(void)
{
    for (int i = 0; i != total_count; ++i)
    {
        spin_lock::scoped_lock lock(sl);
        ++count;
    }
}


}

BOOST_AUTO_TEST_CASE( spinlock_test )
{
    thread_group g;

    for (int i = 0; i != thread_count; ++i)
        g.create_thread(test_fn);
    g.join_all();

    BOOST_REQUIRE_EQUAL(count, thread_count * total_count);
}
