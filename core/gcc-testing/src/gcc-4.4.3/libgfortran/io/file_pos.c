/* Copyright (C) 2002-2003, 2005, 2006, 2007, 2009 Free Software Foundation, Inc.
   Contributed by Andy Vaught and Janne Blomqvist

This file is part of the GNU Fortran runtime library (libgfortran).

Libgfortran is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

Libgfortran is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#include "io.h"
#include <string.h>

/* file_pos.c-- Implement the file positioning statements, i.e. BACKSPACE,
   ENDFILE, and REWIND as well as the FLUSH statement.  */


/* formatted_backspace(fpp, u)-- Move the file back one line.  The
   current position is after the newline that terminates the previous
   record, and we have to sift backwards to find the newline before
   that or the start of the file, whichever comes first.  */

static const int READ_CHUNK = 4096;

static void
formatted_backspace (st_parameter_filepos *fpp, gfc_unit *u)
{
  gfc_offset base;
  char p[READ_CHUNK];
  ssize_t n;

  base = stell (u->s) - 1;

  do
    {
      n = (base < READ_CHUNK) ? base : READ_CHUNK;
      base -= n;
      if (sseek (u->s, base, SEEK_SET) < 0)
        goto io_error;
      if (sread (u->s, p, n) != n)
	goto io_error;

      /* We have moved backwards from the current position, it should
         not be possible to get a short read.  Because it is not
         clear what to do about such thing, we ignore the possibility.  */

      /* There is no memrchr() in the C library, so we have to do it
         ourselves.  */

      while (n > 0)
	{
          n--;
	  if (p[n] == '\n')
	    {
	      base += n + 1;
	      goto done;
	    }
	}

    }
  while (base != 0);

  /* base is the new pointer.  Seek to it exactly.  */
 done:
  if (sseek (u->s, base, SEEK_SET) < 0)
    goto io_error;
  u->last_record--;
  u->endfile = NO_ENDFILE;

  return;

 io_error:
  generate_error (&fpp->common, LIBERROR_OS, NULL);
}


/* unformatted_backspace(fpp) -- Move the file backwards for an unformatted
   sequential file.  We are guaranteed to be between records on entry and 
   we have to shift to the previous record.  Loop over subrecords.  */

static void
unformatted_backspace (st_parameter_filepos *fpp, gfc_unit *u)
{
  gfc_offset m, slen;
  GFC_INTEGER_4 m4;
  GFC_INTEGER_8 m8;
  ssize_t length;
  int continued;
  char p[sizeof (GFC_INTEGER_8)];

  if (compile_options.record_marker == 0)
    length = sizeof (GFC_INTEGER_4);
  else
    length = compile_options.record_marker;

  do
    {
      slen = - (gfc_offset) length;
      if (sseek (u->s, slen, SEEK_CUR) < 0)
        goto io_error;
      if (sread (u->s, p, length) != length)
        goto io_error;

      /* Only GFC_CONVERT_NATIVE and GFC_CONVERT_SWAP are valid here.  */
      if (likely (u->flags.convert == GFC_CONVERT_NATIVE))
	{
	  switch (length)
	    {
	    case sizeof(GFC_INTEGER_4):
	      memcpy (&m4, p, sizeof (m4));
	      m = m4;
	      break;

	    case sizeof(GFC_INTEGER_8):
	      memcpy (&m8, p, sizeof (m8));
	      m = m8;
	      break;

	    default:
	      runtime_error ("Illegal value for record marker");
	      break;
	    }
	}
      else
	{
	  switch (length)
	    {
	    case sizeof(GFC_INTEGER_4):
	      reverse_memcpy (&m4, p, sizeof (m4));
	      m = m4;
	      break;

	    case sizeof(GFC_INTEGER_8):
	      reverse_memcpy (&m8, p, sizeof (m8));
	      m = m8;
	      break;

	    default:
	      runtime_error ("Illegal value for record marker");
	      break;
	    }

	}

      continued = m < 0;
      if (continued)
	m = -m;

      if (sseek (u->s, -m -2 * length, SEEK_CUR) < 0)
	goto io_error;
    } while (continued);

  u->last_record--;
  return;

 io_error:
  generate_error (&fpp->common, LIBERROR_OS, NULL);
}


extern void st_backspace (st_parameter_filepos *);
export_proto(st_backspace);

void
st_backspace (st_parameter_filepos *fpp)
{
  gfc_unit *u;

  library_start (&fpp->common);

  u = find_unit (fpp->common.unit);
  if (u == NULL)
    {
      generate_error (&fpp->common, LIBERROR_BAD_UNIT, NULL);
      goto done;
    }

  /* Direct access is prohibited, and so is unformatted stream access.  */


  if (u->flags.access == ACCESS_DIRECT)
    {
      generate_error (&fpp->common, LIBERROR_OPTION_CONFLICT,
		      "Cannot BACKSPACE a file opened for DIRECT access");
      goto done;
    }

  if (u->flags.access == ACCESS_STREAM && u->flags.form == FORM_UNFORMATTED)
    {
      generate_error (&fpp->common, LIBERROR_OPTION_CONFLICT,
                      "Cannot BACKSPACE an unformatted stream file");
      goto done;
    }

  /* Make sure format buffer is flushed and reset.  */
  if (u->flags.form == FORM_FORMATTED)
    {
      int pos = fbuf_reset (u);
      if (pos != 0)
        sseek (u->s, pos, SEEK_CUR);
    }

  
  /* Check for special cases involving the ENDFILE record first.  */

  if (u->endfile == AFTER_ENDFILE)
    {
      u->endfile = AT_ENDFILE;
      u->flags.position = POSITION_APPEND;
      sflush (u->s);
    }
  else
    {
      if (stell (u->s) == 0)
	{
	  u->flags.position = POSITION_REWIND;
	  goto done;		/* Common special case */
	}

      if (u->mode == WRITING)
	{
	  /* If there are previously written bytes from a write with
	     ADVANCE="no", add a record marker before performing the
	     BACKSPACE.  */

	  if (u->previous_nonadvancing_write)
	    finish_last_advance_record (u);

	  u->previous_nonadvancing_write = 0;

	  unit_truncate (u, stell (u->s), &fpp->common);
	  u->mode = READING;
        }

      if (u->flags.form == FORM_FORMATTED)
	formatted_backspace (fpp, u);
      else
	unformatted_backspace (fpp, u);

      u->flags.position = POSITION_UNSPECIFIED;
      u->endfile = NO_ENDFILE;
      u->current_record = 0;
      u->bytes_left = 0;
    }

 done:
  if (u != NULL)
    unlock_unit (u);

  library_end ();
}


extern void st_endfile (st_parameter_filepos *);
export_proto(st_endfile);

void
st_endfile (st_parameter_filepos *fpp)
{
  gfc_unit *u;

  library_start (&fpp->common);

  u = find_unit (fpp->common.unit);
  if (u != NULL)
    {
      if (u->flags.access == ACCESS_DIRECT)
	{
	  generate_error (&fpp->common, LIBERROR_OPTION_CONFLICT,
			  "Cannot perform ENDFILE on a file opened"
			  " for DIRECT access");
	  goto done;
	}

      /* If there are previously written bytes from a write with ADVANCE="no",
	 add a record marker before performing the ENDFILE.  */

      if (u->previous_nonadvancing_write)
	finish_last_advance_record (u);

      u->previous_nonadvancing_write = 0;

      if (u->current_record)
	{
	  st_parameter_dt dtp;
	  dtp.common = fpp->common;
	  memset (&dtp.u.p, 0, sizeof (dtp.u.p));
	  dtp.u.p.current_unit = u;
	  next_record (&dtp, 1);
	}

      unit_truncate (u, stell (u->s), &fpp->common);
      u->endfile = AFTER_ENDFILE;
      if (0 == stell (u->s))
        u->flags.position = POSITION_REWIND;
    done:
      unlock_unit (u);
    }

  library_end ();
}


extern void st_rewind (st_parameter_filepos *);
export_proto(st_rewind);

void
st_rewind (st_parameter_filepos *fpp)
{
  gfc_unit *u;

  library_start (&fpp->common);

  u = find_unit (fpp->common.unit);
  if (u != NULL)
    {
      if (u->flags.access == ACCESS_DIRECT)
	generate_error (&fpp->common, LIBERROR_BAD_OPTION,
			"Cannot REWIND a file opened for DIRECT access");
      else
	{
	  /* If there are previously written bytes from a write with ADVANCE="no",
	     add a record marker before performing the ENDFILE.  */

	  if (u->previous_nonadvancing_write)
	    finish_last_advance_record (u);

	  u->previous_nonadvancing_write = 0;

	  fbuf_reset (u);

	  u->last_record = 0;

	  if (sseek (u->s, 0, SEEK_SET) < 0)
	    generate_error (&fpp->common, LIBERROR_OS, NULL);

	  /* Handle special files like /dev/null differently.  */
	  if (!is_special (u->s))
	    {
	      /* We are rewinding so we are not at the end.  */
	      u->endfile = NO_ENDFILE;
	    }
	  else
	    {
	      /* Set this for compatibilty with g77 for /dev/null.  */
	      if (file_length (u->s) == 0  && stell (u->s) == 0)
		u->endfile = AT_ENDFILE;
	      /* Future refinements on special files can go here.  */
	    }

	  u->current_record = 0;
	  u->strm_pos = 1;
	  u->read_bad = 0;
	}
      /* Update position for INQUIRE.  */
      u->flags.position = POSITION_REWIND;
      unlock_unit (u);
    }

  library_end ();
}


extern void st_flush (st_parameter_filepos *);
export_proto(st_flush);

void
st_flush (st_parameter_filepos *fpp)
{
  gfc_unit *u;

  library_start (&fpp->common);

  u = find_unit (fpp->common.unit);
  if (u != NULL)
    {
      /* Make sure format buffer is flushed.  */
      if (u->flags.form == FORM_FORMATTED)
        fbuf_flush (u, u->mode);

      sflush (u->s);
      unlock_unit (u);
    }
  else
    /* FLUSH on unconnected unit is illegal: F95 std., 9.3.5. */ 
    generate_error (&fpp->common, LIBERROR_BAD_OPTION,
			"Specified UNIT in FLUSH is not connected");

  library_end ();
}
