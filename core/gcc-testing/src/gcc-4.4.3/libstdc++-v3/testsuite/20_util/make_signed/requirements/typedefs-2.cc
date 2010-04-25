// { dg-options "-std=gnu++0x -funsigned-char -fshort-enums" }
// { dg-options "-std=gnu++0x -funsigned-char -fshort-enums -Wl,--no-enum-size-warning" { target arm*-*-linux*eabi } }

// 2007-05-03  Benjamin Kosnik  <bkoz@redhat.com>
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

// Ensure that this enum has "short" as its underlying type.
enum test_enum { first_selection = ((unsigned char)-1) + 1 };

void test01()
{
  bool test __attribute__((unused)) = true;
  using std::make_signed;
  using std::is_same;

  // Positive tests.
  typedef make_signed<const int>::type  	test2_type;
  VERIFY( (is_same<test2_type, const int>::value) );

  typedef make_signed<const unsigned int>::type  	test21c_type;
  VERIFY( (is_same<test21c_type, const signed int>::value) );

  typedef make_signed<volatile unsigned int>::type  	test21v_type;
  VERIFY( (is_same<test21v_type, volatile signed int>::value) );

  typedef make_signed<const volatile unsigned int>::type  	test21cv_type;
  VERIFY( (is_same<test21cv_type, const volatile signed int>::value) );

  typedef make_signed<const char>::type  	test22_type;
  VERIFY( (is_same<test22_type, const signed char>::value) );

#ifdef _GLIBCXX_USE_WCHAR_T
  typedef make_signed<volatile wchar_t>::type  	test23_type;
  VERIFY( (is_same<test23_type, volatile signed wchar_t>::value) );
#endif

  typedef make_signed<test_enum>::type  	test25_type;
  VERIFY( (is_same<test25_type, short>::value) );
}

int main()
{
  test01();
  return 0;
}
