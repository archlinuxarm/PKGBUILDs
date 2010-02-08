/* Copyright (c) 1997, 1998, 1999, 2004, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Thorsten Kukuk <kukuk@suse.de>, 1997.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpcsvc/nis.h>
#include "nis_xdr.h"

typedef bool_t (*iofct_t) (XDR *, void *);
typedef void (*freefct_t) (void *);


static void *
read_nis_obj (const char *name, iofct_t readfct, freefct_t freefct,
	      size_t objsize)
{
  FILE *in = fopen (name, "rc");
  if (in == NULL)
    return NULL;

  void *obj = calloc (1, objsize);

  if (obj != NULL)
    {
      XDR xdrs;
      xdrstdio_create (&xdrs, in, XDR_DECODE);
      bool_t status = readfct (&xdrs, obj);
      xdr_destroy (&xdrs);

      if (!status)
	{
	  freefct (obj);
	  obj = NULL;
	}
    }

  fclose (in);

  return obj;
}

static bool_t
write_nis_obj (const char *name, const void *obj, iofct_t writefct)
{
  FILE *out = fopen (name, "w");
  if (out == NULL)
    return FALSE;

  XDR xdrs;
  xdrstdio_create (&xdrs, out, XDR_ENCODE);
  bool_t status = writefct (&xdrs, (void *) obj);
  xdr_destroy (&xdrs);
  fclose (out);

  return status;
}


static const char cold_start_file[] = "/var/nis/NIS_COLD_START";

directory_obj *
readColdStartFile (void)
{
  return read_nis_obj (cold_start_file, (iofct_t) _xdr_directory_obj,
		       (freefct_t) nis_free_directory, sizeof (directory_obj));
}
libnsl_hidden_def (readColdStartFile)

bool_t
writeColdStartFile (const directory_obj *obj)
{
  return write_nis_obj (cold_start_file, obj, (iofct_t) _xdr_directory_obj);
}

nis_object *
nis_read_obj (const char *name)
{
  return read_nis_obj (name, (iofct_t) _xdr_nis_object,
		       (freefct_t) nis_free_object, sizeof (nis_object));
}

bool_t
nis_write_obj (const char *name, const nis_object *obj)
{
  return write_nis_obj (name, obj, (iofct_t) _xdr_nis_object);
}
