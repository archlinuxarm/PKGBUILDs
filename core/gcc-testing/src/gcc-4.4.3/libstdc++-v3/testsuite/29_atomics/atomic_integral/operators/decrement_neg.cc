// { dg-options "-std=gnu++0x" }
// { dg-do compile }
// -*- C++ -*-
 
// Copyright (C) 2008, 2009 Free Software Foundation, Inc.
 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.
 
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
 
// You should have received a copy of the GNU General Public License
// along with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.
 
 
#include <cstdatomic>
#include <testsuite_common_types.h>

int main()
{
  __gnu_test::has_decrement_operators test;
  test.operator()<std::atomic_bool>();
  return 0;
}

// { dg-error "operator" "" { target *-*-* } 375 }
// { dg-error "operator" "" { target *-*-* } 376 }
// { dg-error "operator" "" { target *-*-* } 377 }
// { dg-excess-errors "In file included from" }
