//  cross-platform wrapper for nanosleep
//  Copyright (C) 2010 Tim Blechmann
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

#ifndef NOVA_TT_THREAD_PRITORITY_HPP
#define NOVA_TT_THREAD_PRITORITY_HPP

#if defined(unix) || defined(__unix__) || defined(__unix)
# include <unistd.h>
#endif


#if (_POSIX_PRIORITY_SCHEDULING - 0) >=  200112L || (_POSIX_MEMLOCK - 0) >=  200112L
#include "thread_priority_pthread.hpp"
#elif (__APPLE__)
#include "thread_priority_mach.hpp"
#else
#include "thread_priority_fallback.hpp"
#endif

#endif /* NOVA_TT_THREAD_PRITORITY_HPP */
