# IBM POWER __mpn_add_n -- Add two limb vectors of equal, non-zero length.

# Copyright (C) 1992, 1994, 1995, 1996 Free Software Foundation, Inc.

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
# s2_ptr	r5
# size		r6

	.toc
	.extern __mpn_add_n[DS]
	.extern .__mpn_add_n
.csect [PR]
	.align 2
	.globl __mpn_add_n
	.globl .__mpn_add_n
	.csect __mpn_add_n[DS]
__mpn_add_n:
	.long .__mpn_add_n, TOC[tc0], 0
	.csect [PR]
.__mpn_add_n:
	andil.	10,6,1		# odd or even number of limbs?
	l	8,0(4)		# load least significant s1 limb
	l	0,0(5)		# load least significant s2 limb
	cal	3,-4(3)		# offset res_ptr, it's updated before it's used
	sri	10,6,1		# count for unrolled loop
	a	7,0,8		# add least significant limbs, set cy
	mtctr	10		# copy count into CTR
	beq	0,Leven		# branch if even # of limbs (# of limbs >= 2)

# We have an odd # of limbs.  Add the first limbs separately.
	cmpi	1,10,0		# is count for unrolled loop zero?
	bne	1,L1		# branch if not
	st	7,4(3)
	aze	3,10		# use the fact that r10 is zero...
	br			# return

# We added least significant limbs.  Now reload the next limbs to enter loop.
L1:	lu	8,4(4)		# load s1 limb and update s1_ptr
	lu	0,4(5)		# load s2 limb and update s2_ptr
	stu	7,4(3)
	ae	7,0,8		# add limbs, set cy
Leven:	lu	9,4(4)		# load s1 limb and update s1_ptr
	lu	10,4(5)		# load s2 limb and update s2_ptr
	bdz	Lend		# If done, skip loop

Loop:	lu	8,4(4)		# load s1 limb and update s1_ptr
	lu	0,4(5)		# load s2 limb and update s2_ptr
	ae	11,9,10		# add previous limbs with cy, set cy
	stu	7,4(3)		# 
	lu	9,4(4)		# load s1 limb and update s1_ptr
	lu	10,4(5)		# load s2 limb and update s2_ptr
	ae	7,0,8		# add previous limbs with cy, set cy
	stu	11,4(3)		# 
	bdn	Loop		# decrement CTR and loop back

Lend:	ae	11,9,10		# add limbs with cy, set cy
	st	7,4(3)		# 
	st	11,8(3)		# 
	lil	3,0		# load cy into ...
	aze	3,3		# ... return value register
	br
