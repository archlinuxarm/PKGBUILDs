/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)memchr.s	5.1 (Berkeley) 5/29/90"
#endif /* LIBC_SCCS and not lint */

/*
 * Find the first occurrence of c in the memory at cp (length n).
 * Return pointer to match or null pointer.
 *
 * This code optimises the usual case (0 < n < 65535).
 *
 * void *
 * memchr(cp, c, n)
 *	char *cp, c;
 *	size_t n;
 */

#include "DEFS.h"

ENTRY(__memchr, 0)
	movq	4(ap),r1	# r1 = cp; r2 = c
	movl	12(ap),r0	# r0 = n
	movzwl	$65535,r4	# handy constant
0:
	cmpl	r0,r4		# check for annoying locc limit
	bgtru	3f

	/* n <= 65535 */
	locc	r2,r0,(r1)	# search n bytes for c
	beql	2f		# done if not found (r0 already 0)
1:	/* found character c at (r1) */
	movl	r1,r0
2:
	ret

3:	/* n > 65535 */
	locc	r2,r4,(r1)	# search 65535 bytes for c
	beql	1b		# done if found
	decw	r0		# from 0 to 65535
	subl2	r0,r4		# adjust n
	brb	0b		# and loop

weak_alias (__memchr, memchr)
#if !__BOUNDED_POINTERS__
weak_alias (__memchr, __ubp_memchr)
#endif
