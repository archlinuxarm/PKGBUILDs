// Copyright (C) 2002, 2003, 2004, 2005, 2009 Free Software Foundation
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

// 27.4.2.1 - Types [lib.ios.types]

#include <ios>
#include <testsuite_hooks.h>

// libstdc++/7219
// Annex D, deprecated.
void test01()
{
  typedef std::ios_base::streampos streampos_type;
  typedef std::ios_base::streamoff streamoff_type;
}

int main(void)
{
  test01();
  return 0;
}
