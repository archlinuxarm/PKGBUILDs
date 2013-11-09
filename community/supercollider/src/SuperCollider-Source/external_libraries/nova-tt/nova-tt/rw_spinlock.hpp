//  reader-writer spinlock
//  Copyright (C) 2009-2011 Tim Blechmann
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

/** \file rw_spinlock.hpp */

#ifndef RW_SPINLOCK_HPP
#define RW_SPINLOCK_HPP

#include <cassert>
#include <cerrno>


#include "boost/atomic.hpp"
#include <boost/cstdint.hpp>

#include "pause.hpp"

namespace nova {

/** non-recursive reader-writer spinlock, implements a subset of the SharedLockable concept
 *
 *  except for bool timed_lock_shared(boost::system_time  const&  abs_time) all SharedLockable members
 *  are provided
 */
class rw_spinlock
{
    typedef boost::uint32_t uint32_t;

    static const uint32_t unlocked_state = 0;
    static const uint32_t locked_state = 0x80000000;
    static const uint32_t reader_mask  = 0x7fffffff;

public:
    struct scoped_lock
    {
        scoped_lock(rw_spinlock & sl):
            sl_(sl)
        {
            sl_.lock();
        }

        ~scoped_lock(void)
        {
            sl_.unlock();
        }

        rw_spinlock & sl_;
    };

    typedef scoped_lock unique_lock;

    struct shared_lock
    {
        shared_lock(rw_spinlock & sl):
            sl_(sl)
        {
            sl_.lock_shared();
        }

        ~shared_lock(void)
        {
            sl_.unlock_shared();
        }

        rw_spinlock & sl_;
    };

    rw_spinlock(void):
        state(uint32_t(unlocked_state))
    {}

    ~rw_spinlock(void)
    {
        assert(state == unlocked_state);
    }

    void lock(void)
    {
        for (;;) {
            while (state.load(boost::memory_order_relaxed) != unlocked_state)
                detail::pause();

            uint32_t expected = unlocked_state;
            if (state.compare_exchange_weak(expected, locked_state, boost::memory_order_acquire))
                break;
        }
    }

    bool try_lock(void)
    {
        uint32_t expected = unlocked_state;
        if (state.compare_exchange_strong(expected, locked_state, boost::memory_order_acquire))
            return true;
        else
            return false;
    }

    void unlock(void)
    {
        assert(state.load(boost::memory_order_relaxed) == locked_state);
        state.store(unlocked_state, boost::memory_order_release);
    }

    void lock_shared(void)
    {
        for(;;) {
            /* with the mask, the cas will fail, locked exclusively */
            uint32_t current_state    = state.load(boost::memory_order_acquire) & reader_mask;
            const uint32_t next_state = current_state + 1;

            if (state.compare_exchange_weak(current_state, next_state, boost::memory_order_acquire))
                break;
            detail::pause();
        }
    }

    bool try_lock_shared(void)
    {
        /* with the mask, the cas will fail, locked exclusively */
        uint32_t current_state    = state.load(boost::memory_order_acquire) & reader_mask;
        const uint32_t next_state = current_state + 1;

        if (state.compare_exchange_strong(current_state, next_state, boost::memory_order_acquire))
            return true;
        else
            return false;
    }

    void unlock_shared(void)
    {
        for(;;) {
            uint32_t current_state    = state.load(boost::memory_order_relaxed); /* we don't need the reader_mask */
            const uint32_t next_state = current_state - 1;

            if (state.compare_exchange_weak(current_state, uint32_t(next_state)))
                break;
            detail::pause();
        }
    }

private:
    boost::atomic<uint32_t> state;
};

class padded_rw_spinlock:
    public rw_spinlock
{
    static const int padding_bytes = 64 - sizeof(rw_spinlock);
    boost::uint8_t padding[padding_bytes];
};


} /* namespace nova */

#endif /* RW_SPINLOCK_HPP */
