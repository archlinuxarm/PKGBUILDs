/* Emit RTL for the GCC expander.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */


/* Middle-to-low level generation of rtx code and insns.

   This file contains support functions for creating rtl expressions
   and manipulating them in the doubly-linked chain of insns.

   The patterns of the insns are created by machine-dependent
   routines in insn-emit.c, which is generated automatically from
   the machine description.  These routines make the individual rtx's
   of the pattern with `gen_rtx_fmt_ee' and others in genrtl.[ch],
   which are automatically generated from rtl.def; what is machine
   dependent is the kind of rtx's they make and what arguments they
   use.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "toplev.h"
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "hashtab.h"
#include "insn-config.h"
#include "recog.h"
#include "real.h"
#include "fixed-value.h"
#include "bitmap.h"
#include "basic-block.h"
#include "ggc.h"
#include "debug.h"
#include "langhooks.h"
#include "tree-pass.h"
#include "df.h"

/* Commonly used modes.  */

enum machine_mode byte_mode;	/* Mode whose width is BITS_PER_UNIT.  */
enum machine_mode word_mode;	/* Mode whose width is BITS_PER_WORD.  */
enum machine_mode double_mode;	/* Mode whose width is DOUBLE_TYPE_SIZE.  */
enum machine_mode ptr_mode;	/* Mode whose width is POINTER_SIZE.  */

/* Datastructures maintained for currently processed function in RTL form.  */

struct rtl_data x_rtl;

/* Indexed by pseudo register number, gives the rtx for that pseudo.
   Allocated in parallel with regno_pointer_align.  
   FIXME: We could put it into emit_status struct, but gengtype is not able to deal
   with length attribute nested in top level structures.  */

rtx * regno_reg_rtx;

/* This is *not* reset after each function.  It gives each CODE_LABEL
   in the entire compilation a unique label number.  */

static GTY(()) int label_num = 1;

/* Nonzero means do not generate NOTEs for source line numbers.  */

static int no_line_numbers;

/* Commonly used rtx's, so that we only need space for one copy.
   These are initialized once for the entire compilation.
   All of these are unique; no other rtx-object will be equal to any
   of these.  */

rtx global_rtl[GR_MAX];

/* Commonly used RTL for hard registers.  These objects are not necessarily
   unique, so we allocate them separately from global_rtl.  They are
   initialized once per compilation unit, then copied into regno_reg_rtx
   at the beginning of each function.  */
static GTY(()) rtx static_regno_reg_rtx[FIRST_PSEUDO_REGISTER];

/* We record floating-point CONST_DOUBLEs in each floating-point mode for
   the values of 0, 1, and 2.  For the integer entries and VOIDmode, we
   record a copy of const[012]_rtx.  */

rtx const_tiny_rtx[3][(int) MAX_MACHINE_MODE];

rtx const_true_rtx;

REAL_VALUE_TYPE dconst0;
REAL_VALUE_TYPE dconst1;
REAL_VALUE_TYPE dconst2;
REAL_VALUE_TYPE dconstm1;
REAL_VALUE_TYPE dconsthalf;

/* Record fixed-point constant 0 and 1.  */
FIXED_VALUE_TYPE fconst0[MAX_FCONST0];
FIXED_VALUE_TYPE fconst1[MAX_FCONST1];

/* All references to the following fixed hard registers go through
   these unique rtl objects.  On machines where the frame-pointer and
   arg-pointer are the same register, they use the same unique object.

   After register allocation, other rtl objects which used to be pseudo-regs
   may be clobbered to refer to the frame-pointer register.
   But references that were originally to the frame-pointer can be
   distinguished from the others because they contain frame_pointer_rtx.

   When to use frame_pointer_rtx and hard_frame_pointer_rtx is a little
   tricky: until register elimination has taken place hard_frame_pointer_rtx
   should be used if it is being set, and frame_pointer_rtx otherwise.  After
   register elimination hard_frame_pointer_rtx should always be used.
   On machines where the two registers are same (most) then these are the
   same.

   In an inline procedure, the stack and frame pointer rtxs may not be
   used for anything else.  */
rtx static_chain_rtx;		/* (REG:Pmode STATIC_CHAIN_REGNUM) */
rtx static_chain_incoming_rtx;	/* (REG:Pmode STATIC_CHAIN_INCOMING_REGNUM) */
rtx pic_offset_table_rtx;	/* (REG:Pmode PIC_OFFSET_TABLE_REGNUM) */

/* This is used to implement __builtin_return_address for some machines.
   See for instance the MIPS port.  */
rtx return_address_pointer_rtx;	/* (REG:Pmode RETURN_ADDRESS_POINTER_REGNUM) */

/* We make one copy of (const_int C) where C is in
   [- MAX_SAVED_CONST_INT, MAX_SAVED_CONST_INT]
   to save space during the compilation and simplify comparisons of
   integers.  */

rtx const_int_rtx[MAX_SAVED_CONST_INT * 2 + 1];

/* A hash table storing CONST_INTs whose absolute value is greater
   than MAX_SAVED_CONST_INT.  */

static GTY ((if_marked ("ggc_marked_p"), param_is (struct rtx_def)))
     htab_t const_int_htab;

/* A hash table storing memory attribute structures.  */
static GTY ((if_marked ("ggc_marked_p"), param_is (struct mem_attrs)))
     htab_t mem_attrs_htab;

/* A hash table storing register attribute structures.  */
static GTY ((if_marked ("ggc_marked_p"), param_is (struct reg_attrs)))
     htab_t reg_attrs_htab;

/* A hash table storing all CONST_DOUBLEs.  */
static GTY ((if_marked ("ggc_marked_p"), param_is (struct rtx_def)))
     htab_t const_double_htab;

/* A hash table storing all CONST_FIXEDs.  */
static GTY ((if_marked ("ggc_marked_p"), param_is (struct rtx_def)))
     htab_t const_fixed_htab;

#define first_insn (crtl->emit.x_first_insn)
#define last_insn (crtl->emit.x_last_insn)
#define cur_insn_uid (crtl->emit.x_cur_insn_uid)
#define last_location (crtl->emit.x_last_location)
#define first_label_num (crtl->emit.x_first_label_num)

static rtx make_call_insn_raw (rtx);
static rtx change_address_1 (rtx, enum machine_mode, rtx, int);
static void set_used_decls (tree);
static void mark_label_nuses (rtx);
static hashval_t const_int_htab_hash (const void *);
static int const_int_htab_eq (const void *, const void *);
static hashval_t const_double_htab_hash (const void *);
static int const_double_htab_eq (const void *, const void *);
static rtx lookup_const_double (rtx);
static hashval_t const_fixed_htab_hash (const void *);
static int const_fixed_htab_eq (const void *, const void *);
static rtx lookup_const_fixed (rtx);
static hashval_t mem_attrs_htab_hash (const void *);
static int mem_attrs_htab_eq (const void *, const void *);
static mem_attrs *get_mem_attrs (alias_set_type, tree, rtx, rtx, unsigned int,
				 enum machine_mode);
static hashval_t reg_attrs_htab_hash (const void *);
static int reg_attrs_htab_eq (const void *, const void *);
static reg_attrs *get_reg_attrs (tree, int);
static tree component_ref_for_mem_expr (tree);
static rtx gen_const_vector (enum machine_mode, int);
static void copy_rtx_if_shared_1 (rtx *orig);

/* Probability of the conditional branch currently proceeded by try_split.
   Set to -1 otherwise.  */
int split_branch_probability = -1;

/* Returns a hash code for X (which is a really a CONST_INT).  */

static hashval_t
const_int_htab_hash (const void *x)
{
  return (hashval_t) INTVAL ((const_rtx) x);
}

/* Returns nonzero if the value represented by X (which is really a
   CONST_INT) is the same as that given by Y (which is really a
   HOST_WIDE_INT *).  */

static int
const_int_htab_eq (const void *x, const void *y)
{
  return (INTVAL ((const_rtx) x) == *((const HOST_WIDE_INT *) y));
}

/* Returns a hash code for X (which is really a CONST_DOUBLE).  */
static hashval_t
const_double_htab_hash (const void *x)
{
  const_rtx const value = (const_rtx) x;
  hashval_t h;

  if (GET_MODE (value) == VOIDmode)
    h = CONST_DOUBLE_LOW (value) ^ CONST_DOUBLE_HIGH (value);
  else
    {
      h = real_hash (CONST_DOUBLE_REAL_VALUE (value));
      /* MODE is used in the comparison, so it should be in the hash.  */
      h ^= GET_MODE (value);
    }
  return h;
}

/* Returns nonzero if the value represented by X (really a ...)
   is the same as that represented by Y (really a ...) */
static int
const_double_htab_eq (const void *x, const void *y)
{
  const_rtx const a = (const_rtx)x, b = (const_rtx)y;

  if (GET_MODE (a) != GET_MODE (b))
    return 0;
  if (GET_MODE (a) == VOIDmode)
    return (CONST_DOUBLE_LOW (a) == CONST_DOUBLE_LOW (b)
	    && CONST_DOUBLE_HIGH (a) == CONST_DOUBLE_HIGH (b));
  else
    return real_identical (CONST_DOUBLE_REAL_VALUE (a),
			   CONST_DOUBLE_REAL_VALUE (b));
}

/* Returns a hash code for X (which is really a CONST_FIXED).  */

static hashval_t
const_fixed_htab_hash (const void *x)
{
  const_rtx const value = (const_rtx) x;
  hashval_t h;

  h = fixed_hash (CONST_FIXED_VALUE (value));
  /* MODE is used in the comparison, so it should be in the hash.  */
  h ^= GET_MODE (value);
  return h;
}

/* Returns nonzero if the value represented by X (really a ...)
   is the same as that represented by Y (really a ...).  */

static int
const_fixed_htab_eq (const void *x, const void *y)
{
  const_rtx const a = (const_rtx) x, b = (const_rtx) y;

  if (GET_MODE (a) != GET_MODE (b))
    return 0;
  return fixed_identical (CONST_FIXED_VALUE (a), CONST_FIXED_VALUE (b));
}

/* Returns a hash code for X (which is a really a mem_attrs *).  */

static hashval_t
mem_attrs_htab_hash (const void *x)
{
  const mem_attrs *const p = (const mem_attrs *) x;

  return (p->alias ^ (p->align * 1000)
	  ^ ((p->offset ? INTVAL (p->offset) : 0) * 50000)
	  ^ ((p->size ? INTVAL (p->size) : 0) * 2500000)
	  ^ (size_t) iterative_hash_expr (p->expr, 0));
}

/* Returns nonzero if the value represented by X (which is really a
   mem_attrs *) is the same as that given by Y (which is also really a
   mem_attrs *).  */

static int
mem_attrs_htab_eq (const void *x, const void *y)
{
  const mem_attrs *const p = (const mem_attrs *) x;
  const mem_attrs *const q = (const mem_attrs *) y;

  return (p->alias == q->alias && p->offset == q->offset
	  && p->size == q->size && p->align == q->align
	  && (p->expr == q->expr
	      || (p->expr != NULL_TREE && q->expr != NULL_TREE
		  && operand_equal_p (p->expr, q->expr, 0))));
}

/* Allocate a new mem_attrs structure and insert it into the hash table if
   one identical to it is not already in the table.  We are doing this for
   MEM of mode MODE.  */

static mem_attrs *
get_mem_attrs (alias_set_type alias, tree expr, rtx offset, rtx size,
	       unsigned int align, enum machine_mode mode)
{
  mem_attrs attrs;
  void **slot;

  /* If everything is the default, we can just return zero.
     This must match what the corresponding MEM_* macros return when the
     field is not present.  */
  if (alias == 0 && expr == 0 && offset == 0
      && (size == 0
	  || (mode != BLKmode && GET_MODE_SIZE (mode) == INTVAL (size)))
      && (STRICT_ALIGNMENT && mode != BLKmode
	  ? align == GET_MODE_ALIGNMENT (mode) : align == BITS_PER_UNIT))
    return 0;

  attrs.alias = alias;
  attrs.expr = expr;
  attrs.offset = offset;
  attrs.size = size;
  attrs.align = align;

  slot = htab_find_slot (mem_attrs_htab, &attrs, INSERT);
  if (*slot == 0)
    {
      *slot = ggc_alloc (sizeof (mem_attrs));
      memcpy (*slot, &attrs, sizeof (mem_attrs));
    }

  return (mem_attrs *) *slot;
}

/* Returns a hash code for X (which is a really a reg_attrs *).  */

static hashval_t
reg_attrs_htab_hash (const void *x)
{
  const reg_attrs *const p = (const reg_attrs *) x;

  return ((p->offset * 1000) ^ (long) p->decl);
}

/* Returns nonzero if the value represented by X (which is really a
   reg_attrs *) is the same as that given by Y (which is also really a
   reg_attrs *).  */

static int
reg_attrs_htab_eq (const void *x, const void *y)
{
  const reg_attrs *const p = (const reg_attrs *) x;
  const reg_attrs *const q = (const reg_attrs *) y;

  return (p->decl == q->decl && p->offset == q->offset);
}
/* Allocate a new reg_attrs structure and insert it into the hash table if
   one identical to it is not already in the table.  We are doing this for
   MEM of mode MODE.  */

static reg_attrs *
get_reg_attrs (tree decl, int offset)
{
  reg_attrs attrs;
  void **slot;

  /* If everything is the default, we can just return zero.  */
  if (decl == 0 && offset == 0)
    return 0;

  attrs.decl = decl;
  attrs.offset = offset;

  slot = htab_find_slot (reg_attrs_htab, &attrs, INSERT);
  if (*slot == 0)
    {
      *slot = ggc_alloc (sizeof (reg_attrs));
      memcpy (*slot, &attrs, sizeof (reg_attrs));
    }

  return (reg_attrs *) *slot;
}


#if !HAVE_blockage
/* Generate an empty ASM_INPUT, which is used to block attempts to schedule
   across this insn. */

rtx
gen_blockage (void)
{
  rtx x = gen_rtx_ASM_INPUT (VOIDmode, "");
  MEM_VOLATILE_P (x) = true;
  return x;
}
#endif


/* Generate a new REG rtx.  Make sure ORIGINAL_REGNO is set properly, and
   don't attempt to share with the various global pieces of rtl (such as
   frame_pointer_rtx).  */

rtx
gen_raw_REG (enum machine_mode mode, int regno)
{
  rtx x = gen_rtx_raw_REG (mode, regno);
  ORIGINAL_REGNO (x) = regno;
  return x;
}

/* There are some RTL codes that require special attention; the generation
   functions do the raw handling.  If you add to this list, modify
   special_rtx in gengenrtl.c as well.  */

rtx
gen_rtx_CONST_INT (enum machine_mode mode ATTRIBUTE_UNUSED, HOST_WIDE_INT arg)
{
  void **slot;

  if (arg >= - MAX_SAVED_CONST_INT && arg <= MAX_SAVED_CONST_INT)
    return const_int_rtx[arg + MAX_SAVED_CONST_INT];

#if STORE_FLAG_VALUE != 1 && STORE_FLAG_VALUE != -1
  if (const_true_rtx && arg == STORE_FLAG_VALUE)
    return const_true_rtx;
#endif

  /* Look up the CONST_INT in the hash table.  */
  slot = htab_find_slot_with_hash (const_int_htab, &arg,
				   (hashval_t) arg, INSERT);
  if (*slot == 0)
    *slot = gen_rtx_raw_CONST_INT (VOIDmode, arg);

  return (rtx) *slot;
}

rtx
gen_int_mode (HOST_WIDE_INT c, enum machine_mode mode)
{
  return GEN_INT (trunc_int_for_mode (c, mode));
}

/* CONST_DOUBLEs might be created from pairs of integers, or from
   REAL_VALUE_TYPEs.  Also, their length is known only at run time,
   so we cannot use gen_rtx_raw_CONST_DOUBLE.  */

/* Determine whether REAL, a CONST_DOUBLE, already exists in the
   hash table.  If so, return its counterpart; otherwise add it
   to the hash table and return it.  */
static rtx
lookup_const_double (rtx real)
{
  void **slot = htab_find_slot (const_double_htab, real, INSERT);
  if (*slot == 0)
    *slot = real;

  return (rtx) *slot;
}

/* Return a CONST_DOUBLE rtx for a floating-point value specified by
   VALUE in mode MODE.  */
rtx
const_double_from_real_value (REAL_VALUE_TYPE value, enum machine_mode mode)
{
  rtx real = rtx_alloc (CONST_DOUBLE);
  PUT_MODE (real, mode);

  real->u.rv = value;

  return lookup_const_double (real);
}

/* Determine whether FIXED, a CONST_FIXED, already exists in the
   hash table.  If so, return its counterpart; otherwise add it
   to the hash table and return it.  */

static rtx
lookup_const_fixed (rtx fixed)
{
  void **slot = htab_find_slot (const_fixed_htab, fixed, INSERT);
  if (*slot == 0)
    *slot = fixed;

  return (rtx) *slot;
}

/* Return a CONST_FIXED rtx for a fixed-point value specified by
   VALUE in mode MODE.  */

rtx
const_fixed_from_fixed_value (FIXED_VALUE_TYPE value, enum machine_mode mode)
{
  rtx fixed = rtx_alloc (CONST_FIXED);
  PUT_MODE (fixed, mode);

  fixed->u.fv = value;

  return lookup_const_fixed (fixed);
}

/* Return a CONST_DOUBLE or CONST_INT for a value specified as a pair
   of ints: I0 is the low-order word and I1 is the high-order word.
   Do not use this routine for non-integer modes; convert to
   REAL_VALUE_TYPE and use CONST_DOUBLE_FROM_REAL_VALUE.  */

rtx
immed_double_const (HOST_WIDE_INT i0, HOST_WIDE_INT i1, enum machine_mode mode)
{
  rtx value;
  unsigned int i;

  /* There are the following cases (note that there are no modes with
     HOST_BITS_PER_WIDE_INT < GET_MODE_BITSIZE (mode) < 2 * HOST_BITS_PER_WIDE_INT):

     1) If GET_MODE_BITSIZE (mode) <= HOST_BITS_PER_WIDE_INT, then we use
	gen_int_mode.
     2) GET_MODE_BITSIZE (mode) == 2 * HOST_BITS_PER_WIDE_INT, but the value of
	the integer fits into HOST_WIDE_INT anyway (i.e., i1 consists only
	from copies of the sign bit, and sign of i0 and i1 are the same),  then 
	we return a CONST_INT for i0.
     3) Otherwise, we create a CONST_DOUBLE for i0 and i1.  */
  if (mode != VOIDmode)
    {
      gcc_assert (GET_MODE_CLASS (mode) == MODE_INT
		  || GET_MODE_CLASS (mode) == MODE_PARTIAL_INT
		  /* We can get a 0 for an error mark.  */
		  || GET_MODE_CLASS (mode) == MODE_VECTOR_INT
		  || GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT);

      if (GET_MODE_BITSIZE (mode) <= HOST_BITS_PER_WIDE_INT)
	return gen_int_mode (i0, mode);

      gcc_assert (GET_MODE_BITSIZE (mode) == 2 * HOST_BITS_PER_WIDE_INT);
    }

  /* If this integer fits in one word, return a CONST_INT.  */
  if ((i1 == 0 && i0 >= 0) || (i1 == ~0 && i0 < 0))
    return GEN_INT (i0);

  /* We use VOIDmode for integers.  */
  value = rtx_alloc (CONST_DOUBLE);
  PUT_MODE (value, VOIDmode);

  CONST_DOUBLE_LOW (value) = i0;
  CONST_DOUBLE_HIGH (value) = i1;

  for (i = 2; i < (sizeof CONST_DOUBLE_FORMAT - 1); i++)
    XWINT (value, i) = 0;

  return lookup_const_double (value);
}

rtx
gen_rtx_REG (enum machine_mode mode, unsigned int regno)
{
  /* In case the MD file explicitly references the frame pointer, have
     all such references point to the same frame pointer.  This is
     used during frame pointer elimination to distinguish the explicit
     references to these registers from pseudos that happened to be
     assigned to them.

     If we have eliminated the frame pointer or arg pointer, we will
     be using it as a normal register, for example as a spill
     register.  In such cases, we might be accessing it in a mode that
     is not Pmode and therefore cannot use the pre-allocated rtx.

     Also don't do this when we are making new REGs in reload, since
     we don't want to get confused with the real pointers.  */

  if (mode == Pmode && !reload_in_progress)
    {
      if (regno == FRAME_POINTER_REGNUM
	  && (!reload_completed || frame_pointer_needed))
	return frame_pointer_rtx;
#if FRAME_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
      if (regno == HARD_FRAME_POINTER_REGNUM
	  && (!reload_completed || frame_pointer_needed))
	return hard_frame_pointer_rtx;
#endif
#if FRAME_POINTER_REGNUM != ARG_POINTER_REGNUM && HARD_FRAME_POINTER_REGNUM != ARG_POINTER_REGNUM
      if (regno == ARG_POINTER_REGNUM)
	return arg_pointer_rtx;
#endif
#ifdef RETURN_ADDRESS_POINTER_REGNUM
      if (regno == RETURN_ADDRESS_POINTER_REGNUM)
	return return_address_pointer_rtx;
#endif
      if (regno == (unsigned) PIC_OFFSET_TABLE_REGNUM
	  && fixed_regs[PIC_OFFSET_TABLE_REGNUM])
	return pic_offset_table_rtx;
      if (regno == STACK_POINTER_REGNUM)
	return stack_pointer_rtx;
    }

#if 0
  /* If the per-function register table has been set up, try to re-use
     an existing entry in that table to avoid useless generation of RTL.

     This code is disabled for now until we can fix the various backends
     which depend on having non-shared hard registers in some cases.   Long
     term we want to re-enable this code as it can significantly cut down
     on the amount of useless RTL that gets generated.

     We'll also need to fix some code that runs after reload that wants to
     set ORIGINAL_REGNO.  */

  if (cfun
      && cfun->emit
      && regno_reg_rtx
      && regno < FIRST_PSEUDO_REGISTER
      && reg_raw_mode[regno] == mode)
    return regno_reg_rtx[regno];
#endif

  return gen_raw_REG (mode, regno);
}

rtx
gen_rtx_MEM (enum machine_mode mode, rtx addr)
{
  rtx rt = gen_rtx_raw_MEM (mode, addr);

  /* This field is not cleared by the mere allocation of the rtx, so
     we clear it here.  */
  MEM_ATTRS (rt) = 0;

  return rt;
}

/* Generate a memory referring to non-trapping constant memory.  */

rtx
gen_const_mem (enum machine_mode mode, rtx addr)
{
  rtx mem = gen_rtx_MEM (mode, addr);
  MEM_READONLY_P (mem) = 1;
  MEM_NOTRAP_P (mem) = 1;
  return mem;
}

/* Generate a MEM referring to fixed portions of the frame, e.g., register
   save areas.  */

rtx
gen_frame_mem (enum machine_mode mode, rtx addr)
{
  rtx mem = gen_rtx_MEM (mode, addr);
  MEM_NOTRAP_P (mem) = 1;
  set_mem_alias_set (mem, get_frame_alias_set ());
  return mem;
}

/* Generate a MEM referring to a temporary use of the stack, not part
    of the fixed stack frame.  For example, something which is pushed
    by a target splitter.  */
rtx
gen_tmp_stack_mem (enum machine_mode mode, rtx addr)
{
  rtx mem = gen_rtx_MEM (mode, addr);
  MEM_NOTRAP_P (mem) = 1;
  if (!cfun->calls_alloca)
    set_mem_alias_set (mem, get_frame_alias_set ());
  return mem;
}

/* We want to create (subreg:OMODE (obj:IMODE) OFFSET).  Return true if
   this construct would be valid, and false otherwise.  */

bool
validate_subreg (enum machine_mode omode, enum machine_mode imode,
		 const_rtx reg, unsigned int offset)
{
  unsigned int isize = GET_MODE_SIZE (imode);
  unsigned int osize = GET_MODE_SIZE (omode);

  /* All subregs must be aligned.  */
  if (offset % osize != 0)
    return false;

  /* The subreg offset cannot be outside the inner object.  */
  if (offset >= isize)
    return false;

  /* ??? This should not be here.  Temporarily continue to allow word_mode
     subregs of anything.  The most common offender is (subreg:SI (reg:DF)).
     Generally, backends are doing something sketchy but it'll take time to
     fix them all.  */
  if (omode == word_mode)
    ;
  /* ??? Similarly, e.g. with (subreg:DF (reg:TI)).  Though store_bit_field
     is the culprit here, and not the backends.  */
  else if (osize >= UNITS_PER_WORD && isize >= osize)
    ;
  /* Allow component subregs of complex and vector.  Though given the below
     extraction rules, it's not always clear what that means.  */
  else if ((COMPLEX_MODE_P (imode) || VECTOR_MODE_P (imode))
	   && GET_MODE_INNER (imode) == omode)
    ;
  /* ??? x86 sse code makes heavy use of *paradoxical* vector subregs,
     i.e. (subreg:V4SF (reg:SF) 0).  This surely isn't the cleanest way to
     represent this.  It's questionable if this ought to be represented at
     all -- why can't this all be hidden in post-reload splitters that make
     arbitrarily mode changes to the registers themselves.  */
  else if (VECTOR_MODE_P (omode) && GET_MODE_INNER (omode) == imode)
    ;
  /* Subregs involving floating point modes are not allowed to
     change size.  Therefore (subreg:DI (reg:DF) 0) is fine, but
     (subreg:SI (reg:DF) 0) isn't.  */
  else if (FLOAT_MODE_P (imode) || FLOAT_MODE_P (omode))
    {
      if (isize != osize)
	return false;
    }

  /* Paradoxical subregs must have offset zero.  */
  if (osize > isize)
    return offset == 0;

  /* This is a normal subreg.  Verify that the offset is representable.  */

  /* For hard registers, we already have most of these rules collected in
     subreg_offset_representable_p.  */
  if (reg && REG_P (reg) && HARD_REGISTER_P (reg))
    {
      unsigned int regno = REGNO (reg);

#ifdef CANNOT_CHANGE_MODE_CLASS
      if ((COMPLEX_MODE_P (imode) || VECTOR_MODE_P (imode))
	  && GET_MODE_INNER (imode) == omode)
	;
      else if (REG_CANNOT_CHANGE_MODE_P (regno, imode, omode))
	return false;
#endif

      return subreg_offset_representable_p (regno, imode, offset, omode);
    }

  /* For pseudo registers, we want most of the same checks.  Namely:
     If the register no larger than a word, the subreg must be lowpart.
     If the register is larger than a word, the subreg must be the lowpart
     of a subword.  A subreg does *not* perform arbitrary bit extraction.
     Given that we've already checked mode/offset alignment, we only have
     to check subword subregs here.  */
  if (osize < UNITS_PER_WORD)
    {
      enum machine_mode wmode = isize > UNITS_PER_WORD ? word_mode : imode;
      unsigned int low_off = subreg_lowpart_offset (omode, wmode);
      if (offset % UNITS_PER_WORD != low_off)
	return false;
    }
  return true;
}

rtx
gen_rtx_SUBREG (enum machine_mode mode, rtx reg, int offset)
{
  gcc_assert (validate_subreg (mode, GET_MODE (reg), reg, offset));
  return gen_rtx_raw_SUBREG (mode, reg, offset);
}

/* Generate a SUBREG representing the least-significant part of REG if MODE
   is smaller than mode of REG, otherwise paradoxical SUBREG.  */

rtx
gen_lowpart_SUBREG (enum machine_mode mode, rtx reg)
{
  enum machine_mode inmode;

  inmode = GET_MODE (reg);
  if (inmode == VOIDmode)
    inmode = mode;
  return gen_rtx_SUBREG (mode, reg,
			 subreg_lowpart_offset (mode, inmode));
}


/* Create an rtvec and stores within it the RTXen passed in the arguments.  */

rtvec
gen_rtvec (int n, ...)
{
  int i;
  rtvec rt_val;
  va_list p;

  va_start (p, n);

  /* Don't allocate an empty rtvec...  */
  if (n == 0)
    return NULL_RTVEC;

  rt_val = rtvec_alloc (n);

  for (i = 0; i < n; i++)
    rt_val->elem[i] = va_arg (p, rtx);

  va_end (p);
  return rt_val;
}

rtvec
gen_rtvec_v (int n, rtx *argp)
{
  int i;
  rtvec rt_val;

  /* Don't allocate an empty rtvec...  */
  if (n == 0)
    return NULL_RTVEC;

  rt_val = rtvec_alloc (n);

  for (i = 0; i < n; i++)
    rt_val->elem[i] = *argp++;

  return rt_val;
}

/* Return the number of bytes between the start of an OUTER_MODE
   in-memory value and the start of an INNER_MODE in-memory value,
   given that the former is a lowpart of the latter.  It may be a
   paradoxical lowpart, in which case the offset will be negative
   on big-endian targets.  */

int
byte_lowpart_offset (enum machine_mode outer_mode,
		     enum machine_mode inner_mode)
{
  if (GET_MODE_SIZE (outer_mode) < GET_MODE_SIZE (inner_mode))
    return subreg_lowpart_offset (outer_mode, inner_mode);
  else
    return -subreg_lowpart_offset (inner_mode, outer_mode);
}

/* Generate a REG rtx for a new pseudo register of mode MODE.
   This pseudo is assigned the next sequential register number.  */

rtx
gen_reg_rtx (enum machine_mode mode)
{
  rtx val;
  unsigned int align = GET_MODE_ALIGNMENT (mode);

  gcc_assert (can_create_pseudo_p ());

  /* If a virtual register with bigger mode alignment is generated,
     increase stack alignment estimation because it might be spilled
     to stack later.  */
  if (SUPPORTS_STACK_ALIGNMENT 
      && crtl->stack_alignment_estimated < align
      && !crtl->stack_realign_processed)
    {
      unsigned int min_align = MINIMUM_ALIGNMENT (NULL, mode, align);
      if (crtl->stack_alignment_estimated < min_align)
	crtl->stack_alignment_estimated = min_align;
    }

  if (generating_concat_p
      && (GET_MODE_CLASS (mode) == MODE_COMPLEX_FLOAT
	  || GET_MODE_CLASS (mode) == MODE_COMPLEX_INT))
    {
      /* For complex modes, don't make a single pseudo.
	 Instead, make a CONCAT of two pseudos.
	 This allows noncontiguous allocation of the real and imaginary parts,
	 which makes much better code.  Besides, allocating DCmode
	 pseudos overstrains reload on some machines like the 386.  */
      rtx realpart, imagpart;
      enum machine_mode partmode = GET_MODE_INNER (mode);

      realpart = gen_reg_rtx (partmode);
      imagpart = gen_reg_rtx (partmode);
      return gen_rtx_CONCAT (mode, realpart, imagpart);
    }

  /* Make sure regno_pointer_align, and regno_reg_rtx are large
     enough to have an element for this pseudo reg number.  */

  if (reg_rtx_no == crtl->emit.regno_pointer_align_length)
    {
      int old_size = crtl->emit.regno_pointer_align_length;
      char *tmp;
      rtx *new1;

      tmp = XRESIZEVEC (char, crtl->emit.regno_pointer_align, old_size * 2);
      memset (tmp + old_size, 0, old_size);
      crtl->emit.regno_pointer_align = (unsigned char *) tmp;

      new1 = GGC_RESIZEVEC (rtx, regno_reg_rtx, old_size * 2);
      memset (new1 + old_size, 0, old_size * sizeof (rtx));
      regno_reg_rtx = new1;

      crtl->emit.regno_pointer_align_length = old_size * 2;
    }

  val = gen_raw_REG (mode, reg_rtx_no);
  regno_reg_rtx[reg_rtx_no++] = val;
  return val;
}

/* Update NEW with the same attributes as REG, but with OFFSET added
   to the REG_OFFSET.  */

static void
update_reg_offset (rtx new_rtx, rtx reg, int offset)
{
  REG_ATTRS (new_rtx) = get_reg_attrs (REG_EXPR (reg),
				   REG_OFFSET (reg) + offset);
}

/* Generate a register with same attributes as REG, but with OFFSET
   added to the REG_OFFSET.  */

rtx
gen_rtx_REG_offset (rtx reg, enum machine_mode mode, unsigned int regno,
		    int offset)
{
  rtx new_rtx = gen_rtx_REG (mode, regno);

  update_reg_offset (new_rtx, reg, offset);
  return new_rtx;
}

/* Generate a new pseudo-register with the same attributes as REG, but
   with OFFSET added to the REG_OFFSET.  */

rtx
gen_reg_rtx_offset (rtx reg, enum machine_mode mode, int offset)
{
  rtx new_rtx = gen_reg_rtx (mode);

  update_reg_offset (new_rtx, reg, offset);
  return new_rtx;
}

/* Adjust REG in-place so that it has mode MODE.  It is assumed that the
   new register is a (possibly paradoxical) lowpart of the old one.  */

void
adjust_reg_mode (rtx reg, enum machine_mode mode)
{
  update_reg_offset (reg, reg, byte_lowpart_offset (mode, GET_MODE (reg)));
  PUT_MODE (reg, mode);
}

/* Copy REG's attributes from X, if X has any attributes.  If REG and X
   have different modes, REG is a (possibly paradoxical) lowpart of X.  */

void
set_reg_attrs_from_value (rtx reg, rtx x)
{
  int offset;

  /* Hard registers can be reused for multiple purposes within the same
     function, so setting REG_ATTRS, REG_POINTER and REG_POINTER_ALIGN
     on them is wrong.  */
  if (HARD_REGISTER_P (reg))
    return;

  offset = byte_lowpart_offset (GET_MODE (reg), GET_MODE (x));
  if (MEM_P (x))
    {
      if (MEM_OFFSET (x) && GET_CODE (MEM_OFFSET (x)) == CONST_INT)
	REG_ATTRS (reg)
	  = get_reg_attrs (MEM_EXPR (x), INTVAL (MEM_OFFSET (x)) + offset);
      if (MEM_POINTER (x))
	mark_reg_pointer (reg, 0);
    }
  else if (REG_P (x))
    {
      if (REG_ATTRS (x))
	update_reg_offset (reg, x, offset);
      if (REG_POINTER (x))
	mark_reg_pointer (reg, REGNO_POINTER_ALIGN (REGNO (x)));
    }
}

/* Generate a REG rtx for a new pseudo register, copying the mode
   and attributes from X.  */

rtx
gen_reg_rtx_and_attrs (rtx x)
{
  rtx reg = gen_reg_rtx (GET_MODE (x));
  set_reg_attrs_from_value (reg, x);
  return reg;
}

/* Set the register attributes for registers contained in PARM_RTX.
   Use needed values from memory attributes of MEM.  */

void
set_reg_attrs_for_parm (rtx parm_rtx, rtx mem)
{
  if (REG_P (parm_rtx))
    set_reg_attrs_from_value (parm_rtx, mem);
  else if (GET_CODE (parm_rtx) == PARALLEL)
    {
      /* Check for a NULL entry in the first slot, used to indicate that the
	 parameter goes both on the stack and in registers.  */
      int i = XEXP (XVECEXP (parm_rtx, 0, 0), 0) ? 0 : 1;
      for (; i < XVECLEN (parm_rtx, 0); i++)
	{
	  rtx x = XVECEXP (parm_rtx, 0, i);
	  if (REG_P (XEXP (x, 0)))
	    REG_ATTRS (XEXP (x, 0))
	      = get_reg_attrs (MEM_EXPR (mem),
			       INTVAL (XEXP (x, 1)));
	}
    }
}

/* Set the REG_ATTRS for registers in value X, given that X represents
   decl T.  */

static void
set_reg_attrs_for_decl_rtl (tree t, rtx x)
{
  if (GET_CODE (x) == SUBREG)
    {
      gcc_assert (subreg_lowpart_p (x));
      x = SUBREG_REG (x);
    }
  if (REG_P (x))
    REG_ATTRS (x)
      = get_reg_attrs (t, byte_lowpart_offset (GET_MODE (x),
					       DECL_MODE (t)));
  if (GET_CODE (x) == CONCAT)
    {
      if (REG_P (XEXP (x, 0)))
        REG_ATTRS (XEXP (x, 0)) = get_reg_attrs (t, 0);
      if (REG_P (XEXP (x, 1)))
	REG_ATTRS (XEXP (x, 1))
	  = get_reg_attrs (t, GET_MODE_UNIT_SIZE (GET_MODE (XEXP (x, 0))));
    }
  if (GET_CODE (x) == PARALLEL)
    {
      int i, start;

      /* Check for a NULL entry, used to indicate that the parameter goes
	 both on the stack and in registers.  */
      if (XEXP (XVECEXP (x, 0, 0), 0))
	start = 0;
      else
	start = 1;

      for (i = start; i < XVECLEN (x, 0); i++)
	{
	  rtx y = XVECEXP (x, 0, i);
	  if (REG_P (XEXP (y, 0)))
	    REG_ATTRS (XEXP (y, 0)) = get_reg_attrs (t, INTVAL (XEXP (y, 1)));
	}
    }
}

/* Assign the RTX X to declaration T.  */

void
set_decl_rtl (tree t, rtx x)
{
  DECL_WRTL_CHECK (t)->decl_with_rtl.rtl = x;
  if (x)
    set_reg_attrs_for_decl_rtl (t, x);
}

/* Assign the RTX X to parameter declaration T.  BY_REFERENCE_P is true
   if the ABI requires the parameter to be passed by reference.  */

void
set_decl_incoming_rtl (tree t, rtx x, bool by_reference_p)
{
  DECL_INCOMING_RTL (t) = x;
  if (x && !by_reference_p)
    set_reg_attrs_for_decl_rtl (t, x);
}

/* Identify REG (which may be a CONCAT) as a user register.  */

void
mark_user_reg (rtx reg)
{
  if (GET_CODE (reg) == CONCAT)
    {
      REG_USERVAR_P (XEXP (reg, 0)) = 1;
      REG_USERVAR_P (XEXP (reg, 1)) = 1;
    }
  else
    {
      gcc_assert (REG_P (reg));
      REG_USERVAR_P (reg) = 1;
    }
}

/* Identify REG as a probable pointer register and show its alignment
   as ALIGN, if nonzero.  */

void
mark_reg_pointer (rtx reg, int align)
{
  if (! REG_POINTER (reg))
    {
      REG_POINTER (reg) = 1;

      if (align)
	REGNO_POINTER_ALIGN (REGNO (reg)) = align;
    }
  else if (align && align < REGNO_POINTER_ALIGN (REGNO (reg)))
    /* We can no-longer be sure just how aligned this pointer is.  */
    REGNO_POINTER_ALIGN (REGNO (reg)) = align;
}

/* Return 1 plus largest pseudo reg number used in the current function.  */

int
max_reg_num (void)
{
  return reg_rtx_no;
}

/* Return 1 + the largest label number used so far in the current function.  */

int
max_label_num (void)
{
  return label_num;
}

/* Return first label number used in this function (if any were used).  */

int
get_first_label_num (void)
{
  return first_label_num;
}

/* If the rtx for label was created during the expansion of a nested
   function, then first_label_num won't include this label number.
   Fix this now so that array indices work later.  */

void
maybe_set_first_label_num (rtx x)
{
  if (CODE_LABEL_NUMBER (x) < first_label_num)
    first_label_num = CODE_LABEL_NUMBER (x);
}

/* Return a value representing some low-order bits of X, where the number
   of low-order bits is given by MODE.  Note that no conversion is done
   between floating-point and fixed-point values, rather, the bit
   representation is returned.

   This function handles the cases in common between gen_lowpart, below,
   and two variants in cse.c and combine.c.  These are the cases that can
   be safely handled at all points in the compilation.

   If this is not a case we can handle, return 0.  */

rtx
gen_lowpart_common (enum machine_mode mode, rtx x)
{
  int msize = GET_MODE_SIZE (mode);
  int xsize;
  int offset = 0;
  enum machine_mode innermode;

  /* Unfortunately, this routine doesn't take a parameter for the mode of X,
     so we have to make one up.  Yuk.  */
  innermode = GET_MODE (x);
  if (GET_CODE (x) == CONST_INT
      && msize * BITS_PER_UNIT <= HOST_BITS_PER_WIDE_INT)
    innermode = mode_for_size (HOST_BITS_PER_WIDE_INT, MODE_INT, 0);
  else if (innermode == VOIDmode)
    innermode = mode_for_size (HOST_BITS_PER_WIDE_INT * 2, MODE_INT, 0);
  
  xsize = GET_MODE_SIZE (innermode);

  gcc_assert (innermode != VOIDmode && innermode != BLKmode);

  if (innermode == mode)
    return x;

  /* MODE must occupy no more words than the mode of X.  */
  if ((msize + (UNITS_PER_WORD - 1)) / UNITS_PER_WORD
      > ((xsize + (UNITS_PER_WORD - 1)) / UNITS_PER_WORD))
    return 0;

  /* Don't allow generating paradoxical FLOAT_MODE subregs.  */
  if (SCALAR_FLOAT_MODE_P (mode) && msize > xsize)
    return 0;

  offset = subreg_lowpart_offset (mode, innermode);

  if ((GET_CODE (x) == ZERO_EXTEND || GET_CODE (x) == SIGN_EXTEND)
      && (GET_MODE_CLASS (mode) == MODE_INT
	  || GET_MODE_CLASS (mode) == MODE_PARTIAL_INT))
    {
      /* If we are getting the low-order part of something that has been
	 sign- or zero-extended, we can either just use the object being
	 extended or make a narrower extension.  If we want an even smaller
	 piece than the size of the object being extended, call ourselves
	 recursively.

	 This case is used mostly by combine and cse.  */

      if (GET_MODE (XEXP (x, 0)) == mode)
	return XEXP (x, 0);
      else if (msize < GET_MODE_SIZE (GET_MODE (XEXP (x, 0))))
	return gen_lowpart_common (mode, XEXP (x, 0));
      else if (msize < xsize)
	return gen_rtx_fmt_e (GET_CODE (x), mode, XEXP (x, 0));
    }
  else if (GET_CODE (x) == SUBREG || REG_P (x)
	   || GET_CODE (x) == CONCAT || GET_CODE (x) == CONST_VECTOR
	   || GET_CODE (x) == CONST_DOUBLE || GET_CODE (x) == CONST_INT)
    return simplify_gen_subreg (mode, x, innermode, offset);

  /* Otherwise, we can't do this.  */
  return 0;
}

rtx
gen_highpart (enum machine_mode mode, rtx x)
{
  unsigned int msize = GET_MODE_SIZE (mode);
  rtx result;

  /* This case loses if X is a subreg.  To catch bugs early,
     complain if an invalid MODE is used even in other cases.  */
  gcc_assert (msize <= UNITS_PER_WORD
	      || msize == (unsigned int) GET_MODE_UNIT_SIZE (GET_MODE (x)));

  result = simplify_gen_subreg (mode, x, GET_MODE (x),
				subreg_highpart_offset (mode, GET_MODE (x)));
  gcc_assert (result);
  
  /* simplify_gen_subreg is not guaranteed to return a valid operand for
     the target if we have a MEM.  gen_highpart must return a valid operand,
     emitting code if necessary to do so.  */
  if (MEM_P (result))
    {
      result = validize_mem (result);
      gcc_assert (result);
    }
  
  return result;
}

/* Like gen_highpart, but accept mode of EXP operand in case EXP can
   be VOIDmode constant.  */
rtx
gen_highpart_mode (enum machine_mode outermode, enum machine_mode innermode, rtx exp)
{
  if (GET_MODE (exp) != VOIDmode)
    {
      gcc_assert (GET_MODE (exp) == innermode);
      return gen_highpart (outermode, exp);
    }
  return simplify_gen_subreg (outermode, exp, innermode,
			      subreg_highpart_offset (outermode, innermode));
}

/* Return the SUBREG_BYTE for an OUTERMODE lowpart of an INNERMODE value.  */

unsigned int
subreg_lowpart_offset (enum machine_mode outermode, enum machine_mode innermode)
{
  unsigned int offset = 0;
  int difference = (GET_MODE_SIZE (innermode) - GET_MODE_SIZE (outermode));

  if (difference > 0)
    {
      if (WORDS_BIG_ENDIAN)
	offset += (difference / UNITS_PER_WORD) * UNITS_PER_WORD;
      if (BYTES_BIG_ENDIAN)
	offset += difference % UNITS_PER_WORD;
    }

  return offset;
}

/* Return offset in bytes to get OUTERMODE high part
   of the value in mode INNERMODE stored in memory in target format.  */
unsigned int
subreg_highpart_offset (enum machine_mode outermode, enum machine_mode innermode)
{
  unsigned int offset = 0;
  int difference = (GET_MODE_SIZE (innermode) - GET_MODE_SIZE (outermode));

  gcc_assert (GET_MODE_SIZE (innermode) >= GET_MODE_SIZE (outermode));

  if (difference > 0)
    {
      if (! WORDS_BIG_ENDIAN)
	offset += (difference / UNITS_PER_WORD) * UNITS_PER_WORD;
      if (! BYTES_BIG_ENDIAN)
	offset += difference % UNITS_PER_WORD;
    }

  return offset;
}

/* Return 1 iff X, assumed to be a SUBREG,
   refers to the least significant part of its containing reg.
   If X is not a SUBREG, always return 1 (it is its own low part!).  */

int
subreg_lowpart_p (const_rtx x)
{
  if (GET_CODE (x) != SUBREG)
    return 1;
  else if (GET_MODE (SUBREG_REG (x)) == VOIDmode)
    return 0;

  return (subreg_lowpart_offset (GET_MODE (x), GET_MODE (SUBREG_REG (x)))
	  == SUBREG_BYTE (x));
}

/* Return subword OFFSET of operand OP.
   The word number, OFFSET, is interpreted as the word number starting
   at the low-order address.  OFFSET 0 is the low-order word if not
   WORDS_BIG_ENDIAN, otherwise it is the high-order word.

   If we cannot extract the required word, we return zero.  Otherwise,
   an rtx corresponding to the requested word will be returned.

   VALIDATE_ADDRESS is nonzero if the address should be validated.  Before
   reload has completed, a valid address will always be returned.  After
   reload, if a valid address cannot be returned, we return zero.

   If VALIDATE_ADDRESS is zero, we simply form the required address; validating
   it is the responsibility of the caller.

   MODE is the mode of OP in case it is a CONST_INT.

   ??? This is still rather broken for some cases.  The problem for the
   moment is that all callers of this thing provide no 'goal mode' to
   tell us to work with.  This exists because all callers were written
   in a word based SUBREG world.
   Now use of this function can be deprecated by simplify_subreg in most
   cases.
 */

rtx
operand_subword (rtx op, unsigned int offset, int validate_address, enum machine_mode mode)
{
  if (mode == VOIDmode)
    mode = GET_MODE (op);

  gcc_assert (mode != VOIDmode);

  /* If OP is narrower than a word, fail.  */
  if (mode != BLKmode
      && (GET_MODE_SIZE (mode) < UNITS_PER_WORD))
    return 0;

  /* If we want a word outside OP, return zero.  */
  if (mode != BLKmode
      && (offset + 1) * UNITS_PER_WORD > GET_MODE_SIZE (mode))
    return const0_rtx;

  /* Form a new MEM at the requested address.  */
  if (MEM_P (op))
    {
      rtx new_rtx = adjust_address_nv (op, word_mode, offset * UNITS_PER_WORD);

      if (! validate_address)
	return new_rtx;

      else if (reload_completed)
	{
	  if (! strict_memory_address_p (word_mode, XEXP (new_rtx, 0)))
	    return 0;
	}
      else
	return replace_equiv_address (new_rtx, XEXP (new_rtx, 0));
    }

  /* Rest can be handled by simplify_subreg.  */
  return simplify_gen_subreg (word_mode, op, mode, (offset * UNITS_PER_WORD));
}

/* Similar to `operand_subword', but never return 0.  If we can't
   extract the required subword, put OP into a register and try again.
   The second attempt must succeed.  We always validate the address in
   this case.

   MODE is the mode of OP, in case it is CONST_INT.  */

rtx
operand_subword_force (rtx op, unsigned int offset, enum machine_mode mode)
{
  rtx result = operand_subword (op, offset, 1, mode);

  if (result)
    return result;

  if (mode != BLKmode && mode != VOIDmode)
    {
      /* If this is a register which can not be accessed by words, copy it
	 to a pseudo register.  */
      if (REG_P (op))
	op = copy_to_reg (op);
      else
	op = force_reg (mode, op);
    }

  result = operand_subword (op, offset, 1, mode);
  gcc_assert (result);

  return result;
}

/* Within a MEM_EXPR, we care about either (1) a component ref of a decl,
   or (2) a component ref of something variable.  Represent the later with
   a NULL expression.  */

static tree
component_ref_for_mem_expr (tree ref)
{
  tree inner = TREE_OPERAND (ref, 0);

  if (TREE_CODE (inner) == COMPONENT_REF)
    inner = component_ref_for_mem_expr (inner);
  else
    {
      /* Now remove any conversions: they don't change what the underlying
	 object is.  Likewise for SAVE_EXPR.  */
      while (CONVERT_EXPR_P (inner)
	     || TREE_CODE (inner) == VIEW_CONVERT_EXPR
	     || TREE_CODE (inner) == SAVE_EXPR)
	inner = TREE_OPERAND (inner, 0);

      if (! DECL_P (inner))
	inner = NULL_TREE;
    }

  if (inner == TREE_OPERAND (ref, 0))
    return ref;
  else
    return build3 (COMPONENT_REF, TREE_TYPE (ref), inner,
		   TREE_OPERAND (ref, 1), NULL_TREE);
}

/* Returns 1 if both MEM_EXPR can be considered equal
   and 0 otherwise.  */

int
mem_expr_equal_p (const_tree expr1, const_tree expr2)
{
  if (expr1 == expr2)
    return 1;

  if (! expr1 || ! expr2)
    return 0;

  if (TREE_CODE (expr1) != TREE_CODE (expr2))
    return 0;

  if (TREE_CODE (expr1) == COMPONENT_REF)
    return 
      mem_expr_equal_p (TREE_OPERAND (expr1, 0),
			TREE_OPERAND (expr2, 0))
      && mem_expr_equal_p (TREE_OPERAND (expr1, 1), /* field decl */
			   TREE_OPERAND (expr2, 1));
  
  if (INDIRECT_REF_P (expr1))
    return mem_expr_equal_p (TREE_OPERAND (expr1, 0),
			     TREE_OPERAND (expr2, 0));

  /* ARRAY_REFs, ARRAY_RANGE_REFs and BIT_FIELD_REFs should already
	      have been resolved here.  */
  gcc_assert (DECL_P (expr1));
  
  /* Decls with different pointers can't be equal.  */
  return 0;
}

/* Return OFFSET if XEXP (MEM, 0) - OFFSET is known to be ALIGN
   bits aligned for 0 <= OFFSET < ALIGN / BITS_PER_UNIT, or
   -1 if not known.  */

int
get_mem_align_offset (rtx mem, int align)
{
  tree expr;
  unsigned HOST_WIDE_INT offset;

  /* This function can't use
     if (!MEM_EXPR (mem) || !MEM_OFFSET (mem)
	 || !CONST_INT_P (MEM_OFFSET (mem))
	 || (get_object_alignment (MEM_EXPR (mem), MEM_ALIGN (mem), align)
	     < align))
       return -1;
     else
       return (- INTVAL (MEM_OFFSET (mem))) & (align / BITS_PER_UNIT - 1);
     for two reasons:
     - COMPONENT_REFs in MEM_EXPR can have NULL first operand,
       for <variable>.  get_inner_reference doesn't handle it and
       even if it did, the alignment in that case needs to be determined
       from DECL_FIELD_CONTEXT's TYPE_ALIGN.
     - it would do suboptimal job for COMPONENT_REFs, even if MEM_EXPR
       isn't sufficiently aligned, the object it is in might be.  */
  gcc_assert (MEM_P (mem));
  expr = MEM_EXPR (mem);
  if (expr == NULL_TREE
      || MEM_OFFSET (mem) == NULL_RTX
      || !CONST_INT_P (MEM_OFFSET (mem)))
    return -1;

  offset = INTVAL (MEM_OFFSET (mem));
  if (DECL_P (expr))
    {
      if (DECL_ALIGN (expr) < align)
	return -1;
    }
  else if (INDIRECT_REF_P (expr))
    {
      if (TYPE_ALIGN (TREE_TYPE (expr)) < (unsigned int) align)
	return -1;
    }
  else if (TREE_CODE (expr) == COMPONENT_REF)
    {
      while (1)
	{
	  tree inner = TREE_OPERAND (expr, 0);
	  tree field = TREE_OPERAND (expr, 1);
	  tree byte_offset = component_ref_field_offset (expr);
	  tree bit_offset = DECL_FIELD_BIT_OFFSET (field);

	  if (!byte_offset
	      || !host_integerp (byte_offset, 1)
	      || !host_integerp (bit_offset, 1))
	    return -1;

	  offset += tree_low_cst (byte_offset, 1);
	  offset += tree_low_cst (bit_offset, 1) / BITS_PER_UNIT;

	  if (inner == NULL_TREE)
	    {
	      if (TYPE_ALIGN (DECL_FIELD_CONTEXT (field))
		  < (unsigned int) align)
		return -1;
	      break;
	    }
	  else if (DECL_P (inner))
	    {
	      if (DECL_ALIGN (inner) < align)
		return -1;
	      break;
	    }
	  else if (TREE_CODE (inner) != COMPONENT_REF)
	    return -1;
	  expr = inner;
	}
    }
  else
    return -1;

  return offset & ((align / BITS_PER_UNIT) - 1);
}

/* Given REF (a MEM) and T, either the type of X or the expression
   corresponding to REF, set the memory attributes.  OBJECTP is nonzero
   if we are making a new object of this type.  BITPOS is nonzero if
   there is an offset outstanding on T that will be applied later.  */

void
set_mem_attributes_minus_bitpos (rtx ref, tree t, int objectp,
				 HOST_WIDE_INT bitpos)
{
  alias_set_type alias = MEM_ALIAS_SET (ref);
  tree expr = MEM_EXPR (ref);
  rtx offset = MEM_OFFSET (ref);
  rtx size = MEM_SIZE (ref);
  unsigned int align = MEM_ALIGN (ref);
  HOST_WIDE_INT apply_bitpos = 0;
  tree type;

  /* It can happen that type_for_mode was given a mode for which there
     is no language-level type.  In which case it returns NULL, which
     we can see here.  */
  if (t == NULL_TREE)
    return;

  type = TYPE_P (t) ? t : TREE_TYPE (t);
  if (type == error_mark_node)
    return;

  /* If we have already set DECL_RTL = ref, get_alias_set will get the
     wrong answer, as it assumes that DECL_RTL already has the right alias
     info.  Callers should not set DECL_RTL until after the call to
     set_mem_attributes.  */
  gcc_assert (!DECL_P (t) || ref != DECL_RTL_IF_SET (t));

  /* Get the alias set from the expression or type (perhaps using a
     front-end routine) and use it.  */
  alias = get_alias_set (t);

  MEM_VOLATILE_P (ref) |= TYPE_VOLATILE (type);
  MEM_IN_STRUCT_P (ref)
    = AGGREGATE_TYPE_P (type) || TREE_CODE (type) == COMPLEX_TYPE;
  MEM_POINTER (ref) = POINTER_TYPE_P (type);

  /* If we are making an object of this type, or if this is a DECL, we know
     that it is a scalar if the type is not an aggregate.  */
  if ((objectp || DECL_P (t))
      && ! AGGREGATE_TYPE_P (type)
      && TREE_CODE (type) != COMPLEX_TYPE)
    MEM_SCALAR_P (ref) = 1;

  /* We can set the alignment from the type if we are making an object,
     this is an INDIRECT_REF, or if TYPE_ALIGN_OK.  */
  if (objectp || TREE_CODE (t) == INDIRECT_REF 
      || TREE_CODE (t) == ALIGN_INDIRECT_REF 
      || TYPE_ALIGN_OK (type))
    align = MAX (align, TYPE_ALIGN (type));
  else 
    if (TREE_CODE (t) == MISALIGNED_INDIRECT_REF)
      {
	if (integer_zerop (TREE_OPERAND (t, 1)))
	  /* We don't know anything about the alignment.  */
	  align = BITS_PER_UNIT;
	else
	  align = tree_low_cst (TREE_OPERAND (t, 1), 1);
      }

  /* If the size is known, we can set that.  */
  if (TYPE_SIZE_UNIT (type) && host_integerp (TYPE_SIZE_UNIT (type), 1))
    size = GEN_INT (tree_low_cst (TYPE_SIZE_UNIT (type), 1));

  /* If T is not a type, we may be able to deduce some more information about
     the expression.  */
  if (! TYPE_P (t))
    {
      tree base;
      bool align_computed = false;

      if (TREE_THIS_VOLATILE (t))
	MEM_VOLATILE_P (ref) = 1;

      /* Now remove any conversions: they don't change what the underlying
	 object is.  Likewise for SAVE_EXPR.  */
      while (CONVERT_EXPR_P (t)
	     || TREE_CODE (t) == VIEW_CONVERT_EXPR
	     || TREE_CODE (t) == SAVE_EXPR)
	t = TREE_OPERAND (t, 0);

      /* We may look through structure-like accesses for the purposes of
	 examining TREE_THIS_NOTRAP, but not array-like accesses.  */
      base = t;
      while (TREE_CODE (base) == COMPONENT_REF
	     || TREE_CODE (base) == REALPART_EXPR
	     || TREE_CODE (base) == IMAGPART_EXPR
	     || TREE_CODE (base) == BIT_FIELD_REF)
	base = TREE_OPERAND (base, 0);

      if (DECL_P (base))
	{
	  if (CODE_CONTAINS_STRUCT (TREE_CODE (base), TS_DECL_WITH_VIS))
	    MEM_NOTRAP_P (ref) = !DECL_WEAK (base);
	  else
	    MEM_NOTRAP_P (ref) = 1;
	}
      else
	MEM_NOTRAP_P (ref) = TREE_THIS_NOTRAP (base);

      base = get_base_address (base);
      if (base && DECL_P (base)
	  && TREE_READONLY (base)
	  && (TREE_STATIC (base) || DECL_EXTERNAL (base)))
	{
	  tree base_type = TREE_TYPE (base);
	  gcc_assert (!(base_type && TYPE_NEEDS_CONSTRUCTING (base_type))
		      || DECL_ARTIFICIAL (base));
	  MEM_READONLY_P (ref) = 1;
	}

      /* If this expression uses it's parent's alias set, mark it such
	 that we won't change it.  */
      if (component_uses_parent_alias_set (t))
	MEM_KEEP_ALIAS_SET_P (ref) = 1;

      /* If this is a decl, set the attributes of the MEM from it.  */
      if (DECL_P (t))
	{
	  expr = t;
	  offset = const0_rtx;
	  apply_bitpos = bitpos;
	  size = (DECL_SIZE_UNIT (t)
		  && host_integerp (DECL_SIZE_UNIT (t), 1)
		  ? GEN_INT (tree_low_cst (DECL_SIZE_UNIT (t), 1)) : 0);
	  align = DECL_ALIGN (t);
	  align_computed = true;
	}

      /* If this is a constant, we know the alignment.  */
      else if (CONSTANT_CLASS_P (t))
	{
	  align = TYPE_ALIGN (type);
#ifdef CONSTANT_ALIGNMENT
	  align = CONSTANT_ALIGNMENT (t, align);
#endif
	  align_computed = true;
	}

      /* If this is a field reference and not a bit-field, record it.  */
      /* ??? There is some information that can be gleaned from bit-fields,
	 such as the word offset in the structure that might be modified.
	 But skip it for now.  */
      else if (TREE_CODE (t) == COMPONENT_REF
	       && ! DECL_BIT_FIELD (TREE_OPERAND (t, 1)))
	{
	  expr = component_ref_for_mem_expr (t);
	  offset = const0_rtx;
	  apply_bitpos = bitpos;
	  /* ??? Any reason the field size would be different than
	     the size we got from the type?  */
	}

      /* If this is an array reference, look for an outer field reference.  */
      else if (TREE_CODE (t) == ARRAY_REF)
	{
	  tree off_tree = size_zero_node;
	  /* We can't modify t, because we use it at the end of the
	     function.  */
	  tree t2 = t;

	  do
	    {
	      tree index = TREE_OPERAND (t2, 1);
	      tree low_bound = array_ref_low_bound (t2);
	      tree unit_size = array_ref_element_size (t2);

	      /* We assume all arrays have sizes that are a multiple of a byte.
		 First subtract the lower bound, if any, in the type of the
		 index, then convert to sizetype and multiply by the size of
		 the array element.  */
	      if (! integer_zerop (low_bound))
		index = fold_build2 (MINUS_EXPR, TREE_TYPE (index),
				     index, low_bound);

	      off_tree = size_binop (PLUS_EXPR,
				     size_binop (MULT_EXPR,
						 fold_convert (sizetype,
							       index),
						 unit_size),
				     off_tree);
	      t2 = TREE_OPERAND (t2, 0);
	    }
	  while (TREE_CODE (t2) == ARRAY_REF);

	  if (DECL_P (t2))
	    {
	      expr = t2;
	      offset = NULL;
	      if (host_integerp (off_tree, 1))
		{
		  HOST_WIDE_INT ioff = tree_low_cst (off_tree, 1);
		  HOST_WIDE_INT aoff = (ioff & -ioff) * BITS_PER_UNIT;
		  align = DECL_ALIGN (t2);
		  if (aoff && (unsigned HOST_WIDE_INT) aoff < align)
	            align = aoff;
		  align_computed = true;
		  offset = GEN_INT (ioff);
		  apply_bitpos = bitpos;
		}
	    }
	  else if (TREE_CODE (t2) == COMPONENT_REF)
	    {
	      expr = component_ref_for_mem_expr (t2);
	      if (host_integerp (off_tree, 1))
		{
		  offset = GEN_INT (tree_low_cst (off_tree, 1));
		  apply_bitpos = bitpos;
		}
	      /* ??? Any reason the field size would be different than
		 the size we got from the type?  */
	    }
	  else if (flag_argument_noalias > 1
		   && (INDIRECT_REF_P (t2))
		   && TREE_CODE (TREE_OPERAND (t2, 0)) == PARM_DECL)
	    {
	      expr = t2;
	      offset = NULL;
	    }
	}

      /* If this is a Fortran indirect argument reference, record the
	 parameter decl.  */
      else if (flag_argument_noalias > 1
	       && (INDIRECT_REF_P (t))
	       && TREE_CODE (TREE_OPERAND (t, 0)) == PARM_DECL)
	{
	  expr = t;
	  offset = NULL;
	}

      if (!align_computed && !INDIRECT_REF_P (t))
	{
	  unsigned int obj_align
	    = get_object_alignment (t, align, BIGGEST_ALIGNMENT);
	  align = MAX (align, obj_align);
	}
    }

  /* If we modified OFFSET based on T, then subtract the outstanding
     bit position offset.  Similarly, increase the size of the accessed
     object to contain the negative offset.  */
  if (apply_bitpos)
    {
      offset = plus_constant (offset, -(apply_bitpos / BITS_PER_UNIT));
      if (size)
	size = plus_constant (size, apply_bitpos / BITS_PER_UNIT);
    }

  if (TREE_CODE (t) == ALIGN_INDIRECT_REF)
    {
      /* Force EXPR and OFFSET to NULL, since we don't know exactly what
	 we're overlapping.  */
      offset = NULL;
      expr = NULL;
    }

  /* Now set the attributes we computed above.  */
  MEM_ATTRS (ref)
    = get_mem_attrs (alias, expr, offset, size, align, GET_MODE (ref));

  /* If this is already known to be a scalar or aggregate, we are done.  */
  if (MEM_IN_STRUCT_P (ref) || MEM_SCALAR_P (ref))
    return;

  /* If it is a reference into an aggregate, this is part of an aggregate.
     Otherwise we don't know.  */
  else if (TREE_CODE (t) == COMPONENT_REF || TREE_CODE (t) == ARRAY_REF
	   || TREE_CODE (t) == ARRAY_RANGE_REF
	   || TREE_CODE (t) == BIT_FIELD_REF)
    MEM_IN_STRUCT_P (ref) = 1;
}

void
set_mem_attributes (rtx ref, tree t, int objectp)
{
  set_mem_attributes_minus_bitpos (ref, t, objectp, 0);
}

/* Set MEM to the decl that REG refers to.  */

void
set_mem_attrs_from_reg (rtx mem, rtx reg)
{
  MEM_ATTRS (mem)
    = get_mem_attrs (MEM_ALIAS_SET (mem), REG_EXPR (reg),
		     GEN_INT (REG_OFFSET (reg)),
		     MEM_SIZE (mem), MEM_ALIGN (mem), GET_MODE (mem));
}

/* Set the alias set of MEM to SET.  */

void
set_mem_alias_set (rtx mem, alias_set_type set)
{
#ifdef ENABLE_CHECKING
  /* If the new and old alias sets don't conflict, something is wrong.  */
  gcc_assert (alias_sets_conflict_p (set, MEM_ALIAS_SET (mem)));
#endif

  MEM_ATTRS (mem) = get_mem_attrs (set, MEM_EXPR (mem), MEM_OFFSET (mem),
				   MEM_SIZE (mem), MEM_ALIGN (mem),
				   GET_MODE (mem));
}

/* Set the alignment of MEM to ALIGN bits.  */

void
set_mem_align (rtx mem, unsigned int align)
{
  MEM_ATTRS (mem) = get_mem_attrs (MEM_ALIAS_SET (mem), MEM_EXPR (mem),
				   MEM_OFFSET (mem), MEM_SIZE (mem), align,
				   GET_MODE (mem));
}

/* Set the expr for MEM to EXPR.  */

void
set_mem_expr (rtx mem, tree expr)
{
  MEM_ATTRS (mem)
    = get_mem_attrs (MEM_ALIAS_SET (mem), expr, MEM_OFFSET (mem),
		     MEM_SIZE (mem), MEM_ALIGN (mem), GET_MODE (mem));
}

/* Set the offset of MEM to OFFSET.  */

void
set_mem_offset (rtx mem, rtx offset)
{
  MEM_ATTRS (mem) = get_mem_attrs (MEM_ALIAS_SET (mem), MEM_EXPR (mem),
				   offset, MEM_SIZE (mem), MEM_ALIGN (mem),
				   GET_MODE (mem));
}

/* Set the size of MEM to SIZE.  */

void
set_mem_size (rtx mem, rtx size)
{
  MEM_ATTRS (mem) = get_mem_attrs (MEM_ALIAS_SET (mem), MEM_EXPR (mem),
				   MEM_OFFSET (mem), size, MEM_ALIGN (mem),
				   GET_MODE (mem));
}

/* Return a memory reference like MEMREF, but with its mode changed to MODE
   and its address changed to ADDR.  (VOIDmode means don't change the mode.
   NULL for ADDR means don't change the address.)  VALIDATE is nonzero if the
   returned memory location is required to be valid.  The memory
   attributes are not changed.  */

static rtx
change_address_1 (rtx memref, enum machine_mode mode, rtx addr, int validate)
{
  rtx new_rtx;

  gcc_assert (MEM_P (memref));
  if (mode == VOIDmode)
    mode = GET_MODE (memref);
  if (addr == 0)
    addr = XEXP (memref, 0);
  if (mode == GET_MODE (memref) && addr == XEXP (memref, 0)
      && (!validate || memory_address_p (mode, addr)))
    return memref;

  if (validate)
    {
      if (reload_in_progress || reload_completed)
	gcc_assert (memory_address_p (mode, addr));
      else
	addr = memory_address (mode, addr);
    }

  if (rtx_equal_p (addr, XEXP (memref, 0)) && mode == GET_MODE (memref))
    return memref;

  new_rtx = gen_rtx_MEM (mode, addr);
  MEM_COPY_ATTRIBUTES (new_rtx, memref);
  return new_rtx;
}

/* Like change_address_1 with VALIDATE nonzero, but we are not saying in what
   way we are changing MEMREF, so we only preserve the alias set.  */

rtx
change_address (rtx memref, enum machine_mode mode, rtx addr)
{
  rtx new_rtx = change_address_1 (memref, mode, addr, 1), size;
  enum machine_mode mmode = GET_MODE (new_rtx);
  unsigned int align;

  size = mmode == BLKmode ? 0 : GEN_INT (GET_MODE_SIZE (mmode));
  align = mmode == BLKmode ? BITS_PER_UNIT : GET_MODE_ALIGNMENT (mmode);

  /* If there are no changes, just return the original memory reference.  */
  if (new_rtx == memref)
    {
      if (MEM_ATTRS (memref) == 0
	  || (MEM_EXPR (memref) == NULL
	      && MEM_OFFSET (memref) == NULL
	      && MEM_SIZE (memref) == size
	      && MEM_ALIGN (memref) == align))
	return new_rtx;

      new_rtx = gen_rtx_MEM (mmode, XEXP (memref, 0));
      MEM_COPY_ATTRIBUTES (new_rtx, memref);
    }

  MEM_ATTRS (new_rtx)
    = get_mem_attrs (MEM_ALIAS_SET (memref), 0, 0, size, align, mmode);

  return new_rtx;
}

/* Return a memory reference like MEMREF, but with its mode changed
   to MODE and its address offset by OFFSET bytes.  If VALIDATE is
   nonzero, the memory address is forced to be valid.
   If ADJUST is zero, OFFSET is only used to update MEM_ATTRS
   and caller is responsible for adjusting MEMREF base register.  */

rtx
adjust_address_1 (rtx memref, enum machine_mode mode, HOST_WIDE_INT offset,
		  int validate, int adjust)
{
  rtx addr = XEXP (memref, 0);
  rtx new_rtx;
  rtx memoffset = MEM_OFFSET (memref);
  rtx size = 0;
  unsigned int memalign = MEM_ALIGN (memref);
  int pbits;

  /* If there are no changes, just return the original memory reference.  */
  if (mode == GET_MODE (memref) && !offset
      && (!validate || memory_address_p (mode, addr)))
    return memref;

  /* ??? Prefer to create garbage instead of creating shared rtl.
     This may happen even if offset is nonzero -- consider
     (plus (plus reg reg) const_int) -- so do this always.  */
  addr = copy_rtx (addr);

  /* Convert a possibly large offset to a signed value within the
     range of the target address space.  */
  pbits = GET_MODE_BITSIZE (Pmode);
  if (HOST_BITS_PER_WIDE_INT > pbits)
    {
      int shift = HOST_BITS_PER_WIDE_INT - pbits;
      offset = (((HOST_WIDE_INT) ((unsigned HOST_WIDE_INT) offset << shift))
		>> shift);
    }

  if (adjust)
    {
      /* If MEMREF is a LO_SUM and the offset is within the alignment of the
	 object, we can merge it into the LO_SUM.  */
      if (GET_MODE (memref) != BLKmode && GET_CODE (addr) == LO_SUM
	  && offset >= 0
	  && (unsigned HOST_WIDE_INT) offset
	      < GET_MODE_ALIGNMENT (GET_MODE (memref)) / BITS_PER_UNIT)
	addr = gen_rtx_LO_SUM (Pmode, XEXP (addr, 0),
			       plus_constant (XEXP (addr, 1), offset));
      else
	addr = plus_constant (addr, offset);
    }

  new_rtx = change_address_1 (memref, mode, addr, validate);

  /* If the address is a REG, change_address_1 rightfully returns memref,
     but this would destroy memref's MEM_ATTRS.  */
  if (new_rtx == memref && offset != 0)
    new_rtx = copy_rtx (new_rtx);

  /* Compute the new values of the memory attributes due to this adjustment.
     We add the offsets and update the alignment.  */
  if (memoffset)
    memoffset = GEN_INT (offset + INTVAL (memoffset));

  /* Compute the new alignment by taking the MIN of the alignment and the
     lowest-order set bit in OFFSET, but don't change the alignment if OFFSET
     if zero.  */
  if (offset != 0)
    memalign
      = MIN (memalign,
	     (unsigned HOST_WIDE_INT) (offset & -offset) * BITS_PER_UNIT);

  /* We can compute the size in a number of ways.  */
  if (GET_MODE (new_rtx) != BLKmode)
    size = GEN_INT (GET_MODE_SIZE (GET_MODE (new_rtx)));
  else if (MEM_SIZE (memref))
    size = plus_constant (MEM_SIZE (memref), -offset);

  MEM_ATTRS (new_rtx) = get_mem_attrs (MEM_ALIAS_SET (memref), MEM_EXPR (memref),
				   memoffset, size, memalign, GET_MODE (new_rtx));

  /* At some point, we should validate that this offset is within the object,
     if all the appropriate values are known.  */
  return new_rtx;
}

/* Return a memory reference like MEMREF, but with its mode changed
   to MODE and its address changed to ADDR, which is assumed to be
   MEMREF offset by OFFSET bytes.  If VALIDATE is
   nonzero, the memory address is forced to be valid.  */

rtx
adjust_automodify_address_1 (rtx memref, enum machine_mode mode, rtx addr,
			     HOST_WIDE_INT offset, int validate)
{
  memref = change_address_1 (memref, VOIDmode, addr, validate);
  return adjust_address_1 (memref, mode, offset, validate, 0);
}

/* Return a memory reference like MEMREF, but whose address is changed by
   adding OFFSET, an RTX, to it.  POW2 is the highest power of two factor
   known to be in OFFSET (possibly 1).  */

rtx
offset_address (rtx memref, rtx offset, unsigned HOST_WIDE_INT pow2)
{
  rtx new_rtx, addr = XEXP (memref, 0);

  new_rtx = simplify_gen_binary (PLUS, Pmode, addr, offset);

  /* At this point we don't know _why_ the address is invalid.  It
     could have secondary memory references, multiplies or anything.

     However, if we did go and rearrange things, we can wind up not
     being able to recognize the magic around pic_offset_table_rtx.
     This stuff is fragile, and is yet another example of why it is
     bad to expose PIC machinery too early.  */
  if (! memory_address_p (GET_MODE (memref), new_rtx)
      && GET_CODE (addr) == PLUS
      && XEXP (addr, 0) == pic_offset_table_rtx)
    {
      addr = force_reg (GET_MODE (addr), addr);
      new_rtx = simplify_gen_binary (PLUS, Pmode, addr, offset);
    }

  update_temp_slot_address (XEXP (memref, 0), new_rtx);
  new_rtx = change_address_1 (memref, VOIDmode, new_rtx, 1);

  /* If there are no changes, just return the original memory reference.  */
  if (new_rtx == memref)
    return new_rtx;

  /* Update the alignment to reflect the offset.  Reset the offset, which
     we don't know.  */
  MEM_ATTRS (new_rtx)
    = get_mem_attrs (MEM_ALIAS_SET (memref), MEM_EXPR (memref), 0, 0,
		     MIN (MEM_ALIGN (memref), pow2 * BITS_PER_UNIT),
		     GET_MODE (new_rtx));
  return new_rtx;
}

/* Return a memory reference like MEMREF, but with its address changed to
   ADDR.  The caller is asserting that the actual piece of memory pointed
   to is the same, just the form of the address is being changed, such as
   by putting something into a register.  */

rtx
replace_equiv_address (rtx memref, rtx addr)
{
  /* change_address_1 copies the memory attribute structure without change
     and that's exactly what we want here.  */
  update_temp_slot_address (XEXP (memref, 0), addr);
  return change_address_1 (memref, VOIDmode, addr, 1);
}

/* Likewise, but the reference is not required to be valid.  */

rtx
replace_equiv_address_nv (rtx memref, rtx addr)
{
  return change_address_1 (memref, VOIDmode, addr, 0);
}

/* Return a memory reference like MEMREF, but with its mode widened to
   MODE and offset by OFFSET.  This would be used by targets that e.g.
   cannot issue QImode memory operations and have to use SImode memory
   operations plus masking logic.  */

rtx
widen_memory_access (rtx memref, enum machine_mode mode, HOST_WIDE_INT offset)
{
  rtx new_rtx = adjust_address_1 (memref, mode, offset, 1, 1);
  tree expr = MEM_EXPR (new_rtx);
  rtx memoffset = MEM_OFFSET (new_rtx);
  unsigned int size = GET_MODE_SIZE (mode);

  /* If there are no changes, just return the original memory reference.  */
  if (new_rtx == memref)
    return new_rtx;

  /* If we don't know what offset we were at within the expression, then
     we can't know if we've overstepped the bounds.  */
  if (! memoffset)
    expr = NULL_TREE;

  while (expr)
    {
      if (TREE_CODE (expr) == COMPONENT_REF)
	{
	  tree field = TREE_OPERAND (expr, 1);
	  tree offset = component_ref_field_offset (expr);

	  if (! DECL_SIZE_UNIT (field))
	    {
	      expr = NULL_TREE;
	      break;
	    }

	  /* Is the field at least as large as the access?  If so, ok,
	     otherwise strip back to the containing structure.  */
	  if (TREE_CODE (DECL_SIZE_UNIT (field)) == INTEGER_CST
	      && compare_tree_int (DECL_SIZE_UNIT (field), size) >= 0
	      && INTVAL (memoffset) >= 0)
	    break;

	  if (! host_integerp (offset, 1))
	    {
	      expr = NULL_TREE;
	      break;
	    }

	  expr = TREE_OPERAND (expr, 0);
	  memoffset
	    = (GEN_INT (INTVAL (memoffset)
			+ tree_low_cst (offset, 1)
			+ (tree_low_cst (DECL_FIELD_BIT_OFFSET (field), 1)
			   / BITS_PER_UNIT)));
	}
      /* Similarly for the decl.  */
      else if (DECL_P (expr)
	       && DECL_SIZE_UNIT (expr)
	       && TREE_CODE (DECL_SIZE_UNIT (expr)) == INTEGER_CST
	       && compare_tree_int (DECL_SIZE_UNIT (expr), size) >= 0
	       && (! memoffset || INTVAL (memoffset) >= 0))
	break;
      else
	{
	  /* The widened memory access overflows the expression, which means
	     that it could alias another expression.  Zap it.  */
	  expr = NULL_TREE;
	  break;
	}
    }

  if (! expr)
    memoffset = NULL_RTX;

  /* The widened memory may alias other stuff, so zap the alias set.  */
  /* ??? Maybe use get_alias_set on any remaining expression.  */

  MEM_ATTRS (new_rtx) = get_mem_attrs (0, expr, memoffset, GEN_INT (size),
				   MEM_ALIGN (new_rtx), mode);

  return new_rtx;
}

/* A fake decl that is used as the MEM_EXPR of spill slots.  */
static GTY(()) tree spill_slot_decl;

tree
get_spill_slot_decl (bool force_build_p)
{
  tree d = spill_slot_decl;
  rtx rd;

  if (d || !force_build_p)
    return d;

  d = build_decl (VAR_DECL, get_identifier ("%sfp"), void_type_node);
  DECL_ARTIFICIAL (d) = 1;
  DECL_IGNORED_P (d) = 1;
  TREE_USED (d) = 1;
  TREE_THIS_NOTRAP (d) = 1;
  spill_slot_decl = d;

  rd = gen_rtx_MEM (BLKmode, frame_pointer_rtx);
  MEM_NOTRAP_P (rd) = 1;
  MEM_ATTRS (rd) = get_mem_attrs (new_alias_set (), d, const0_rtx,
				  NULL_RTX, 0, BLKmode);
  SET_DECL_RTL (d, rd);

  return d;
}

/* Given MEM, a result from assign_stack_local, fill in the memory
   attributes as appropriate for a register allocator spill slot.
   These slots are not aliasable by other memory.  We arrange for
   them all to use a single MEM_EXPR, so that the aliasing code can
   work properly in the case of shared spill slots.  */

void
set_mem_attrs_for_spill (rtx mem)
{
  alias_set_type alias;
  rtx addr, offset;
  tree expr;

  expr = get_spill_slot_decl (true);
  alias = MEM_ALIAS_SET (DECL_RTL (expr));

  /* We expect the incoming memory to be of the form:
	(mem:MODE (plus (reg sfp) (const_int offset)))
     with perhaps the plus missing for offset = 0.  */
  addr = XEXP (mem, 0);
  offset = const0_rtx;
  if (GET_CODE (addr) == PLUS
      && GET_CODE (XEXP (addr, 1)) == CONST_INT)
    offset = XEXP (addr, 1);

  MEM_ATTRS (mem) = get_mem_attrs (alias, expr, offset,
				   MEM_SIZE (mem), MEM_ALIGN (mem),
				   GET_MODE (mem));
  MEM_NOTRAP_P (mem) = 1;
}

/* Return a newly created CODE_LABEL rtx with a unique label number.  */

rtx
gen_label_rtx (void)
{
  return gen_rtx_CODE_LABEL (VOIDmode, 0, NULL_RTX, NULL_RTX,
			     NULL, label_num++, NULL);
}

/* For procedure integration.  */

/* Install new pointers to the first and last insns in the chain.
   Also, set cur_insn_uid to one higher than the last in use.
   Used for an inline-procedure after copying the insn chain.  */

void
set_new_first_and_last_insn (rtx first, rtx last)
{
  rtx insn;

  first_insn = first;
  last_insn = last;
  cur_insn_uid = 0;

  for (insn = first; insn; insn = NEXT_INSN (insn))
    cur_insn_uid = MAX (cur_insn_uid, INSN_UID (insn));

  cur_insn_uid++;
}

/* Go through all the RTL insn bodies and copy any invalid shared
   structure.  This routine should only be called once.  */

static void
unshare_all_rtl_1 (rtx insn)
{
  /* Unshare just about everything else.  */
  unshare_all_rtl_in_chain (insn);

  /* Make sure the addresses of stack slots found outside the insn chain
     (such as, in DECL_RTL of a variable) are not shared
     with the insn chain.

     This special care is necessary when the stack slot MEM does not
     actually appear in the insn chain.  If it does appear, its address
     is unshared from all else at that point.  */
  stack_slot_list = copy_rtx_if_shared (stack_slot_list);
}

/* Go through all the RTL insn bodies and copy any invalid shared
   structure, again.  This is a fairly expensive thing to do so it
   should be done sparingly.  */

void
unshare_all_rtl_again (rtx insn)
{
  rtx p;
  tree decl;

  for (p = insn; p; p = NEXT_INSN (p))
    if (INSN_P (p))
      {
	reset_used_flags (PATTERN (p));
	reset_used_flags (REG_NOTES (p));
      }

  /* Make sure that virtual stack slots are not shared.  */
  set_used_decls (DECL_INITIAL (cfun->decl));

  /* Make sure that virtual parameters are not shared.  */
  for (decl = DECL_ARGUMENTS (cfun->decl); decl; decl = TREE_CHAIN (decl))
    set_used_flags (DECL_RTL (decl));

  reset_used_flags (stack_slot_list);

  unshare_all_rtl_1 (insn);
}

unsigned int
unshare_all_rtl (void)
{
  unshare_all_rtl_1 (get_insns ());
  return 0;
}

struct rtl_opt_pass pass_unshare_all_rtl =
{
 {
  RTL_PASS,
  "unshare",                            /* name */
  NULL,                                 /* gate */
  unshare_all_rtl,                      /* execute */
  NULL,                                 /* sub */
  NULL,                                 /* next */
  0,                                    /* static_pass_number */
  0,                                    /* tv_id */
  0,                                    /* properties_required */
  0,                                    /* properties_provided */
  0,                                    /* properties_destroyed */
  0,                                    /* todo_flags_start */
  TODO_dump_func | TODO_verify_rtl_sharing /* todo_flags_finish */
 }
};


/* Check that ORIG is not marked when it should not be and mark ORIG as in use,
   Recursively does the same for subexpressions.  */

static void
verify_rtx_sharing (rtx orig, rtx insn)
{
  rtx x = orig;
  int i;
  enum rtx_code code;
  const char *format_ptr;

  if (x == 0)
    return;

  code = GET_CODE (x);

  /* These types may be freely shared.  */

  switch (code)
    {
    case REG:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_FIXED:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case LABEL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
    case SCRATCH:
      return;
      /* SCRATCH must be shared because they represent distinct values.  */
    case CLOBBER:
      if (REG_P (XEXP (x, 0)) && REGNO (XEXP (x, 0)) < FIRST_PSEUDO_REGISTER)
	return;
      break;

    case CONST:
      if (shared_const_p (orig))
	return;
      break;

    case MEM:
      /* A MEM is allowed to be shared if its address is constant.  */
      if (CONSTANT_ADDRESS_P (XEXP (x, 0))
	  || reload_completed || reload_in_progress)
	return;

      break;

    default:
      break;
    }

  /* This rtx may not be shared.  If it has already been seen,
     replace it with a copy of itself.  */
#ifdef ENABLE_CHECKING
  if (RTX_FLAG (x, used))
    {
      error ("invalid rtl sharing found in the insn");
      debug_rtx (insn);
      error ("shared rtx");
      debug_rtx (x);
      internal_error ("internal consistency failure");
    }
#endif
  gcc_assert (!RTX_FLAG (x, used));
  
  RTX_FLAG (x, used) = 1;

  /* Now scan the subexpressions recursively.  */

  format_ptr = GET_RTX_FORMAT (code);

  for (i = 0; i < GET_RTX_LENGTH (code); i++)
    {
      switch (*format_ptr++)
	{
	case 'e':
	  verify_rtx_sharing (XEXP (x, i), insn);
	  break;

	case 'E':
	  if (XVEC (x, i) != NULL)
	    {
	      int j;
	      int len = XVECLEN (x, i);

	      for (j = 0; j < len; j++)
		{
		  /* We allow sharing of ASM_OPERANDS inside single
		     instruction.  */
		  if (j && GET_CODE (XVECEXP (x, i, j)) == SET
		      && (GET_CODE (SET_SRC (XVECEXP (x, i, j)))
			  == ASM_OPERANDS))
		    verify_rtx_sharing (SET_DEST (XVECEXP (x, i, j)), insn);
		  else
		    verify_rtx_sharing (XVECEXP (x, i, j), insn);
		}
	    }
	  break;
	}
    }
  return;
}

/* Go through all the RTL insn bodies and check that there is no unexpected
   sharing in between the subexpressions.  */

void
verify_rtl_sharing (void)
{
  rtx p;

  for (p = get_insns (); p; p = NEXT_INSN (p))
    if (INSN_P (p))
      {
	reset_used_flags (PATTERN (p));
	reset_used_flags (REG_NOTES (p));
	if (GET_CODE (PATTERN (p)) == SEQUENCE)
	  {
	    int i;
	    rtx q, sequence = PATTERN (p);

	    for (i = 0; i < XVECLEN (sequence, 0); i++)
	      {
		q = XVECEXP (sequence, 0, i);
		gcc_assert (INSN_P (q));
		reset_used_flags (PATTERN (q));
		reset_used_flags (REG_NOTES (q));
	      }
	  }
      }

  for (p = get_insns (); p; p = NEXT_INSN (p))
    if (INSN_P (p))
      {
	verify_rtx_sharing (PATTERN (p), p);
	verify_rtx_sharing (REG_NOTES (p), p);
      }
}

/* Go through all the RTL insn bodies and copy any invalid shared structure.
   Assumes the mark bits are cleared at entry.  */

void
unshare_all_rtl_in_chain (rtx insn)
{
  for (; insn; insn = NEXT_INSN (insn))
    if (INSN_P (insn))
      {
	PATTERN (insn) = copy_rtx_if_shared (PATTERN (insn));
	REG_NOTES (insn) = copy_rtx_if_shared (REG_NOTES (insn));
      }
}

/* Go through all virtual stack slots of a function and mark them as
   shared.  We never replace the DECL_RTLs themselves with a copy,
   but expressions mentioned into a DECL_RTL cannot be shared with
   expressions in the instruction stream.

   Note that reload may convert pseudo registers into memories in-place.
   Pseudo registers are always shared, but MEMs never are.  Thus if we
   reset the used flags on MEMs in the instruction stream, we must set
   them again on MEMs that appear in DECL_RTLs.  */

static void
set_used_decls (tree blk)
{
  tree t;

  /* Mark decls.  */
  for (t = BLOCK_VARS (blk); t; t = TREE_CHAIN (t))
    if (DECL_RTL_SET_P (t))
      set_used_flags (DECL_RTL (t));

  /* Now process sub-blocks.  */
  for (t = BLOCK_SUBBLOCKS (blk); t; t = BLOCK_CHAIN (t))
    set_used_decls (t);
}

/* Mark ORIG as in use, and return a copy of it if it was already in use.
   Recursively does the same for subexpressions.  Uses
   copy_rtx_if_shared_1 to reduce stack space.  */

rtx
copy_rtx_if_shared (rtx orig)
{
  copy_rtx_if_shared_1 (&orig);
  return orig;
}

/* Mark *ORIG1 as in use, and set it to a copy of it if it was already in
   use.  Recursively does the same for subexpressions.  */

static void
copy_rtx_if_shared_1 (rtx *orig1)
{
  rtx x;
  int i;
  enum rtx_code code;
  rtx *last_ptr;
  const char *format_ptr;
  int copied = 0;
  int length;

  /* Repeat is used to turn tail-recursion into iteration.  */
repeat:
  x = *orig1;

  if (x == 0)
    return;

  code = GET_CODE (x);

  /* These types may be freely shared.  */

  switch (code)
    {
    case REG:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_FIXED:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case LABEL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
    case SCRATCH:
      /* SCRATCH must be shared because they represent distinct values.  */
      return;
    case CLOBBER:
      if (REG_P (XEXP (x, 0)) && REGNO (XEXP (x, 0)) < FIRST_PSEUDO_REGISTER)
	return;
      break;

    case CONST:
      if (shared_const_p (x))
	return;
      break;

    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case NOTE:
    case BARRIER:
      /* The chain of insns is not being copied.  */
      return;

    default:
      break;
    }

  /* This rtx may not be shared.  If it has already been seen,
     replace it with a copy of itself.  */

  if (RTX_FLAG (x, used))
    {
      x = shallow_copy_rtx (x);
      copied = 1;
    }
  RTX_FLAG (x, used) = 1;

  /* Now scan the subexpressions recursively.
     We can store any replaced subexpressions directly into X
     since we know X is not shared!  Any vectors in X
     must be copied if X was copied.  */

  format_ptr = GET_RTX_FORMAT (code);
  length = GET_RTX_LENGTH (code);
  last_ptr = NULL;
  
  for (i = 0; i < length; i++)
    {
      switch (*format_ptr++)
	{
	case 'e':
          if (last_ptr)
            copy_rtx_if_shared_1 (last_ptr);
	  last_ptr = &XEXP (x, i);
	  break;

	case 'E':
	  if (XVEC (x, i) != NULL)
	    {
	      int j;
	      int len = XVECLEN (x, i);
              
              /* Copy the vector iff I copied the rtx and the length
		 is nonzero.  */
	      if (copied && len > 0)
		XVEC (x, i) = gen_rtvec_v (len, XVEC (x, i)->elem);
              
              /* Call recursively on all inside the vector.  */
	      for (j = 0; j < len; j++)
                {
		  if (last_ptr)
		    copy_rtx_if_shared_1 (last_ptr);
                  last_ptr = &XVECEXP (x, i, j);
                }
	    }
	  break;
	}
    }
  *orig1 = x;
  if (last_ptr)
    {
      orig1 = last_ptr;
      goto repeat;
    }
  return;
}

/* Clear all the USED bits in X to allow copy_rtx_if_shared to be used
   to look for shared sub-parts.  */

void
reset_used_flags (rtx x)
{
  int i, j;
  enum rtx_code code;
  const char *format_ptr;
  int length;

  /* Repeat is used to turn tail-recursion into iteration.  */
repeat:
  if (x == 0)
    return;

  code = GET_CODE (x);

  /* These types may be freely shared so we needn't do any resetting
     for them.  */

  switch (code)
    {
    case REG:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_FIXED:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
      return;

    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case NOTE:
    case LABEL_REF:
    case BARRIER:
      /* The chain of insns is not being copied.  */
      return;

    default:
      break;
    }

  RTX_FLAG (x, used) = 0;

  format_ptr = GET_RTX_FORMAT (code);
  length = GET_RTX_LENGTH (code);
  
  for (i = 0; i < length; i++)
    {
      switch (*format_ptr++)
	{
	case 'e':
          if (i == length-1)
            {
              x = XEXP (x, i);
	      goto repeat;
            }
	  reset_used_flags (XEXP (x, i));
	  break;

	case 'E':
	  for (j = 0; j < XVECLEN (x, i); j++)
	    reset_used_flags (XVECEXP (x, i, j));
	  break;
	}
    }
}

/* Set all the USED bits in X to allow copy_rtx_if_shared to be used
   to look for shared sub-parts.  */

void
set_used_flags (rtx x)
{
  int i, j;
  enum rtx_code code;
  const char *format_ptr;

  if (x == 0)
    return;

  code = GET_CODE (x);

  /* These types may be freely shared so we needn't do any resetting
     for them.  */

  switch (code)
    {
    case REG:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_FIXED:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
      return;

    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case NOTE:
    case LABEL_REF:
    case BARRIER:
      /* The chain of insns is not being copied.  */
      return;

    default:
      break;
    }

  RTX_FLAG (x, used) = 1;

  format_ptr = GET_RTX_FORMAT (code);
  for (i = 0; i < GET_RTX_LENGTH (code); i++)
    {
      switch (*format_ptr++)
	{
	case 'e':
	  set_used_flags (XEXP (x, i));
	  break;

	case 'E':
	  for (j = 0; j < XVECLEN (x, i); j++)
	    set_used_flags (XVECEXP (x, i, j));
	  break;
	}
    }
}

/* Copy X if necessary so that it won't be altered by changes in OTHER.
   Return X or the rtx for the pseudo reg the value of X was copied into.
   OTHER must be valid as a SET_DEST.  */

rtx
make_safe_from (rtx x, rtx other)
{
  while (1)
    switch (GET_CODE (other))
      {
      case SUBREG:
	other = SUBREG_REG (other);
	break;
      case STRICT_LOW_PART:
      case SIGN_EXTEND:
      case ZERO_EXTEND:
	other = XEXP (other, 0);
	break;
      default:
	goto done;
      }
 done:
  if ((MEM_P (other)
       && ! CONSTANT_P (x)
       && !REG_P (x)
       && GET_CODE (x) != SUBREG)
      || (REG_P (other)
	  && (REGNO (other) < FIRST_PSEUDO_REGISTER
	      || reg_mentioned_p (other, x))))
    {
      rtx temp = gen_reg_rtx (GET_MODE (x));
      emit_move_insn (temp, x);
      return temp;
    }
  return x;
}

/* Emission of insns (adding them to the doubly-linked list).  */

/* Return the first insn of the current sequence or current function.  */

rtx
get_insns (void)
{
  return first_insn;
}

/* Specify a new insn as the first in the chain.  */

void
set_first_insn (rtx insn)
{
  gcc_assert (!PREV_INSN (insn));
  first_insn = insn;
}

/* Return the last insn emitted in current sequence or current function.  */

rtx
get_last_insn (void)
{
  return last_insn;
}

/* Specify a new insn as the last in the chain.  */

void
set_last_insn (rtx insn)
{
  gcc_assert (!NEXT_INSN (insn));
  last_insn = insn;
}

/* Return the last insn emitted, even if it is in a sequence now pushed.  */

rtx
get_last_insn_anywhere (void)
{
  struct sequence_stack *stack;
  if (last_insn)
    return last_insn;
  for (stack = seq_stack; stack; stack = stack->next)
    if (stack->last != 0)
      return stack->last;
  return 0;
}

/* Return the first nonnote insn emitted in current sequence or current
   function.  This routine looks inside SEQUENCEs.  */

rtx
get_first_nonnote_insn (void)
{
  rtx insn = first_insn;

  if (insn)
    {
      if (NOTE_P (insn))
	for (insn = next_insn (insn);
	     insn && NOTE_P (insn);
	     insn = next_insn (insn))
	  continue;
      else
	{
	  if (NONJUMP_INSN_P (insn)
	      && GET_CODE (PATTERN (insn)) == SEQUENCE)
	    insn = XVECEXP (PATTERN (insn), 0, 0);
	}
    }

  return insn;
}

/* Return the last nonnote insn emitted in current sequence or current
   function.  This routine looks inside SEQUENCEs.  */

rtx
get_last_nonnote_insn (void)
{
  rtx insn = last_insn;

  if (insn)
    {
      if (NOTE_P (insn))
	for (insn = previous_insn (insn);
	     insn && NOTE_P (insn);
	     insn = previous_insn (insn))
	  continue;
      else
	{
	  if (NONJUMP_INSN_P (insn)
	      && GET_CODE (PATTERN (insn)) == SEQUENCE)
	    insn = XVECEXP (PATTERN (insn), 0,
			    XVECLEN (PATTERN (insn), 0) - 1);
	}
    }

  return insn;
}

/* Return a number larger than any instruction's uid in this function.  */

int
get_max_uid (void)
{
  return cur_insn_uid;
}

/* Return the next insn.  If it is a SEQUENCE, return the first insn
   of the sequence.  */

rtx
next_insn (rtx insn)
{
  if (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn && NONJUMP_INSN_P (insn)
	  && GET_CODE (PATTERN (insn)) == SEQUENCE)
	insn = XVECEXP (PATTERN (insn), 0, 0);
    }

  return insn;
}

/* Return the previous insn.  If it is a SEQUENCE, return the last insn
   of the sequence.  */

rtx
previous_insn (rtx insn)
{
  if (insn)
    {
      insn = PREV_INSN (insn);
      if (insn && NONJUMP_INSN_P (insn)
	  && GET_CODE (PATTERN (insn)) == SEQUENCE)
	insn = XVECEXP (PATTERN (insn), 0, XVECLEN (PATTERN (insn), 0) - 1);
    }

  return insn;
}

/* Return the next insn after INSN that is not a NOTE.  This routine does not
   look inside SEQUENCEs.  */

rtx
next_nonnote_insn (rtx insn)
{
  while (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn == 0 || !NOTE_P (insn))
	break;
    }

  return insn;
}

/* Return the previous insn before INSN that is not a NOTE.  This routine does
   not look inside SEQUENCEs.  */

rtx
prev_nonnote_insn (rtx insn)
{
  while (insn)
    {
      insn = PREV_INSN (insn);
      if (insn == 0 || !NOTE_P (insn))
	break;
    }

  return insn;
}

/* Return the next INSN, CALL_INSN or JUMP_INSN after INSN;
   or 0, if there is none.  This routine does not look inside
   SEQUENCEs.  */

rtx
next_real_insn (rtx insn)
{
  while (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn == 0 || INSN_P (insn))
	break;
    }

  return insn;
}

/* Return the last INSN, CALL_INSN or JUMP_INSN before INSN;
   or 0, if there is none.  This routine does not look inside
   SEQUENCEs.  */

rtx
prev_real_insn (rtx insn)
{
  while (insn)
    {
      insn = PREV_INSN (insn);
      if (insn == 0 || INSN_P (insn))
	break;
    }

  return insn;
}

/* Return the last CALL_INSN in the current list, or 0 if there is none.
   This routine does not look inside SEQUENCEs.  */

rtx
last_call_insn (void)
{
  rtx insn;

  for (insn = get_last_insn ();
       insn && !CALL_P (insn);
       insn = PREV_INSN (insn))
    ;

  return insn;
}

/* Find the next insn after INSN that really does something.  This routine
   does not look inside SEQUENCEs.  Until reload has completed, this is the
   same as next_real_insn.  */

int
active_insn_p (const_rtx insn)
{
  return (CALL_P (insn) || JUMP_P (insn)
	  || (NONJUMP_INSN_P (insn)
	      && (! reload_completed
		  || (GET_CODE (PATTERN (insn)) != USE
		      && GET_CODE (PATTERN (insn)) != CLOBBER))));
}

rtx
next_active_insn (rtx insn)
{
  while (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn == 0 || active_insn_p (insn))
	break;
    }

  return insn;
}

/* Find the last insn before INSN that really does something.  This routine
   does not look inside SEQUENCEs.  Until reload has completed, this is the
   same as prev_real_insn.  */

rtx
prev_active_insn (rtx insn)
{
  while (insn)
    {
      insn = PREV_INSN (insn);
      if (insn == 0 || active_insn_p (insn))
	break;
    }

  return insn;
}

/* Return the next CODE_LABEL after the insn INSN, or 0 if there is none.  */

rtx
next_label (rtx insn)
{
  while (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn == 0 || LABEL_P (insn))
	break;
    }

  return insn;
}

/* Return the last CODE_LABEL before the insn INSN, or 0 if there is none.  */

rtx
prev_label (rtx insn)
{
  while (insn)
    {
      insn = PREV_INSN (insn);
      if (insn == 0 || LABEL_P (insn))
	break;
    }

  return insn;
}

/* Return the last label to mark the same position as LABEL.  Return null
   if LABEL itself is null.  */

rtx
skip_consecutive_labels (rtx label)
{
  rtx insn;

  for (insn = label; insn != 0 && !INSN_P (insn); insn = NEXT_INSN (insn))
    if (LABEL_P (insn))
      label = insn;

  return label;
}

#ifdef HAVE_cc0
/* INSN uses CC0 and is being moved into a delay slot.  Set up REG_CC_SETTER
   and REG_CC_USER notes so we can find it.  */

void
link_cc0_insns (rtx insn)
{
  rtx user = next_nonnote_insn (insn);

  if (NONJUMP_INSN_P (user) && GET_CODE (PATTERN (user)) == SEQUENCE)
    user = XVECEXP (PATTERN (user), 0, 0);

  add_reg_note (user, REG_CC_SETTER, insn);
  add_reg_note (insn, REG_CC_USER, user);
}

/* Return the next insn that uses CC0 after INSN, which is assumed to
   set it.  This is the inverse of prev_cc0_setter (i.e., prev_cc0_setter
   applied to the result of this function should yield INSN).

   Normally, this is simply the next insn.  However, if a REG_CC_USER note
   is present, it contains the insn that uses CC0.

   Return 0 if we can't find the insn.  */

rtx
next_cc0_user (rtx insn)
{
  rtx note = find_reg_note (insn, REG_CC_USER, NULL_RTX);

  if (note)
    return XEXP (note, 0);

  insn = next_nonnote_insn (insn);
  if (insn && NONJUMP_INSN_P (insn) && GET_CODE (PATTERN (insn)) == SEQUENCE)
    insn = XVECEXP (PATTERN (insn), 0, 0);

  if (insn && INSN_P (insn) && reg_mentioned_p (cc0_rtx, PATTERN (insn)))
    return insn;

  return 0;
}

/* Find the insn that set CC0 for INSN.  Unless INSN has a REG_CC_SETTER
   note, it is the previous insn.  */

rtx
prev_cc0_setter (rtx insn)
{
  rtx note = find_reg_note (insn, REG_CC_SETTER, NULL_RTX);

  if (note)
    return XEXP (note, 0);

  insn = prev_nonnote_insn (insn);
  gcc_assert (sets_cc0_p (PATTERN (insn)));

  return insn;
}
#endif

#ifdef AUTO_INC_DEC
/* Find a RTX_AUTOINC class rtx which matches DATA.  */

static int
find_auto_inc (rtx *xp, void *data)
{
  rtx x = *xp;
  rtx reg = (rtx) data;

  if (GET_RTX_CLASS (GET_CODE (x)) != RTX_AUTOINC)
    return 0;

  switch (GET_CODE (x))
    {
      case PRE_DEC:
      case PRE_INC:
      case POST_DEC:
      case POST_INC:
      case PRE_MODIFY:
      case POST_MODIFY:
	if (rtx_equal_p (reg, XEXP (x, 0)))
	  return 1;
	break;

      default:
	gcc_unreachable ();
    }
  return -1;
}
#endif

/* Increment the label uses for all labels present in rtx.  */

static void
mark_label_nuses (rtx x)
{
  enum rtx_code code;
  int i, j;
  const char *fmt;

  code = GET_CODE (x);
  if (code == LABEL_REF && LABEL_P (XEXP (x, 0)))
    LABEL_NUSES (XEXP (x, 0))++;

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'e')
	mark_label_nuses (XEXP (x, i));
      else if (fmt[i] == 'E')
	for (j = XVECLEN (x, i) - 1; j >= 0; j--)
	  mark_label_nuses (XVECEXP (x, i, j));
    }
}


/* Try splitting insns that can be split for better scheduling.
   PAT is the pattern which might split.
   TRIAL is the insn providing PAT.
   LAST is nonzero if we should return the last insn of the sequence produced.

   If this routine succeeds in splitting, it returns the first or last
   replacement insn depending on the value of LAST.  Otherwise, it
   returns TRIAL.  If the insn to be returned can be split, it will be.  */

rtx
try_split (rtx pat, rtx trial, int last)
{
  rtx before = PREV_INSN (trial);
  rtx after = NEXT_INSN (trial);
  int has_barrier = 0;
  rtx note, seq, tem;
  int probability;
  rtx insn_last, insn;
  int njumps = 0;

  if (any_condjump_p (trial)
      && (note = find_reg_note (trial, REG_BR_PROB, 0)))
    split_branch_probability = INTVAL (XEXP (note, 0));
  probability = split_branch_probability;

  seq = split_insns (pat, trial);

  split_branch_probability = -1;

  /* If we are splitting a JUMP_INSN, it might be followed by a BARRIER.
     We may need to handle this specially.  */
  if (after && BARRIER_P (after))
    {
      has_barrier = 1;
      after = NEXT_INSN (after);
    }

  if (!seq)
    return trial;

  /* Avoid infinite loop if any insn of the result matches
     the original pattern.  */
  insn_last = seq;
  while (1)
    {
      if (INSN_P (insn_last)
	  && rtx_equal_p (PATTERN (insn_last), pat))
	return trial;
      if (!NEXT_INSN (insn_last))
	break;
      insn_last = NEXT_INSN (insn_last);
    }

  /* We will be adding the new sequence to the function.  The splitters
     may have introduced invalid RTL sharing, so unshare the sequence now.  */
  unshare_all_rtl_in_chain (seq);

  /* Mark labels.  */
  for (insn = insn_last; insn ; insn = PREV_INSN (insn))
    {
      if (JUMP_P (insn))
	{
	  mark_jump_label (PATTERN (insn), insn, 0);
	  njumps++;
	  if (probability != -1
	      && any_condjump_p (insn)
	      && !find_reg_note (insn, REG_BR_PROB, 0))
	    {
	      /* We can preserve the REG_BR_PROB notes only if exactly
		 one jump is created, otherwise the machine description
		 is responsible for this step using
		 split_branch_probability variable.  */
	      gcc_assert (njumps == 1);
	      add_reg_note (insn, REG_BR_PROB, GEN_INT (probability));
	    }
	}
    }

  /* If we are splitting a CALL_INSN, look for the CALL_INSN
     in SEQ and copy our CALL_INSN_FUNCTION_USAGE to it.  */
  if (CALL_P (trial))
    {
      for (insn = insn_last; insn ; insn = PREV_INSN (insn))
	if (CALL_P (insn))
	  {
	    rtx *p = &CALL_INSN_FUNCTION_USAGE (insn);
	    while (*p)
	      p = &XEXP (*p, 1);
	    *p = CALL_INSN_FUNCTION_USAGE (trial);
	    SIBLING_CALL_P (insn) = SIBLING_CALL_P (trial);
	  }
    }

  /* Copy notes, particularly those related to the CFG.  */
  for (note = REG_NOTES (trial); note; note = XEXP (note, 1))
    {
      switch (REG_NOTE_KIND (note))
	{
	case REG_EH_REGION:
	  for (insn = insn_last; insn != NULL_RTX; insn = PREV_INSN (insn))
	    {
	      if (CALL_P (insn)
		  || (flag_non_call_exceptions && INSN_P (insn)
		      && may_trap_p (PATTERN (insn))))
		add_reg_note (insn, REG_EH_REGION, XEXP (note, 0));
	    }
	  break;

	case REG_NORETURN:
	case REG_SETJMP:
	  for (insn = insn_last; insn != NULL_RTX; insn = PREV_INSN (insn))
	    {
	      if (CALL_P (insn))
		add_reg_note (insn, REG_NOTE_KIND (note), XEXP (note, 0));
	    }
	  break;

	case REG_NON_LOCAL_GOTO:
	  for (insn = insn_last; insn != NULL_RTX; insn = PREV_INSN (insn))
	    {
	      if (JUMP_P (insn))
		add_reg_note (insn, REG_NOTE_KIND (note), XEXP (note, 0));
	    }
	  break;

#ifdef AUTO_INC_DEC
	case REG_INC:
	  for (insn = insn_last; insn != NULL_RTX; insn = PREV_INSN (insn))
	    {
	      rtx reg = XEXP (note, 0);
	      if (!FIND_REG_INC_NOTE (insn, reg)
		  && for_each_rtx (&PATTERN (insn), find_auto_inc, reg) > 0)
		add_reg_note (insn, REG_INC, reg);
	    }
	  break;
#endif

	default:
	  break;
	}
    }

  /* If there are LABELS inside the split insns increment the
     usage count so we don't delete the label.  */
  if (INSN_P (trial))
    {
      insn = insn_last;
      while (insn != NULL_RTX)
	{
	  /* JUMP_P insns have already been "marked" above.  */
	  if (NONJUMP_INSN_P (insn))
	    mark_label_nuses (PATTERN (insn));

	  insn = PREV_INSN (insn);
	}
    }

  tem = emit_insn_after_setloc (seq, trial, INSN_LOCATOR (trial));

  delete_insn (trial);
  if (has_barrier)
    emit_barrier_after (tem);

  /* Recursively call try_split for each new insn created; by the
     time control returns here that insn will be fully split, so
     set LAST and continue from the insn after the one returned.
     We can't use next_active_insn here since AFTER may be a note.
     Ignore deleted insns, which can be occur if not optimizing.  */
  for (tem = NEXT_INSN (before); tem != after; tem = NEXT_INSN (tem))
    if (! INSN_DELETED_P (tem) && INSN_P (tem))
      tem = try_split (PATTERN (tem), tem, 1);

  /* Return either the first or the last insn, depending on which was
     requested.  */
  return last
    ? (after ? PREV_INSN (after) : last_insn)
    : NEXT_INSN (before);
}

/* Make and return an INSN rtx, initializing all its slots.
   Store PATTERN in the pattern slots.  */

rtx
make_insn_raw (rtx pattern)
{
  rtx insn;

  insn = rtx_alloc (INSN);

  INSN_UID (insn) = cur_insn_uid++;
  PATTERN (insn) = pattern;
  INSN_CODE (insn) = -1;
  REG_NOTES (insn) = NULL;
  INSN_LOCATOR (insn) = curr_insn_locator ();
  BLOCK_FOR_INSN (insn) = NULL;

#ifdef ENABLE_RTL_CHECKING
  if (insn
      && INSN_P (insn)
      && (returnjump_p (insn)
	  || (GET_CODE (insn) == SET
	      && SET_DEST (insn) == pc_rtx)))
    {
      warning (0, "ICE: emit_insn used where emit_jump_insn needed:\n");
      debug_rtx (insn);
    }
#endif

  return insn;
}

/* Like `make_insn_raw' but make a JUMP_INSN instead of an insn.  */

rtx
make_jump_insn_raw (rtx pattern)
{
  rtx insn;

  insn = rtx_alloc (JUMP_INSN);
  INSN_UID (insn) = cur_insn_uid++;

  PATTERN (insn) = pattern;
  INSN_CODE (insn) = -1;
  REG_NOTES (insn) = NULL;
  JUMP_LABEL (insn) = NULL;
  INSN_LOCATOR (insn) = curr_insn_locator ();
  BLOCK_FOR_INSN (insn) = NULL;

  return insn;
}

/* Like `make_insn_raw' but make a CALL_INSN instead of an insn.  */

static rtx
make_call_insn_raw (rtx pattern)
{
  rtx insn;

  insn = rtx_alloc (CALL_INSN);
  INSN_UID (insn) = cur_insn_uid++;

  PATTERN (insn) = pattern;
  INSN_CODE (insn) = -1;
  REG_NOTES (insn) = NULL;
  CALL_INSN_FUNCTION_USAGE (insn) = NULL;
  INSN_LOCATOR (insn) = curr_insn_locator ();
  BLOCK_FOR_INSN (insn) = NULL;

  return insn;
}

/* Add INSN to the end of the doubly-linked list.
   INSN may be an INSN, JUMP_INSN, CALL_INSN, CODE_LABEL, BARRIER or NOTE.  */

void
add_insn (rtx insn)
{
  PREV_INSN (insn) = last_insn;
  NEXT_INSN (insn) = 0;

  if (NULL != last_insn)
    NEXT_INSN (last_insn) = insn;

  if (NULL == first_insn)
    first_insn = insn;

  last_insn = insn;
}

/* Add INSN into the doubly-linked list after insn AFTER.  This and
   the next should be the only functions called to insert an insn once
   delay slots have been filled since only they know how to update a
   SEQUENCE.  */

void
add_insn_after (rtx insn, rtx after, basic_block bb)
{
  rtx next = NEXT_INSN (after);

  gcc_assert (!optimize || !INSN_DELETED_P (after));

  NEXT_INSN (insn) = next;
  PREV_INSN (insn) = after;

  if (next)
    {
      PREV_INSN (next) = insn;
      if (NONJUMP_INSN_P (next) && GET_CODE (PATTERN (next)) == SEQUENCE)
	PREV_INSN (XVECEXP (PATTERN (next), 0, 0)) = insn;
    }
  else if (last_insn == after)
    last_insn = insn;
  else
    {
      struct sequence_stack *stack = seq_stack;
      /* Scan all pending sequences too.  */
      for (; stack; stack = stack->next)
	if (after == stack->last)
	  {
	    stack->last = insn;
	    break;
	  }

      gcc_assert (stack);
    }

  if (!BARRIER_P (after)
      && !BARRIER_P (insn)
      && (bb = BLOCK_FOR_INSN (after)))
    {
      set_block_for_insn (insn, bb);
      if (INSN_P (insn))
	df_insn_rescan (insn);
      /* Should not happen as first in the BB is always
	 either NOTE or LABEL.  */
      if (BB_END (bb) == after
	  /* Avoid clobbering of structure when creating new BB.  */
	  && !BARRIER_P (insn)
	  && !NOTE_INSN_BASIC_BLOCK_P (insn))
	BB_END (bb) = insn;
    }

  NEXT_INSN (after) = insn;
  if (NONJUMP_INSN_P (after) && GET_CODE (PATTERN (after)) == SEQUENCE)
    {
      rtx sequence = PATTERN (after);
      NEXT_INSN (XVECEXP (sequence, 0, XVECLEN (sequence, 0) - 1)) = insn;
    }
}

/* Add INSN into the doubly-linked list before insn BEFORE.  This and
   the previous should be the only functions called to insert an insn
   once delay slots have been filled since only they know how to
   update a SEQUENCE.  If BB is NULL, an attempt is made to infer the
   bb from before.  */

void
add_insn_before (rtx insn, rtx before, basic_block bb)
{
  rtx prev = PREV_INSN (before);

  gcc_assert (!optimize || !INSN_DELETED_P (before));

  PREV_INSN (insn) = prev;
  NEXT_INSN (insn) = before;

  if (prev)
    {
      NEXT_INSN (prev) = insn;
      if (NONJUMP_INSN_P (prev) && GET_CODE (PATTERN (prev)) == SEQUENCE)
	{
	  rtx sequence = PATTERN (prev);
	  NEXT_INSN (XVECEXP (sequence, 0, XVECLEN (sequence, 0) - 1)) = insn;
	}
    }
  else if (first_insn == before)
    first_insn = insn;
  else
    {
      struct sequence_stack *stack = seq_stack;
      /* Scan all pending sequences too.  */
      for (; stack; stack = stack->next)
	if (before == stack->first)
	  {
	    stack->first = insn;
	    break;
	  }

      gcc_assert (stack);
    }

  if (!bb 
      && !BARRIER_P (before)
      && !BARRIER_P (insn))
    bb = BLOCK_FOR_INSN (before);

  if (bb)
    {
      set_block_for_insn (insn, bb);
      if (INSN_P (insn))
	df_insn_rescan (insn);
      /* Should not happen as first in the BB is always either NOTE or
	 LABEL.  */
      gcc_assert (BB_HEAD (bb) != insn
		  /* Avoid clobbering of structure when creating new BB.  */
		  || BARRIER_P (insn)
		  || NOTE_INSN_BASIC_BLOCK_P (insn));
    }

  PREV_INSN (before) = insn;
  if (NONJUMP_INSN_P (before) && GET_CODE (PATTERN (before)) == SEQUENCE)
    PREV_INSN (XVECEXP (PATTERN (before), 0, 0)) = insn;
}


/* Replace insn with an deleted instruction note.  */

void
set_insn_deleted (rtx insn)
{
  df_insn_delete (BLOCK_FOR_INSN (insn), INSN_UID (insn));
  PUT_CODE (insn, NOTE);
  NOTE_KIND (insn) = NOTE_INSN_DELETED;
}


/* Remove an insn from its doubly-linked list.  This function knows how
   to handle sequences.  */
void
remove_insn (rtx insn)
{
  rtx next = NEXT_INSN (insn);
  rtx prev = PREV_INSN (insn);
  basic_block bb;

  /* Later in the code, the block will be marked dirty.  */
  df_insn_delete (NULL, INSN_UID (insn));

  if (prev)
    {
      NEXT_INSN (prev) = next;
      if (NONJUMP_INSN_P (prev) && GET_CODE (PATTERN (prev)) == SEQUENCE)
	{
	  rtx sequence = PATTERN (prev);
	  NEXT_INSN (XVECEXP (sequence, 0, XVECLEN (sequence, 0) - 1)) = next;
	}
    }
  else if (first_insn == insn)
    first_insn = next;
  else
    {
      struct sequence_stack *stack = seq_stack;
      /* Scan all pending sequences too.  */
      for (; stack; stack = stack->next)
	if (insn == stack->first)
	  {
	    stack->first = next;
	    break;
	  }

      gcc_assert (stack);
    }

  if (next)
    {
      PREV_INSN (next) = prev;
      if (NONJUMP_INSN_P (next) && GET_CODE (PATTERN (next)) == SEQUENCE)
	PREV_INSN (XVECEXP (PATTERN (next), 0, 0)) = prev;
    }
  else if (last_insn == insn)
    last_insn = prev;
  else
    {
      struct sequence_stack *stack = seq_stack;
      /* Scan all pending sequences too.  */
      for (; stack; stack = stack->next)
	if (insn == stack->last)
	  {
	    stack->last = prev;
	    break;
	  }

      gcc_assert (stack);
    }
  if (!BARRIER_P (insn)
      && (bb = BLOCK_FOR_INSN (insn)))
    {
      if (INSN_P (insn))
	df_set_bb_dirty (bb);
      if (BB_HEAD (bb) == insn)
	{
	  /* Never ever delete the basic block note without deleting whole
	     basic block.  */
	  gcc_assert (!NOTE_P (insn));
	  BB_HEAD (bb) = next;
	}
      if (BB_END (bb) == insn)
	BB_END (bb) = prev;
    }
}

/* Append CALL_FUSAGE to the CALL_INSN_FUNCTION_USAGE for CALL_INSN.  */

void
add_function_usage_to (rtx call_insn, rtx call_fusage)
{
  gcc_assert (call_insn && CALL_P (call_insn));

  /* Put the register usage information on the CALL.  If there is already
     some usage information, put ours at the end.  */
  if (CALL_INSN_FUNCTION_USAGE (call_insn))
    {
      rtx link;

      for (link = CALL_INSN_FUNCTION_USAGE (call_insn); XEXP (link, 1) != 0;
	   link = XEXP (link, 1))
	;

      XEXP (link, 1) = call_fusage;
    }
  else
    CALL_INSN_FUNCTION_USAGE (call_insn) = call_fusage;
}

/* Delete all insns made since FROM.
   FROM becomes the new last instruction.  */

void
delete_insns_since (rtx from)
{
  if (from == 0)
    first_insn = 0;
  else
    NEXT_INSN (from) = 0;
  last_insn = from;
}

/* This function is deprecated, please use sequences instead.

   Move a consecutive bunch of insns to a different place in the chain.
   The insns to be moved are those between FROM and TO.
   They are moved to a new position after the insn AFTER.
   AFTER must not be FROM or TO or any insn in between.

   This function does not know about SEQUENCEs and hence should not be
   called after delay-slot filling has been done.  */

void
reorder_insns_nobb (rtx from, rtx to, rtx after)
{
  /* Splice this bunch out of where it is now.  */
  if (PREV_INSN (from))
    NEXT_INSN (PREV_INSN (from)) = NEXT_INSN (to);
  if (NEXT_INSN (to))
    PREV_INSN (NEXT_INSN (to)) = PREV_INSN (from);
  if (last_insn == to)
    last_insn = PREV_INSN (from);
  if (first_insn == from)
    first_insn = NEXT_INSN (to);

  /* Make the new neighbors point to it and it to them.  */
  if (NEXT_INSN (after))
    PREV_INSN (NEXT_INSN (after)) = to;

  NEXT_INSN (to) = NEXT_INSN (after);
  PREV_INSN (from) = after;
  NEXT_INSN (after) = from;
  if (after == last_insn)
    last_insn = to;
}

/* Same as function above, but take care to update BB boundaries.  */
void
reorder_insns (rtx from, rtx to, rtx after)
{
  rtx prev = PREV_INSN (from);
  basic_block bb, bb2;

  reorder_insns_nobb (from, to, after);

  if (!BARRIER_P (after)
      && (bb = BLOCK_FOR_INSN (after)))
    {
      rtx x;
      df_set_bb_dirty (bb);

      if (!BARRIER_P (from)
	  && (bb2 = BLOCK_FOR_INSN (from)))
	{
	  if (BB_END (bb2) == to)
	    BB_END (bb2) = prev;
	  df_set_bb_dirty (bb2);
	}

      if (BB_END (bb) == after)
	BB_END (bb) = to;

      for (x = from; x != NEXT_INSN (to); x = NEXT_INSN (x))
	if (!BARRIER_P (x))
	  df_insn_change_bb (x, bb);
    }
}


/* Emit insn(s) of given code and pattern
   at a specified place within the doubly-linked list.

   All of the emit_foo global entry points accept an object
   X which is either an insn list or a PATTERN of a single
   instruction.

   There are thus a few canonical ways to generate code and
   emit it at a specific place in the instruction stream.  For
   example, consider the instruction named SPOT and the fact that
   we would like to emit some instructions before SPOT.  We might
   do it like this:

	start_sequence ();
	... emit the new instructions ...
	insns_head = get_insns ();
	end_sequence ();

	emit_insn_before (insns_head, SPOT);

   It used to be common to generate SEQUENCE rtl instead, but that
   is a relic of the past which no longer occurs.  The reason is that
   SEQUENCE rtl results in much fragmented RTL memory since the SEQUENCE
   generated would almost certainly die right after it was created.  */

/* Make X be output before the instruction BEFORE.  */

rtx
emit_insn_before_noloc (rtx x, rtx before, basic_block bb)
{
  rtx last = before;
  rtx insn;

  gcc_assert (before);

  if (x == NULL_RTX)
    return last;

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      insn = x;
      while (insn)
	{
	  rtx next = NEXT_INSN (insn);
	  add_insn_before (insn, before, bb);
	  last = insn;
	  insn = next;
	}
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      last = make_insn_raw (x);
      add_insn_before (last, before, bb);
      break;
    }

  return last;
}

/* Make an instruction with body X and code JUMP_INSN
   and output it before the instruction BEFORE.  */

rtx
emit_jump_insn_before_noloc (rtx x, rtx before)
{
  rtx insn, last = NULL_RTX;

  gcc_assert (before);

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      insn = x;
      while (insn)
	{
	  rtx next = NEXT_INSN (insn);
	  add_insn_before (insn, before, NULL);
	  last = insn;
	  insn = next;
	}
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      last = make_jump_insn_raw (x);
      add_insn_before (last, before, NULL);
      break;
    }

  return last;
}

/* Make an instruction with body X and code CALL_INSN
   and output it before the instruction BEFORE.  */

rtx
emit_call_insn_before_noloc (rtx x, rtx before)
{
  rtx last = NULL_RTX, insn;

  gcc_assert (before);

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      insn = x;
      while (insn)
	{
	  rtx next = NEXT_INSN (insn);
	  add_insn_before (insn, before, NULL);
	  last = insn;
	  insn = next;
	}
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      last = make_call_insn_raw (x);
      add_insn_before (last, before, NULL);
      break;
    }

  return last;
}

/* Make an insn of code BARRIER
   and output it before the insn BEFORE.  */

rtx
emit_barrier_before (rtx before)
{
  rtx insn = rtx_alloc (BARRIER);

  INSN_UID (insn) = cur_insn_uid++;

  add_insn_before (insn, before, NULL);
  return insn;
}

/* Emit the label LABEL before the insn BEFORE.  */

rtx
emit_label_before (rtx label, rtx before)
{
  /* This can be called twice for the same label as a result of the
     confusion that follows a syntax error!  So make it harmless.  */
  if (INSN_UID (label) == 0)
    {
      INSN_UID (label) = cur_insn_uid++;
      add_insn_before (label, before, NULL);
    }

  return label;
}

/* Emit a note of subtype SUBTYPE before the insn BEFORE.  */

rtx
emit_note_before (enum insn_note subtype, rtx before)
{
  rtx note = rtx_alloc (NOTE);
  INSN_UID (note) = cur_insn_uid++;
  NOTE_KIND (note) = subtype;
  BLOCK_FOR_INSN (note) = NULL;
  memset (&NOTE_DATA (note), 0, sizeof (NOTE_DATA (note)));

  add_insn_before (note, before, NULL);
  return note;
}

/* Helper for emit_insn_after, handles lists of instructions
   efficiently.  */

static rtx
emit_insn_after_1 (rtx first, rtx after, basic_block bb)
{
  rtx last;
  rtx after_after;
  if (!bb && !BARRIER_P (after))
    bb = BLOCK_FOR_INSN (after);

  if (bb)
    {
      df_set_bb_dirty (bb);
      for (last = first; NEXT_INSN (last); last = NEXT_INSN (last))
	if (!BARRIER_P (last))
	  {
	    set_block_for_insn (last, bb);
	    df_insn_rescan (last);
	  }
      if (!BARRIER_P (last))
	{
	  set_block_for_insn (last, bb);
	  df_insn_rescan (last);
	}
      if (BB_END (bb) == after)
	BB_END (bb) = last;
    }
  else
    for (last = first; NEXT_INSN (last); last = NEXT_INSN (last))
      continue;

  after_after = NEXT_INSN (after);

  NEXT_INSN (after) = first;
  PREV_INSN (first) = after;
  NEXT_INSN (last) = after_after;
  if (after_after)
    PREV_INSN (after_after) = last;

  if (after == last_insn)
    last_insn = last;

  return last;
}

/* Make X be output after the insn AFTER and set the BB of insn.  If
   BB is NULL, an attempt is made to infer the BB from AFTER.  */

rtx
emit_insn_after_noloc (rtx x, rtx after, basic_block bb)
{
  rtx last = after;

  gcc_assert (after);

  if (x == NULL_RTX)
    return last;

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      last = emit_insn_after_1 (x, after, bb);
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      last = make_insn_raw (x);
      add_insn_after (last, after, bb);
      break;
    }

  return last;
}


/* Make an insn of code JUMP_INSN with body X
   and output it after the insn AFTER.  */

rtx
emit_jump_insn_after_noloc (rtx x, rtx after)
{
  rtx last;

  gcc_assert (after);

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      last = emit_insn_after_1 (x, after, NULL);
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      last = make_jump_insn_raw (x);
      add_insn_after (last, after, NULL);
      break;
    }

  return last;
}

/* Make an instruction with body X and code CALL_INSN
   and output it after the instruction AFTER.  */

rtx
emit_call_insn_after_noloc (rtx x, rtx after)
{
  rtx last;

  gcc_assert (after);

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      last = emit_insn_after_1 (x, after, NULL);
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      last = make_call_insn_raw (x);
      add_insn_after (last, after, NULL);
      break;
    }

  return last;
}

/* Make an insn of code BARRIER
   and output it after the insn AFTER.  */

rtx
emit_barrier_after (rtx after)
{
  rtx insn = rtx_alloc (BARRIER);

  INSN_UID (insn) = cur_insn_uid++;

  add_insn_after (insn, after, NULL);
  return insn;
}

/* Emit the label LABEL after the insn AFTER.  */

rtx
emit_label_after (rtx label, rtx after)
{
  /* This can be called twice for the same label
     as a result of the confusion that follows a syntax error!
     So make it harmless.  */
  if (INSN_UID (label) == 0)
    {
      INSN_UID (label) = cur_insn_uid++;
      add_insn_after (label, after, NULL);
    }

  return label;
}

/* Emit a note of subtype SUBTYPE after the insn AFTER.  */

rtx
emit_note_after (enum insn_note subtype, rtx after)
{
  rtx note = rtx_alloc (NOTE);
  INSN_UID (note) = cur_insn_uid++;
  NOTE_KIND (note) = subtype;
  BLOCK_FOR_INSN (note) = NULL;
  memset (&NOTE_DATA (note), 0, sizeof (NOTE_DATA (note)));
  add_insn_after (note, after, NULL);
  return note;
}

/* Like emit_insn_after_noloc, but set INSN_LOCATOR according to SCOPE.  */
rtx
emit_insn_after_setloc (rtx pattern, rtx after, int loc)
{
  rtx last = emit_insn_after_noloc (pattern, after, NULL);

  if (pattern == NULL_RTX || !loc)
    return last;

  after = NEXT_INSN (after);
  while (1)
    {
      if (active_insn_p (after) && !INSN_LOCATOR (after))
	INSN_LOCATOR (after) = loc;
      if (after == last)
	break;
      after = NEXT_INSN (after);
    }
  return last;
}

/* Like emit_insn_after_noloc, but set INSN_LOCATOR according to AFTER.  */
rtx
emit_insn_after (rtx pattern, rtx after)
{
  if (INSN_P (after))
    return emit_insn_after_setloc (pattern, after, INSN_LOCATOR (after));
  else
    return emit_insn_after_noloc (pattern, after, NULL);
}

/* Like emit_jump_insn_after_noloc, but set INSN_LOCATOR according to SCOPE.  */
rtx
emit_jump_insn_after_setloc (rtx pattern, rtx after, int loc)
{
  rtx last = emit_jump_insn_after_noloc (pattern, after);

  if (pattern == NULL_RTX || !loc)
    return last;

  after = NEXT_INSN (after);
  while (1)
    {
      if (active_insn_p (after) && !INSN_LOCATOR (after))
	INSN_LOCATOR (after) = loc;
      if (after == last)
	break;
      after = NEXT_INSN (after);
    }
  return last;
}

/* Like emit_jump_insn_after_noloc, but set INSN_LOCATOR according to AFTER.  */
rtx
emit_jump_insn_after (rtx pattern, rtx after)
{
  if (INSN_P (after))
    return emit_jump_insn_after_setloc (pattern, after, INSN_LOCATOR (after));
  else
    return emit_jump_insn_after_noloc (pattern, after);
}

/* Like emit_call_insn_after_noloc, but set INSN_LOCATOR according to SCOPE.  */
rtx
emit_call_insn_after_setloc (rtx pattern, rtx after, int loc)
{
  rtx last = emit_call_insn_after_noloc (pattern, after);

  if (pattern == NULL_RTX || !loc)
    return last;

  after = NEXT_INSN (after);
  while (1)
    {
      if (active_insn_p (after) && !INSN_LOCATOR (after))
	INSN_LOCATOR (after) = loc;
      if (after == last)
	break;
      after = NEXT_INSN (after);
    }
  return last;
}

/* Like emit_call_insn_after_noloc, but set INSN_LOCATOR according to AFTER.  */
rtx
emit_call_insn_after (rtx pattern, rtx after)
{
  if (INSN_P (after))
    return emit_call_insn_after_setloc (pattern, after, INSN_LOCATOR (after));
  else
    return emit_call_insn_after_noloc (pattern, after);
}

/* Like emit_insn_before_noloc, but set INSN_LOCATOR according to SCOPE.  */
rtx
emit_insn_before_setloc (rtx pattern, rtx before, int loc)
{
  rtx first = PREV_INSN (before);
  rtx last = emit_insn_before_noloc (pattern, before, NULL);

  if (pattern == NULL_RTX || !loc)
    return last;

  if (!first)
    first = get_insns ();
  else
    first = NEXT_INSN (first);
  while (1)
    {
      if (active_insn_p (first) && !INSN_LOCATOR (first))
	INSN_LOCATOR (first) = loc;
      if (first == last)
	break;
      first = NEXT_INSN (first);
    }
  return last;
}

/* Like emit_insn_before_noloc, but set INSN_LOCATOR according to BEFORE.  */
rtx
emit_insn_before (rtx pattern, rtx before)
{
  if (INSN_P (before))
    return emit_insn_before_setloc (pattern, before, INSN_LOCATOR (before));
  else
    return emit_insn_before_noloc (pattern, before, NULL);
}

/* like emit_insn_before_noloc, but set insn_locator according to scope.  */
rtx
emit_jump_insn_before_setloc (rtx pattern, rtx before, int loc)
{
  rtx first = PREV_INSN (before);
  rtx last = emit_jump_insn_before_noloc (pattern, before);

  if (pattern == NULL_RTX)
    return last;

  first = NEXT_INSN (first);
  while (1)
    {
      if (active_insn_p (first) && !INSN_LOCATOR (first))
	INSN_LOCATOR (first) = loc;
      if (first == last)
	break;
      first = NEXT_INSN (first);
    }
  return last;
}

/* Like emit_jump_insn_before_noloc, but set INSN_LOCATOR according to BEFORE.  */
rtx
emit_jump_insn_before (rtx pattern, rtx before)
{
  if (INSN_P (before))
    return emit_jump_insn_before_setloc (pattern, before, INSN_LOCATOR (before));
  else
    return emit_jump_insn_before_noloc (pattern, before);
}

/* like emit_insn_before_noloc, but set insn_locator according to scope.  */
rtx
emit_call_insn_before_setloc (rtx pattern, rtx before, int loc)
{
  rtx first = PREV_INSN (before);
  rtx last = emit_call_insn_before_noloc (pattern, before);

  if (pattern == NULL_RTX)
    return last;

  first = NEXT_INSN (first);
  while (1)
    {
      if (active_insn_p (first) && !INSN_LOCATOR (first))
	INSN_LOCATOR (first) = loc;
      if (first == last)
	break;
      first = NEXT_INSN (first);
    }
  return last;
}

/* like emit_call_insn_before_noloc,
   but set insn_locator according to before.  */
rtx
emit_call_insn_before (rtx pattern, rtx before)
{
  if (INSN_P (before))
    return emit_call_insn_before_setloc (pattern, before, INSN_LOCATOR (before));
  else
    return emit_call_insn_before_noloc (pattern, before);
}

/* Take X and emit it at the end of the doubly-linked
   INSN list.

   Returns the last insn emitted.  */

rtx
emit_insn (rtx x)
{
  rtx last = last_insn;
  rtx insn;

  if (x == NULL_RTX)
    return last;

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      insn = x;
      while (insn)
	{
	  rtx next = NEXT_INSN (insn);
	  add_insn (insn);
	  last = insn;
	  insn = next;
	}
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      last = make_insn_raw (x);
      add_insn (last);
      break;
    }

  return last;
}

/* Make an insn of code JUMP_INSN with pattern X
   and add it to the end of the doubly-linked list.  */

rtx
emit_jump_insn (rtx x)
{
  rtx last = NULL_RTX, insn;

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      insn = x;
      while (insn)
	{
	  rtx next = NEXT_INSN (insn);
	  add_insn (insn);
	  last = insn;
	  insn = next;
	}
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      last = make_jump_insn_raw (x);
      add_insn (last);
      break;
    }

  return last;
}

/* Make an insn of code CALL_INSN with pattern X
   and add it to the end of the doubly-linked list.  */

rtx
emit_call_insn (rtx x)
{
  rtx insn;

  switch (GET_CODE (x))
    {
    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case CODE_LABEL:
    case BARRIER:
    case NOTE:
      insn = emit_insn (x);
      break;

#ifdef ENABLE_RTL_CHECKING
    case SEQUENCE:
      gcc_unreachable ();
      break;
#endif

    default:
      insn = make_call_insn_raw (x);
      add_insn (insn);
      break;
    }

  return insn;
}

/* Add the label LABEL to the end of the doubly-linked list.  */

rtx
emit_label (rtx label)
{
  /* This can be called twice for the same label
     as a result of the confusion that follows a syntax error!
     So make it harmless.  */
  if (INSN_UID (label) == 0)
    {
      INSN_UID (label) = cur_insn_uid++;
      add_insn (label);
    }
  return label;
}

/* Make an insn of code BARRIER
   and add it to the end of the doubly-linked list.  */

rtx
emit_barrier (void)
{
  rtx barrier = rtx_alloc (BARRIER);
  INSN_UID (barrier) = cur_insn_uid++;
  add_insn (barrier);
  return barrier;
}

/* Emit a copy of note ORIG.  */

rtx
emit_note_copy (rtx orig)
{
  rtx note;
  
  note = rtx_alloc (NOTE);
  
  INSN_UID (note) = cur_insn_uid++;
  NOTE_DATA (note) = NOTE_DATA (orig);
  NOTE_KIND (note) = NOTE_KIND (orig);
  BLOCK_FOR_INSN (note) = NULL;
  add_insn (note);
  
  return note;
}

/* Make an insn of code NOTE or type NOTE_NO
   and add it to the end of the doubly-linked list.  */

rtx
emit_note (enum insn_note kind)
{
  rtx note;

  note = rtx_alloc (NOTE);
  INSN_UID (note) = cur_insn_uid++;
  NOTE_KIND (note) = kind;
  memset (&NOTE_DATA (note), 0, sizeof (NOTE_DATA (note)));
  BLOCK_FOR_INSN (note) = NULL;
  add_insn (note);
  return note;
}

/* Emit a clobber of lvalue X.  */

rtx
emit_clobber (rtx x)
{
  /* CONCATs should not appear in the insn stream.  */
  if (GET_CODE (x) == CONCAT)
    {
      emit_clobber (XEXP (x, 0));
      return emit_clobber (XEXP (x, 1));
    }
  return emit_insn (gen_rtx_CLOBBER (VOIDmode, x));
}

/* Return a sequence of insns to clobber lvalue X.  */

rtx
gen_clobber (rtx x)
{
  rtx seq;

  start_sequence ();
  emit_clobber (x);
  seq = get_insns ();
  end_sequence ();
  return seq;
}

/* Emit a use of rvalue X.  */

rtx
emit_use (rtx x)
{
  /* CONCATs should not appear in the insn stream.  */
  if (GET_CODE (x) == CONCAT)
    {
      emit_use (XEXP (x, 0));
      return emit_use (XEXP (x, 1));
    }
  return emit_insn (gen_rtx_USE (VOIDmode, x));
}

/* Return a sequence of insns to use rvalue X.  */

rtx
gen_use (rtx x)
{
  rtx seq;

  start_sequence ();
  emit_use (x);
  seq = get_insns ();
  end_sequence ();
  return seq;
}

/* Cause next statement to emit a line note even if the line number
   has not changed.  */

void
force_next_line_note (void)
{
  last_location = -1;
}

/* Place a note of KIND on insn INSN with DATUM as the datum. If a
   note of this type already exists, remove it first.  */

rtx
set_unique_reg_note (rtx insn, enum reg_note kind, rtx datum)
{
  rtx note = find_reg_note (insn, kind, NULL_RTX);

  switch (kind)
    {
    case REG_EQUAL:
    case REG_EQUIV:
      /* Don't add REG_EQUAL/REG_EQUIV notes if the insn
	 has multiple sets (some callers assume single_set
	 means the insn only has one set, when in fact it
	 means the insn only has one * useful * set).  */
      if (GET_CODE (PATTERN (insn)) == PARALLEL && multiple_sets (insn))
	{
	  gcc_assert (!note);
	  return NULL_RTX;
	}

      /* Don't add ASM_OPERAND REG_EQUAL/REG_EQUIV notes.
	 It serves no useful purpose and breaks eliminate_regs.  */
      if (GET_CODE (datum) == ASM_OPERANDS)
	return NULL_RTX;

      if (note)
	{
	  XEXP (note, 0) = datum;
	  df_notes_rescan (insn);
	  return note;
	}
      break;

    default:
      if (note)
	{
	  XEXP (note, 0) = datum;
	  return note;
	}
      break;
    }

  add_reg_note (insn, kind, datum);

  switch (kind)
    {
    case REG_EQUAL:
    case REG_EQUIV:
      df_notes_rescan (insn);
      break;
    default:
      break;
    }

  return REG_NOTES (insn);
}

/* Return an indication of which type of insn should have X as a body.
   The value is CODE_LABEL, INSN, CALL_INSN or JUMP_INSN.  */

static enum rtx_code
classify_insn (rtx x)
{
  if (LABEL_P (x))
    return CODE_LABEL;
  if (GET_CODE (x) == CALL)
    return CALL_INSN;
  if (GET_CODE (x) == RETURN)
    return JUMP_INSN;
  if (GET_CODE (x) == SET)
    {
      if (SET_DEST (x) == pc_rtx)
	return JUMP_INSN;
      else if (GET_CODE (SET_SRC (x)) == CALL)
	return CALL_INSN;
      else
	return INSN;
    }
  if (GET_CODE (x) == PARALLEL)
    {
      int j;
      for (j = XVECLEN (x, 0) - 1; j >= 0; j--)
	if (GET_CODE (XVECEXP (x, 0, j)) == CALL)
	  return CALL_INSN;
	else if (GET_CODE (XVECEXP (x, 0, j)) == SET
		 && SET_DEST (XVECEXP (x, 0, j)) == pc_rtx)
	  return JUMP_INSN;
	else if (GET_CODE (XVECEXP (x, 0, j)) == SET
		 && GET_CODE (SET_SRC (XVECEXP (x, 0, j))) == CALL)
	  return CALL_INSN;
    }
  return INSN;
}

/* Emit the rtl pattern X as an appropriate kind of insn.
   If X is a label, it is simply added into the insn chain.  */

rtx
emit (rtx x)
{
  enum rtx_code code = classify_insn (x);

  switch (code)
    {
    case CODE_LABEL:
      return emit_label (x);
    case INSN:
      return emit_insn (x);
    case  JUMP_INSN:
      {
	rtx insn = emit_jump_insn (x);
	if (any_uncondjump_p (insn) || GET_CODE (x) == RETURN)
	  return emit_barrier ();
	return insn;
      }
    case CALL_INSN:
      return emit_call_insn (x);
    default:
      gcc_unreachable ();
    }
}

/* Space for free sequence stack entries.  */
static GTY ((deletable)) struct sequence_stack *free_sequence_stack;

/* Begin emitting insns to a sequence.  If this sequence will contain
   something that might cause the compiler to pop arguments to function
   calls (because those pops have previously been deferred; see
   INHIBIT_DEFER_POP for more details), use do_pending_stack_adjust
   before calling this function.  That will ensure that the deferred
   pops are not accidentally emitted in the middle of this sequence.  */

void
start_sequence (void)
{
  struct sequence_stack *tem;

  if (free_sequence_stack != NULL)
    {
      tem = free_sequence_stack;
      free_sequence_stack = tem->next;
    }
  else
    tem = GGC_NEW (struct sequence_stack);

  tem->next = seq_stack;
  tem->first = first_insn;
  tem->last = last_insn;

  seq_stack = tem;

  first_insn = 0;
  last_insn = 0;
}

/* Set up the insn chain starting with FIRST as the current sequence,
   saving the previously current one.  See the documentation for
   start_sequence for more information about how to use this function.  */

void
push_to_sequence (rtx first)
{
  rtx last;

  start_sequence ();

  for (last = first; last && NEXT_INSN (last); last = NEXT_INSN (last));

  first_insn = first;
  last_insn = last;
}

/* Like push_to_sequence, but take the last insn as an argument to avoid
   looping through the list.  */

void
push_to_sequence2 (rtx first, rtx last)
{
  start_sequence ();

  first_insn = first;
  last_insn = last;
}

/* Set up the outer-level insn chain
   as the current sequence, saving the previously current one.  */

void
push_topmost_sequence (void)
{
  struct sequence_stack *stack, *top = NULL;

  start_sequence ();

  for (stack = seq_stack; stack; stack = stack->next)
    top = stack;

  first_insn = top->first;
  last_insn = top->last;
}

/* After emitting to the outer-level insn chain, update the outer-level
   insn chain, and restore the previous saved state.  */

void
pop_topmost_sequence (void)
{
  struct sequence_stack *stack, *top = NULL;

  for (stack = seq_stack; stack; stack = stack->next)
    top = stack;

  top->first = first_insn;
  top->last = last_insn;

  end_sequence ();
}

/* After emitting to a sequence, restore previous saved state.

   To get the contents of the sequence just made, you must call
   `get_insns' *before* calling here.

   If the compiler might have deferred popping arguments while
   generating this sequence, and this sequence will not be immediately
   inserted into the instruction stream, use do_pending_stack_adjust
   before calling get_insns.  That will ensure that the deferred
   pops are inserted into this sequence, and not into some random
   location in the instruction stream.  See INHIBIT_DEFER_POP for more
   information about deferred popping of arguments.  */

void
end_sequence (void)
{
  struct sequence_stack *tem = seq_stack;

  first_insn = tem->first;
  last_insn = tem->last;
  seq_stack = tem->next;

  memset (tem, 0, sizeof (*tem));
  tem->next = free_sequence_stack;
  free_sequence_stack = tem;
}

/* Return 1 if currently emitting into a sequence.  */

int
in_sequence_p (void)
{
  return seq_stack != 0;
}

/* Put the various virtual registers into REGNO_REG_RTX.  */

static void
init_virtual_regs (void)
{
  regno_reg_rtx[VIRTUAL_INCOMING_ARGS_REGNUM] = virtual_incoming_args_rtx;
  regno_reg_rtx[VIRTUAL_STACK_VARS_REGNUM] = virtual_stack_vars_rtx;
  regno_reg_rtx[VIRTUAL_STACK_DYNAMIC_REGNUM] = virtual_stack_dynamic_rtx;
  regno_reg_rtx[VIRTUAL_OUTGOING_ARGS_REGNUM] = virtual_outgoing_args_rtx;
  regno_reg_rtx[VIRTUAL_CFA_REGNUM] = virtual_cfa_rtx;
}


/* Used by copy_insn_1 to avoid copying SCRATCHes more than once.  */
static rtx copy_insn_scratch_in[MAX_RECOG_OPERANDS];
static rtx copy_insn_scratch_out[MAX_RECOG_OPERANDS];
static int copy_insn_n_scratches;

/* When an insn is being copied by copy_insn_1, this is nonzero if we have
   copied an ASM_OPERANDS.
   In that case, it is the original input-operand vector.  */
static rtvec orig_asm_operands_vector;

/* When an insn is being copied by copy_insn_1, this is nonzero if we have
   copied an ASM_OPERANDS.
   In that case, it is the copied input-operand vector.  */
static rtvec copy_asm_operands_vector;

/* Likewise for the constraints vector.  */
static rtvec orig_asm_constraints_vector;
static rtvec copy_asm_constraints_vector;

/* Recursively create a new copy of an rtx for copy_insn.
   This function differs from copy_rtx in that it handles SCRATCHes and
   ASM_OPERANDs properly.
   Normally, this function is not used directly; use copy_insn as front end.
   However, you could first copy an insn pattern with copy_insn and then use
   this function afterwards to properly copy any REG_NOTEs containing
   SCRATCHes.  */

rtx
copy_insn_1 (rtx orig)
{
  rtx copy;
  int i, j;
  RTX_CODE code;
  const char *format_ptr;

  code = GET_CODE (orig);

  switch (code)
    {
    case REG:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_FIXED:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
      return orig;
    case CLOBBER:
      if (REG_P (XEXP (orig, 0)) && REGNO (XEXP (orig, 0)) < FIRST_PSEUDO_REGISTER)
	return orig;
      break;

    case SCRATCH:
      for (i = 0; i < copy_insn_n_scratches; i++)
	if (copy_insn_scratch_in[i] == orig)
	  return copy_insn_scratch_out[i];
      break;

    case CONST:
      if (shared_const_p (orig))
	return orig;
      break;

      /* A MEM with a constant address is not sharable.  The problem is that
	 the constant address may need to be reloaded.  If the mem is shared,
	 then reloading one copy of this mem will cause all copies to appear
	 to have been reloaded.  */

    default:
      break;
    }

  /* Copy the various flags, fields, and other information.  We assume
     that all fields need copying, and then clear the fields that should
     not be copied.  That is the sensible default behavior, and forces
     us to explicitly document why we are *not* copying a flag.  */
  copy = shallow_copy_rtx (orig);

  /* We do not copy the USED flag, which is used as a mark bit during
     walks over the RTL.  */
  RTX_FLAG (copy, used) = 0;

  /* We do not copy JUMP, CALL, or FRAME_RELATED for INSNs.  */
  if (INSN_P (orig))
    {
      RTX_FLAG (copy, jump) = 0;
      RTX_FLAG (copy, call) = 0;
      RTX_FLAG (copy, frame_related) = 0;
    }

  format_ptr = GET_RTX_FORMAT (GET_CODE (copy));

  for (i = 0; i < GET_RTX_LENGTH (GET_CODE (copy)); i++)
    switch (*format_ptr++)
      {
      case 'e':
	if (XEXP (orig, i) != NULL)
	  XEXP (copy, i) = copy_insn_1 (XEXP (orig, i));
	break;

      case 'E':
      case 'V':
	if (XVEC (orig, i) == orig_asm_constraints_vector)
	  XVEC (copy, i) = copy_asm_constraints_vector;
	else if (XVEC (orig, i) == orig_asm_operands_vector)
	  XVEC (copy, i) = copy_asm_operands_vector;
	else if (XVEC (orig, i) != NULL)
	  {
	    XVEC (copy, i) = rtvec_alloc (XVECLEN (orig, i));
	    for (j = 0; j < XVECLEN (copy, i); j++)
	      XVECEXP (copy, i, j) = copy_insn_1 (XVECEXP (orig, i, j));
	  }
	break;

      case 't':
      case 'w':
      case 'i':
      case 's':
      case 'S':
      case 'u':
      case '0':
	/* These are left unchanged.  */
	break;

      default:
	gcc_unreachable ();
      }

  if (code == SCRATCH)
    {
      i = copy_insn_n_scratches++;
      gcc_assert (i < MAX_RECOG_OPERANDS);
      copy_insn_scratch_in[i] = orig;
      copy_insn_scratch_out[i] = copy;
    }
  else if (code == ASM_OPERANDS)
    {
      orig_asm_operands_vector = ASM_OPERANDS_INPUT_VEC (orig);
      copy_asm_operands_vector = ASM_OPERANDS_INPUT_VEC (copy);
      orig_asm_constraints_vector = ASM_OPERANDS_INPUT_CONSTRAINT_VEC (orig);
      copy_asm_constraints_vector = ASM_OPERANDS_INPUT_CONSTRAINT_VEC (copy);
    }

  return copy;
}

/* Create a new copy of an rtx.
   This function differs from copy_rtx in that it handles SCRATCHes and
   ASM_OPERANDs properly.
   INSN doesn't really have to be a full INSN; it could be just the
   pattern.  */
rtx
copy_insn (rtx insn)
{
  copy_insn_n_scratches = 0;
  orig_asm_operands_vector = 0;
  orig_asm_constraints_vector = 0;
  copy_asm_operands_vector = 0;
  copy_asm_constraints_vector = 0;
  return copy_insn_1 (insn);
}

/* Initialize data structures and variables in this file
   before generating rtl for each function.  */

void
init_emit (void)
{
  first_insn = NULL;
  last_insn = NULL;
  cur_insn_uid = 1;
  reg_rtx_no = LAST_VIRTUAL_REGISTER + 1;
  last_location = UNKNOWN_LOCATION;
  first_label_num = label_num;
  seq_stack = NULL;

  /* Init the tables that describe all the pseudo regs.  */

  crtl->emit.regno_pointer_align_length = LAST_VIRTUAL_REGISTER + 101;

  crtl->emit.regno_pointer_align
    = XCNEWVEC (unsigned char, crtl->emit.regno_pointer_align_length);

  regno_reg_rtx
    = GGC_NEWVEC (rtx, crtl->emit.regno_pointer_align_length);

  /* Put copies of all the hard registers into regno_reg_rtx.  */
  memcpy (regno_reg_rtx,
	  static_regno_reg_rtx,
	  FIRST_PSEUDO_REGISTER * sizeof (rtx));

  /* Put copies of all the virtual register rtx into regno_reg_rtx.  */
  init_virtual_regs ();

  /* Indicate that the virtual registers and stack locations are
     all pointers.  */
  REG_POINTER (stack_pointer_rtx) = 1;
  REG_POINTER (frame_pointer_rtx) = 1;
  REG_POINTER (hard_frame_pointer_rtx) = 1;
  REG_POINTER (arg_pointer_rtx) = 1;

  REG_POINTER (virtual_incoming_args_rtx) = 1;
  REG_POINTER (virtual_stack_vars_rtx) = 1;
  REG_POINTER (virtual_stack_dynamic_rtx) = 1;
  REG_POINTER (virtual_outgoing_args_rtx) = 1;
  REG_POINTER (virtual_cfa_rtx) = 1;

#ifdef STACK_BOUNDARY
  REGNO_POINTER_ALIGN (STACK_POINTER_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (FRAME_POINTER_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (HARD_FRAME_POINTER_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (ARG_POINTER_REGNUM) = STACK_BOUNDARY;

  REGNO_POINTER_ALIGN (VIRTUAL_INCOMING_ARGS_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (VIRTUAL_STACK_VARS_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (VIRTUAL_STACK_DYNAMIC_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (VIRTUAL_OUTGOING_ARGS_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (VIRTUAL_CFA_REGNUM) = BITS_PER_WORD;
#endif

#ifdef INIT_EXPANDERS
  INIT_EXPANDERS;
#endif
}

/* Generate a vector constant for mode MODE and constant value CONSTANT.  */

static rtx
gen_const_vector (enum machine_mode mode, int constant)
{
  rtx tem;
  rtvec v;
  int units, i;
  enum machine_mode inner;

  units = GET_MODE_NUNITS (mode);
  inner = GET_MODE_INNER (mode);

  gcc_assert (!DECIMAL_FLOAT_MODE_P (inner));

  v = rtvec_alloc (units);

  /* We need to call this function after we set the scalar const_tiny_rtx
     entries.  */
  gcc_assert (const_tiny_rtx[constant][(int) inner]);

  for (i = 0; i < units; ++i)
    RTVEC_ELT (v, i) = const_tiny_rtx[constant][(int) inner];

  tem = gen_rtx_raw_CONST_VECTOR (mode, v);
  return tem;
}

/* Generate a vector like gen_rtx_raw_CONST_VEC, but use the zero vector when
   all elements are zero, and the one vector when all elements are one.  */
rtx
gen_rtx_CONST_VECTOR (enum machine_mode mode, rtvec v)
{
  enum machine_mode inner = GET_MODE_INNER (mode);
  int nunits = GET_MODE_NUNITS (mode);
  rtx x;
  int i;

  /* Check to see if all of the elements have the same value.  */
  x = RTVEC_ELT (v, nunits - 1);
  for (i = nunits - 2; i >= 0; i--)
    if (RTVEC_ELT (v, i) != x)
      break;

  /* If the values are all the same, check to see if we can use one of the
     standard constant vectors.  */
  if (i == -1)
    {
      if (x == CONST0_RTX (inner))
	return CONST0_RTX (mode);
      else if (x == CONST1_RTX (inner))
	return CONST1_RTX (mode);
    }

  return gen_rtx_raw_CONST_VECTOR (mode, v);
}

/* Initialise global register information required by all functions.  */

void
init_emit_regs (void)
{
  int i;

  /* Reset register attributes */
  htab_empty (reg_attrs_htab);

  /* We need reg_raw_mode, so initialize the modes now.  */
  init_reg_modes_target ();

  /* Assign register numbers to the globally defined register rtx.  */
  pc_rtx = gen_rtx_PC (VOIDmode);
  cc0_rtx = gen_rtx_CC0 (VOIDmode);
  stack_pointer_rtx = gen_raw_REG (Pmode, STACK_POINTER_REGNUM);
  frame_pointer_rtx = gen_raw_REG (Pmode, FRAME_POINTER_REGNUM);
  hard_frame_pointer_rtx = gen_raw_REG (Pmode, HARD_FRAME_POINTER_REGNUM);
  arg_pointer_rtx = gen_raw_REG (Pmode, ARG_POINTER_REGNUM);
  virtual_incoming_args_rtx =
    gen_raw_REG (Pmode, VIRTUAL_INCOMING_ARGS_REGNUM);
  virtual_stack_vars_rtx =
    gen_raw_REG (Pmode, VIRTUAL_STACK_VARS_REGNUM);
  virtual_stack_dynamic_rtx =
    gen_raw_REG (Pmode, VIRTUAL_STACK_DYNAMIC_REGNUM);
  virtual_outgoing_args_rtx =
    gen_raw_REG (Pmode, VIRTUAL_OUTGOING_ARGS_REGNUM);
  virtual_cfa_rtx = gen_raw_REG (Pmode, VIRTUAL_CFA_REGNUM);

  /* Initialize RTL for commonly used hard registers.  These are
     copied into regno_reg_rtx as we begin to compile each function.  */
  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    static_regno_reg_rtx[i] = gen_raw_REG (reg_raw_mode[i], i);

#ifdef RETURN_ADDRESS_POINTER_REGNUM
  return_address_pointer_rtx
    = gen_raw_REG (Pmode, RETURN_ADDRESS_POINTER_REGNUM);
#endif

#ifdef STATIC_CHAIN_REGNUM
  static_chain_rtx = gen_rtx_REG (Pmode, STATIC_CHAIN_REGNUM);

#ifdef STATIC_CHAIN_INCOMING_REGNUM
  if (STATIC_CHAIN_INCOMING_REGNUM != STATIC_CHAIN_REGNUM)
    static_chain_incoming_rtx
      = gen_rtx_REG (Pmode, STATIC_CHAIN_INCOMING_REGNUM);
  else
#endif
    static_chain_incoming_rtx = static_chain_rtx;
#endif

#ifdef STATIC_CHAIN
  static_chain_rtx = STATIC_CHAIN;

#ifdef STATIC_CHAIN_INCOMING
  static_chain_incoming_rtx = STATIC_CHAIN_INCOMING;
#else
  static_chain_incoming_rtx = static_chain_rtx;
#endif
#endif

  if ((unsigned) PIC_OFFSET_TABLE_REGNUM != INVALID_REGNUM)
    pic_offset_table_rtx = gen_raw_REG (Pmode, PIC_OFFSET_TABLE_REGNUM);
  else
    pic_offset_table_rtx = NULL_RTX;
}

/* Create some permanent unique rtl objects shared between all functions.
   LINE_NUMBERS is nonzero if line numbers are to be generated.  */

void
init_emit_once (int line_numbers)
{
  int i;
  enum machine_mode mode;
  enum machine_mode double_mode;

  /* Initialize the CONST_INT, CONST_DOUBLE, CONST_FIXED, and memory attribute
     hash tables.  */
  const_int_htab = htab_create_ggc (37, const_int_htab_hash,
				    const_int_htab_eq, NULL);

  const_double_htab = htab_create_ggc (37, const_double_htab_hash,
				       const_double_htab_eq, NULL);

  const_fixed_htab = htab_create_ggc (37, const_fixed_htab_hash,
				      const_fixed_htab_eq, NULL);

  mem_attrs_htab = htab_create_ggc (37, mem_attrs_htab_hash,
				    mem_attrs_htab_eq, NULL);
  reg_attrs_htab = htab_create_ggc (37, reg_attrs_htab_hash,
				    reg_attrs_htab_eq, NULL);

  no_line_numbers = ! line_numbers;

  /* Compute the word and byte modes.  */

  byte_mode = VOIDmode;
  word_mode = VOIDmode;
  double_mode = VOIDmode;

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_INT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      if (GET_MODE_BITSIZE (mode) == BITS_PER_UNIT
	  && byte_mode == VOIDmode)
	byte_mode = mode;

      if (GET_MODE_BITSIZE (mode) == BITS_PER_WORD
	  && word_mode == VOIDmode)
	word_mode = mode;
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_FLOAT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      if (GET_MODE_BITSIZE (mode) == DOUBLE_TYPE_SIZE
	  && double_mode == VOIDmode)
	double_mode = mode;
    }

  ptr_mode = mode_for_size (POINTER_SIZE, GET_MODE_CLASS (Pmode), 0);

#ifdef INIT_EXPANDERS
  /* This is to initialize {init|mark|free}_machine_status before the first
     call to push_function_context_to.  This is needed by the Chill front
     end which calls push_function_context_to before the first call to
     init_function_start.  */
  INIT_EXPANDERS;
#endif

  /* Create the unique rtx's for certain rtx codes and operand values.  */

  /* Don't use gen_rtx_CONST_INT here since gen_rtx_CONST_INT in this case
     tries to use these variables.  */
  for (i = - MAX_SAVED_CONST_INT; i <= MAX_SAVED_CONST_INT; i++)
    const_int_rtx[i + MAX_SAVED_CONST_INT] =
      gen_rtx_raw_CONST_INT (VOIDmode, (HOST_WIDE_INT) i);

  if (STORE_FLAG_VALUE >= - MAX_SAVED_CONST_INT
      && STORE_FLAG_VALUE <= MAX_SAVED_CONST_INT)
    const_true_rtx = const_int_rtx[STORE_FLAG_VALUE + MAX_SAVED_CONST_INT];
  else
    const_true_rtx = gen_rtx_CONST_INT (VOIDmode, STORE_FLAG_VALUE);

  REAL_VALUE_FROM_INT (dconst0,   0,  0, double_mode);
  REAL_VALUE_FROM_INT (dconst1,   1,  0, double_mode);
  REAL_VALUE_FROM_INT (dconst2,   2,  0, double_mode);

  dconstm1 = dconst1;
  dconstm1.sign = 1;

  dconsthalf = dconst1;
  SET_REAL_EXP (&dconsthalf, REAL_EXP (&dconsthalf) - 1);

  for (i = 0; i < (int) ARRAY_SIZE (const_tiny_rtx); i++)
    {
      const REAL_VALUE_TYPE *const r =
	(i == 0 ? &dconst0 : i == 1 ? &dconst1 : &dconst2);

      for (mode = GET_CLASS_NARROWEST_MODE (MODE_FLOAT);
	   mode != VOIDmode;
	   mode = GET_MODE_WIDER_MODE (mode))
	const_tiny_rtx[i][(int) mode] =
	  CONST_DOUBLE_FROM_REAL_VALUE (*r, mode);

      for (mode = GET_CLASS_NARROWEST_MODE (MODE_DECIMAL_FLOAT);
	   mode != VOIDmode;
	   mode = GET_MODE_WIDER_MODE (mode))
	const_tiny_rtx[i][(int) mode] =
	  CONST_DOUBLE_FROM_REAL_VALUE (*r, mode);

      const_tiny_rtx[i][(int) VOIDmode] = GEN_INT (i);

      for (mode = GET_CLASS_NARROWEST_MODE (MODE_INT);
	   mode != VOIDmode;
	   mode = GET_MODE_WIDER_MODE (mode))
	const_tiny_rtx[i][(int) mode] = GEN_INT (i);

      for (mode = GET_CLASS_NARROWEST_MODE (MODE_PARTIAL_INT);
	   mode != VOIDmode;
	   mode = GET_MODE_WIDER_MODE (mode))
	const_tiny_rtx[i][(int) mode] = GEN_INT (i);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_COMPLEX_INT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      rtx inner = const_tiny_rtx[0][(int)GET_MODE_INNER (mode)];
      const_tiny_rtx[0][(int) mode] = gen_rtx_CONCAT (mode, inner, inner);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_COMPLEX_FLOAT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      rtx inner = const_tiny_rtx[0][(int)GET_MODE_INNER (mode)];
      const_tiny_rtx[0][(int) mode] = gen_rtx_CONCAT (mode, inner, inner);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_INT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      const_tiny_rtx[0][(int) mode] = gen_const_vector (mode, 0);
      const_tiny_rtx[1][(int) mode] = gen_const_vector (mode, 1);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_FLOAT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      const_tiny_rtx[0][(int) mode] = gen_const_vector (mode, 0);
      const_tiny_rtx[1][(int) mode] = gen_const_vector (mode, 1);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_FRACT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      FCONST0(mode).data.high = 0;
      FCONST0(mode).data.low = 0;
      FCONST0(mode).mode = mode;
      const_tiny_rtx[0][(int) mode] = CONST_FIXED_FROM_FIXED_VALUE (
				      FCONST0 (mode), mode);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_UFRACT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      FCONST0(mode).data.high = 0;
      FCONST0(mode).data.low = 0;
      FCONST0(mode).mode = mode;
      const_tiny_rtx[0][(int) mode] = CONST_FIXED_FROM_FIXED_VALUE (
				      FCONST0 (mode), mode);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_ACCUM);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      FCONST0(mode).data.high = 0;
      FCONST0(mode).data.low = 0;
      FCONST0(mode).mode = mode;
      const_tiny_rtx[0][(int) mode] = CONST_FIXED_FROM_FIXED_VALUE (
				      FCONST0 (mode), mode);

      /* We store the value 1.  */
      FCONST1(mode).data.high = 0;
      FCONST1(mode).data.low = 0;
      FCONST1(mode).mode = mode;
      lshift_double (1, 0, GET_MODE_FBIT (mode),
                     2 * HOST_BITS_PER_WIDE_INT,
                     &FCONST1(mode).data.low,
		     &FCONST1(mode).data.high,
                     SIGNED_FIXED_POINT_MODE_P (mode));
      const_tiny_rtx[1][(int) mode] = CONST_FIXED_FROM_FIXED_VALUE (
				      FCONST1 (mode), mode);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_UACCUM);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      FCONST0(mode).data.high = 0;
      FCONST0(mode).data.low = 0;
      FCONST0(mode).mode = mode;
      const_tiny_rtx[0][(int) mode] = CONST_FIXED_FROM_FIXED_VALUE (
				      FCONST0 (mode), mode);

      /* We store the value 1.  */
      FCONST1(mode).data.high = 0;
      FCONST1(mode).data.low = 0;
      FCONST1(mode).mode = mode;
      lshift_double (1, 0, GET_MODE_FBIT (mode),
                     2 * HOST_BITS_PER_WIDE_INT,
                     &FCONST1(mode).data.low,
		     &FCONST1(mode).data.high,
                     SIGNED_FIXED_POINT_MODE_P (mode));
      const_tiny_rtx[1][(int) mode] = CONST_FIXED_FROM_FIXED_VALUE (
				      FCONST1 (mode), mode);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_FRACT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      const_tiny_rtx[0][(int) mode] = gen_const_vector (mode, 0);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_UFRACT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      const_tiny_rtx[0][(int) mode] = gen_const_vector (mode, 0);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_ACCUM);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      const_tiny_rtx[0][(int) mode] = gen_const_vector (mode, 0);
      const_tiny_rtx[1][(int) mode] = gen_const_vector (mode, 1);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_UACCUM);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      const_tiny_rtx[0][(int) mode] = gen_const_vector (mode, 0);
      const_tiny_rtx[1][(int) mode] = gen_const_vector (mode, 1);
    }

  for (i = (int) CCmode; i < (int) MAX_MACHINE_MODE; ++i)
    if (GET_MODE_CLASS ((enum machine_mode) i) == MODE_CC)
      const_tiny_rtx[0][i] = const0_rtx;

  const_tiny_rtx[0][(int) BImode] = const0_rtx;
  if (STORE_FLAG_VALUE == 1)
    const_tiny_rtx[1][(int) BImode] = const1_rtx;
}

/* Produce exact duplicate of insn INSN after AFTER.
   Care updating of libcall regions if present.  */

rtx
emit_copy_of_insn_after (rtx insn, rtx after)
{
  rtx new_rtx, link;

  switch (GET_CODE (insn))
    {
    case INSN:
      new_rtx = emit_insn_after (copy_insn (PATTERN (insn)), after);
      break;

    case JUMP_INSN:
      new_rtx = emit_jump_insn_after (copy_insn (PATTERN (insn)), after);
      break;

    case CALL_INSN:
      new_rtx = emit_call_insn_after (copy_insn (PATTERN (insn)), after);
      if (CALL_INSN_FUNCTION_USAGE (insn))
	CALL_INSN_FUNCTION_USAGE (new_rtx)
	  = copy_insn (CALL_INSN_FUNCTION_USAGE (insn));
      SIBLING_CALL_P (new_rtx) = SIBLING_CALL_P (insn);
      RTL_CONST_CALL_P (new_rtx) = RTL_CONST_CALL_P (insn);
      RTL_PURE_CALL_P (new_rtx) = RTL_PURE_CALL_P (insn);
      RTL_LOOPING_CONST_OR_PURE_CALL_P (new_rtx) 
	= RTL_LOOPING_CONST_OR_PURE_CALL_P (insn);
      break;

    default:
      gcc_unreachable ();
    }

  /* Update LABEL_NUSES.  */
  mark_jump_label (PATTERN (new_rtx), new_rtx, 0);

  INSN_LOCATOR (new_rtx) = INSN_LOCATOR (insn);

  /* If the old insn is frame related, then so is the new one.  This is
     primarily needed for IA-64 unwind info which marks epilogue insns,
     which may be duplicated by the basic block reordering code.  */
  RTX_FRAME_RELATED_P (new_rtx) = RTX_FRAME_RELATED_P (insn);

  /* Copy all REG_NOTES except REG_LABEL_OPERAND since mark_jump_label
     will make them.  REG_LABEL_TARGETs are created there too, but are
     supposed to be sticky, so we copy them.  */
  for (link = REG_NOTES (insn); link; link = XEXP (link, 1))
    if (REG_NOTE_KIND (link) != REG_LABEL_OPERAND)
      {
	if (GET_CODE (link) == EXPR_LIST)
	  add_reg_note (new_rtx, REG_NOTE_KIND (link),
			copy_insn_1 (XEXP (link, 0)));
	else
	  add_reg_note (new_rtx, REG_NOTE_KIND (link), XEXP (link, 0));
      }

  INSN_CODE (new_rtx) = INSN_CODE (insn);
  return new_rtx;
}

static GTY((deletable)) rtx hard_reg_clobbers [NUM_MACHINE_MODES][FIRST_PSEUDO_REGISTER];
rtx
gen_hard_reg_clobber (enum machine_mode mode, unsigned int regno)
{
  if (hard_reg_clobbers[mode][regno])
    return hard_reg_clobbers[mode][regno];
  else
    return (hard_reg_clobbers[mode][regno] =
	    gen_rtx_CLOBBER (VOIDmode, gen_rtx_REG (mode, regno)));
}

#include "gt-emit-rtl.h"
