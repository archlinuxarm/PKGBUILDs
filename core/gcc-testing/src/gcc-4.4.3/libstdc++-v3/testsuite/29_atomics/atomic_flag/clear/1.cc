// { dg-options "-std=gnu++0x" }

// Copyright (C) 2009 Free Software Foundation, Inc.
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

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <cstdatomic>
#include <testsuite_hooks.h>

int main()
{
  bool test __attribute__((unused)) = true;
  std::atomic_flag f = ATOMIC_FLAG_INIT;

  f.clear(); // set to false
  VERIFY( false == f.test_and_set() ); // return previous false, set to true
  VERIFY( true == f.test_and_set() ); // return true

  return 0;
}
