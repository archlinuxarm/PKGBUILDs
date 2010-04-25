// 2001-05-23 Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2001, 2002, 2003, 2009 Free Software Foundation, Inc.
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

// 27.7.3.2 member functions (ostringstream_members)

#include <sstream>
#include <testsuite_hooks.h>

void test01()
{
  bool test __attribute__((unused)) = true;
  std::ostringstream os01;
  const std::string str00; 
  const std::string str01 = "123";
  std::string str02;

  std::ios_base::iostate statefail, stateeof;
  statefail = std::ios_base::failbit;
  stateeof = std::ios_base::eofbit;

  // string str() const
  str02 = os01.str();
  VERIFY( str00 == str02 );

  // void str(const basic_string&)
  os01.str(str01);
  str02 = os01.str();
  VERIFY( str01 == str02 );
}

int main()
{
  test01();
  return 0;
}
