//  semaphore class, boost.thread wrapper
//  based on API proposed in N2043
//
//  Copyright (C) 2011 Tim Blechmann
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

#ifndef NOVA_TT_SEMAPHORE_PTHREADS_HPP
#define NOVA_TT_SEMAPHORE_PTHREADS_HPP

#include <boost/noncopyable.hpp>

#include <pthread.h>

namespace nova {
namespace nova_tt {

/** semaphore class */
template <bool has_timed_wait = false>
class semaphore:
    boost::noncopyable
{
    struct scoped_lock
    {
        scoped_lock(pthread_mutex_t & mutex):
            mutex(mutex)
        {
            pthread_mutex_lock(&mutex);
        }

        ~scoped_lock(void)
        {
            pthread_mutex_unlock(&mutex);
        }

        pthread_mutex_t & mutex;
    };

public:
    semaphore(int i=0):
        m_count(i)
    {
        pthread_mutex_init (&m_mutex, NULL);
        pthread_cond_init (&m_cond, NULL);
    }

    ~semaphore(void)
    {
        pthread_mutex_destroy (&m_mutex);
        pthread_cond_destroy (&m_cond);
    }

    /** signal semaphore */
    void post(void)
    {
        scoped_lock lock(m_mutex);
        ++m_count;
        pthread_cond_signal(&m_cond);
    }

    /** wait until this semaphore is signaled */
    void wait(void)
    {
        scoped_lock lock(m_mutex);
        while (m_count==0)
            pthread_cond_wait(&m_cond, &m_mutex);
        --m_count;
    }

    /** try to wait for the semaphore
     *
     * \return true, if the value can be decremented
     *         false, otherweise
     */
    bool try_wait(void)
    {
        int status = pthread_mutex_trylock(&m_mutex);
        if (status != 0)
            return false;

        bool ret;
        if (m_count == 0)
            ret = false;
        else {
            --m_count;
            ret = true;
        }

        pthread_mutex_unlock(&m_mutex);
        return ret;
    }

    /** try to wait for the semaphore until timeout
     *
     * \return true, if the value can be decremented
     *         false, otherweise
     */
    bool timed_wait(struct timespec const & absolute_timeout)
    {
        int status = pthread_mutex_timedlock (&m_mutex, &absolute_timeout);
        if (status)
            return false;

        while (m_count < 1) {
            int status = pthread_cond_timedwait (&m_cond, &m_mutex, &absolute_timeout);
            if (status) {
                pthread_mutex_unlock (&m_mutex);
                return false;
            }
        }
        m_count--;
        pthread_mutex_unlock (&m_mutex);
        return true;
    }

    int value(void)
    {
        scoped_lock lock(m_mutex);
        return m_count;
    }

private:
    unsigned int m_count;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

} // namespace nova_tt
} // namespace nova

#endif /* NOVA_TT_SEMAPHORE_PTHREADS_HPP */
