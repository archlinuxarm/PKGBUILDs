/* Enqueue a list of read or write requests.  Stub version.
   Copyright (C) 2001 Free Software Foundation, Inc.
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

#include <aio.h>
#include <errno.h>

#ifdef BE_AIO64
#define lio_listio	lio_listio64
#define aiocb		aiocb64
#define aio_read	aio_read64
#define aio_write	aio_write64
#define aio_suspend	aio_suspend64
#endif


int
lio_listio (int mode,
	    struct aiocb *const list[], int nent,
	    struct sigevent *sig)
{
  __set_errno (ENOSYS);
  return -1;
}

stub_warning (lio_listio)
#include <stub-tag.h>
