// 2001-06-14  Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2001, 2002, 2004, 2005, 2009 Free Software Foundation, Inc.
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

// 20.4.1.1 allocator members

#include <memory>
#include <stdexcept>
#include <testsuite_hooks.h>

// libstdc++/8230
void test02()
{
  bool test __attribute__((unused)) = true;
  try 
    {
      std::allocator<int> alloc;
      const std::allocator<int>::size_type n = alloc.max_size();
      int* p = alloc.allocate(n + 1);
      p[n] = 2002;
    } 
  catch(const std::bad_alloc& e) 
    {
      // Allowed.
      test = true;
    }
  catch(...) 
    {
      test = false;
    }
  VERIFY( test );
}

int main()
{
  test02();
  return 0;
}
