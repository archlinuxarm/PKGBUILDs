//
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

#ifndef NOVA_TT_THREAD_PRITORITY_FALLBACK_HPP
#define NOVA_TT_THREAD_PRITORITY_FALLBACK_HPP

#include <utility>

namespace nova
{

inline int thread_priority(void)
{
    return 0;
}

inline std::pair<int, int> thread_priority_interval(void)
{
    return std::make_pair(0, 0);
}

inline bool thread_set_priority(int priority)
{
    return false;
}

}  /* namespace nova */

#undef USE_PTHREAD

#endif /* NOVA_TT_THREAD_PRITORITY_FALLBACK_HPP */
