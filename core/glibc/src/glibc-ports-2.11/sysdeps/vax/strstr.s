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
	.asciz "@(#)strstr.s	5.2 (Berkeley) 8/21/90"
#endif /* LIBC_SCCS and not lint */

/*
 * Find the first occurrence of s2 as a substring in s1.
 * If s2 is empty, return s1.
 *
 * char *strstr(s1, s2)
 *	const char *s1, *s2;
 */
#include "DEFS.h"

ENTRY(strstr, 0)
	movq	4(ap),r3	/* r3 = s1, r4 = s2 */
	movzwl	$65535,r2	/* r2 = locc/matchc limit */
	locc	$0,r2,(r4)	/* find '\0' in s2 */
	beql	4f
	subl3	r4,r1,r5	/* r5 = strlen(s2) */
	beql	1f		/* if r5 == 0, return s1 */

	/*
	 * s2 is short enough to apply matchc.
	 * If s1 is long, we have to do it in stages.
	 */
0:	locc	$0,r2,(r3)	/* find '\0' in s1 */
	beql	3f

	/*
	 * Both strings are `short'; we can use matchc directly.
	 */
	subl3	r3,r1,r1	/* r1 = strlen(s1) */
	matchc	r5,(r4),r1,(r3)	/* find substring */
	bneq	2f

	/*
	 * r3 points r5 bytes past match.  Return the match.
	 */
1:	subl3	r5,r3,r0	/* return (byte_past_match - strlen(s2)) */
	ret

	/*
	 * There is no matching substring.
	 */
2:	clrl	r0		/* return NULL */
	ret

	/*
	 * s1 is too long (> 65535 bytes) to apply matchc directly,
	 * but s2 is short enough.  Apply s2 to s1, then (if not
	 * found yet) advancing s1 by (65536-strlen(s2)) bytes and
	 * loop.
	 */
3:	matchc	r5,(r4),r2,(r3)	/* search */
	beql	1b		/* if found, go return it */
	decw	r2		/* from 0 to 65535 */
	incl	r3		/* already advanced 65535, now 65536 */
	subl2	r5,r3		/* ... minus strlen(s2) */
	brb	0b

	/*
	 * s2 is too long (> 65535 bytes) to bother with matchc.
	 */
4:	locc	$0,r2,(r1)	/* continue working on strlen(s2) */
	beql	4b
	subl3	r1,r4,r5	/* r5 = strlen(s2) */
	movb	(r4)+,r2	/* r2 = *s2++ */
	decl	r5		/* fix up length */
5:	movb	(r3)+,r0	/* r0 = *s1++ */
	beql	2b		/* if '\0', return NULL */
	cmpb	r0,r2
	bneq	5b		/* loop until first char found */
	pushr	R5|R4|R3|R2	/* save c, s1, s2, n */
	pushr	R5|R4|R3	/* strncmp(s1, s2, n) */
	calls	$3,_strncmp
	popr	R2|R3|R4|R5	/* restore */
	tstl	r0
	bneq	5b		/* loop until strncmp says rest same too */
	subl3	$1,r3,r0	/* return previous s1 */
	ret
