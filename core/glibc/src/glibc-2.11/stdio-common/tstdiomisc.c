#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static int
t1 (void)
{
  int n = -1;
  sscanf ("abc  ", "abc %n", &n);
  printf ("t1: count=%d\n", n);

  return n != 5;
}

static int
t2 (void)
{
  int result = 0;
  int n;
  long N;
  int retval;
#define SCAN(INPUT, FORMAT, VAR, EXP_RES, EXP_VAL) \
  VAR = -1; \
  retval = sscanf (INPUT, FORMAT, &VAR); \
  printf ("sscanf (\"%s\", \"%s\", &x) => %d, x = %ld\n", \
	  INPUT, FORMAT, retval, (long int) VAR); \
  result |= retval != EXP_RES || VAR != EXP_VAL

  SCAN ("12345", "%ld", N, 1, 12345);
  SCAN ("12345", "%llllld", N, 0, -1);
  SCAN ("12345", "%LLLLLd", N, 0, -1);
  SCAN ("test ", "%*s%n",  n, 0, 4);
  SCAN ("test ", "%2*s%n",  n, 0, -1);
  SCAN ("12 ",   "%l2d",  n, 0, -1);
  SCAN ("12 ",   "%2ld",  N, 1, 12);

  n = -1;
  N = -1;
  retval = sscanf ("1 1", "%d %Z", &n, &N);
  printf ("sscanf (\"1 1\", \"%%d %%Z\", &n, &N) => %d, n = %d, N = %ld\n", \
	  retval, n, N); \
  result |= retval != 1 || n != 1 || N != -1;

  return result;
}

volatile double nanval;
volatile double infval;
volatile long double lnanval;
volatile long double linfval;


static int
F (void)
{
  char buf[80];
  wchar_t wbuf[40];
  int result;

  nanval = NAN;

  snprintf (buf, sizeof buf, "%a %A %e %E %f %F %g %G",
	    nanval, nanval, nanval, nanval, nanval, nanval, nanval, nanval);
  result = strcmp (buf, "nan NAN nan NAN nan NAN nan NAN") != 0;
  printf ("expected \"nan NAN nan NAN nan NAN nan NAN\", got \"%s\"\n", buf);

  snprintf (buf, sizeof buf, "%a %A %e %E %f %F %g %G",
	    -nanval, -nanval, -nanval, -nanval,
	    -nanval, -nanval, -nanval, -nanval);
  result = strcmp (buf, "-nan -NAN -nan -NAN -nan -NAN -nan -NAN") != 0;
  printf ("expected \"-nan -NAN -nan -NAN -nan -NAN -nan -NAN\", got \"%s\"\n",
	  buf);

  infval = DBL_MAX * DBL_MAX;

  snprintf (buf, sizeof buf, "%a %A %e %E %f %F %g %G",
	    infval, infval, infval, infval, infval, infval, infval, infval);
  result |= strcmp (buf, "inf INF inf INF inf INF inf INF") != 0;
  printf ("expected \"inf INF inf INF inf INF inf INF\", got \"%s\"\n", buf);

  snprintf (buf, sizeof buf, "%a %A %e %E %f %F %g %G",
	    -infval, -infval, -infval, -infval,
	    -infval, -infval, -infval, -infval);
  result |= strcmp (buf, "-inf -INF -inf -INF -inf -INF -inf -INF") != 0;
  printf ("expected \"-inf -INF -inf -INF -inf -INF -inf -INF\", got \"%s\"\n",
	  buf);

  swprintf (wbuf, sizeof wbuf / sizeof (wbuf[0]), L"%a %A %e %E %f %F %g %G",
	    nanval, nanval, nanval, nanval, nanval, nanval, nanval, nanval);
  result |= wcscmp (wbuf, L"nan NAN nan NAN nan NAN nan NAN") != 0;
  printf ("expected L\"nan NAN nan NAN nan NAN nan NAN\", got L\"%S\"\n", wbuf);

  swprintf (wbuf, sizeof wbuf / sizeof (wbuf[0]), L"%a %A %e %E %f %F %g %G",
	    -nanval, -nanval, -nanval, -nanval,
	    -nanval, -nanval, -nanval, -nanval);
  result |= wcscmp (wbuf, L"-nan -NAN -nan -NAN -nan -NAN -nan -NAN") != 0;
  printf ("expected L\"-nan -NAN -nan -NAN -nan -NAN -nan -NAN\", got L\"%S\"\n",
	  wbuf);

  swprintf (wbuf, sizeof wbuf / sizeof (wbuf[0]), L"%a %A %e %E %f %F %g %G",
	    infval, infval, infval, infval, infval, infval, infval, infval);
  result |= wcscmp (wbuf, L"inf INF inf INF inf INF inf INF") != 0;
  printf ("expected L\"inf INF inf INF inf INF inf INF\", got L\"%S\"\n", wbuf);

  swprintf (wbuf, sizeof wbuf / sizeof (wbuf[0]), L"%a %A %e %E %f %F %g %G",
	    -infval, -infval, -infval, -infval,
	    -infval, -infval, -infval, -infval);
  result |= wcscmp (wbuf, L"-inf -INF -inf -INF -inf -INF -inf -INF") != 0;
  printf ("expected L\"-inf -INF -inf -INF -inf -INF -inf -INF\", got L\"%S\"\n",
	  wbuf);

  lnanval = NAN;

  snprintf (buf, sizeof buf, "%La %LA %Le %LE %Lf %LF %Lg %LG",
	    lnanval, lnanval, lnanval, lnanval,
	    lnanval, lnanval, lnanval, lnanval);
  result = strcmp (buf, "nan NAN nan NAN nan NAN nan NAN") != 0;
  printf ("expected \"nan NAN nan NAN nan NAN nan NAN\", got \"%s\"\n", buf);

  snprintf (buf, sizeof buf, "%La %LA %Le %LE %Lf %LF %Lg %LG",
	    -lnanval, -lnanval, -lnanval, -lnanval,
	    -lnanval, -lnanval, -lnanval, -lnanval);
  result = strcmp (buf, "-nan -NAN -nan -NAN -nan -NAN -nan -NAN") != 0;
  printf ("expected \"-nan -NAN -nan -NAN -nan -NAN -nan -NAN\", got \"%s\"\n",
	  buf);

  linfval = LDBL_MAX * LDBL_MAX;

  snprintf (buf, sizeof buf, "%La %LA %Le %LE %Lf %LF %Lg %LG",
	    linfval, linfval, linfval, linfval,
	    linfval, linfval, linfval, linfval);
  result |= strcmp (buf, "inf INF inf INF inf INF inf INF") != 0;
  printf ("expected \"inf INF inf INF inf INF inf INF\", got \"%s\"\n", buf);

  snprintf (buf, sizeof buf, "%La %LA %Le %LE %Lf %LF %Lg %LG",
	    -linfval, -linfval, -linfval, -linfval,
	    -linfval, -linfval, -linfval, -linfval);
  result |= strcmp (buf, "-inf -INF -inf -INF -inf -INF -inf -INF") != 0;
  printf ("expected \"-inf -INF -inf -INF -inf -INF -inf -INF\", got \"%s\"\n",
	  buf);

  swprintf (wbuf, sizeof wbuf / sizeof (wbuf[0]),
	    L"%La %LA %Le %LE %Lf %LF %Lg %LG",
	    lnanval, lnanval, lnanval, lnanval,
	    lnanval, lnanval, lnanval, lnanval);
  result |= wcscmp (wbuf, L"nan NAN nan NAN nan NAN nan NAN") != 0;
  printf ("expected L\"nan NAN nan NAN nan NAN nan NAN\", got L\"%S\"\n", wbuf);

  swprintf (wbuf, sizeof wbuf / sizeof (wbuf[0]),
	    L"%La %LA %Le %LE %Lf %LF %Lg %LG",
	    -lnanval, -lnanval, -lnanval, -lnanval,
	    -lnanval, -lnanval, -lnanval, -lnanval);
  result |= wcscmp (wbuf, L"-nan -NAN -nan -NAN -nan -NAN -nan -NAN") != 0;
  printf ("expected L\"-nan -NAN -nan -NAN -nan -NAN -nan -NAN\", got L\"%S\"\n",
	  wbuf);

  swprintf (wbuf, sizeof wbuf / sizeof (wbuf[0]),
	    L"%La %LA %Le %LE %Lf %LF %Lg %LG",
	    linfval, linfval, linfval, linfval,
	    linfval, linfval, linfval, linfval);
  result |= wcscmp (wbuf, L"inf INF inf INF inf INF inf INF") != 0;
  printf ("expected L\"inf INF inf INF inf INF inf INF\", got L\"%S\"\n", wbuf);

  swprintf (wbuf, sizeof wbuf / sizeof (wbuf[0]),
	    L"%La %LA %Le %LE %Lf %LF %Lg %LG",
	    -linfval, -linfval, -linfval, -linfval,
	    -linfval, -linfval, -linfval, -linfval);
  result |= wcscmp (wbuf, L"-inf -INF -inf -INF -inf -INF -inf -INF") != 0;
  printf ("expected L\"-inf -INF -inf -INF -inf -INF -inf -INF\", got L\"%S\"\n",
	  wbuf);

  return result;
}

int
main (int argc, char *argv[])
{
  int result = 0;

  result |= t1 ();
  result |= t2 ();
  result |= F ();

  result |= fflush (stdout) == EOF;

  return result;
}
