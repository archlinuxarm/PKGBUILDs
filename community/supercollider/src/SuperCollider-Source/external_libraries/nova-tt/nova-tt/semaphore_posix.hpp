//  semaphore class, posix wrapper
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

#ifndef NOVA_TT_SEMAPHORE_POSIX_HPP
#define NOVA_TT_SEMAPHORE_POSIX_HPP

#include <cassert>
#include <semaphore.h>

#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>


namespace nova {
namespace nova_tt {

/** semaphore class */
template <bool has_timed_wait = false>
class semaphore:
    boost::noncopyable
{
public:
    semaphore(unsigned int i=0)
    {
        sem_init(&sem, 0, i);
    }

    ~semaphore(void)
    {
        sem_destroy(&sem);
    }

    /** signal semaphore */
    void post(void)
    {
        int status = sem_post(&sem);
        assert(status == 0);
    }

    /** wait until this semaphore is signaled */
    void wait(void)
    {
        int status = sem_wait(&sem);
        assert(status == 0);
    }

    /** try to wait for the semaphore
     *
     * \return true, if the value can be decremented
     *         false, otherweise
     */
    bool try_wait(void)
    {
        int status = sem_trywait(&sem);
        return status == 0;
    }

    /** try to wait for the semaphore until timeout
     *
     * \return true, if the value can be decremented
     *         false, otherweise
     */
    bool timed_wait(struct timespec const & absolute_timeout)
    {
        BOOST_STATIC_ASSERT(has_timed_wait);
        int status = sem_timedwait(&sem, &absolute_timeout);
        return status == 0;
    }

    int value(void)
    {
        int ret;
        bool status = sem_getvalue(&sem, &ret);
        assert(status == 0);

        if (ret < 0)
            return 0;
        else
            return ret;
    }

private:
    sem_t sem;
};

} // namespace nova_tt
} // namespace nova

#endif /* NOVA_TT_SEMAPHORE_POSIX_HPP */
