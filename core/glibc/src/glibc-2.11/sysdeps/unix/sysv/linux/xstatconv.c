/* Convert between the kernel's `struct stat' format, and libc's.
   Copyright (C) 1991,1995-1997,2000,2002,2003,2007
   Free Software Foundation, Inc.
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

#include <errno.h>
#include <sys/stat.h>
#include <kernel_stat.h>
#include <kernel-features.h>

#ifdef STAT_IS_KERNEL_STAT

/* Dummy.  */
struct kernel_stat;

#else

#include <string.h>


#if !defined __ASSUME_STAT64_SYSCALL || defined XSTAT_IS_XSTAT64
int
__xstat_conv (int vers, struct kernel_stat *kbuf, void *ubuf)
{
  switch (vers)
    {
    case _STAT_VER_KERNEL:
      /* Nothing to do.  The struct is in the form the kernel expects.
         We should have short-circuted before we got here, but for
         completeness... */
      *(struct kernel_stat *) ubuf = *kbuf;
      break;

    case _STAT_VER_LINUX:
      {
	struct stat *buf = ubuf;

	/* Convert to current kernel version of `struct stat'.  */
	buf->st_dev = kbuf->st_dev;
#ifdef _HAVE_STAT___PAD1
	buf->__pad1 = 0;
#endif
	buf->st_ino = kbuf->st_ino;
	buf->st_mode = kbuf->st_mode;
	buf->st_nlink = kbuf->st_nlink;
	buf->st_uid = kbuf->st_uid;
	buf->st_gid = kbuf->st_gid;
	buf->st_rdev = kbuf->st_rdev;
#ifdef _HAVE_STAT___PAD2
	buf->__pad2 = 0;
#endif
	buf->st_size = kbuf->st_size;
	buf->st_blksize = kbuf->st_blksize;
	buf->st_blocks = kbuf->st_blocks;
#ifdef _HAVE_STAT_NSEC
	buf->st_atim.tv_sec = kbuf->st_atim.tv_sec;
	buf->st_atim.tv_nsec = kbuf->st_atim.tv_nsec;
	buf->st_mtim.tv_sec = kbuf->st_mtim.tv_sec;
	buf->st_mtim.tv_nsec = kbuf->st_mtim.tv_nsec;
	buf->st_ctim.tv_sec = kbuf->st_ctim.tv_sec;
	buf->st_ctim.tv_nsec = kbuf->st_ctim.tv_nsec;
#else
	buf->st_atime = kbuf->st_atime;
	buf->st_mtime = kbuf->st_mtime;
	buf->st_ctime = kbuf->st_ctime;
#endif
#ifdef _HAVE_STAT___UNUSED1
	buf->__unused1 = 0;
#endif
#ifdef _HAVE_STAT___UNUSED2
	buf->__unused2 = 0;
#endif
#ifdef _HAVE_STAT___UNUSED3
	buf->__unused3 = 0;
#endif
#ifdef _HAVE_STAT___UNUSED4
	buf->__unused4 = 0;
#endif
#ifdef _HAVE_STAT___UNUSED5
	buf->__unused5 = 0;
#endif
      }
      break;

    default:
      __set_errno (EINVAL);
      return -1;
    }

  return 0;
}
#endif

int
__xstat64_conv (int vers, struct kernel_stat *kbuf, void *ubuf)
{
#ifdef XSTAT_IS_XSTAT64
  return __xstat_conv (vers, kbuf, ubuf);
#else
  switch (vers)
    {
    case _STAT_VER_LINUX:
      {
	struct stat64 *buf = ubuf;

	/* Convert to current kernel version of `struct stat64'.  */
	buf->st_dev = kbuf->st_dev;
#ifdef _HAVE_STAT64___PAD1
	buf->__pad1 = 0;
#endif
	buf->st_ino = kbuf->st_ino;
#ifdef _HAVE_STAT64___ST_INO
	buf->__st_ino = kbuf->st_ino;
#endif
	buf->st_mode = kbuf->st_mode;
	buf->st_nlink = kbuf->st_nlink;
	buf->st_uid = kbuf->st_uid;
	buf->st_gid = kbuf->st_gid;
	buf->st_rdev = kbuf->st_rdev;
#ifdef _HAVE_STAT64___PAD2
	buf->__pad2 = 0;
#endif
	buf->st_size = kbuf->st_size;
	buf->st_blksize = kbuf->st_blksize;
	buf->st_blocks = kbuf->st_blocks;
#ifdef _HAVE_STAT64_NSEC
	buf->st_atim.tv_sec = kbuf->st_atim.tv_sec;
	buf->st_atim.tv_nsec = kbuf->st_atim.tv_nsec;
	buf->st_mtim.tv_sec = kbuf->st_mtim.tv_sec;
	buf->st_mtim.tv_nsec = kbuf->st_mtim.tv_nsec;
	buf->st_ctim.tv_sec = kbuf->st_ctim.tv_sec;
	buf->st_ctim.tv_nsec = kbuf->st_ctim.tv_nsec;
#else
	buf->st_atime = kbuf->st_atime;
	buf->st_mtime = kbuf->st_mtime;
	buf->st_ctime = kbuf->st_ctime;
#endif
#ifdef _HAVE_STAT64___UNUSED1
	buf->__unused1 = 0;
#endif
#ifdef _HAVE_STAT64___UNUSED2
	buf->__unused2 = 0;
#endif
#ifdef _HAVE_STAT64___UNUSED3
	buf->__unused3 = 0;
#endif
#ifdef _HAVE_STAT64___UNUSED4
	buf->__unused4 = 0;
#endif
#ifdef _HAVE_STAT64___UNUSED5
	buf->__unused5 = 0;
#endif
      }
      break;

      /* If struct stat64 is different from struct stat then
	 _STAT_VER_KERNEL does not make sense.  */
    case _STAT_VER_KERNEL:
    default:
      __set_errno (EINVAL);
      return -1;
    }

  return 0;
#endif
}

int
__xstat32_conv (int vers, struct stat64 *kbuf, struct stat *buf)
{
  switch (vers)
    {
    case _STAT_VER_LINUX:
      {
	/* Convert current kernel version of `struct stat64' to
           `struct stat'.  */
	buf->st_dev = kbuf->st_dev;
#ifdef _HAVE_STAT___PAD1
	buf->__pad1 = 0;
#endif
#ifdef _HAVE_STAT64___ST_INO
# if __ASSUME_ST_INO_64_BIT == 0
	if (kbuf->st_ino == 0)
	  buf->st_ino = kbuf->__st_ino;
	else
# endif
	  {
	    buf->st_ino = kbuf->st_ino;
	    if (sizeof (buf->st_ino) != sizeof (kbuf->st_ino)
		&& buf->st_ino != kbuf->st_ino)
	      {
		__set_errno (EOVERFLOW);
		return -1;
	      }
	  }
#else
	buf->st_ino = kbuf->st_ino;
	if (sizeof (buf->st_ino) != sizeof (kbuf->st_ino)
	    && buf->st_ino != kbuf->st_ino)
	  {
	    __set_errno (EOVERFLOW);
	    return -1;
	  }
#endif
	buf->st_mode = kbuf->st_mode;
	buf->st_nlink = kbuf->st_nlink;
	buf->st_uid = kbuf->st_uid;
	buf->st_gid = kbuf->st_gid;
	buf->st_rdev = kbuf->st_rdev;
#ifdef _HAVE_STAT___PAD2
	buf->__pad2 = 0;
#endif
	buf->st_size = kbuf->st_size;
	/* Check for overflow.  */
	if (sizeof (buf->st_size) != sizeof (kbuf->st_size)
	    && buf->st_size != kbuf->st_size)
	  {
	    __set_errno (EOVERFLOW);
	    return -1;
	  }
	buf->st_blksize = kbuf->st_blksize;
	buf->st_blocks = kbuf->st_blocks;
	/* Check for overflow.  */
	if (sizeof (buf->st_blocks) != sizeof (kbuf->st_blocks)
	    && buf->st_blocks != kbuf->st_blocks)
	  {
	    __set_errno (EOVERFLOW);
	    return -1;
	  }
#ifdef _HAVE_STAT_NSEC
	buf->st_atim.tv_sec = kbuf->st_atim.tv_sec;
	buf->st_atim.tv_nsec = kbuf->st_atim.tv_nsec;
	buf->st_mtim.tv_sec = kbuf->st_mtim.tv_sec;
	buf->st_mtim.tv_nsec = kbuf->st_mtim.tv_nsec;
	buf->st_ctim.tv_sec = kbuf->st_ctim.tv_sec;
	buf->st_ctim.tv_nsec = kbuf->st_ctim.tv_nsec;
#else
	buf->st_atime = kbuf->st_atime;
	buf->st_mtime = kbuf->st_mtime;
	buf->st_ctime = kbuf->st_ctime;
#endif

#ifdef _HAVE_STAT___UNUSED1
	buf->__unused1 = 0;
#endif
#ifdef _HAVE_STAT___UNUSED2
	buf->__unused2 = 0;
#endif
#ifdef _HAVE_STAT___UNUSED3
	buf->__unused3 = 0;
#endif
#ifdef _HAVE_STAT___UNUSED4
	buf->__unused4 = 0;
#endif
#ifdef _HAVE_STAT___UNUSED5
	buf->__unused5 = 0;
#endif
      }
      break;

      /* If struct stat64 is different from struct stat then
	 _STAT_VER_KERNEL does not make sense.  */
    case _STAT_VER_KERNEL:
    default:
      __set_errno (EINVAL);
      return -1;
    }

  return 0;
}

#endif
