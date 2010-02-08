# IBM POWER __mpn_submul_1 -- Multiply a limb vector with a limb and subtract
# the result from a second limb vector.

# Copyright (C) 1992, 1994 Free Software Foundation, Inc.

# This file is part of the GNU MP Library.

# The GNU MP Library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or (at your
# option) any later version.

# The GNU MP Library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
# License for more details.

# You should have received a copy of the GNU Lesser General Public License
# along with the GNU MP Library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
# MA 02111-1307, USA.


# INPUT PARAMETERS
# res_ptr	r3
# s1_ptr	r4
# size		r5
# s2_limb	r6

# The RS/6000 has no unsigned 32x32->64 bit multiplication instruction.  To
# obtain that operation, we have to use the 32x32->64 signed multiplication
# instruction, and add the appropriate compensation to the high limb of the
# result.  We add the multiplicand if the multiplier has its most significant
# bit set, and we add the multiplier if the multiplicand has its most
# significant bit set.  We need to preserve the carry flag between each
# iteration, so we have to compute the compensation carefully (the natural,
# srai+and doesn't work).  Since the POWER architecture has a branch unit
# we can branch in zero cycles, so that's how we perform the additions.

	.toc
	.csect .__mpn_submul_1[PR]
	.align 2
	.globl __mpn_submul_1
	.globl .__mpn_submul_1
	.csect __mpn_submul_1[DS]
__mpn_submul_1:
	.long .__mpn_submul_1[PR], TOC[tc0], 0
	.csect .__mpn_submul_1[PR]
.__mpn_submul_1:

	cal	3,-4(3)
	l	0,0(4)
	cmpi	0,6,0
	mtctr	5
	mul	9,0,6
	srai	7,0,31
	and	7,7,6
	mfmq	11
	cax	9,9,7
	l	7,4(3)
	sf	8,11,7		# add res_limb
	a	11,8,11		# invert cy (r11 is junk)
	blt	Lneg
Lpos:	bdz	Lend

Lploop:	lu	0,4(4)
	stu	8,4(3)
	cmpi	0,0,0
	mul	10,0,6
	mfmq	0
	ae	11,0,9		# low limb + old_cy_limb + old cy
	l	7,4(3)
	aze	10,10		# propagate cy to new cy_limb
	sf	8,11,7		# add res_limb
	a	11,8,11		# invert cy (r11 is junk)
	bge	Lp0
	cax	10,10,6		# adjust high limb for negative limb from s1
Lp0:	bdz	Lend0
	lu	0,4(4)
	stu	8,4(3)
	cmpi	0,0,0
	mul	9,0,6
	mfmq	0
	ae	11,0,10
	l	7,4(3)
	aze	9,9
	sf	8,11,7
	a	11,8,11		# invert cy (r11 is junk)
	bge	Lp1
	cax	9,9,6		# adjust high limb for negative limb from s1
Lp1:	bdn	Lploop

	b	Lend

Lneg:	cax	9,9,0
	bdz	Lend
Lnloop:	lu	0,4(4)
	stu	8,4(3)
	cmpi	0,0,0
	mul	10,0,6
	mfmq	7
	ae	11,7,9
	l	7,4(3)
	ae	10,10,0		# propagate cy to new cy_limb
	sf	8,11,7		# add res_limb
	a	11,8,11		# invert cy (r11 is junk)
	bge	Ln0
	cax	10,10,6		# adjust high limb for negative limb from s1
Ln0:	bdz	Lend0
	lu	0,4(4)
	stu	8,4(3)
	cmpi	0,0,0
	mul	9,0,6
	mfmq	7
	ae	11,7,10
	l	7,4(3)
	ae	9,9,0		# propagate cy to new cy_limb
	sf	8,11,7		# add res_limb
	a	11,8,11		# invert cy (r11 is junk)
	bge	Ln1
	cax	9,9,6		# adjust high limb for negative limb from s1
Ln1:	bdn	Lnloop
	b	Lend

Lend0:	cal	9,0(10)
Lend:	st	8,4(3)
	aze	3,9
	br
