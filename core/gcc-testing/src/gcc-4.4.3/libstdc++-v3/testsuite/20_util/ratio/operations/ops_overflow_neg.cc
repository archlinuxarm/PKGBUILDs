// { dg-do compile }
// { dg-options "-std=gnu++0x" }
// { dg-require-cstdint "" }

// 2008-07-03 Chris Fairles <chris.fairles@gmail.com>

// Copyright (C) 2008, 2009 Free Software Foundation
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

// You should have received a copy of the GNU General Public License
// along with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <ratio>

void
test01()
{
  std::ratio_add<std::ratio<INTMAX_MAX, 1>, std::ratio<1>>::type r1;
}

void
test02()
{  
  std::ratio_multiply<std::ratio<-INTMAX_MAX, 2>, std::ratio<3, 2>>::type r1;
  std::ratio_multiply<std::ratio<INTMAX_MAX>, std::ratio<INTMAX_MAX>>::type r2;
}

// { dg-error "instantiated from here" "" { target *-*-* } 29 }
// { dg-error "instantiated from here" "" { target *-*-* } 35 }
// { dg-error "instantiated from here" "" { target *-*-* } 36 }
// { dg-error "overflow in addition" "" { target *-*-* } 130 }
// { dg-error "overflow in multiplication" "" { target *-*-* } 98 }
// { dg-error "overflow in multiplication" "" { target *-*-* } 100 }
// { dg-error "overflow in multiplication" "" { target *-*-* } 102 }
// { dg-excess-errors "In instantiation of" }
// { dg-excess-errors "out of range" }
