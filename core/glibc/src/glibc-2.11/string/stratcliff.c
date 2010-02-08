/* Test for string function add boundaries of usable memory.
   Copyright (C) 1996,1997,1999-2003,2007, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#define _GNU_SOURCE 1

/* Make sure we don't test the optimized inline functions if we want to
   test the real implementation.  */
#undef __USE_STRING_INLINES

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/param.h>

#ifndef CHAR
# define L(c) c
# define CHAR char
# define MEMSET memset
# define STRLEN strlen
# define STRNLEN strnlen
# define STRCHR strchr
# define STRRCHR strrchr
# define STRCPY strcpy
# define STRNCPY strncpy
# define MEMCMP memcmp
# define STPCPY stpcpy
# define STPNCPY stpncpy
# define MEMCPY memcpy
# define MEMPCPY mempcpy
# define MEMCHR memchr
#endif


#define STRINGIFY(s) STRINGIFY2 (s)
#define STRINGIFY2(s) #s


static int
do_test (void)
{
  int size = sysconf (_SC_PAGESIZE);
  int nchars = size / sizeof (CHAR);
  CHAR *adr;
  CHAR *dest;
  int result = 0;

  adr = (CHAR *) mmap (NULL, 3 * size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANON, -1, 0);
  dest = (CHAR *) mmap (NULL, 3 * size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANON, -1, 0);
  if (adr == MAP_FAILED || dest == MAP_FAILED)
    {
      if (errno == ENOSYS)
        puts ("No test, mmap not available.");
      else
        {
          printf ("mmap failed: %m");
          result = 1;
        }
    }
  else
    {
      int inner, middle, outer;

      mprotect (adr, size, PROT_NONE);
      mprotect (adr + 2 * nchars, size, PROT_NONE);
      adr += nchars;

      mprotect (dest, size, PROT_NONE);
      mprotect (dest + 2 * nchars, size, PROT_NONE);
      dest += nchars;

      MEMSET (adr, L('T'), nchars);

      /* strlen/wcslen test */
      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
          for (inner = MAX (outer, nchars - 64); inner < nchars; ++inner)
	    {
	      adr[inner] = L('\0');

	      if (STRLEN (&adr[outer]) != (size_t) (inner - outer))
		{
		  printf ("%s flunked for outer = %d, inner = %d\n",
			  STRINGIFY (STRLEN), outer, inner);
		  result = 1;
		}

	      adr[inner] = L('T');
	    }
        }

      /* strnlen/wcsnlen test */
      for (outer = nchars; outer >= MAX (0, nchars - 128); --outer)
        {
          for (inner = MAX (outer, nchars - 64); inner < nchars; ++inner)
	    {
	      adr[inner] = L('\0');

	      if (STRNLEN (&adr[outer], inner - outer + 1)
		  != (size_t) (inner - outer))
		{
		  printf ("%s flunked for outer = %d, inner = %d\n",
			  STRINGIFY (STRNLEN), outer, inner);
		  result = 1;
		}

	      adr[inner] = L('T');
	    }
        }
      for (outer = nchars; outer >= MAX (0, nchars - 128); --outer)
        {
	  for (inner = MAX (outer, nchars - 64); inner <= nchars; ++inner)
	    {
	      if (STRNLEN (&adr[outer], inner - outer)
		  != (size_t) (inner - outer))
		{
		  printf ("%s flunked bounded for outer = %d, inner = %d\n",
			  STRINGIFY (STRNLEN), outer, inner);
		  result = 1;
		}
	    }
        }

      /* strchr/wcschr test */
      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
	  for (middle = MAX (outer, nchars - 64); middle < nchars; ++middle)
	    {
	      for (inner = middle; inner < nchars; ++inner)
		{
		  adr[middle] = L('V');
		  adr[inner] = L('\0');

		  CHAR *cp = STRCHR (&adr[outer], L('V'));

		  if ((inner == middle && cp != NULL)
		      || (inner != middle
			  && (cp - &adr[outer]) != middle - outer))
		    {
		      printf ("%s flunked for outer = %d, middle = %d, "
			      "inner = %d\n",
			      STRINGIFY (STRCHR), outer, middle, inner);
		      result = 1;
		    }

		  adr[inner] = L('T');
		  adr[middle] = L('T');
		}
	    }
        }

      /* Special test.  */
      adr[nchars - 1] = L('\0');
      if (STRCHR (&adr[nchars - 1], L('\n')) != NULL)
	{
	  printf ("%s flunked test of empty string at end of page\n",
		  STRINGIFY (STRCHR));
	  result = 1;
	}

      /* strrchr/wcsrchr test */
      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
	  for (middle = MAX (outer, nchars - 64); middle < nchars; ++middle)
	    {
	      for (inner = middle; inner < nchars; ++inner)
		{
		  adr[middle] = L('V');
		  adr[inner] = L('\0');

		  CHAR *cp = STRRCHR (&adr[outer], L('V'));

		  if ((inner == middle && cp != NULL)
		      || (inner != middle
			  && (cp - &adr[outer]) != middle - outer))
		    {
		      printf ("%s flunked for outer = %d, middle = %d, "
			      "inner = %d\n",
			      STRINGIFY (STRRCHR), outer, middle, inner);
		      result = 1;
		    }

		  adr[inner] = L('T');
		  adr[middle] = L('T');
		}
	    }
        }

      /* memchr test */
      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
	  for (middle = MAX (outer, nchars - 64); middle < nchars; ++middle)
	    {
	      adr[middle] = L('V');

	      CHAR *cp = MEMCHR (&adr[outer], L('V'), 3 * size);

	      if (cp - &adr[outer] != middle - outer)
		{
		  printf ("%s flunked for outer = %d, middle = %d\n",
			  STRINGIFY (MEMCHR), outer, middle);
		  result = 1;
		}

	      adr[middle] = L('T');
	    }
        }
      for (outer = nchars; outer >= MAX (0, nchars - 128); --outer)
        {
	  CHAR *cp = MEMCHR (&adr[outer], L('V'), nchars - outer);

	  if (cp != NULL)
	    {
	      printf ("%s flunked for outer = %d\n",
		      STRINGIFY (MEMCHR), outer);
	      result = 1;
	    }
        }

      /* This function only exists for single-byte characters.  */
#ifndef WCSTEST
      /* rawmemchr test */
      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
	  for (middle = MAX (outer, nchars - 64); middle < nchars; ++middle)
	    {
	      adr[middle] = L('V');

	      CHAR *cp = rawmemchr (&adr[outer], L('V'));

	      if (cp - &adr[outer] != middle - outer)
		{
		  printf ("%s flunked for outer = %d, middle = %d\n",
			  STRINGIFY (rawmemchr), outer, middle);
		  result = 1;
		}

	      adr[middle] = L('T');
	    }
        }
#endif

      /* strcpy/wcscpy test */
      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
          for (inner = MAX (outer, nchars - 64); inner < nchars; ++inner)
	    {
	      adr[inner] = L('\0');

	      if (STRCPY (dest, &adr[outer]) != dest
		  || STRLEN (dest) != (size_t) (inner - outer))
		{
		  printf ("%s flunked for outer = %d, inner = %d\n",
			  STRINGIFY (STRCPY), outer, inner);
		  result = 1;
		}

	      adr[inner] = L('T');
	    }
        }

      /* strncpy/wcsncpy tests */
      adr[nchars - 1] = L('T');
      for (outer = nchars; outer >= MAX (0, nchars - 128); --outer)
	{
	  size_t len;

	  for (len = 0; len < nchars - outer; ++len)
	    {
	      if (STRNCPY (dest, &adr[outer], len) != dest
		  || MEMCMP (dest, &adr[outer], len) != 0)
		{
		  printf ("outer %s flunked for outer = %d, len = %Zd\n",
			  STRINGIFY (STRNCPY), outer, len);
		  result = 1;
		}
	    }
        }
      adr[nchars - 1] = L('\0');

      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
          for (inner = MAX (outer, nchars - 64); inner < nchars; ++inner)
	    {
	      size_t len;

	      adr[inner] = L('\0');

	      for (len = 0; len < nchars - outer + 64; ++len)
		{
		  if (STRNCPY (dest, &adr[outer], len) != dest
		      || MEMCMP (dest, &adr[outer],
				 MIN (inner - outer, len)) != 0
		      || (inner - outer < len
			  && STRLEN (dest) != (inner - outer)))
		    {
		      printf ("%s flunked for outer = %d, inner = %d, "
			      "len = %Zd\n",
			      STRINGIFY (STRNCPY), outer, inner, len);
		      result = 1;
		    }
		  if (STRNCPY (dest + 1, &adr[outer], len) != dest + 1
		      || MEMCMP (dest + 1, &adr[outer],
				 MIN (inner - outer, len)) != 0
		      || (inner - outer < len
			  && STRLEN (dest + 1) != (inner - outer)))
		    {
		      printf ("%s+1 flunked for outer = %d, inner = %d, "
			      "len = %Zd\n",
			      STRINGIFY (STRNCPY), outer, inner, len);
		      result = 1;
		    }
		}

	      adr[inner] = L('T');
	    }
        }

      /* stpcpy/wcpcpy test */
      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
          for (inner = MAX (outer, nchars - 64); inner < nchars; ++inner)
	    {
	      adr[inner] = L('\0');

	      if ((STPCPY (dest, &adr[outer]) - dest) != inner - outer)
		{
		  printf ("%s flunked for outer = %d, inner = %d\n",
			  STRINGIFY (STPCPY), outer, inner);
		  result = 1;
		}

	      adr[inner] = L('T');
	    }
        }

      /* stpncpy/wcpncpy test */
      adr[nchars - 1] = L('T');
      for (outer = nchars; outer >= MAX (0, nchars - 128); --outer)
	{
	  size_t len;

	  for (len = 0; len < nchars - outer; ++len)
	    {
	      if (STPNCPY (dest, &adr[outer], len) != dest + len
		  || MEMCMP (dest, &adr[outer], len) != 0)
		{
		  printf ("outer %s flunked for outer = %d, len = %Zd\n",
			  STRINGIFY (STPNCPY), outer, len);
		  result = 1;
		}
	    }
	}
      adr[nchars - 1] = L('\0');

      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
        {
          for (middle = MAX (outer, nchars - 64); middle < nchars; ++middle)
	    {
	      adr[middle] = L('\0');

	      for (inner = 0; inner < nchars - outer; ++ inner)
		{
		  if ((STPNCPY (dest, &adr[outer], inner) - dest)
		      != MIN (inner, middle - outer))
		    {
		      printf ("%s flunked for outer = %d, middle = %d, "
			      "inner = %d\n",
			      STRINGIFY (STPNCPY), outer, middle, inner);
		      result = 1;
		    }
		}

	      adr[middle] = L('T');
	    }
        }

      /* memcpy/wmemcpy test */
      for (outer = nchars; outer >= MAX (0, nchars - 128); --outer)
	for (inner = 0; inner < nchars - outer; ++inner)
	  if (MEMCPY (dest, &adr[outer], inner) !=  dest)
	    {
	      printf ("%s flunked for outer = %d, inner = %d\n",
		      STRINGIFY (MEMCPY), outer, inner);
	      result = 1;
	    }

      /* mempcpy/wmempcpy test */
      for (outer = nchars; outer >= MAX (0, nchars - 128); --outer)
	for (inner = 0; inner < nchars - outer; ++inner)
	  if (MEMPCPY (dest, &adr[outer], inner) !=  dest + inner)
	    {
	      printf ("%s flunked for outer = %d, inner = %d\n",
		      STRINGIFY (MEMPCPY), outer, inner);
	      result = 1;
	    }

      /* This function only exists for single-byte characters.  */
#ifndef WCSTEST
      /* memccpy test */
      memset (adr, '\0', nchars);
      for (outer = nchars; outer >= MAX (0, nchars - 128); --outer)
	for (inner = 0; inner < nchars - outer; ++inner)
	  if (memccpy (dest, &adr[outer], L('\1'), inner) != NULL)
	    {
	      printf ("memccpy flunked full copy for outer = %d, inner = %d\n",
		      outer, inner);
	      result = 1;
	    }
      for (outer = nchars - 1; outer >= MAX (0, nchars - 128); --outer)
	for (middle = 0; middle < nchars - outer; ++middle)
	  {
	    memset (dest, L('\2'), middle + 1);
	    for (inner = 0; inner < middle; ++inner)
	      {
		adr[outer + inner] = L('\1');

		if (memccpy (dest, &adr[outer], '\1', middle + 128)
		    !=  dest + inner + 1)
		  {
		    printf ("\
memccpy flunked partial copy for outer = %d, middle = %d, inner = %d\n",
			    outer, middle, inner);
		    result = 1;
		  }
		else if (dest[inner + 1] != L('\2'))
		  {
		    printf ("\
memccpy copied too much for outer = %d, middle = %d, inner = %d\n",
			    outer, middle, inner);
		    result = 1;
		  }
		adr[outer + inner] = L('\0');
	      }
	  }
#endif
    }

  return result;
}

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"
