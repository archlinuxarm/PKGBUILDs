m4_include(../config/acx.m4)
m4_include(../config/no-executables.m4)

dnl Check that we have a working GNU Fortran compiler
AC_DEFUN([LIBGFOR_WORKING_GFORTRAN], [
AC_MSG_CHECKING([whether the GNU Fortran compiler is working])
AC_LANG_PUSH([Fortran])
AC_COMPILE_IFELSE([[
      program foo
      real, parameter :: bar = sin (12.34 / 2.5)
      end program foo]],
    [AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])
     AC_MSG_ERROR([GNU Fortran is not working; please report a bug in http://gcc.gnu.org/bugzilla, attaching $PWD/config.log])
    ])
AC_LANG_POP([Fortran])
])


sinclude(../libtool.m4)
dnl The lines below arrange for aclocal not to bring an installed
dnl libtool.m4 into aclocal.m4, while still arranging for automake to
dnl add a definition of LIBTOOL to Makefile.in.
ifelse(,,,[AC_SUBST(LIBTOOL)
AC_DEFUN([AM_PROG_LIBTOOL])
AC_DEFUN([AC_LIBTOOL_DLOPEN])
AC_DEFUN([AC_PROG_LD])
])

dnl Check whether the target supports hidden visibility.
AC_DEFUN([LIBGFOR_CHECK_ATTRIBUTE_VISIBILITY], [
  AC_CACHE_CHECK([whether the target supports hidden visibility],
		 libgfor_cv_have_attribute_visibility, [
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror"
  AC_TRY_COMPILE([void __attribute__((visibility("hidden"))) foo(void) { }],
		 [], libgfor_cv_have_attribute_visibility=yes,
		 libgfor_cv_have_attribute_visibility=no)
  CFLAGS="$save_CFLAGS"])
  if test $libgfor_cv_have_attribute_visibility = yes; then
    AC_DEFINE(HAVE_ATTRIBUTE_VISIBILITY, 1,
      [Define to 1 if the target supports __attribute__((visibility(...))).])
  fi])

dnl Check whether the target supports dllexport
AC_DEFUN([LIBGFOR_CHECK_ATTRIBUTE_DLLEXPORT], [
  AC_CACHE_CHECK([whether the target supports dllexport],
		 libgfor_cv_have_attribute_dllexport, [
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Werror"
  AC_TRY_COMPILE([void __attribute__((dllexport)) foo(void) { }],
		 [], libgfor_cv_have_attribute_dllexport=yes,
		 libgfor_cv_have_attribute_dllexport=no)
  CFLAGS="$save_CFLAGS"])
  if test $libgfor_cv_have_attribute_dllexport = yes; then
    AC_DEFINE(HAVE_ATTRIBUTE_DLLEXPORT, 1,
      [Define to 1 if the target supports __attribute__((dllexport)).])
  fi])

dnl Check whether the target supports symbol aliases.
AC_DEFUN([LIBGFOR_CHECK_ATTRIBUTE_ALIAS], [
  AC_CACHE_CHECK([whether the target supports symbol aliases],
		 libgfor_cv_have_attribute_alias, [
  AC_TRY_LINK([
void foo(void) { }
extern void bar(void) __attribute__((alias("foo")));],
    [bar();], libgfor_cv_have_attribute_alias=yes, libgfor_cv_have_attribute_alias=no)])
  if test $libgfor_cv_have_attribute_alias = yes; then
    AC_DEFINE(HAVE_ATTRIBUTE_ALIAS, 1,
      [Define to 1 if the target supports __attribute__((alias(...))).])
  fi])

dnl Check whether the target supports __sync_fetch_and_add.
AC_DEFUN([LIBGFOR_CHECK_SYNC_FETCH_AND_ADD], [
  AC_CACHE_CHECK([whether the target supports __sync_fetch_and_add],
		 libgfor_cv_have_sync_fetch_and_add, [
  AC_TRY_LINK([int foovar = 0;], [
if (foovar <= 0) return __sync_fetch_and_add (&foovar, 1);
if (foovar > 10) return __sync_add_and_fetch (&foovar, -1);],
	      libgfor_cv_have_sync_fetch_and_add=yes, libgfor_cv_have_sync_fetch_and_add=no)])
  if test $libgfor_cv_have_sync_fetch_and_add = yes; then
    AC_DEFINE(HAVE_SYNC_FETCH_AND_ADD, 1,
	      [Define to 1 if the target supports __sync_fetch_and_add])
  fi])

dnl Check if threads are supported.
AC_DEFUN([LIBGFOR_CHECK_GTHR_DEFAULT], [
  AC_CACHE_CHECK([configured target thread model],
		 libgfor_cv_target_thread_file, [
libgfor_cv_target_thread_file=`$CC -v 2>&1 | sed -n 's/^Thread model: //p'`])

  if test $libgfor_cv_target_thread_file != single; then
    AC_DEFINE(HAVE_GTHR_DEFAULT, 1,
	      [Define if the compiler has a thread header that is non single.])
  fi])

dnl Check for pragma weak.
AC_DEFUN([LIBGFOR_GTHREAD_WEAK], [
  AC_CACHE_CHECK([whether pragma weak works],
		 libgfor_cv_have_pragma_weak, [
  gfor_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -Wunknown-pragmas"
  AC_TRY_COMPILE([void foo (void);
#pragma weak foo], [if (foo) foo ();],
		 libgfor_cv_have_pragma_weak=yes, libgfor_cv_have_pragma_weak=no)])
  if test $libgfor_cv_have_pragma_weak = yes; then
    AC_DEFINE(SUPPORTS_WEAK, 1,
	      [Define to 1 if the target supports #pragma weak])
  fi
  case "$host" in
    *-*-darwin* | *-*-hpux* | *-*-cygwin* | *-*-mingw* )
      AC_DEFINE(GTHREAD_USE_WEAK, 0,
		[Define to 0 if the target shouldn't use #pragma weak])
      ;;
  esac])

dnl Check whether target can unlink a file still open.
AC_DEFUN([LIBGFOR_CHECK_UNLINK_OPEN_FILE], [
  AC_CACHE_CHECK([whether the target can unlink an open file],
                  libgfor_cv_have_unlink_open_file, [
  AC_TRY_RUN([
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main ()
{
  int fd;

  fd = open ("testfile", O_RDWR | O_CREAT, S_IWRITE | S_IREAD);
  if (fd <= 0)
    return 0;
  if (unlink ("testfile") == -1)
    return 1;
  write (fd, "This is a test\n", 15);
  close (fd);

  if (open ("testfile", O_RDONLY, S_IWRITE | S_IREAD) == -1 && errno == ENOENT)
    return 0;
  else
    return 1;
}], libgfor_cv_have_unlink_open_file=yes, libgfor_cv_have_unlink_open_file=no, [
case "${target}" in
  *mingw*) libgfor_cv_have_unlink_open_file=no ;;
  *) libgfor_cv_have_unlink_open_file=yes;;
esac])])
if test x"$libgfor_cv_have_unlink_open_file" = xyes; then
  AC_DEFINE(HAVE_UNLINK_OPEN_FILE, 1, [Define if target can unlink open files.])
fi])

dnl Check whether CRLF is the line terminator
AC_DEFUN([LIBGFOR_CHECK_CRLF], [
  AC_CACHE_CHECK([whether the target has CRLF as line terminator],
                  libgfor_cv_have_crlf, [
  AC_TRY_RUN([
/* This test program should exit with status 0 if system uses a CRLF as
   line terminator, and status 1 otherwise.  
   Since it is used to check for mingw systems, and should return 0 in any
   other case, in case of a failure we will not use CRLF.  */
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

int main ()
{
#ifndef O_BINARY
  exit(1);
#else
  int fd, bytes;
  char buff[5];

  fd = open ("foo", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if (fd < 0)
    exit(1);
  if (write (fd, "\n", 1) < 0)
    perror ("write");
  
  close (fd);
  
  if ((fd = open ("foo", O_RDONLY | O_BINARY, S_IRWXU)) < 0)
    exit(1);
  bytes = read (fd, buff, 5);
  if (bytes == 2 && buff[0] == '\r' && buff[1] == '\n')
    exit(0);
  else
    exit(1);
#endif
}], libgfor_cv_have_crlf=yes, libgfor_cv_have_crlf=no, [
case "${target}" in
  *mingw*) libgfor_cv_have_crlf=yes ;;
  *) libgfor_cv_have_crlf=no;;
esac])])
if test x"$libgfor_cv_have_crlf" = xyes; then
  AC_DEFINE(HAVE_CRLF, 1, [Define if CRLF is line terminator.])
fi])

dnl Check whether isfinite is broken.
dnl The most common problem is that it does not work on long doubles.
AC_DEFUN([LIBGFOR_CHECK_FOR_BROKEN_ISFINITE], [
  AC_CACHE_CHECK([whether isfinite is broken],
                  libgfor_cv_have_broken_isfinite, [
  libgfor_check_for_broken_isfinite_save_LIBS=$LIBS
  LIBS="$LIBS -lm"
  AC_TRY_RUN([
#include <math.h>
#include <float.h>
int main ()
{
#ifdef isfinite
#ifdef LDBL_MAX
  if (!isfinite(LDBL_MAX)) return 1;
#endif
#ifdef DBL_MAX
  if (!isfinite(DBL_MAX)) return 1;
#endif
#endif
return 0;
}], libgfor_cv_have_broken_isfinite=no, libgfor_cv_have_broken_isfinite=yes, [
case "${target}" in
  hppa*-*-hpux*) libgfor_cv_have_broken_isfinite=yes ;;
  *) libgfor_cv_have_broken_isfinite=no ;;
esac])]
  LIBS=$libgfor_check_for_broken_isfinite_save_LIBS)
if test x"$libgfor_cv_have_broken_isfinite" = xyes; then
  AC_DEFINE(HAVE_BROKEN_ISFINITE, 1, [Define if isfinite is broken.])
fi])

dnl Check whether isnan is broken.
dnl The most common problem is that it does not work on long doubles.
AC_DEFUN([LIBGFOR_CHECK_FOR_BROKEN_ISNAN], [
  AC_CACHE_CHECK([whether isnan is broken],
                  libgfor_cv_have_broken_isnan, [
  libgfor_check_for_broken_isnan_save_LIBS=$LIBS
  LIBS="$LIBS -lm"
  AC_TRY_RUN([
#include <math.h>
#include <float.h>
int main ()
{
#ifdef isnan
#ifdef LDBL_MAX
  {
    long double x;
    x = __builtin_nanl ("");
    if (!isnan(x)) return 1;
    if (isnan(LDBL_MAX)) return 1;
#ifdef NAN
    x = (long double) NAN;
    if (!isnan(x)) return 1;
#endif
  }
#endif
#ifdef DBL_MAX
  {
    double y;
    y = __builtin_nan ("");
    if (!isnan(y)) return 1;
    if (isnan(DBL_MAX)) return 1;
#ifdef NAN
    y = (double) NAN;
    if (!isnan(y)) return 1;
#endif
  }
#endif
#endif
return 0;
}], libgfor_cv_have_broken_isnan=no, libgfor_cv_have_broken_isnan=yes, [
case "${target}" in
  hppa*-*-hpux*) libgfor_cv_have_broken_isnan=yes ;;
  *) libgfor_cv_have_broken_isnan=no ;;
esac])]
  LIBS=$libgfor_check_for_broken_isnan_save_LIBS)
if test x"$libgfor_cv_have_broken_isnan" = xyes; then
  AC_DEFINE(HAVE_BROKEN_ISNAN, 1, [Define if isnan is broken.])
fi])

dnl Check whether fpclassify is broken.
dnl The most common problem is that it does not work on long doubles.
AC_DEFUN([LIBGFOR_CHECK_FOR_BROKEN_FPCLASSIFY], [
  AC_CACHE_CHECK([whether fpclassify is broken],
                  libgfor_cv_have_broken_fpclassify, [
  libgfor_check_for_broken_fpclassify_save_LIBS=$LIBS
  LIBS="$LIBS -lm"
  AC_TRY_RUN([
#include <math.h>
#include <float.h>
int main ()
{
#ifdef fpclassify
#ifdef LDBL_MAX
        if (fpclassify(LDBL_MAX) == FP_NAN
            || fpclassify(LDBL_MAX) == FP_INFINITE) return 1;
#endif
#ifdef DBL_MAX
        if (fpclassify(DBL_MAX) == FP_NAN
            || fpclassify(DBL_MAX) == FP_INFINITE) return 1;
#endif
#endif
return 0;
}], libgfor_cv_have_broken_fpclassify=no, libgfor_cv_have_broken_fpclassify=yes, [
case "${target}" in
  hppa*-*-hpux*) libgfor_cv_have_broken_fpclassify=yes ;;
  *) libgfor_cv_have_broken_fpclassify=no ;;
esac])]
  LIBS=$libgfor_check_for_broken_fpclassify_save_LIBS)
if test x"$libgfor_cv_have_broken_fpclassify" = xyes; then
  AC_DEFINE(HAVE_BROKEN_FPCLASSIFY, 1, [Define if fpclassify is broken.])
fi])

dnl Check whether the st_ino and st_dev stat fields taken together uniquely
dnl identify the file within the system. This is should be true for POSIX
dnl systems; it is known to be false on mingw32.
AC_DEFUN([LIBGFOR_CHECK_WORKING_STAT], [
  AC_CACHE_CHECK([whether the target stat is reliable],
                  libgfor_cv_have_working_stat, [
  AC_TRY_RUN([
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main ()
{ 
  FILE *f, *g;
  struct stat st1, st2;

  f = fopen ("foo", "w");
  g = fopen ("bar", "w");
  if (stat ("foo", &st1) != 0 || stat ("bar", &st2))
    return 1;
  if (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino)
    return 1;
  fclose(f);
  fclose(g);
  return 0;
}], libgfor_cv_have_working_stat=yes, libgfor_cv_have_working_stat=no, [
case "${target}" in
  *mingw*) libgfor_cv_have_working_stat=no ;;
  *) libgfor_cv_have_working_stat=yes;;
esac])])
if test x"$libgfor_cv_have_working_stat" = xyes; then
  AC_DEFINE(HAVE_WORKING_STAT, 1, [Define if target has a reliable stat.])
fi])

dnl Checks for fpsetmask function.
AC_DEFUN([LIBGFOR_CHECK_FPSETMASK], [
  AC_CACHE_CHECK([whether fpsetmask is present], libgfor_cv_have_fpsetmask, [
    AC_TRY_LINK([
#if HAVE_FLOATINGPOINT_H
# include <floatingpoint.h>
#endif /* HAVE_FLOATINGPOINT_H */
#if HAVE_IEEEFP_H
# include <ieeefp.h>
#endif /* HAVE_IEEEFP_H */],[fpsetmask(0);],
    eval "libgfor_cv_have_fpsetmask=yes", eval "libgfor_cv_have_fpsetmask=no")
  ])
  if test x"$libgfor_cv_have_fpsetmask" = xyes; then
    AC_DEFINE(HAVE_FPSETMASK, 1, [Define if you have fpsetmask.])
  fi
])

dnl Check whether we have a mingw that provides a __mingw_snprintf function
AC_DEFUN([LIBGFOR_CHECK_MINGW_SNPRINTF], [
  AC_CACHE_CHECK([whether __mingw_snprintf is present], libgfor_cv_have_mingw_snprintf, [
    AC_TRY_LINK([
#include <stdio.h>
extern int __mingw_snprintf (char *, size_t, const char *, ...);
],[
__mingw_snprintf (NULL, 0, "%d\n", 1);
],
    eval "libgfor_cv_have_mingw_snprintf=yes", eval "libgfor_cv_have_mingw_snprintf=no")
  ])
  if test x"$libgfor_cv_have_mingw_snprintf" = xyes; then
    AC_DEFINE(HAVE_MINGW_SNPRINTF, 1, [Define if you have __mingw_snprintf.])
  fi
])

dnl Check whether we have a broken powf implementation
AC_DEFUN([LIBGFOR_CHECK_FOR_BROKEN_POWF], [
  AC_CACHE_CHECK([whether powf is broken], libgfor_cv_have_broken_powf, [
case "${target}" in
  hppa*64*-*-hpux*) libgfor_cv_have_broken_powf=yes ;;
  *) libgfor_cv_have_broken_powf=no;;
esac])
  if test x"$libgfor_cv_have_broken_powf" = xyes; then
    AC_DEFINE(HAVE_BROKEN_POWF, 1, [Define if powf is broken.])
  fi
])
