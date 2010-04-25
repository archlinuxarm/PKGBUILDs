// { dg-options "-std=gnu++0x" }
// 2007-06-02  Paolo Carlini  <pcarlini@suse.de>
//
// Copyright (C) 2007, 2009 Free Software Foundation, Inc.
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

#include <type_traits>
#include <testsuite_hooks.h>
#include <testsuite_tr1.h>

void test01()
{
  bool test __attribute__((unused)) = true;
  using std::add_rvalue_reference;
  using std::is_same;
  using namespace __gnu_test;

  VERIFY( (is_same<add_rvalue_reference<int>::type, int&&>::value) );
  VERIFY( (is_same<add_rvalue_reference<int&&>::type, int&&>::value) );
  VERIFY( (is_same<add_rvalue_reference<const int>::type, const int&&>::value) );
  VERIFY( (is_same<add_rvalue_reference<int*>::type, int*&&>::value) );
  VERIFY( (is_same<add_rvalue_reference<ClassType&&>::type, ClassType&&>::value) );
  VERIFY( (is_same<add_rvalue_reference<ClassType>::type, ClassType&&>::value) );
  VERIFY( (is_same<add_rvalue_reference<int(int)>::type, int(&&)(int)>::value) );
  VERIFY( (is_same<add_rvalue_reference<void>::type, void>::value) );
  VERIFY( (is_same<add_rvalue_reference<const void>::type, const void>::value) );  
}

int main()
{
  test01();
  return 0;
}
