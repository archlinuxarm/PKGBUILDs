//  semaphore class
//  based on API proposed in N2043
//
//  Copyright (C) 2009 Tim Blechmann
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

/** \file semaphore.hpp */

#ifndef NOVA_TT_SEMAPHORE_HPP
#define NOVA_TT_SEMAPHORE_HPP

#if defined(unix) || defined(__unix__) || defined(__unix)
# include <unistd.h>
#endif

#if (_POSIX_SEMAPHORES - 0) >= 200112L
#include "semaphore_posix.hpp"

// mach semaphores seem to be broken
// #elif defined(__APPLE__)
// #include "semaphore_mach.hpp"

#elif defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS - 200112L) >= 0L
#include "semaphore_pthreads.hpp"

#else
#include "semaphore_boost_fallback.hpp"
#endif

namespace nova {

typedef nova_tt::semaphore<false> semaphore;
typedef nova_tt::semaphore<true>  timed_semaphore;

/** helper class for semaphore synchronization
 *
 *  destructor will wait for the semaphore to be signaled
 */
template <typename Semaphore>
struct semaphore_sync
{
    semaphore_sync(Semaphore & sem):
        sem(sem)
    {}

    ~semaphore_sync(void)
    {
        sem.wait();
    }

    Semaphore & sem;
};

} /* namespace nova */

#endif /* NOVA_TT_SEMAPHORE_HPP */
