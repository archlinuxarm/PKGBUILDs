/* Copyright (C) 1996,97,2002 Free Software Foundation, Inc.
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

#include <stdio.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>


char *
ether_ntoa_r (const struct ether_addr *addr, char *buf)
{
  sprintf (buf, "%x:%x:%x:%x:%x:%x",
	   addr->ether_addr_octet[0], addr->ether_addr_octet[1],
	   addr->ether_addr_octet[2], addr->ether_addr_octet[3],
	   addr->ether_addr_octet[4], addr->ether_addr_octet[5]);
  return buf;
}
libc_hidden_def (ether_ntoa_r)
