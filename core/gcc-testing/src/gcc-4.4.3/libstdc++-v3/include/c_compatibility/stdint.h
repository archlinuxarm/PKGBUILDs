// -*- C++ -*- compatibility header.

// Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file stdint.h
 *  This is a Standard C++ Library header.
 */

#ifndef _GLIBCXX_STDINT_H
#define _GLIBCXX_STDINT_H 1

#pragma GCC system_header

#include <bits/c++config.h>

#ifdef __GXX_EXPERIMENTAL_CXX0X__

// For 8.22.1/1 (see C99, Notes 219, 220, 222)
# if _GLIBCXX_HAVE_STDINT_H
#  ifndef __STDC_LIMIT_MACROS
#   define _UNDEF__STDC_LIMIT_MACROS
#   define __STDC_LIMIT_MACROS
#  endif
#  ifndef __STDC_CONSTANT_MACROS
#   define _UNDEF__STDC_CONSTANT_MACROS
#   define __STDC_CONSTANT_MACROS
#  endif
#  include_next <stdint.h>
#  ifdef _UNDEF__STDC_LIMIT_MACROS
#   undef __STDC_LIMIT_MACROS
#   undef _UNDEF__STDC_LIMIT_MACROS
#  endif
#  ifdef _UNDEF__STDC_CONSTANT_MACROS
#   undef __STDC_CONSTANT_MACROS
#   undef _UNDEF__STDC_CONSTANT_MACROS
#  endif
# endif

# if defined(_GLIBCXX_INCLUDE_AS_TR1)
#  error C++0x header cannot be included from TR1 header
# endif
# if defined(_GLIBCXX_INCLUDE_AS_CXX0X)
#  include <tr1_impl/cstdint>
# else
#  define _GLIBCXX_INCLUDE_AS_CXX0X
#  define _GLIBCXX_BEGIN_NAMESPACE_TR1
#  define _GLIBCXX_END_NAMESPACE_TR1
#  define _GLIBCXX_TR1
#  include <tr1_impl/cstdint>
#  undef _GLIBCXX_TR1
#  undef _GLIBCXX_END_NAMESPACE_TR1
#  undef _GLIBCXX_BEGIN_NAMESPACE_TR1
#  undef _GLIBCXX_INCLUDE_AS_CXX0X
# endif

#else

# if _GLIBCXX_HAVE_STDINT_H
#  include_next <stdint.h>
# endif

#endif // __GXX_EXPERIMENTAL_CXX0X__

#endif // _GLIBCXX_STDINT_H
