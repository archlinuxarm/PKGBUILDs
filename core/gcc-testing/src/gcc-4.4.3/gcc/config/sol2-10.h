/* Operating system specific defines to be used when targeting GCC for any
   Solaris 2 system starting from Solaris 10.
   Copyright 2006, 2007 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* Solaris 10 has the float and long double forms of math functions.  */
#undef TARGET_C99_FUNCTIONS
#define TARGET_C99_FUNCTIONS 1
