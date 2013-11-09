//  read-write mutex class
//  Copyright (C) 2007, 2009, 2012 Tim Blechmann
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

/** \file rw_mutex.hpp */

#ifndef NOVA_TT_RW_MUTEX_HPP
#define NOVA_TT_RW_MUTEX_HPP

#include <cerrno>
#include <cassert>

#include "boost/thread/shared_mutex.hpp"

#ifdef _WIN32
#include "boost/thread/shared_mutex.hpp"
#endif

#include "pthread.h"
#include "boost/thread/locks.hpp"
#include "branch_hints.hpp"

namespace nova {
namespace nova_tt {

/** non-recursive reader-writer mutex class, implementing a subset of the SharedLockable concept
 *
 *  except for bool timed_lock_shared(boost::system_time  const&  abs_time) all SharedLockable members
 *  are provided
 * */
class nonrecursive_rw_mutex
{
public:
    nonrecursive_rw_mutex(void)
    {
        int status = pthread_rwlock_init(&rwlock, NULL);
        assert(status == 0);
    }

    ~nonrecursive_rw_mutex(void)
    {
        int status = pthread_rwlock_destroy(&rwlock);
        assert(status == 0);
    }

    void lock(void)
    {
        int status = pthread_rwlock_wrlock(&rwlock);
        assert(status == 0);
    }

    bool try_lock(void)
    {
        int status = pthread_rwlock_trywrlock(&rwlock);

        switch (status) {
        case 0:
            return true;

        case EBUSY:
            return false;

        case EDEADLK:
        default:
            assert(false);
            return false;
        }
    }

    /* glibc seems to be buggy ... don't unlock more often than it has been locked
     *
     * http://sourceware.org/bugzilla/show_bug.cgi?id=4825
     */
    void unlock(void)
    {
        int status = pthread_rwlock_unlock(&rwlock);
        assert(status == 0);
    }

    void lock_shared(void)
    {
        int status = pthread_rwlock_rdlock(&rwlock);
        assert(status == 0);
    }

    bool try_lock_shared(void)
    {
        int status = pthread_rwlock_tryrdlock(&rwlock);

        if (status == 0)
            return true;
        if (status == EBUSY)
            return false;

        assert(false);
        return false;
    }

    void unlock_shared(void)
    {
        unlock();
    }

protected:
    pthread_rwlock_t rwlock;

public:
    typedef boost::unique_lock<nonrecursive_rw_mutex> unique_lock;
    typedef boost::shared_lock<nonrecursive_rw_mutex> shared_lock;
};

#ifdef _WIN32

// we cannot use the posix-based rw_mutex, as we cannot access the write_id, so we use boost's implementation instead
typedef boost::shared_mutex rw_mutex;

#else

/** reader-writer mutex class, implementing a subset of the SharedLockable concept
 *
 *  except for bool timed_lock_shared(boost::system_time  const&  abs_time) all SharedLockable members
 *  are provided
 * */
class rw_mutex:
    public nonrecursive_rw_mutex
{
public:
    typedef boost::unique_lock<rw_mutex> unique_lock;
    typedef boost::shared_lock<rw_mutex> shared_lock;

    rw_mutex(void):
        writelock_count(0), writer_id(0)
    {}

    ~rw_mutex(void)
    {
        assert(writer_id == 0);
    }

    void lock(void)
    {
        int status = pthread_rwlock_wrlock(&rwlock);

        switch (status) {
        case 0:
            writer_id = pthread_self();
            assert(writelock_count == 0);

        case EDEADLK:
            assert(writer_id == pthread_self());
            ++writelock_count;
            return;

        default:
            assert(false);
        }
    }

    bool try_lock(void)
    {
        int status = pthread_rwlock_trywrlock(&rwlock);

        switch (status) {
        case 0:
            assert(writer_id == 0);
            assert(writelock_count == 0);
            writer_id = pthread_self();
            ++writelock_count;
            return true;

        case EDEADLK:
            assert(writer_id == pthread_self());

        case EBUSY:
            if (writer_id == pthread_self())
            {
                assert(writelock_count > 0);
                ++writelock_count;
                return true;
            } else {
                assert(writer_id != pthread_self());
                return false;
            }

        default:
            assert(false);
            return false;
        }
    }

    void unlock(void)
    {
        assert(writelock_count > 0);
        assert(writer_id);
        if (--writelock_count == 0) {
            writer_id = 0;
            nonrecursive_rw_mutex::unlock();
        }
    }

    void lock_shared(void)
    {
        if (unlikely(writer_id == pthread_self()))
            /* we're already owning it as writer */
            return;
        nonrecursive_rw_mutex::lock_shared();
    }

    bool try_lock_shared(void)
    {
        if (unlikely(writer_id == pthread_self()))
            /* we're already owning it as writer */
            return true;

        return nonrecursive_rw_mutex::try_lock_shared();
    }

    void unlock_shared(void)
    {
        if (unlikely(writer_id == pthread_self()))
            /* we're owning it as writer */
            return;

        int status = pthread_rwlock_unlock(&rwlock);
        assert(status == 0);
    }


private:
    unsigned int writelock_count;
    pthread_t writer_id; /* id of the writer thread
                          * set during the write lock
                          */
};

#endif

} /* namespace nova-tt */

using nova_tt::nonrecursive_rw_mutex;
using nova_tt::rw_mutex;

} /* namespace nova */

#endif /* NOVA_TT_RW_MUTEX_HPP */
