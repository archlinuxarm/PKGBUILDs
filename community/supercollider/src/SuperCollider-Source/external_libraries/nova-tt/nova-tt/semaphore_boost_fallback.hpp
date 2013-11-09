//  semaphore class, boost.thread wrapper
//  based on API proposed in N2043
//
//  Copyright (C) 2008, 2009 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef NOVA_TT_SEMAPHORE_BOOST_FALLBACK_HPP
#define NOVA_TT_SEMAPHORE_BOOST_FALLBACK_HPP

#include <boost/noncopyable.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <boost/mpl/if.hpp>

#include <pthread.h>

namespace nova {
namespace nova_tt {

/** semaphore class */
template <bool has_timed_wait = false>
class semaphore:
    boost::noncopyable
{
    typedef typename boost::mpl::if_c<has_timed_wait, boost::timed_mutex, boost::mutex>::type mutex_type;

public:
    semaphore(int i=0):
        m_count(i)
    {}

    /** signal semaphore */
    void post(void)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        ++m_count;
        m_cond.notify_one();
    }

    /** wait until this semaphore is signaled */
    void wait(void)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        while (m_count==0)
            m_cond.wait(lock); /** \todo check! valgrind complains about this */
        --m_count;
    }

    /** try to wait for the semaphore
     *
     * \return true, if the value can be decremented
     *         false, otherweise
     */
    bool try_wait(void)
    {
        bool success = m_mutex.try_lock();
        if (!success)
            return false;
        bool ret;

        if (m_count == 0)
            ret = false;
        else {
            --m_count;
            ret = true;
        }

        m_mutex.unlock();
        return ret;
    }

    /** try to wait for the semaphore until timeout
     *
     * \return true, if the value can be decremented
     *         false, otherweise
     */
    bool timed_wait(struct timespec const & absolute_timeout)
    {
        BOOST_STATIC_ASSERT(has_timed_wait);

        using namespace boost::posix_time;
        const ptime epoch(boost::gregorian::date(1970, 1, 1));

        ptime timeout = epoch + seconds(absolute_timeout.tv_sec) + microsec(absolute_timeout.tv_nsec / 1000);

        bool success = m_mutex.timed_lock(timeout);
        if (success) {
            bool ret;
            if (m_count == 0)
                ret = false;
            else {
                --m_count;
                ret = true;
            }

            m_mutex.unlock();
            return ret;
        } else
            return false;
    }

    int value(void)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_count;
    }

private:
    unsigned int m_count;
    mutex_type m_mutex;
    boost::condition m_cond;
};

} // namespace nova_tt
} // namespace nova

#endif /* NOVA_TT_SEMAPHORE_BOOST_FALLBACK_HPP */
