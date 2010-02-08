/* Directory entry structure `struct dirent'.  4.4BSD version.
   Copyright (C) 1996,97,98,2001 Free Software Foundation, Inc.
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

#ifndef _DIRENT_H
# error "Never use <bits/dirent.h> directly; include <dirent.h> instead."
#endif

struct dirent
  {
#ifndef __USE_FILE_OFFSET64
    __ino_t d_ino;		/* File serial number.  */
#else
    __ino64_t d_ino;
#endif
    unsigned short int d_reclen; /* Length of the whole `struct dirent'.  */
    unsigned char d_type;	/* File type, possibly unknown.  */
    unsigned char d_namlen;	/* Length of the file name.  */

    /* Only this member is in the POSIX standard.  */
    char d_name[1];		/* File name (actually longer).  */
  };

#ifdef __USE_LARGEFILE64
struct dirent64
  {
    __ino64_t d_ino;
    unsigned short int d_reclen;
    unsigned char d_type;
    unsigned char d_namlen;

    char d_name[1];
  };
#endif

#define d_fileno	d_ino	/* Backwards compatibility.  */

#define _DIRENT_HAVE_D_RECLEN 1
#define _DIRENT_HAVE_D_NAMLEN 1
#define _DIRENT_HAVE_D_TYPE 1
