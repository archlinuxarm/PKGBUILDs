// 2001-11-25  Phil Edwards  <pme@gcc.gnu.org>
//
// Copyright (C) 2001, 2003, 2004, 2009 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

// 20.4.1.1 allocator members

#include <cstdlib>
#include <ext/mt_allocator.h>
#include <testsuite_hooks.h>
#include <testsuite_allocator.h>

using __gnu_cxx::__mt_alloc;

void* 
operator new(std::size_t n) throw(std::bad_alloc)
{
  new_called = true;
  return std::malloc(n);
}

void
operator delete(void* v) throw()
{
  delete_called = true;
  return std::free(v);
}

void test01() 
{ 
  bool test __attribute__((unused)) = true;
  typedef __mt_alloc<unsigned int> allocator_type;
  VERIFY( bool(__gnu_test::check_delete<allocator_type, false>()) ); 
}

int main()
{
  test01();
  return 0;
}
