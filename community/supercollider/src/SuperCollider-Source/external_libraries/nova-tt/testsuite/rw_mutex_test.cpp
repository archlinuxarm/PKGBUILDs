#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "rw_mutex.hpp"

using namespace nova;

const int thread_count = 8;
const int total_count = 30000;

template <typename Mutex>
void nonrecursive_tests(void)
{
    Mutex mut;

    typedef boost::shared_lock<Mutex> scoped_read_lock;

    mut.lock_shared();
    mut.lock_shared();
    BOOST_REQUIRE_EQUAL( mut.try_lock(), false );
    BOOST_REQUIRE_EQUAL( mut.try_lock_shared(), true );
    {
        scoped_read_lock lock(mut);
    }
    mut.unlock_shared();
    mut.unlock_shared();
    mut.unlock_shared();

    BOOST_REQUIRE_EQUAL( mut.try_lock(), true );
    mut.unlock();
    BOOST_REQUIRE_EQUAL( mut.try_lock_shared(), true );
    mut.unlock_shared();
}

template <typename Mutex>
void recursive_tests(void)
{
    Mutex mut;

    typedef typename Mutex::shared_lock scoped_read_lock;

    mut.lock_shared();
    mut.lock_shared();
    BOOST_REQUIRE_EQUAL( mut.try_lock(), false );
    BOOST_REQUIRE_EQUAL( mut.try_lock_shared(), true );
    {
        scoped_read_lock lock(mut);
    }
    mut.unlock_shared();
    mut.unlock_shared();
    mut.unlock_shared();

    BOOST_REQUIRE_EQUAL( mut.try_lock(), true );
    mut.lock();
    mut.lock();
    BOOST_REQUIRE_EQUAL( mut.try_lock_shared(), true );
    mut.unlock_shared();
    mut.unlock();
    mut.unlock();
    mut.unlock();
}

BOOST_AUTO_TEST_CASE( rw_mutex_test )
{
    nonrecursive_tests<rw_mutex>();
    nonrecursive_tests<nonrecursive_rw_mutex>();
}

namespace
{
    rw_mutex mut;
    typedef boost::shared_lock<rw_mutex> scoped_read_lock;
    typedef boost::unique_lock<rw_mutex> scoped_write_lock;

    void sleep ()
    {
        boost::thread::yield();
    }

    void test_fn(void)
    {
        static rw_mutex mut;

        for (int i = 0; i != total_count; ++i)
        {
            /* recursive read locks */
            {
                scoped_read_lock lock(mut);
                sleep();
                {
                    scoped_read_lock lock(mut);
                    sleep();
                }
                sleep();
            }

            /* recursive write locks */
            {
                scoped_write_lock lock(mut);
                sleep();
                {
                    scoped_write_lock lock(mut);
                    sleep();
                }
                sleep();
            }

            /* nested read / write locks */
            {
                scoped_write_lock lock(mut);
                sleep();
                {
                    scoped_read_lock lock(mut);
                    sleep();
                }
                sleep();
            }
        }
    }

}


BOOST_AUTO_TEST_CASE( rw_mutex_test_2 )
{
    boost::thread_group threads;
    for (int i = 0; i != thread_count; ++i)
        threads.create_thread(&test_fn);

    threads.join_all();
}
