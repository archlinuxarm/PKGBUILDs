/* User file parser in nss_files module.
   Copyright (C) 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <gshadow.h>

#define STRUCTURE	sgrp
#define ENTNAME		sgent
#define DATABASE	"gshadow"
struct sgent_data {};

/* Our parser function is already defined in sgetspent_r.c, so use that
   to parse lines from the database file.  */
#define EXTERN_PARSER
#include "files-parse.c"
#include GENERIC

DB_LOOKUP (sgnam, 1 + strlen (name), (".%s", name),
	   {
	     if (name[0] != '+' && name[0] != '-'
		 && ! strcmp (name, result->sg_namp))
	       break;
	   }, const char *name)
