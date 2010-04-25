// Explicit instantiation file.

// Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2009
// Free Software Foundation, Inc.
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

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

//
// ISO C++ 14882:
//

#include <string>
#include <istream>
#include <ostream>
#include <ext/stdio_sync_filebuf.h>

_GLIBCXX_BEGIN_NAMESPACE(std)

  // string related to iostreams
  template 
    basic_istream<char>& 
    operator>>(basic_istream<char>&, string&);
  template 
    basic_ostream<char>& 
    operator<<(basic_ostream<char>&, const string&);
  template 
    basic_istream<char>& 
    getline(basic_istream<char>&, string&, char);
  template 
    basic_istream<char>& 
    getline(basic_istream<char>&, string&);
#ifdef _GLIBCXX_USE_WCHAR_T
  template 
    basic_istream<wchar_t>& 
    operator>>(basic_istream<wchar_t>&, wstring&);
  template 
    basic_ostream<wchar_t>& 
    operator<<(basic_ostream<wchar_t>&, const wstring&);
  template 
    basic_istream<wchar_t>& 
    getline(basic_istream<wchar_t>&, wstring&, wchar_t);
  template 
    basic_istream<wchar_t>& 
    getline(basic_istream<wchar_t>&, wstring&);
#endif

_GLIBCXX_END_NAMESPACE

_GLIBCXX_BEGIN_NAMESPACE(__gnu_cxx)

  template class stdio_sync_filebuf<char>;
#ifdef _GLIBCXX_USE_WCHAR_T
  template class stdio_sync_filebuf<wchar_t>;
#endif

_GLIBCXX_END_NAMESPACE


