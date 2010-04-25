// { dg-do run { xfail { ! { *-*-linux* } } } }
// { dg-require-namedlocale "" }

// Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2009
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

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.


// 22.2.1.3.2 ctype<char> members

#include <locale>
#include <vector>
#include <testsuite_hooks.h>

typedef char char_type;

// libstdc++/4456, libstdc++/4457, libstdc++/4458
void test02()
{
  using namespace std;
  typedef ctype_base::mask	mask;
  typedef vector<mask> 		vector_type;

  bool test __attribute__((unused)) = true;

  //  const int max = numeric_limits<char>::max();
  const int max = 255;
  const int ctype_mask_max = 10;
  vector_type v_c(max);
  vector_type v_de(max);

  // "C"
  locale loc_c = locale::classic();
  const ctype<char>& ctype_c = use_facet<ctype<char> >(loc_c); 
  for (int i = 0; i < max; ++i)
    {
      char_type c = static_cast<char>(i);
      mask mask_test = static_cast<mask>(0);
      mask mask_is = static_cast<mask>(0);
      for (int j = 0; j <= ctype_mask_max; ++j)
	{
	  mask_test = static_cast<mask>(1 << j);
	  if (ctype_c.is(mask_test, c))
	    mask_is |= mask_test;
	}
      v_c[i] = mask_is;
    }   

  // "de_DE"
  locale loc_de = locale("de_DE");
  const ctype<char>& ctype_de = use_facet<ctype<char> >(loc_de); 
  for (int i = 0; i < max; ++i)
    {
      char_type c = static_cast<char>(i);
      mask mask_test = static_cast<mask>(0);
      mask mask_is = static_cast<mask>(0);
      for (int j = 0; j <= ctype_mask_max; ++j)
	{
	  mask_test = static_cast<mask>(1 << j);
	  if (ctype_de.is(mask_test, c))
	    mask_is |= mask_test;
	}
      v_de[i] = mask_is;
    }   

#if QUANNUM_VERBOSE_LYRICALLY_ADEPT_BAY_AREA_MCS_MODE
    for (int i = 0; i < max; ++i)
    {
      char_type mark = v_c[i] == v_de[i] ? ' ' : '-';
      cout << i << ' ' << mark << ' ' << static_cast<char>(i) << '\t' ;
      cout << "v_c: " << setw(4) << v_c[i] << '\t';
      cout << "v_de: " << setw(4) << v_de[i] << endl;
    }
    cout << (v_c == v_de) << endl;
#endif

  VERIFY( v_c != v_de );
}

int main() 
{
  test02();
  return 0;
}
