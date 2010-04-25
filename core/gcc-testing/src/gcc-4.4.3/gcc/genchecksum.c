/* Generate checksums of executables for PCH validation
   Copyright (C) 2005, 2007
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "bconfig.h"
#include "system.h"
#include "md5.h"

static void
usage (void)
{
  fputs ("Usage: genchecksums <filename>\n", stderr);
}

static void
dosum (const char *file)
{
  FILE *f;
  unsigned char result[16];
  int i;
  
  f = fopen (file, "rb");
  if (!f)
    {
      fprintf (stderr, "opening %s: %s\n", file, xstrerror (errno));
      exit (1);
    }
  
  /* Some executable formats have timestamps in the first 16 bytes, yuck.  */
  if (fseek (f, 16, SEEK_SET) != 0)
     {
      fprintf (stderr, "seeking in %s: %s\n", file, xstrerror (errno));
      exit (1);
    }
  
  if (md5_stream (f, result) != 0
      || fclose (f) != 0)
     {
      fprintf (stderr, "reading %s: %s\n", file, xstrerror (errno));
      exit (1);
    }

  fputs ("const unsigned char executable_checksum[16] = { ", stdout);
  for (i = 0; i < 16; i++)
    printf ("%#02x%s", result[i], i == 15 ? " };\n" : ", ");
}

int
main (int argc, char ** argv)
{
  if (argc != 2)
    {
      usage ();
      return 1;
    }

  dosum (argv[1]);

  return 0;
}
