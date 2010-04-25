// Implementation file for the -*- C++ -*- dynamic memory management header.

// Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004,
// 2005, 2006, 2007, 2009
// Free Software Foundation
//
// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#include "new"

const std::nothrow_t std::nothrow = { };

using std::new_handler;
new_handler __new_handler;

new_handler
std::set_new_handler (new_handler handler) throw()
{
  new_handler prev_handler = __new_handler;
  __new_handler = handler;
  return prev_handler;
}

std::bad_alloc::~bad_alloc() throw() { }

const char* 
std::bad_alloc::what() const throw()
{
  return "std::bad_alloc";
}
