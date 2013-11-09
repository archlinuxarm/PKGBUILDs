//  cross-platform wrapper for nanosleep
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

/** \file nanosleep.hpp */
/** \namespace nova */


#ifndef NOVA_TT_NANOSLEEP_HPP
#define NOVA_TT_NANOSLEEP_HPP

#include <cassert>

#if defined(unix) || defined(__unix__) || defined(__unix)
# include <unistd.h>
#endif

#if (_POSIX_TIMERS - 0) >= 200112L
#include <time.h>
#endif /* _POSIX_TIMERS */

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

namespace nova
{

namespace detail
{
const unsigned long ns_per_s = 1000000000;

inline void nanosleep(unsigned long sec, unsigned long ns)
{
    assert(ns < ns_per_s);

#if _POSIX_C_SOURCE >= 199309L
    struct timespec timeout, remain;
    timeout.tv_sec = sec;
    timeout.tv_nsec = ns;
    nanosleep(&timeout, &remain);
#else
#ifdef BOOST_DATE_TIME_HAS_NANOSECONDS
    boost::this_thread::sleep(boost::posix_time::seconds(sec) + boost::posix_time::nanoseconds(ns));
#else
    boost::this_thread::sleep(boost::posix_time::seconds(sec) + boost::posix_time::microseconds(ns*0.001));
#endif
#endif
}

} /* namespace detail */

/** sleep for ns nanoseconds
 */
inline void nanosleep(unsigned long ns)
{
    if (ns < detail::ns_per_s)
        detail::nanosleep(0, ns);
    else
        detail::nanosleep(ns/detail::ns_per_s, ns%detail::ns_per_s);
}

}  /* namespace nova */

#endif /* NOVA_TT_NANOSLEEP_HPP */
