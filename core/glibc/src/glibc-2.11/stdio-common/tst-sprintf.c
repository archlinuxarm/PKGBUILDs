#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int
main (void)
{
  char buf[100];
  int result = 0;

  if (sprintf (buf, "%.0ls", L"foo") != 0
      || strlen (buf) != 0)
    {
      puts ("sprintf (buf, \"%.0ls\", L\"foo\") produced some output");
      result = 1;
    }

#define SIZE (1024*70000)
#define STR(x) #x

  char *dst = malloc (SIZE + 1);

  if (dst == NULL)
    {
      puts ("memory allocation failure");
      result = 1;
    }
  else
    {
      sprintf (dst, "%*s", SIZE, "");
      if (strnlen (dst, SIZE + 1) != SIZE)
	{
	  puts ("sprintf (dst, \"%*s\", " STR(SIZE) ", \"\") did not produce enough output");
	  result = 1;
	}
      free (dst);
    }

  if (sprintf (buf, "%1$d%3$.*2$s%4$d", 7, 67108863, "x", 8) != 3
      || strcmp (buf, "7x8") != 0)
    {
      printf ("sprintf (buf, \"%%1$d%%3$.*2$s%%4$d\", 7, 67108863, \"x\", 8) produced `%s' output", buf);
      result = 1;
    }

  if (sprintf (buf, "%67108863.16\"%d", 7) != 14
      || strcmp (buf, "%67108863.16\"7") != 0)
    {
      printf ("sprintf (buf, \"%%67108863.16\\\"%%d\", 7) produced `%s' output", buf);
      result = 1;
    }

  if (sprintf (buf, "%*\"%d", 0x3ffffff, 7) != 11
      || strcmp (buf, "%67108863\"7") != 0)
    {
      printf ("sprintf (buf, \"%%*\\\"%%d\", 0x3ffffff, 7) produced `%s' output", buf);
      result = 1;
    }

  return result;
}
