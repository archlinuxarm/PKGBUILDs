/* Conversion from and to IBM1390.
   Copyright (C) 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Masahide Washizawa <washi@jp.ibm.com>, 2005.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#define DATA_HEADER	"ibm1390.h"
#define CHARSET_NAME	"IBM1390//"
#define FROM_LOOP	from_ibm1390
#define TO_LOOP		to_ibm1390
#define SB_TO_UCS4	__ibm1390sb_to_ucs4
#define DB_TO_UCS4_IDX	__ibm1390db_to_ucs4_idx
#define DB_TO_UCS4	__ibm1390db_to_ucs4
#define UCS4_TO_SB_IDX	__ucs4_to_ibm1390sb_idx
#define UCS4_TO_SB	__ucs4_to_ibm1390sb
#define UCS4_TO_DB_IDX	__ucs4_to_ibm1390db_idx
#define UCS4_TO_DB	__ucs4_to_ibm1390db
#define DB_TO_UCS4_COMB	__ibm1390db_to_ucs4_combined
#define UCS4_COMB_TO_DB	__ucs4_combined_to_ibm1390db
#define UCS_LIMIT	0xffffffff

#include "ibm1364.c"
