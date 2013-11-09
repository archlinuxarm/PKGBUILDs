//  cross-platform wrapper for memory locking
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

/** \file mlock.hpp */

#ifndef NOVA_TT_MLOCK_HPP
#define NOVA_TT_MLOCK_HPP

#if defined(unix) || defined(__unix__) || defined(__unix)
# include <unistd.h>
#endif

#ifdef _POSIX_MEMLOCK_RANGE
#include <sys/mman.h>
#else

inline int mlock(const void * addr, size_t len)
{
    return 0;
}

inline int munlock(const void * addr, size_t len)
{
    return 0;
}

#endif /* _POSIX_MEMLOCK_RANGE */

#endif /* NOVA_TT_MLOCK_HPP */
