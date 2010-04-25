/****************************************************************************
 *                                                                          *
 *                         GNAT COMPILER COMPONENTS                         *
 *                                                                          *
 *                               S O C K E T                                *
 *                                                                          *
 *                          C Implementation File                           *
 *                                                                          *
 *          Copyright (C) 2003-2009, Free Software Foundation, Inc.         *
 *                                                                          *
 * GNAT is free software;  you can  redistribute it  and/or modify it under *
 * terms of the  GNU General Public License as published  by the Free Soft- *
 * ware  Foundation;  either version 3,  or (at your option) any later ver- *
 * sion.  GNAT is distributed in the hope that it will be useful, but WITH- *
 * OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.                                     *
 *                                                                          *
 * As a special exception under Section 7 of GPL version 3, you are granted *
 * additional permissions described in the GCC Runtime Library Exception,   *
 * version 3.1, as published by the Free Software Foundation.               *
 *                                                                          *
 * You should have received a copy of the GNU General Public License and    *
 * a copy of the GCC Runtime Library Exception along with this program;     *
 * see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    *
 * <http://www.gnu.org/licenses/>.                                          *
 *                                                                          *
 * GNAT was originally developed  by the GNAT team at  New York University. *
 * Extensive contributions were provided by Ada Core Technologies Inc.      *
 *                                                                          *
 ****************************************************************************/

/*  This file provides a portable binding to the sockets API                */

#include "gsocket.h"

#if defined(HAVE_SOCKETS)

/* Include all the necessary system-specific headers and define the
 * necessary macros (shared with gen-oscons).
 */

#if !defined(SO_NOSIGPIPE) && !defined (MSG_NOSIGNAL)
#include <signal.h>
#endif
/* Required if we will be calling signal() in __gnat_disable_all_sigpipes() */

#include "raise.h"
/* Required for __gnat_malloc() */

#include <string.h>
/* Required for memcpy() */

extern void __gnat_disable_sigpipe (int fd);
extern void __gnat_disable_all_sigpipes (void);
extern int  __gnat_create_signalling_fds (int *fds);
extern int  __gnat_read_signalling_fd (int rsig);
extern int  __gnat_write_signalling_fd (int wsig);
extern void  __gnat_close_signalling_fd (int sig);
extern void __gnat_free_socket_set (fd_set *);
extern void __gnat_last_socket_in_set (fd_set *, int *);
extern void __gnat_get_socket_from_set (fd_set *, int *, int *);
extern void __gnat_insert_socket_in_set (fd_set *, int);
extern int __gnat_is_socket_in_set (fd_set *, int);
extern fd_set *__gnat_new_socket_set (fd_set *);
extern void __gnat_remove_socket_from_set (fd_set *, int);
extern int  __gnat_get_h_errno (void);

/* Disable the sending of SIGPIPE for writes on a broken stream */

void
__gnat_disable_sigpipe (int fd)
{
#ifdef SO_NOSIGPIPE
  int val = 1;
  (void) setsockopt (fd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof val);
#endif
}

void
__gnat_disable_all_sigpipes (void)
{
#if !defined(SO_NOSIGPIPE) && !defined(MSG_NOSIGNAL) && defined(SIGPIPE)
  (void) signal (SIGPIPE, SIG_IGN);
#endif
}

#if defined (_WIN32) || defined (__vxworks) || defined (VMS)
/*
 * Signalling FDs operations are implemented in Ada for these platforms
 * (see subunit GNAT.Sockets.Thin.Signalling_Fds).
 */
#else
/*
 * Create a pair of connected file descriptors fds[0] and fds[1] used for
 * signalling by a Selector object. fds[0] is the read end, and fds[1] the
 * write end.
 */
int
__gnat_create_signalling_fds (int *fds) {
  return pipe (fds);
}

/*
 * Read one byte of data from rsig, the read end of a pair of signalling fds
 * created by __gnat_create_signalling_fds.
 */
int
__gnat_read_signalling_fd (int rsig) {
  char c;
  return read (rsig, &c, 1);
}

/*
 * Write one byte of data to wsig, the write end of a pair of signalling fds
 * created by __gnat_create_signalling_fds.
 */
int
__gnat_write_signalling_fd (int wsig) {
  char c = 0;
  return write (wsig, &c, 1);
}

/*
 * Close one end of a pair of signalling fds
 */
void
__gnat_close_signalling_fd (int sig) {
  (void) close (sig);
}
#endif

/*
 * GetXXXbyYYY wrappers
 * These functions are used by the default implementation of g-socthi,
 * and also by the Windows version.
 *
 * They can be used for any platform that either provides an intrinsically
 * task safe implementation of getXXXbyYYY, or a reentrant variant
 * getXXXbyYYY_r. Otherwise, a task safe wrapper, including proper mutual
 * exclusion if appropriate, must be implemented in the target specific
 * version of g-socthi.
 */

#ifdef HAVE_THREAD_SAFE_GETxxxBYyyy
int
__gnat_safe_gethostbyname (const char *name,
  struct hostent *ret, char *buf, size_t buflen,
  int *h_errnop)
{
  struct hostent *rh;
  rh = gethostbyname (name);
  if (rh == NULL) {
    *h_errnop = h_errno;
    return -1;
  }
  *ret = *rh;
  *h_errnop = 0;
  return 0;
}

int
__gnat_safe_gethostbyaddr (const char *addr, int len, int type,
  struct hostent *ret, char *buf, size_t buflen,
  int *h_errnop)
{
  struct hostent *rh;
  rh = gethostbyaddr (addr, len, type);
  if (rh == NULL) {
    *h_errnop = h_errno;
    return -1;
  }
  *ret = *rh;
  *h_errnop = 0;
  return 0;
}

int
__gnat_safe_getservbyname (const char *name, const char *proto,
  struct servent *ret, char *buf, size_t buflen)
{
  struct servent *rh;
  rh = getservbyname (name, proto);
  if (rh == NULL)
    return -1;
  *ret = *rh;
  return 0;
}

int
__gnat_safe_getservbyport (int port, const char *proto,
  struct servent *ret, char *buf, size_t buflen)
{
  struct servent *rh;
  rh = getservbyport (port, proto);
  if (rh == NULL)
    return -1;
  *ret = *rh;
  return 0;
}
#elif HAVE_GETxxxBYyyy_R
int
__gnat_safe_gethostbyname (const char *name,
  struct hostent *ret, char *buf, size_t buflen,
  int *h_errnop)
{
  struct hostent *rh;
  int ri;

#if defined(__linux__) || defined(__GLIBC__)
  (void) gethostbyname_r (name, ret, buf, buflen, &rh, h_errnop);
#else
  rh = gethostbyname_r (name, ret, buf, buflen, h_errnop);
#endif
  ri = (rh == NULL) ? -1 : 0;
  return ri;
}

int
__gnat_safe_gethostbyaddr (const char *addr, int len, int type,
  struct hostent *ret, char *buf, size_t buflen,
  int *h_errnop)
{
  struct hostent *rh;
  int ri;

#if defined(__linux__) || defined(__GLIBC__)
  (void) gethostbyaddr_r (addr, len, type, ret, buf, buflen, &rh, h_errnop);
#else
  rh = gethostbyaddr_r (addr, len, type, ret, buf, buflen, h_errnop);
#endif
  ri = (rh == NULL) ? -1 : 0;
  return ri;
}

int
__gnat_safe_getservbyname (const char *name, const char *proto,
  struct servent *ret, char *buf, size_t buflen)
{
  struct servent *rh;
  int ri;

#if defined(__linux__) || defined(__GLIBC__) || defined(__rtems__)
  (void) getservbyname_r (name, proto, ret, buf, buflen, &rh);
#else
  rh = getservbyname_r (name, proto, ret, buf, buflen);
#endif
  ri = (rh == NULL) ? -1 : 0;
  return ri;
}

int
__gnat_safe_getservbyport (int port, const char *proto,
  struct servent *ret, char *buf, size_t buflen)
{
  struct servent *rh;
  int ri;

#if defined(__linux__) || defined(__GLIBC__) || defined(__rtems__)
  (void) getservbyport_r (port, proto, ret, buf, buflen, &rh);
#else
  rh = getservbyport_r (port, proto, ret, buf, buflen);
#endif
  ri = (rh == NULL) ? -1 : 0;
  return ri;
}
#endif

/* Free socket set. */

void
__gnat_free_socket_set (fd_set *set)
{
  __gnat_free (set);
}

/* Find the largest socket in the socket set SET. This is needed for
   `select'.  LAST is the maximum value for the largest socket. This hint is
   used to avoid scanning very large socket sets.  On return, LAST is the
   actual largest socket in the socket set. */

void
__gnat_last_socket_in_set (fd_set *set, int *last)
{
  int s;
  int l;
  l = -1;

#ifdef _WIN32
  /* More efficient method for NT. */
  for (s = 0; s < set->fd_count; s++)
    if ((int) set->fd_array[s] > l)
      l = set->fd_array[s];

#else

  for (s = *last; s != -1; s--)
    if (FD_ISSET (s, set))
      {
	l = s;
	break;
      }
#endif

  *last = l;
}

/* Get last socket and remove it from the socket set SET.  LAST is the
   maximum value of the largest socket.  This hint is used to avoid scanning
   very large socket sets.  On return, LAST is set to the actual largest
   socket in the socket set. */

void
__gnat_get_socket_from_set (fd_set *set, int *last, int *socket)
{
  *socket = *last;
  FD_CLR (*socket, set);
  __gnat_last_socket_in_set (set, last);
}

/* Insert SOCKET in the socket set SET. */

void
__gnat_insert_socket_in_set (fd_set *set, int socket)
{
  FD_SET (socket, set);
}

/* Check whether a given SOCKET is in the socket set SET. */

int
__gnat_is_socket_in_set (fd_set *set, int socket)
{
  return FD_ISSET (socket, set);
}

/* Allocate a new socket set and set it as empty.  */

fd_set *
__gnat_new_socket_set (fd_set *set)
{
  fd_set *new;

#ifdef VMS
extern void *__gnat_malloc32 (__SIZE_TYPE__);
  new = (fd_set *) __gnat_malloc32 (sizeof (fd_set));
#else
  new = (fd_set *) __gnat_malloc (sizeof (fd_set));
#endif

  if (set)
    memcpy (new, set, sizeof (fd_set));
  else
    FD_ZERO (new);

  return new;
}

/* Remove SOCKET from the socket set SET. */

void
__gnat_remove_socket_from_set (fd_set *set, int socket)
{
  FD_CLR (socket, set);
}

/* Get the value of the last host error */

int
__gnat_get_h_errno (void) {
#ifdef __vxworks
  int vxw_errno = errno;

  switch (vxw_errno) {
    case 0:
      return 0;

    case S_resolvLib_HOST_NOT_FOUND:
    case S_hostLib_UNKNOWN_HOST:
      return HOST_NOT_FOUND;

    case S_resolvLib_TRY_AGAIN:
      return TRY_AGAIN;

    case S_resolvLib_NO_RECOVERY:
    case S_resolvLib_BUFFER_2_SMALL:
    case S_resolvLib_INVALID_PARAMETER:
    case S_resolvLib_INVALID_ADDRESS:
    case S_hostLib_INVALID_PARAMETER:
      return NO_RECOVERY;

    case S_resolvLib_NO_DATA:
      return NO_DATA;

    default:
      return -1;
  }

#elif defined (VMS)
  /* h_errno is defined as follows in OpenVMS' version of <netdb.h>.
   * However this header file is not available when building the GNAT
   * runtime library using GCC, so we are hardcoding the definition
   * directly. Note that the returned address is thread-specific.
   */
  extern int *decc$h_errno_get_addr ();
  return *decc$h_errno_get_addr ();

#elif defined (__rtems__)
  /* At this stage in the tool build, no networking .h files are available.
   * Newlib does not provide networking .h files and RTEMS is not built yet.
   * So we need to explicitly extern h_errno to access it.
   */
  extern int h_errno;
  return h_errno;

#else
  return h_errno;
#endif
}

#else
#warning Sockets are not supported on this platform
#endif /* defined(HAVE_SOCKETS) */
