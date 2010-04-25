/* A pass for lowering trees to RTL.
   Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "basic-block.h"
#include "function.h"
#include "expr.h"
#include "langhooks.h"
#include "tree-flow.h"
#include "timevar.h"
#include "tree-dump.h"
#include "tree-pass.h"
#include "except.h"
#include "flags.h"
#include "diagnostic.h"
#include "toplev.h"
#include "debug.h"
#include "params.h"
#include "tree-inline.h"
#include "value-prof.h"
#include "target.h"


/* Return an expression tree corresponding to the RHS of GIMPLE
   statement STMT.  */

tree
gimple_assign_rhs_to_tree (gimple stmt)
{
  tree t;
  enum gimple_rhs_class grhs_class;
    
  grhs_class = get_gimple_rhs_class (gimple_expr_code (stmt));

  if (grhs_class == GIMPLE_BINARY_RHS)
    t = build2 (gimple_assign_rhs_code (stmt),
		TREE_TYPE (gimple_assign_lhs (stmt)),
		gimple_assign_rhs1 (stmt),
		gimple_assign_rhs2 (stmt));
  else if (grhs_class == GIMPLE_UNARY_RHS)
    t = build1 (gimple_assign_rhs_code (stmt),
		TREE_TYPE (gimple_assign_lhs (stmt)),
		gimple_assign_rhs1 (stmt));
  else if (grhs_class == GIMPLE_SINGLE_RHS)
    t = gimple_assign_rhs1 (stmt);
  else
    gcc_unreachable ();

  return t;
}

/* Return an expression tree corresponding to the PREDICATE of GIMPLE_COND
   statement STMT.  */

static tree
gimple_cond_pred_to_tree (gimple stmt)
{
  return build2 (gimple_cond_code (stmt), boolean_type_node,
		 gimple_cond_lhs (stmt), gimple_cond_rhs (stmt));
}

/* Helper for gimple_to_tree.  Set EXPR_LOCATION for every expression
   inside *TP.  DATA is the location to set.  */

static tree
set_expr_location_r (tree *tp, int *ws ATTRIBUTE_UNUSED, void *data)
{
  location_t *loc = (location_t *) data;
  if (EXPR_P (*tp))
    SET_EXPR_LOCATION (*tp, *loc);

  return NULL_TREE;
}


/* RTL expansion has traditionally been done on trees, so the
   transition to doing it on GIMPLE tuples is very invasive to the RTL
   expander.  To facilitate the transition, this function takes a
   GIMPLE tuple STMT and returns the same statement in the form of a
   tree.  */

static tree
gimple_to_tree (gimple stmt)
{
  tree t;
  int rn;
  tree_ann_common_t ann;
  location_t loc;

  switch (gimple_code (stmt))
    {
    case GIMPLE_ASSIGN:
      {
	tree lhs = gimple_assign_lhs (stmt);

	t = gimple_assign_rhs_to_tree (stmt);
	t = build2 (MODIFY_EXPR, TREE_TYPE (lhs), lhs, t);
	if (gimple_assign_nontemporal_move_p (stmt))
	  MOVE_NONTEMPORAL (t) = true;
      }
      break;
	                                 
    case GIMPLE_COND:
      t = gimple_cond_pred_to_tree (stmt);
      t = build3 (COND_EXPR, void_type_node, t, NULL_TREE, NULL_TREE);
      break;

    case GIMPLE_GOTO:
      t = build1 (GOTO_EXPR, void_type_node, gimple_goto_dest (stmt));
      break;

    case GIMPLE_LABEL:
      t = build1 (LABEL_EXPR, void_type_node, gimple_label_label (stmt));
      break;

    case GIMPLE_RETURN:
      {
	tree retval = gimple_return_retval (stmt);

	if (retval && retval != error_mark_node)
	  {
	    tree result = DECL_RESULT (current_function_decl);

	    /* If we are not returning the current function's RESULT_DECL,
	       build an assignment to it.  */
	    if (retval != result)
	      {
		/* I believe that a function's RESULT_DECL is unique.  */
		gcc_assert (TREE_CODE (retval) != RESULT_DECL);

		retval = build2 (MODIFY_EXPR, TREE_TYPE (result),
				 result, retval);
	      }
	  }
	t = build1 (RETURN_EXPR, void_type_node, retval);
      }
      break;

    case GIMPLE_ASM:
      {
	size_t i, n;
	tree out, in, cl;
	const char *s;

	out = NULL_TREE;
	n = gimple_asm_noutputs (stmt);
	if (n > 0)
	  {
	    t = out = gimple_asm_output_op (stmt, 0);
	    for (i = 1; i < n; i++)
	      {
		TREE_CHAIN (t) = gimple_asm_output_op (stmt, i);
		t = gimple_asm_output_op (stmt, i);
	      }
	  }

	in = NULL_TREE;
	n = gimple_asm_ninputs (stmt);
	if (n > 0)
	  {
	    t = in = gimple_asm_input_op (stmt, 0);
	    for (i = 1; i < n; i++)
	      {
		TREE_CHAIN (t) = gimple_asm_input_op (stmt, i);
		t = gimple_asm_input_op (stmt, i);
	      }
	  }

	cl = NULL_TREE;
	n = gimple_asm_nclobbers (stmt);
	if (n > 0)
	  {
	    t = cl = gimple_asm_clobber_op (stmt, 0);
	    for (i = 1; i < n; i++)
	      {
		TREE_CHAIN (t) = gimple_asm_clobber_op (stmt, i);
		t = gimple_asm_clobber_op (stmt, i);
	      }
	  }

	s = gimple_asm_string (stmt);
	t = build4 (ASM_EXPR, void_type_node, build_string (strlen (s), s),
	            out, in, cl);
        ASM_VOLATILE_P (t) = gimple_asm_volatile_p (stmt);
        ASM_INPUT_P (t) = gimple_asm_input_p (stmt);
      }
    break;

    case GIMPLE_CALL:
      {
	size_t i;
        tree fn;
	tree_ann_common_t ann;
        
	t = build_vl_exp (CALL_EXPR, gimple_call_num_args (stmt) + 3);

        CALL_EXPR_FN (t) = gimple_call_fn (stmt);
        TREE_TYPE (t) = gimple_call_return_type (stmt);
	CALL_EXPR_STATIC_CHAIN (t) = gimple_call_chain (stmt);

	for (i = 0; i < gimple_call_num_args (stmt); i++)
	  CALL_EXPR_ARG (t, i) = gimple_call_arg (stmt, i);

	if (!(gimple_call_flags (stmt) & (ECF_CONST | ECF_PURE)))
	  TREE_SIDE_EFFECTS (t) = 1;

	if (gimple_call_flags (stmt) & ECF_NOTHROW)
	  TREE_NOTHROW (t) = 1;

        CALL_EXPR_TAILCALL (t) = gimple_call_tail_p (stmt);
        CALL_EXPR_RETURN_SLOT_OPT (t) = gimple_call_return_slot_opt_p (stmt);
        CALL_FROM_THUNK_P (t) = gimple_call_from_thunk_p (stmt);
        CALL_CANNOT_INLINE_P (t) = gimple_call_cannot_inline_p (stmt);
        CALL_EXPR_VA_ARG_PACK (t) = gimple_call_va_arg_pack_p (stmt);

        /* If the call has a LHS then create a MODIFY_EXPR to hold it.  */
	{
	  tree lhs = gimple_call_lhs (stmt);

	  if (lhs)
	    t = build2 (MODIFY_EXPR, TREE_TYPE (lhs), lhs, t);
	}

        /* Record the original call statement, as it may be used
           to retrieve profile information during expansion.  */

	if ((fn = gimple_call_fndecl (stmt)) != NULL_TREE
	    && DECL_BUILT_IN (fn))
	  {
	    ann = get_tree_common_ann (t);
	    ann->stmt = stmt;
	  }
      }
    break;

    case GIMPLE_SWITCH:
      {
	tree label_vec;
	size_t i;
	tree elt = gimple_switch_label (stmt, 0);

	label_vec = make_tree_vec (gimple_switch_num_labels (stmt));

	if (!CASE_LOW (elt) && !CASE_HIGH (elt))
	  {
	    for (i = 1; i < gimple_switch_num_labels (stmt); i++)
	      TREE_VEC_ELT (label_vec, i - 1) = gimple_switch_label (stmt, i);

	    /* The default case in a SWITCH_EXPR must be at the end of
	       the label vector.  */
	    TREE_VEC_ELT (label_vec, i - 1) = gimple_switch_label (stmt, 0);
	  }
	else
	  {
	    for (i = 0; i < gimple_switch_num_labels (stmt); i++)
	      TREE_VEC_ELT (label_vec, i) = gimple_switch_label (stmt, i);
	  }

	t = build3 (SWITCH_EXPR, void_type_node, gimple_switch_index (stmt),
		    NULL, label_vec);
      }
    break;

    case GIMPLE_NOP:
    case GIMPLE_PREDICT:
      t = build1 (NOP_EXPR, void_type_node, size_zero_node);
      break;

    case GIMPLE_RESX:
      t = build_resx (gimple_resx_region (stmt));
      break;
	
    default:
      if (errorcount == 0)
	{
	  error ("Unrecognized GIMPLE statement during RTL expansion");
	  print_gimple_stmt (stderr, stmt, 4, 0);
	  gcc_unreachable ();
	}
      else
	{
	  /* Ignore any bad gimple codes if we're going to die anyhow,
	     so we can at least set TREE_ASM_WRITTEN and have the rest
	     of compilation advance without sudden ICE death.  */
	  t = build1 (NOP_EXPR, void_type_node, size_zero_node);
	  break;
	}
    }

  /* If STMT is inside an exception region, record it in the generated
     expression.  */
  rn = lookup_stmt_eh_region (stmt);
  if (rn >= 0)
    {
      tree call = get_call_expr_in (t);

      ann = get_tree_common_ann (t);
      ann->rn = rn;
      
      /* For a CALL_EXPR on the RHS of an assignment, calls.c looks up
 	 the CALL_EXPR not the assignment statment for EH region number. */
      if (call && call != t)
	{
	  ann = get_tree_common_ann (call);
	  ann->rn = rn;
	}
    }

  /* Set EXPR_LOCATION in all the embedded expressions.  */
  loc = gimple_location (stmt);
  walk_tree (&t, set_expr_location_r, (void *) &loc, NULL);

  TREE_BLOCK (t) = gimple_block (stmt);

  return t;
}


/* Release back to GC memory allocated by gimple_to_tree.  */

static void
release_stmt_tree (gimple stmt, tree stmt_tree)
{
  tree_ann_common_t ann;

  switch (gimple_code (stmt))
    {
    case GIMPLE_ASSIGN:
      if (get_gimple_rhs_class (gimple_expr_code (stmt)) != GIMPLE_SINGLE_RHS)
	ggc_free (TREE_OPERAND (stmt_tree, 1));
      break;
    case GIMPLE_COND:
      ggc_free (COND_EXPR_COND (stmt_tree));
      break;
    case GIMPLE_RETURN:
      if (TREE_OPERAND (stmt_tree, 0)
	  && TREE_CODE (TREE_OPERAND (stmt_tree, 0)) == MODIFY_EXPR)
	ggc_free (TREE_OPERAND (stmt_tree, 0));
      break;
    case GIMPLE_CALL:
      if (gimple_call_lhs (stmt))
	{
	  ann = tree_common_ann (TREE_OPERAND (stmt_tree, 1));
	  if (ann)
	    ggc_free (ann);
	  ggc_free (TREE_OPERAND (stmt_tree, 1));
	}
      break;
    default:
      break;
    }
  ann = tree_common_ann (stmt_tree);
  if (ann)
    ggc_free (ann);
  ggc_free (stmt_tree);
}


/* Verify that there is exactly single jump instruction since last and attach
   REG_BR_PROB note specifying probability.
   ??? We really ought to pass the probability down to RTL expanders and let it
   re-distribute it when the conditional expands into multiple conditionals.
   This is however difficult to do.  */
void
add_reg_br_prob_note (rtx last, int probability)
{
  if (profile_status == PROFILE_ABSENT)
    return;
  for (last = NEXT_INSN (last); last && NEXT_INSN (last); last = NEXT_INSN (last))
    if (JUMP_P (last))
      {
	/* It is common to emit condjump-around-jump sequence when we don't know
	   how to reverse the conditional.  Special case this.  */
	if (!any_condjump_p (last)
	    || !JUMP_P (NEXT_INSN (last))
	    || !simplejump_p (NEXT_INSN (last))
	    || !NEXT_INSN (NEXT_INSN (last))
	    || !BARRIER_P (NEXT_INSN (NEXT_INSN (last)))
	    || !NEXT_INSN (NEXT_INSN (NEXT_INSN (last)))
	    || !LABEL_P (NEXT_INSN (NEXT_INSN (NEXT_INSN (last))))
	    || NEXT_INSN (NEXT_INSN (NEXT_INSN (NEXT_INSN (last)))))
	  goto failed;
	gcc_assert (!find_reg_note (last, REG_BR_PROB, 0));
	add_reg_note (last, REG_BR_PROB,
		      GEN_INT (REG_BR_PROB_BASE - probability));
	return;
      }
  if (!last || !JUMP_P (last) || !any_condjump_p (last))
    goto failed;
  gcc_assert (!find_reg_note (last, REG_BR_PROB, 0));
  add_reg_note (last, REG_BR_PROB, GEN_INT (probability));
  return;
failed:
  if (dump_file)
    fprintf (dump_file, "Failed to add probability note\n");
}


#ifndef STACK_ALIGNMENT_NEEDED
#define STACK_ALIGNMENT_NEEDED 1
#endif


/* This structure holds data relevant to one variable that will be
   placed in a stack slot.  */
struct stack_var
{
  /* The Variable.  */
  tree decl;

  /* The offset of the variable.  During partitioning, this is the
     offset relative to the partition.  After partitioning, this
     is relative to the stack frame.  */
  HOST_WIDE_INT offset;

  /* Initially, the size of the variable.  Later, the size of the partition,
     if this variable becomes it's partition's representative.  */
  HOST_WIDE_INT size;

  /* The *byte* alignment required for this variable.  Or as, with the
     size, the alignment for this partition.  */
  unsigned int alignb;

  /* The partition representative.  */
  size_t representative;

  /* The next stack variable in the partition, or EOC.  */
  size_t next;
};

#define EOC  ((size_t)-1)

/* We have an array of such objects while deciding allocation.  */
static struct stack_var *stack_vars;
static size_t stack_vars_alloc;
static size_t stack_vars_num;

/* An array of indices such that stack_vars[stack_vars_sorted[i]].size
   is non-decreasing.  */
static size_t *stack_vars_sorted;

/* We have an interference graph between such objects.  This graph
   is lower triangular.  */
static bool *stack_vars_conflict;
static size_t stack_vars_conflict_alloc;

/* The phase of the stack frame.  This is the known misalignment of
   virtual_stack_vars_rtx from PREFERRED_STACK_BOUNDARY.  That is,
   (frame_offset+frame_phase) % PREFERRED_STACK_BOUNDARY == 0.  */
static int frame_phase;

/* Used during expand_used_vars to remember if we saw any decls for
   which we'd like to enable stack smashing protection.  */
static bool has_protected_decls;

/* Used during expand_used_vars.  Remember if we say a character buffer
   smaller than our cutoff threshold.  Used for -Wstack-protector.  */
static bool has_short_buffer;

/* Discover the byte alignment to use for DECL.  Ignore alignment
   we can't do with expected alignment of the stack boundary.  */

static unsigned int
get_decl_align_unit (tree decl)
{
  unsigned int align;

  align = LOCAL_DECL_ALIGNMENT (decl);

  if (align > MAX_SUPPORTED_STACK_ALIGNMENT)
    align = MAX_SUPPORTED_STACK_ALIGNMENT;

  if (SUPPORTS_STACK_ALIGNMENT)
    {
      if (crtl->stack_alignment_estimated < align)
	{
	  gcc_assert(!crtl->stack_realign_processed);
          crtl->stack_alignment_estimated = align;
	}
    }

  /* stack_alignment_needed > PREFERRED_STACK_BOUNDARY is permitted.
     So here we only make sure stack_alignment_needed >= align.  */
  if (crtl->stack_alignment_needed < align)
    crtl->stack_alignment_needed = align;
  if (crtl->max_used_stack_slot_alignment < crtl->stack_alignment_needed)
    crtl->max_used_stack_slot_alignment = crtl->stack_alignment_needed;

  return align / BITS_PER_UNIT;
}

/* Allocate SIZE bytes at byte alignment ALIGN from the stack frame.
   Return the frame offset.  */

static HOST_WIDE_INT
alloc_stack_frame_space (HOST_WIDE_INT size, HOST_WIDE_INT align)
{
  HOST_WIDE_INT offset, new_frame_offset;

  new_frame_offset = frame_offset;
  if (FRAME_GROWS_DOWNWARD)
    {
      new_frame_offset -= size + frame_phase;
      new_frame_offset &= -align;
      new_frame_offset += frame_phase;
      offset = new_frame_offset;
    }
  else
    {
      new_frame_offset -= frame_phase;
      new_frame_offset += align - 1;
      new_frame_offset &= -align;
      new_frame_offset += frame_phase;
      offset = new_frame_offset;
      new_frame_offset += size;
    }
  frame_offset = new_frame_offset;

  if (frame_offset_overflow (frame_offset, cfun->decl))
    frame_offset = offset = 0;

  return offset;
}

/* Accumulate DECL into STACK_VARS.  */

static void
add_stack_var (tree decl)
{
  if (stack_vars_num >= stack_vars_alloc)
    {
      if (stack_vars_alloc)
	stack_vars_alloc = stack_vars_alloc * 3 / 2;
      else
	stack_vars_alloc = 32;
      stack_vars
	= XRESIZEVEC (struct stack_var, stack_vars, stack_vars_alloc);
    }
  stack_vars[stack_vars_num].decl = decl;
  stack_vars[stack_vars_num].offset = 0;
  stack_vars[stack_vars_num].size = tree_low_cst (DECL_SIZE_UNIT (decl), 1);
  stack_vars[stack_vars_num].alignb = get_decl_align_unit (decl);

  /* All variables are initially in their own partition.  */
  stack_vars[stack_vars_num].representative = stack_vars_num;
  stack_vars[stack_vars_num].next = EOC;

  /* Ensure that this decl doesn't get put onto the list twice.  */
  SET_DECL_RTL (decl, pc_rtx);

  stack_vars_num++;
}

/* Compute the linear index of a lower-triangular coordinate (I, J).  */

static size_t
triangular_index (size_t i, size_t j)
{
  if (i < j)
    {
      size_t t;
      t = i, i = j, j = t;
    }
  return (i * (i + 1)) / 2 + j;
}

/* Ensure that STACK_VARS_CONFLICT is large enough for N objects.  */

static void
resize_stack_vars_conflict (size_t n)
{
  size_t size = triangular_index (n-1, n-1) + 1;

  if (size <= stack_vars_conflict_alloc)
    return;

  stack_vars_conflict = XRESIZEVEC (bool, stack_vars_conflict, size);
  memset (stack_vars_conflict + stack_vars_conflict_alloc, 0,
	  (size - stack_vars_conflict_alloc) * sizeof (bool));
  stack_vars_conflict_alloc = size;
}

/* Make the decls associated with luid's X and Y conflict.  */

static void
add_stack_var_conflict (size_t x, size_t y)
{
  size_t index = triangular_index (x, y);
  gcc_assert (index < stack_vars_conflict_alloc);
  stack_vars_conflict[index] = true;
}

/* Check whether the decls associated with luid's X and Y conflict.  */

static bool
stack_var_conflict_p (size_t x, size_t y)
{
  size_t index = triangular_index (x, y);
  gcc_assert (index < stack_vars_conflict_alloc);
  return stack_vars_conflict[index];
}
 
/* Returns true if TYPE is or contains a union type.  */

static bool
aggregate_contains_union_type (tree type)
{
  tree field;

  if (TREE_CODE (type) == UNION_TYPE
      || TREE_CODE (type) == QUAL_UNION_TYPE)
    return true;
  if (TREE_CODE (type) == ARRAY_TYPE)
    return aggregate_contains_union_type (TREE_TYPE (type));
  if (TREE_CODE (type) != RECORD_TYPE)
    return false;

  for (field = TYPE_FIELDS (type); field; field = TREE_CHAIN (field))
    if (TREE_CODE (field) == FIELD_DECL)
      if (aggregate_contains_union_type (TREE_TYPE (field)))
	return true;

  return false;
}

/* A subroutine of expand_used_vars.  If two variables X and Y have alias
   sets that do not conflict, then do add a conflict for these variables
   in the interference graph.  We also need to make sure to add conflicts
   for union containing structures.  Else RTL alias analysis comes along
   and due to type based aliasing rules decides that for two overlapping
   union temporaries { short s; int i; } accesses to the same mem through
   different types may not alias and happily reorders stores across
   life-time boundaries of the temporaries (See PR25654).
   We also have to mind MEM_IN_STRUCT_P and MEM_SCALAR_P.  */

static void
add_alias_set_conflicts (void)
{
  size_t i, j, n = stack_vars_num;

  for (i = 0; i < n; ++i)
    {
      tree type_i = TREE_TYPE (stack_vars[i].decl);
      bool aggr_i = AGGREGATE_TYPE_P (type_i);
      bool contains_union;

      contains_union = aggregate_contains_union_type (type_i);
      for (j = 0; j < i; ++j)
	{
	  tree type_j = TREE_TYPE (stack_vars[j].decl);
	  bool aggr_j = AGGREGATE_TYPE_P (type_j);
	  if (aggr_i != aggr_j
	      /* Either the objects conflict by means of type based
		 aliasing rules, or we need to add a conflict.  */
	      || !objects_must_conflict_p (type_i, type_j)
	      /* In case the types do not conflict ensure that access
		 to elements will conflict.  In case of unions we have
		 to be careful as type based aliasing rules may say
		 access to the same memory does not conflict.  So play
		 safe and add a conflict in this case.  */
	      || contains_union)
	    add_stack_var_conflict (i, j);
	}
    }
}

/* A subroutine of partition_stack_vars.  A comparison function for qsort,
   sorting an array of indices by the size of the object.  */

static int
stack_var_size_cmp (const void *a, const void *b)
{
  HOST_WIDE_INT sa = stack_vars[*(const size_t *)a].size;
  HOST_WIDE_INT sb = stack_vars[*(const size_t *)b].size;
  unsigned int uida = DECL_UID (stack_vars[*(const size_t *)a].decl);
  unsigned int uidb = DECL_UID (stack_vars[*(const size_t *)b].decl);

  if (sa < sb)
    return -1;
  if (sa > sb)
    return 1;
  /* For stack variables of the same size use the uid of the decl
     to make the sort stable.  */
  if (uida < uidb)
    return -1;
  if (uida > uidb)
    return 1;
  return 0;
}

/* A subroutine of partition_stack_vars.  The UNION portion of a UNION/FIND
   partitioning algorithm.  Partitions A and B are known to be non-conflicting.
   Merge them into a single partition A.

   At the same time, add OFFSET to all variables in partition B.  At the end
   of the partitioning process we've have a nice block easy to lay out within
   the stack frame.  */

static void
union_stack_vars (size_t a, size_t b, HOST_WIDE_INT offset)
{
  size_t i, last;

  /* Update each element of partition B with the given offset,
     and merge them into partition A.  */
  for (last = i = b; i != EOC; last = i, i = stack_vars[i].next)
    {
      stack_vars[i].offset += offset;
      stack_vars[i].representative = a;
    }
  stack_vars[last].next = stack_vars[a].next;
  stack_vars[a].next = b;

  /* Update the required alignment of partition A to account for B.  */
  if (stack_vars[a].alignb < stack_vars[b].alignb)
    stack_vars[a].alignb = stack_vars[b].alignb;

  /* Update the interference graph and merge the conflicts.  */
  for (last = stack_vars_num, i = 0; i < last; ++i)
    if (stack_var_conflict_p (b, i))
      add_stack_var_conflict (a, i);
}

/* A subroutine of expand_used_vars.  Binpack the variables into
   partitions constrained by the interference graph.  The overall
   algorithm used is as follows:

	Sort the objects by size.
	For each object A {
	  S = size(A)
	  O = 0
	  loop {
	    Look for the largest non-conflicting object B with size <= S.
	    UNION (A, B)
	    offset(B) = O
	    O += size(B)
	    S -= size(B)
	  }
	}
*/

static void
partition_stack_vars (void)
{
  size_t si, sj, n = stack_vars_num;

  stack_vars_sorted = XNEWVEC (size_t, stack_vars_num);
  for (si = 0; si < n; ++si)
    stack_vars_sorted[si] = si;

  if (n == 1)
    return;

  qsort (stack_vars_sorted, n, sizeof (size_t), stack_var_size_cmp);

  /* Special case: detect when all variables conflict, and thus we can't
     do anything during the partitioning loop.  It isn't uncommon (with
     C code at least) to declare all variables at the top of the function,
     and if we're not inlining, then all variables will be in the same scope.
     Take advantage of very fast libc routines for this scan.  */
  gcc_assert (sizeof(bool) == sizeof(char));
  if (memchr (stack_vars_conflict, false, stack_vars_conflict_alloc) == NULL)
    return;

  for (si = 0; si < n; ++si)
    {
      size_t i = stack_vars_sorted[si];
      HOST_WIDE_INT isize = stack_vars[i].size;
      HOST_WIDE_INT offset = 0;

      for (sj = si; sj-- > 0; )
	{
	  size_t j = stack_vars_sorted[sj];
	  HOST_WIDE_INT jsize = stack_vars[j].size;
	  unsigned int jalign = stack_vars[j].alignb;

	  /* Ignore objects that aren't partition representatives.  */
	  if (stack_vars[j].representative != j)
	    continue;

	  /* Ignore objects too large for the remaining space.  */
	  if (isize < jsize)
	    continue;

	  /* Ignore conflicting objects.  */
	  if (stack_var_conflict_p (i, j))
	    continue;

	  /* Refine the remaining space check to include alignment.  */
	  if (offset & (jalign - 1))
	    {
	      HOST_WIDE_INT toff = offset;
	      toff += jalign - 1;
	      toff &= -(HOST_WIDE_INT)jalign;
	      if (isize - (toff - offset) < jsize)
		continue;

	      isize -= toff - offset;
	      offset = toff;
	    }

	  /* UNION the objects, placing J at OFFSET.  */
	  union_stack_vars (i, j, offset);

	  isize -= jsize;
	  if (isize == 0)
	    break;
	}
    }
}

/* A debugging aid for expand_used_vars.  Dump the generated partitions.  */

static void
dump_stack_var_partition (void)
{
  size_t si, i, j, n = stack_vars_num;

  for (si = 0; si < n; ++si)
    {
      i = stack_vars_sorted[si];

      /* Skip variables that aren't partition representatives, for now.  */
      if (stack_vars[i].representative != i)
	continue;

      fprintf (dump_file, "Partition %lu: size " HOST_WIDE_INT_PRINT_DEC
	       " align %u\n", (unsigned long) i, stack_vars[i].size,
	       stack_vars[i].alignb);

      for (j = i; j != EOC; j = stack_vars[j].next)
	{
	  fputc ('\t', dump_file);
	  print_generic_expr (dump_file, stack_vars[j].decl, dump_flags);
	  fprintf (dump_file, ", offset " HOST_WIDE_INT_PRINT_DEC "\n",
		   stack_vars[j].offset);
	}
    }
}

/* Assign rtl to DECL at frame offset OFFSET.  */

static void
expand_one_stack_var_at (tree decl, HOST_WIDE_INT offset)
{
  HOST_WIDE_INT align;
  rtx x;

  /* If this fails, we've overflowed the stack frame.  Error nicely?  */
  gcc_assert (offset == trunc_int_for_mode (offset, Pmode));

  x = plus_constant (virtual_stack_vars_rtx, offset);
  x = gen_rtx_MEM (DECL_MODE (decl), x);

  /* Set alignment we actually gave this decl.  */
  offset -= frame_phase;
  align = offset & -offset;
  align *= BITS_PER_UNIT;
  if (align > STACK_BOUNDARY || align == 0)
    align = STACK_BOUNDARY;
  DECL_ALIGN (decl) = align;
  DECL_USER_ALIGN (decl) = 0;

  set_mem_attributes (x, decl, true);
  SET_DECL_RTL (decl, x);
}

/* A subroutine of expand_used_vars.  Give each partition representative
   a unique location within the stack frame.  Update each partition member
   with that location.  */

static void
expand_stack_vars (bool (*pred) (tree))
{
  size_t si, i, j, n = stack_vars_num;

  for (si = 0; si < n; ++si)
    {
      HOST_WIDE_INT offset;

      i = stack_vars_sorted[si];

      /* Skip variables that aren't partition representatives, for now.  */
      if (stack_vars[i].representative != i)
	continue;

      /* Skip variables that have already had rtl assigned.  See also
	 add_stack_var where we perpetrate this pc_rtx hack.  */
      if (DECL_RTL (stack_vars[i].decl) != pc_rtx)
	continue;

      /* Check the predicate to see whether this variable should be
	 allocated in this pass.  */
      if (pred && !pred (stack_vars[i].decl))
	continue;

      offset = alloc_stack_frame_space (stack_vars[i].size,
					stack_vars[i].alignb);

      /* Create rtl for each variable based on their location within the
	 partition.  */
      for (j = i; j != EOC; j = stack_vars[j].next)
	{
	  gcc_assert (stack_vars[j].offset <= stack_vars[i].size);
	  expand_one_stack_var_at (stack_vars[j].decl,
				   stack_vars[j].offset + offset);
	}
    }
}

/* Take into account all sizes of partitions and reset DECL_RTLs.  */
static HOST_WIDE_INT
account_stack_vars (void)
{
  size_t si, j, i, n = stack_vars_num;
  HOST_WIDE_INT size = 0;

  for (si = 0; si < n; ++si)
    {
      i = stack_vars_sorted[si];

      /* Skip variables that aren't partition representatives, for now.  */
      if (stack_vars[i].representative != i)
	continue;

      size += stack_vars[i].size;
      for (j = i; j != EOC; j = stack_vars[j].next)
	SET_DECL_RTL (stack_vars[j].decl, NULL);
    }
  return size;
}

/* A subroutine of expand_one_var.  Called to immediately assign rtl
   to a variable to be allocated in the stack frame.  */

static void
expand_one_stack_var (tree var)
{
  HOST_WIDE_INT size, offset, align;

  size = tree_low_cst (DECL_SIZE_UNIT (var), 1);
  align = get_decl_align_unit (var);
  offset = alloc_stack_frame_space (size, align);

  expand_one_stack_var_at (var, offset);
}

/* A subroutine of expand_one_var.  Called to assign rtl to a VAR_DECL
   that will reside in a hard register.  */

static void
expand_one_hard_reg_var (tree var)
{
  rest_of_decl_compilation (var, 0, 0);
}

/* A subroutine of expand_one_var.  Called to assign rtl to a VAR_DECL
   that will reside in a pseudo register.  */

static void
expand_one_register_var (tree var)
{
  tree type = TREE_TYPE (var);
  int unsignedp = TYPE_UNSIGNED (type);
  enum machine_mode reg_mode
    = promote_mode (type, DECL_MODE (var), &unsignedp, 0);
  rtx x = gen_reg_rtx (reg_mode);

  SET_DECL_RTL (var, x);

  /* Note if the object is a user variable.  */
  if (!DECL_ARTIFICIAL (var))
      mark_user_reg (x);

  if (POINTER_TYPE_P (type))
    mark_reg_pointer (x, TYPE_ALIGN (TREE_TYPE (TREE_TYPE (var))));
}

/* A subroutine of expand_one_var.  Called to assign rtl to a VAR_DECL that
   has some associated error, e.g. its type is error-mark.  We just need
   to pick something that won't crash the rest of the compiler.  */

static void
expand_one_error_var (tree var)
{
  enum machine_mode mode = DECL_MODE (var);
  rtx x;

  if (mode == BLKmode)
    x = gen_rtx_MEM (BLKmode, const0_rtx);
  else if (mode == VOIDmode)
    x = const0_rtx;
  else
    x = gen_reg_rtx (mode);

  SET_DECL_RTL (var, x);
}

/* A subroutine of expand_one_var.  VAR is a variable that will be
   allocated to the local stack frame.  Return true if we wish to
   add VAR to STACK_VARS so that it will be coalesced with other
   variables.  Return false to allocate VAR immediately.

   This function is used to reduce the number of variables considered
   for coalescing, which reduces the size of the quadratic problem.  */

static bool
defer_stack_allocation (tree var, bool toplevel)
{
  /* If stack protection is enabled, *all* stack variables must be deferred,
     so that we can re-order the strings to the top of the frame.  */
  if (flag_stack_protect)
    return true;

  /* Variables in the outermost scope automatically conflict with
     every other variable.  The only reason to want to defer them
     at all is that, after sorting, we can more efficiently pack
     small variables in the stack frame.  Continue to defer at -O2.  */
  if (toplevel && optimize < 2)
    return false;

  /* Without optimization, *most* variables are allocated from the
     stack, which makes the quadratic problem large exactly when we
     want compilation to proceed as quickly as possible.  On the
     other hand, we don't want the function's stack frame size to
     get completely out of hand.  So we avoid adding scalars and
     "small" aggregates to the list at all.  */
  if (optimize == 0 && tree_low_cst (DECL_SIZE_UNIT (var), 1) < 32)
    return false;

  return true;
}

/* A subroutine of expand_used_vars.  Expand one variable according to
   its flavor.  Variables to be placed on the stack are not actually
   expanded yet, merely recorded.  
   When REALLY_EXPAND is false, only add stack values to be allocated.
   Return stack usage this variable is supposed to take.
*/

static HOST_WIDE_INT
expand_one_var (tree var, bool toplevel, bool really_expand)
{
  if (SUPPORTS_STACK_ALIGNMENT
      && TREE_TYPE (var) != error_mark_node
      && TREE_CODE (var) == VAR_DECL)
    {
      unsigned int align;

      /* Because we don't know if VAR will be in register or on stack,
	 we conservatively assume it will be on stack even if VAR is
	 eventually put into register after RA pass.  For non-automatic
	 variables, which won't be on stack, we collect alignment of
	 type and ignore user specified alignment.  */
      if (TREE_STATIC (var) || DECL_EXTERNAL (var))
	align = MINIMUM_ALIGNMENT (TREE_TYPE (var),
				   TYPE_MODE (TREE_TYPE (var)),
				   TYPE_ALIGN (TREE_TYPE (var)));
      else
	align = MINIMUM_ALIGNMENT (var, DECL_MODE (var), DECL_ALIGN (var));

      if (crtl->stack_alignment_estimated < align)
        {
          /* stack_alignment_estimated shouldn't change after stack
             realign decision made */
          gcc_assert(!crtl->stack_realign_processed);
	  crtl->stack_alignment_estimated = align;
	}
    }

  if (TREE_CODE (var) != VAR_DECL)
    ;
  else if (DECL_EXTERNAL (var))
    ;
  else if (DECL_HAS_VALUE_EXPR_P (var))
    ;
  else if (TREE_STATIC (var))
    ;
  else if (DECL_RTL_SET_P (var))
    ;
  else if (TREE_TYPE (var) == error_mark_node)
    {
      if (really_expand)
        expand_one_error_var (var);
    }
  else if (DECL_HARD_REGISTER (var))
    {
      if (really_expand)
        expand_one_hard_reg_var (var);
    }
  else if (use_register_for_decl (var))
    {
      if (really_expand)
        expand_one_register_var (var);
    }
  else if (!host_integerp (DECL_SIZE_UNIT (var), 1))
    {
      if (really_expand)
	{
	  error ("size of variable %q+D is too large", var);
	  expand_one_error_var (var);
	}
    }
  else if (defer_stack_allocation (var, toplevel))
    add_stack_var (var);
  else
    {
      if (really_expand)
        expand_one_stack_var (var);
      return tree_low_cst (DECL_SIZE_UNIT (var), 1);
    }
  return 0;
}

/* A subroutine of expand_used_vars.  Walk down through the BLOCK tree
   expanding variables.  Those variables that can be put into registers
   are allocated pseudos; those that can't are put on the stack.

   TOPLEVEL is true if this is the outermost BLOCK.  */

static void
expand_used_vars_for_block (tree block, bool toplevel)
{
  size_t i, j, old_sv_num, this_sv_num, new_sv_num;
  tree t;

  old_sv_num = toplevel ? 0 : stack_vars_num;

  /* Expand all variables at this level.  */
  for (t = BLOCK_VARS (block); t ; t = TREE_CHAIN (t))
    if (TREE_USED (t))
      expand_one_var (t, toplevel, true);

  this_sv_num = stack_vars_num;

  /* Expand all variables at containing levels.  */
  for (t = BLOCK_SUBBLOCKS (block); t ; t = BLOCK_CHAIN (t))
    expand_used_vars_for_block (t, false);

  /* Since we do not track exact variable lifetimes (which is not even
     possible for variables whose address escapes), we mirror the block
     tree in the interference graph.  Here we cause all variables at this
     level, and all sublevels, to conflict.  Do make certain that a
     variable conflicts with itself.  */
  if (old_sv_num < this_sv_num)
    {
      new_sv_num = stack_vars_num;
      resize_stack_vars_conflict (new_sv_num);

      for (i = old_sv_num; i < new_sv_num; ++i)
	for (j = i < this_sv_num ? i+1 : this_sv_num; j-- > old_sv_num ;)
	  add_stack_var_conflict (i, j);
    }
}

/* A subroutine of expand_used_vars.  Walk down through the BLOCK tree
   and clear TREE_USED on all local variables.  */

static void
clear_tree_used (tree block)
{
  tree t;

  for (t = BLOCK_VARS (block); t ; t = TREE_CHAIN (t))
    /* if (!TREE_STATIC (t) && !DECL_EXTERNAL (t)) */
      TREE_USED (t) = 0;

  for (t = BLOCK_SUBBLOCKS (block); t ; t = BLOCK_CHAIN (t))
    clear_tree_used (t);
}

/* Examine TYPE and determine a bit mask of the following features.  */

#define SPCT_HAS_LARGE_CHAR_ARRAY	1
#define SPCT_HAS_SMALL_CHAR_ARRAY	2
#define SPCT_HAS_ARRAY			4
#define SPCT_HAS_AGGREGATE		8

static unsigned int
stack_protect_classify_type (tree type)
{
  unsigned int ret = 0;
  tree t;

  switch (TREE_CODE (type))
    {
    case ARRAY_TYPE:
      t = TYPE_MAIN_VARIANT (TREE_TYPE (type));
      if (t == char_type_node
	  || t == signed_char_type_node
	  || t == unsigned_char_type_node)
	{
	  unsigned HOST_WIDE_INT max = PARAM_VALUE (PARAM_SSP_BUFFER_SIZE);
	  unsigned HOST_WIDE_INT len;

	  if (!TYPE_SIZE_UNIT (type)
	      || !host_integerp (TYPE_SIZE_UNIT (type), 1))
	    len = max;
	  else
	    len = tree_low_cst (TYPE_SIZE_UNIT (type), 1);

	  if (len < max)
	    ret = SPCT_HAS_SMALL_CHAR_ARRAY | SPCT_HAS_ARRAY;
	  else
	    ret = SPCT_HAS_LARGE_CHAR_ARRAY | SPCT_HAS_ARRAY;
	}
      else
	ret = SPCT_HAS_ARRAY;
      break;

    case UNION_TYPE:
    case QUAL_UNION_TYPE:
    case RECORD_TYPE:
      ret = SPCT_HAS_AGGREGATE;
      for (t = TYPE_FIELDS (type); t ; t = TREE_CHAIN (t))
	if (TREE_CODE (t) == FIELD_DECL)
	  ret |= stack_protect_classify_type (TREE_TYPE (t));
      break;

    default:
      break;
    }

  return ret;
}

/* Return nonzero if DECL should be segregated into the "vulnerable" upper
   part of the local stack frame.  Remember if we ever return nonzero for
   any variable in this function.  The return value is the phase number in
   which the variable should be allocated.  */

static int
stack_protect_decl_phase (tree decl)
{
  unsigned int bits = stack_protect_classify_type (TREE_TYPE (decl));
  int ret = 0;

  if (bits & SPCT_HAS_SMALL_CHAR_ARRAY)
    has_short_buffer = true;

  if (flag_stack_protect == 2)
    {
      if ((bits & (SPCT_HAS_SMALL_CHAR_ARRAY | SPCT_HAS_LARGE_CHAR_ARRAY))
	  && !(bits & SPCT_HAS_AGGREGATE))
	ret = 1;
      else if (bits & SPCT_HAS_ARRAY)
	ret = 2;
    }
  else
    ret = (bits & SPCT_HAS_LARGE_CHAR_ARRAY) != 0;

  if (ret)
    has_protected_decls = true;

  return ret;
}

/* Two helper routines that check for phase 1 and phase 2.  These are used
   as callbacks for expand_stack_vars.  */

static bool
stack_protect_decl_phase_1 (tree decl)
{
  return stack_protect_decl_phase (decl) == 1;
}

static bool
stack_protect_decl_phase_2 (tree decl)
{
  return stack_protect_decl_phase (decl) == 2;
}

/* Ensure that variables in different stack protection phases conflict
   so that they are not merged and share the same stack slot.  */

static void
add_stack_protection_conflicts (void)
{
  size_t i, j, n = stack_vars_num;
  unsigned char *phase;

  phase = XNEWVEC (unsigned char, n);
  for (i = 0; i < n; ++i)
    phase[i] = stack_protect_decl_phase (stack_vars[i].decl);

  for (i = 0; i < n; ++i)
    {
      unsigned char ph_i = phase[i];
      for (j = 0; j < i; ++j)
	if (ph_i != phase[j])
	  add_stack_var_conflict (i, j);
    }

  XDELETEVEC (phase);
}

/* Create a decl for the guard at the top of the stack frame.  */

static void
create_stack_guard (void)
{
  tree guard = build_decl (VAR_DECL, NULL, ptr_type_node);
  TREE_THIS_VOLATILE (guard) = 1;
  TREE_USED (guard) = 1;
  expand_one_stack_var (guard);
  crtl->stack_protect_guard = guard;
}

/* A subroutine of expand_used_vars.  Walk down through the BLOCK tree
   expanding variables.  Those variables that can be put into registers
   are allocated pseudos; those that can't are put on the stack.

   TOPLEVEL is true if this is the outermost BLOCK.  */

static HOST_WIDE_INT
account_used_vars_for_block (tree block, bool toplevel)
{
  size_t i, j, old_sv_num, this_sv_num, new_sv_num;
  tree t;
  HOST_WIDE_INT size = 0;

  old_sv_num = toplevel ? 0 : stack_vars_num;

  /* Expand all variables at this level.  */
  for (t = BLOCK_VARS (block); t ; t = TREE_CHAIN (t))
    if (TREE_USED (t))
      size += expand_one_var (t, toplevel, false);

  this_sv_num = stack_vars_num;

  /* Expand all variables at containing levels.  */
  for (t = BLOCK_SUBBLOCKS (block); t ; t = BLOCK_CHAIN (t))
    size += account_used_vars_for_block (t, false);

  /* Since we do not track exact variable lifetimes (which is not even
     possible for variables whose address escapes), we mirror the block
     tree in the interference graph.  Here we cause all variables at this
     level, and all sublevels, to conflict.  Do make certain that a
     variable conflicts with itself.  */
  if (old_sv_num < this_sv_num)
    {
      new_sv_num = stack_vars_num;
      resize_stack_vars_conflict (new_sv_num);

      for (i = old_sv_num; i < new_sv_num; ++i)
	for (j = i < this_sv_num ? i+1 : this_sv_num; j-- > old_sv_num ;)
	  add_stack_var_conflict (i, j);
    }
  return size;
}

/* Prepare for expanding variables.  */
static void 
init_vars_expansion (void)
{
  tree t;
  /* Set TREE_USED on all variables in the local_decls.  */
  for (t = cfun->local_decls; t; t = TREE_CHAIN (t))
    TREE_USED (TREE_VALUE (t)) = 1;

  /* Clear TREE_USED on all variables associated with a block scope.  */
  clear_tree_used (DECL_INITIAL (current_function_decl));

  /* Initialize local stack smashing state.  */
  has_protected_decls = false;
  has_short_buffer = false;
}

/* Free up stack variable graph data.  */
static void
fini_vars_expansion (void)
{
  XDELETEVEC (stack_vars);
  XDELETEVEC (stack_vars_sorted);
  XDELETEVEC (stack_vars_conflict);
  stack_vars = NULL;
  stack_vars_alloc = stack_vars_num = 0;
  stack_vars_conflict = NULL;
  stack_vars_conflict_alloc = 0;
}

/* Make a fair guess for the size of the stack frame of the current
   function.  This doesn't have to be exact, the result is only used
   in the inline heuristics.  So we don't want to run the full stack
   var packing algorithm (which is quadratic in the number of stack
   vars).  Instead, we calculate the total size of all stack vars.
   This turns out to be a pretty fair estimate -- packing of stack
   vars doesn't happen very often.  */

HOST_WIDE_INT
estimated_stack_frame_size (void)
{
  HOST_WIDE_INT size = 0;
  size_t i;
  tree t, outer_block = DECL_INITIAL (current_function_decl);

  init_vars_expansion ();

  for (t = cfun->local_decls; t; t = TREE_CHAIN (t))
    {
      tree var = TREE_VALUE (t);

      if (TREE_USED (var))
        size += expand_one_var (var, true, false);
      TREE_USED (var) = 1;
    }
  size += account_used_vars_for_block (outer_block, true);

  if (stack_vars_num > 0)
    {
      /* Fake sorting the stack vars for account_stack_vars ().  */
      stack_vars_sorted = XNEWVEC (size_t, stack_vars_num);
      for (i = 0; i < stack_vars_num; ++i)
	stack_vars_sorted[i] = i;
      size += account_stack_vars ();
      fini_vars_expansion ();
    }

  return size;
}

/* Expand all variables used in the function.  */

static void
expand_used_vars (void)
{
  tree t, next, outer_block = DECL_INITIAL (current_function_decl);

  /* Compute the phase of the stack frame for this function.  */
  {
    int align = PREFERRED_STACK_BOUNDARY / BITS_PER_UNIT;
    int off = STARTING_FRAME_OFFSET % align;
    frame_phase = off ? align - off : 0;
  }

  init_vars_expansion ();

  /* At this point all variables on the local_decls with TREE_USED
     set are not associated with any block scope.  Lay them out.  */
  t = cfun->local_decls;
  cfun->local_decls = NULL_TREE;
  for (; t; t = next)
    {
      tree var = TREE_VALUE (t);
      bool expand_now = false;

      next = TREE_CHAIN (t);

      /* We didn't set a block for static or extern because it's hard
	 to tell the difference between a global variable (re)declared
	 in a local scope, and one that's really declared there to
	 begin with.  And it doesn't really matter much, since we're
	 not giving them stack space.  Expand them now.  */
      if (TREE_STATIC (var) || DECL_EXTERNAL (var))
	expand_now = true;

      /* Any variable that could have been hoisted into an SSA_NAME
	 will have been propagated anywhere the optimizers chose,
	 i.e. not confined to their original block.  Allocate them
	 as if they were defined in the outermost scope.  */
      else if (is_gimple_reg (var))
	expand_now = true;

      /* If the variable is not associated with any block, then it
	 was created by the optimizers, and could be live anywhere
	 in the function.  */
      else if (TREE_USED (var))
	expand_now = true;

      /* Finally, mark all variables on the list as used.  We'll use
	 this in a moment when we expand those associated with scopes.  */
      TREE_USED (var) = 1;

      if (expand_now)
	{
	  expand_one_var (var, true, true);
	  if (DECL_ARTIFICIAL (var) && !DECL_IGNORED_P (var))
	    {
	      rtx rtl = DECL_RTL_IF_SET (var);

	      /* Keep artificial non-ignored vars in cfun->local_decls
		 chain until instantiate_decls.  */
	      if (rtl && (MEM_P (rtl) || GET_CODE (rtl) == CONCAT))
		{
		  TREE_CHAIN (t) = cfun->local_decls;
		  cfun->local_decls = t;
		  continue;
		}
	    }
	}

      ggc_free (t);
    }

  /* At this point, all variables within the block tree with TREE_USED
     set are actually used by the optimized function.  Lay them out.  */
  expand_used_vars_for_block (outer_block, true);

  if (stack_vars_num > 0)
    {
      /* Due to the way alias sets work, no variables with non-conflicting
	 alias sets may be assigned the same address.  Add conflicts to
	 reflect this.  */
      add_alias_set_conflicts ();

      /* If stack protection is enabled, we don't share space between
	 vulnerable data and non-vulnerable data.  */
      if (flag_stack_protect)
	add_stack_protection_conflicts ();

      /* Now that we have collected all stack variables, and have computed a
	 minimal interference graph, attempt to save some stack space.  */
      partition_stack_vars ();
      if (dump_file)
	dump_stack_var_partition ();
    }

  /* There are several conditions under which we should create a
     stack guard: protect-all, alloca used, protected decls present.  */
  if (flag_stack_protect == 2
      || (flag_stack_protect
	  && (cfun->calls_alloca || has_protected_decls)))
    create_stack_guard ();

  /* Assign rtl to each variable based on these partitions.  */
  if (stack_vars_num > 0)
    {
      /* Reorder decls to be protected by iterating over the variables
	 array multiple times, and allocating out of each phase in turn.  */
      /* ??? We could probably integrate this into the qsort we did
	 earlier, such that we naturally see these variables first,
	 and thus naturally allocate things in the right order.  */
      if (has_protected_decls)
	{
	  /* Phase 1 contains only character arrays.  */
	  expand_stack_vars (stack_protect_decl_phase_1);

	  /* Phase 2 contains other kinds of arrays.  */
	  if (flag_stack_protect == 2)
	    expand_stack_vars (stack_protect_decl_phase_2);
	}

      expand_stack_vars (NULL);

      fini_vars_expansion ();
    }

  /* If the target requires that FRAME_OFFSET be aligned, do it.  */
  if (STACK_ALIGNMENT_NEEDED)
    {
      HOST_WIDE_INT align = PREFERRED_STACK_BOUNDARY / BITS_PER_UNIT;
      if (!FRAME_GROWS_DOWNWARD)
	frame_offset += align - 1;
      frame_offset &= -align;
    }
}


/* If we need to produce a detailed dump, print the tree representation
   for STMT to the dump file.  SINCE is the last RTX after which the RTL
   generated for STMT should have been appended.  */

static void
maybe_dump_rtl_for_gimple_stmt (gimple stmt, rtx since)
{
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "\n;; ");
      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
      fprintf (dump_file, "\n");

      print_rtl (dump_file, since ? NEXT_INSN (since) : since);
    }
}

/* Maps the blocks that do not contain tree labels to rtx labels.  */

static struct pointer_map_t *lab_rtx_for_bb;

/* Returns the label_rtx expression for a label starting basic block BB.  */

static rtx
label_rtx_for_bb (basic_block bb ATTRIBUTE_UNUSED)
{
  gimple_stmt_iterator gsi;
  tree lab;
  gimple lab_stmt;
  void **elt;

  if (bb->flags & BB_RTL)
    return block_label (bb);

  elt = pointer_map_contains (lab_rtx_for_bb, bb);
  if (elt)
    return (rtx) *elt;

  /* Find the tree label if it is present.  */
     
  for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      lab_stmt = gsi_stmt (gsi);
      if (gimple_code (lab_stmt) != GIMPLE_LABEL)
	break;

      lab = gimple_label_label (lab_stmt);
      if (DECL_NONLOCAL (lab))
	break;

      return label_rtx (lab);
    }

  elt = pointer_map_insert (lab_rtx_for_bb, bb);
  *elt = gen_label_rtx ();
  return (rtx) *elt;
}


/* A subroutine of expand_gimple_basic_block.  Expand one GIMPLE_COND.
   Returns a new basic block if we've terminated the current basic
   block and created a new one.  */

static basic_block
expand_gimple_cond (basic_block bb, gimple stmt)
{
  basic_block new_bb, dest;
  edge new_edge;
  edge true_edge;
  edge false_edge;
  tree pred = gimple_cond_pred_to_tree (stmt);
  rtx last2, last;

  last2 = last = get_last_insn ();

  extract_true_false_edges_from_block (bb, &true_edge, &false_edge);
  if (gimple_has_location (stmt))
    {
      set_curr_insn_source_location (gimple_location (stmt));
      set_curr_insn_block (gimple_block (stmt));
    }

  /* These flags have no purpose in RTL land.  */
  true_edge->flags &= ~EDGE_TRUE_VALUE;
  false_edge->flags &= ~EDGE_FALSE_VALUE;

  /* We can either have a pure conditional jump with one fallthru edge or
     two-way jump that needs to be decomposed into two basic blocks.  */
  if (false_edge->dest == bb->next_bb)
    {
      jumpif (pred, label_rtx_for_bb (true_edge->dest));
      add_reg_br_prob_note (last, true_edge->probability);
      maybe_dump_rtl_for_gimple_stmt (stmt, last);
      if (true_edge->goto_locus)
	{
	  set_curr_insn_source_location (true_edge->goto_locus);
	  set_curr_insn_block (true_edge->goto_block);
	  true_edge->goto_locus = curr_insn_locator ();
	}
      true_edge->goto_block = NULL;
      false_edge->flags |= EDGE_FALLTHRU;
      ggc_free (pred);
      return NULL;
    }
  if (true_edge->dest == bb->next_bb)
    {
      jumpifnot (pred, label_rtx_for_bb (false_edge->dest));
      add_reg_br_prob_note (last, false_edge->probability);
      maybe_dump_rtl_for_gimple_stmt (stmt, last);
      if (false_edge->goto_locus)
	{
	  set_curr_insn_source_location (false_edge->goto_locus);
	  set_curr_insn_block (false_edge->goto_block);
	  false_edge->goto_locus = curr_insn_locator ();
	}
      false_edge->goto_block = NULL;
      true_edge->flags |= EDGE_FALLTHRU;
      ggc_free (pred);
      return NULL;
    }

  jumpif (pred, label_rtx_for_bb (true_edge->dest));
  add_reg_br_prob_note (last, true_edge->probability);
  last = get_last_insn ();
  if (false_edge->goto_locus)
    {
      set_curr_insn_source_location (false_edge->goto_locus);
      set_curr_insn_block (false_edge->goto_block);
      false_edge->goto_locus = curr_insn_locator ();
    }
  false_edge->goto_block = NULL;
  emit_jump (label_rtx_for_bb (false_edge->dest));

  BB_END (bb) = last;
  if (BARRIER_P (BB_END (bb)))
    BB_END (bb) = PREV_INSN (BB_END (bb));
  update_bb_for_insn (bb);

  new_bb = create_basic_block (NEXT_INSN (last), get_last_insn (), bb);
  dest = false_edge->dest;
  redirect_edge_succ (false_edge, new_bb);
  false_edge->flags |= EDGE_FALLTHRU;
  new_bb->count = false_edge->count;
  new_bb->frequency = EDGE_FREQUENCY (false_edge);
  new_edge = make_edge (new_bb, dest, 0);
  new_edge->probability = REG_BR_PROB_BASE;
  new_edge->count = new_bb->count;
  if (BARRIER_P (BB_END (new_bb)))
    BB_END (new_bb) = PREV_INSN (BB_END (new_bb));
  update_bb_for_insn (new_bb);

  maybe_dump_rtl_for_gimple_stmt (stmt, last2);

  if (true_edge->goto_locus)
    {
      set_curr_insn_source_location (true_edge->goto_locus);
      set_curr_insn_block (true_edge->goto_block);
      true_edge->goto_locus = curr_insn_locator ();
    }
  true_edge->goto_block = NULL;

  ggc_free (pred);
  return new_bb;
}

/* A subroutine of expand_gimple_basic_block.  Expand one GIMPLE_CALL
   that has CALL_EXPR_TAILCALL set.  Returns non-null if we actually
   generated a tail call (something that might be denied by the ABI
   rules governing the call; see calls.c).

   Sets CAN_FALLTHRU if we generated a *conditional* tail call, and
   can still reach the rest of BB.  The case here is __builtin_sqrt,
   where the NaN result goes through the external function (with a
   tailcall) and the normal result happens via a sqrt instruction.  */

static basic_block
expand_gimple_tailcall (basic_block bb, gimple stmt, bool *can_fallthru)
{
  rtx last2, last;
  edge e;
  edge_iterator ei;
  int probability;
  gcov_type count;
  tree stmt_tree = gimple_to_tree (stmt);

  last2 = last = get_last_insn ();

  expand_expr_stmt (stmt_tree);

  release_stmt_tree (stmt, stmt_tree);

  for (last = NEXT_INSN (last); last; last = NEXT_INSN (last))
    if (CALL_P (last) && SIBLING_CALL_P (last))
      goto found;

  maybe_dump_rtl_for_gimple_stmt (stmt, last2);

  *can_fallthru = true;
  return NULL;

 found:
  /* ??? Wouldn't it be better to just reset any pending stack adjust?
     Any instructions emitted here are about to be deleted.  */
  do_pending_stack_adjust ();

  /* Remove any non-eh, non-abnormal edges that don't go to exit.  */
  /* ??? I.e. the fallthrough edge.  HOWEVER!  If there were to be
     EH or abnormal edges, we shouldn't have created a tail call in
     the first place.  So it seems to me we should just be removing
     all edges here, or redirecting the existing fallthru edge to
     the exit block.  */

  probability = 0;
  count = 0;

  for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei)); )
    {
      if (!(e->flags & (EDGE_ABNORMAL | EDGE_EH)))
	{
	  if (e->dest != EXIT_BLOCK_PTR)
	    {
	      e->dest->count -= e->count;
	      e->dest->frequency -= EDGE_FREQUENCY (e);
	      if (e->dest->count < 0)
		e->dest->count = 0;
	      if (e->dest->frequency < 0)
		e->dest->frequency = 0;
	    }
	  count += e->count;
	  probability += e->probability;
	  remove_edge (e);
	}
      else
	ei_next (&ei);
    }

  /* This is somewhat ugly: the call_expr expander often emits instructions
     after the sibcall (to perform the function return).  These confuse the
     find_many_sub_basic_blocks code, so we need to get rid of these.  */
  last = NEXT_INSN (last);
  gcc_assert (BARRIER_P (last));

  *can_fallthru = false;
  while (NEXT_INSN (last))
    {
      /* For instance an sqrt builtin expander expands if with
	 sibcall in the then and label for `else`.  */
      if (LABEL_P (NEXT_INSN (last)))
	{
	  *can_fallthru = true;
	  break;
	}
      delete_insn (NEXT_INSN (last));
    }

  e = make_edge (bb, EXIT_BLOCK_PTR, EDGE_ABNORMAL | EDGE_SIBCALL);
  e->probability += probability;
  e->count += count;
  BB_END (bb) = last;
  update_bb_for_insn (bb);

  if (NEXT_INSN (last))
    {
      bb = create_basic_block (NEXT_INSN (last), get_last_insn (), bb);

      last = BB_END (bb);
      if (BARRIER_P (last))
	BB_END (bb) = PREV_INSN (last);
    }

  maybe_dump_rtl_for_gimple_stmt (stmt, last2);

  return bb;
}

/* Expand basic block BB from GIMPLE trees to RTL.  */

static basic_block
expand_gimple_basic_block (basic_block bb)
{
  gimple_stmt_iterator gsi;
  gimple_seq stmts;
  gimple stmt = NULL;
  rtx note, last;
  edge e;
  edge_iterator ei;
  void **elt;

  if (dump_file)
    fprintf (dump_file, "\n;; Generating RTL for gimple basic block %d\n",
	     bb->index);

  /* Note that since we are now transitioning from GIMPLE to RTL, we
     cannot use the gsi_*_bb() routines because they expect the basic
     block to be in GIMPLE, instead of RTL.  Therefore, we need to
     access the BB sequence directly.  */
  stmts = bb_seq (bb);
  bb->il.gimple = NULL;
  rtl_profile_for_bb (bb);
  init_rtl_bb_info (bb);
  bb->flags |= BB_RTL;

  /* Remove the RETURN_EXPR if we may fall though to the exit
     instead.  */
  gsi = gsi_last (stmts);
  if (!gsi_end_p (gsi)
      && gimple_code (gsi_stmt (gsi)) == GIMPLE_RETURN)
    {
      gimple ret_stmt = gsi_stmt (gsi);

      gcc_assert (single_succ_p (bb));
      gcc_assert (single_succ (bb) == EXIT_BLOCK_PTR);

      if (bb->next_bb == EXIT_BLOCK_PTR
	  && !gimple_return_retval (ret_stmt))
	{
	  gsi_remove (&gsi, false);
	  single_succ_edge (bb)->flags |= EDGE_FALLTHRU;
	}
    }

  gsi = gsi_start (stmts);
  if (!gsi_end_p (gsi))
    {
      stmt = gsi_stmt (gsi);
      if (gimple_code (stmt) != GIMPLE_LABEL)
	stmt = NULL;
    }

  elt = pointer_map_contains (lab_rtx_for_bb, bb);

  if (stmt || elt)
    {
      last = get_last_insn ();

      if (stmt)
	{
	  tree stmt_tree = gimple_to_tree (stmt);
	  expand_expr_stmt (stmt_tree);
	  release_stmt_tree (stmt, stmt_tree);
	  gsi_next (&gsi);
	}

      if (elt)
	emit_label ((rtx) *elt);

      /* Java emits line number notes in the top of labels.
	 ??? Make this go away once line number notes are obsoleted.  */
      BB_HEAD (bb) = NEXT_INSN (last);
      if (NOTE_P (BB_HEAD (bb)))
	BB_HEAD (bb) = NEXT_INSN (BB_HEAD (bb));
      note = emit_note_after (NOTE_INSN_BASIC_BLOCK, BB_HEAD (bb));

      maybe_dump_rtl_for_gimple_stmt (stmt, last);
    }
  else
    note = BB_HEAD (bb) = emit_note (NOTE_INSN_BASIC_BLOCK);

  NOTE_BASIC_BLOCK (note) = bb;

  for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei)); )
    {
      /* Clear EDGE_EXECUTABLE.  This flag is never used in the backend.  */
      e->flags &= ~EDGE_EXECUTABLE;

      /* At the moment not all abnormal edges match the RTL representation.
	 It is safe to remove them here as find_many_sub_basic_blocks will
	 rediscover them.  In the future we should get this fixed properly.  */
      if (e->flags & EDGE_ABNORMAL)
	remove_edge (e);
      else
	ei_next (&ei);
    }

  for (; !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gimple stmt = gsi_stmt (gsi);
      basic_block new_bb;

      /* Expand this statement, then evaluate the resulting RTL and
	 fixup the CFG accordingly.  */
      if (gimple_code (stmt) == GIMPLE_COND)
	{
	  new_bb = expand_gimple_cond (bb, stmt);
	  if (new_bb)
	    return new_bb;
	}
      else
	{
	  if (is_gimple_call (stmt) && gimple_call_tail_p (stmt))
	    {
	      bool can_fallthru;
	      new_bb = expand_gimple_tailcall (bb, stmt, &can_fallthru);
	      if (new_bb)
		{
		  if (can_fallthru)
		    bb = new_bb;
		  else
		    return new_bb;
		}
	    }
	  else if (gimple_code (stmt) != GIMPLE_CHANGE_DYNAMIC_TYPE)
	    {
	      tree stmt_tree = gimple_to_tree (stmt);
	      last = get_last_insn ();
	      expand_expr_stmt (stmt_tree);
	      maybe_dump_rtl_for_gimple_stmt (stmt, last);
	      release_stmt_tree (stmt, stmt_tree);
	    }
	}
    }

  /* Expand implicit goto and convert goto_locus.  */
  FOR_EACH_EDGE (e, ei, bb->succs)
    {
      if (e->goto_locus && e->goto_block)
	{
	  set_curr_insn_source_location (e->goto_locus);
	  set_curr_insn_block (e->goto_block);
	  e->goto_locus = curr_insn_locator ();
	}
      e->goto_block = NULL;
      if ((e->flags & EDGE_FALLTHRU) && e->dest != bb->next_bb)
	{
	  emit_jump (label_rtx_for_bb (e->dest));
	  e->flags &= ~EDGE_FALLTHRU;
	}
    }

  do_pending_stack_adjust ();

  /* Find the block tail.  The last insn in the block is the insn
     before a barrier and/or table jump insn.  */
  last = get_last_insn ();
  if (BARRIER_P (last))
    last = PREV_INSN (last);
  if (JUMP_TABLE_DATA_P (last))
    last = PREV_INSN (PREV_INSN (last));
  BB_END (bb) = last;

  update_bb_for_insn (bb);

  return bb;
}


/* Create a basic block for initialization code.  */

static basic_block
construct_init_block (void)
{
  basic_block init_block, first_block;
  edge e = NULL;
  int flags;

  /* Multiple entry points not supported yet.  */
  gcc_assert (EDGE_COUNT (ENTRY_BLOCK_PTR->succs) == 1);
  init_rtl_bb_info (ENTRY_BLOCK_PTR);
  init_rtl_bb_info (EXIT_BLOCK_PTR);
  ENTRY_BLOCK_PTR->flags |= BB_RTL;
  EXIT_BLOCK_PTR->flags |= BB_RTL;

  e = EDGE_SUCC (ENTRY_BLOCK_PTR, 0);

  /* When entry edge points to first basic block, we don't need jump,
     otherwise we have to jump into proper target.  */
  if (e && e->dest != ENTRY_BLOCK_PTR->next_bb)
    {
      tree label = gimple_block_label (e->dest);

      emit_jump (label_rtx (label));
      flags = 0;
    }
  else
    flags = EDGE_FALLTHRU;

  init_block = create_basic_block (NEXT_INSN (get_insns ()),
				   get_last_insn (),
				   ENTRY_BLOCK_PTR);
  init_block->frequency = ENTRY_BLOCK_PTR->frequency;
  init_block->count = ENTRY_BLOCK_PTR->count;
  if (e)
    {
      first_block = e->dest;
      redirect_edge_succ (e, init_block);
      e = make_edge (init_block, first_block, flags);
    }
  else
    e = make_edge (init_block, EXIT_BLOCK_PTR, EDGE_FALLTHRU);
  e->probability = REG_BR_PROB_BASE;
  e->count = ENTRY_BLOCK_PTR->count;

  update_bb_for_insn (init_block);
  return init_block;
}

/* For each lexical block, set BLOCK_NUMBER to the depth at which it is
   found in the block tree.  */

static void
set_block_levels (tree block, int level)
{
  while (block)
    {
      BLOCK_NUMBER (block) = level;
      set_block_levels (BLOCK_SUBBLOCKS (block), level + 1);
      block = BLOCK_CHAIN (block);
    }
}

/* Create a block containing landing pads and similar stuff.  */

static void
construct_exit_block (void)
{
  rtx head = get_last_insn ();
  rtx end;
  basic_block exit_block;
  edge e, e2;
  unsigned ix;
  edge_iterator ei;
  rtx orig_end = BB_END (EXIT_BLOCK_PTR->prev_bb);

  rtl_profile_for_bb (EXIT_BLOCK_PTR);

  /* Make sure the locus is set to the end of the function, so that
     epilogue line numbers and warnings are set properly.  */
  if (cfun->function_end_locus != UNKNOWN_LOCATION)
    input_location = cfun->function_end_locus;

  /* The following insns belong to the top scope.  */
  set_curr_insn_block (DECL_INITIAL (current_function_decl));

  /* Generate rtl for function exit.  */
  expand_function_end ();

  end = get_last_insn ();
  if (head == end)
    return;
  /* While emitting the function end we could move end of the last basic block.
   */
  BB_END (EXIT_BLOCK_PTR->prev_bb) = orig_end;
  while (NEXT_INSN (head) && NOTE_P (NEXT_INSN (head)))
    head = NEXT_INSN (head);
  exit_block = create_basic_block (NEXT_INSN (head), end,
				   EXIT_BLOCK_PTR->prev_bb);
  exit_block->frequency = EXIT_BLOCK_PTR->frequency;
  exit_block->count = EXIT_BLOCK_PTR->count;

  ix = 0;
  while (ix < EDGE_COUNT (EXIT_BLOCK_PTR->preds))
    {
      e = EDGE_PRED (EXIT_BLOCK_PTR, ix);
      if (!(e->flags & EDGE_ABNORMAL))
	redirect_edge_succ (e, exit_block);
      else
	ix++;
    }

  e = make_edge (exit_block, EXIT_BLOCK_PTR, EDGE_FALLTHRU);
  e->probability = REG_BR_PROB_BASE;
  e->count = EXIT_BLOCK_PTR->count;
  FOR_EACH_EDGE (e2, ei, EXIT_BLOCK_PTR->preds)
    if (e2 != e)
      {
	e->count -= e2->count;
	exit_block->count -= e2->count;
	exit_block->frequency -= EDGE_FREQUENCY (e2);
      }
  if (e->count < 0)
    e->count = 0;
  if (exit_block->count < 0)
    exit_block->count = 0;
  if (exit_block->frequency < 0)
    exit_block->frequency = 0;
  update_bb_for_insn (exit_block);
}

/* Helper function for discover_nonconstant_array_refs.
   Look for ARRAY_REF nodes with non-constant indexes and mark them
   addressable.  */

static tree
discover_nonconstant_array_refs_r (tree * tp, int *walk_subtrees,
				   void *data ATTRIBUTE_UNUSED)
{
  tree t = *tp;

  if (IS_TYPE_OR_DECL_P (t))
    *walk_subtrees = 0;
  else if (TREE_CODE (t) == ARRAY_REF || TREE_CODE (t) == ARRAY_RANGE_REF)
    {
      while (((TREE_CODE (t) == ARRAY_REF || TREE_CODE (t) == ARRAY_RANGE_REF)
	      && is_gimple_min_invariant (TREE_OPERAND (t, 1))
	      && (!TREE_OPERAND (t, 2)
		  || is_gimple_min_invariant (TREE_OPERAND (t, 2))))
	     || (TREE_CODE (t) == COMPONENT_REF
		 && (!TREE_OPERAND (t,2)
		     || is_gimple_min_invariant (TREE_OPERAND (t, 2))))
	     || TREE_CODE (t) == BIT_FIELD_REF
	     || TREE_CODE (t) == REALPART_EXPR
	     || TREE_CODE (t) == IMAGPART_EXPR
	     || TREE_CODE (t) == VIEW_CONVERT_EXPR
	     || CONVERT_EXPR_P (t))
	t = TREE_OPERAND (t, 0);

      if (TREE_CODE (t) == ARRAY_REF || TREE_CODE (t) == ARRAY_RANGE_REF)
	{
	  t = get_base_address (t);
	  if (t && DECL_P (t))
	    TREE_ADDRESSABLE (t) = 1;
	}

      *walk_subtrees = 0;
    }

  return NULL_TREE;
}

/* RTL expansion is not able to compile array references with variable
   offsets for arrays stored in single register.  Discover such
   expressions and mark variables as addressable to avoid this
   scenario.  */

static void
discover_nonconstant_array_refs (void)
{
  basic_block bb;
  gimple_stmt_iterator gsi;

  FOR_EACH_BB (bb)
    for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
      {
	gimple stmt = gsi_stmt (gsi);
	walk_gimple_op (stmt, discover_nonconstant_array_refs_r, NULL);
      }
}

/* This function sets crtl->args.internal_arg_pointer to a virtual
   register if DRAP is needed.  Local register allocator will replace
   virtual_incoming_args_rtx with the virtual register.  */

static void
expand_stack_alignment (void)
{
  rtx drap_rtx;
  unsigned int preferred_stack_boundary;

  if (! SUPPORTS_STACK_ALIGNMENT)
    return;
  
  if (cfun->calls_alloca
      || cfun->has_nonlocal_label
      || crtl->has_nonlocal_goto)
    crtl->need_drap = true;

  gcc_assert (crtl->stack_alignment_needed
	      <= crtl->stack_alignment_estimated);

  /* Update crtl->stack_alignment_estimated and use it later to align
     stack.  We check PREFERRED_STACK_BOUNDARY if there may be non-call
     exceptions since callgraph doesn't collect incoming stack alignment
     in this case.  */
  if (flag_non_call_exceptions
      && PREFERRED_STACK_BOUNDARY > crtl->preferred_stack_boundary)
    preferred_stack_boundary = PREFERRED_STACK_BOUNDARY;
  else
    preferred_stack_boundary = crtl->preferred_stack_boundary;
  if (preferred_stack_boundary > crtl->stack_alignment_estimated)
    crtl->stack_alignment_estimated = preferred_stack_boundary;
  if (preferred_stack_boundary > crtl->stack_alignment_needed)
    crtl->stack_alignment_needed = preferred_stack_boundary;

  crtl->stack_realign_needed
    = INCOMING_STACK_BOUNDARY < crtl->stack_alignment_estimated;
  crtl->stack_realign_tried = crtl->stack_realign_needed;

  crtl->stack_realign_processed = true;

  /* Target has to redefine TARGET_GET_DRAP_RTX to support stack
     alignment.  */
  gcc_assert (targetm.calls.get_drap_rtx != NULL);
  drap_rtx = targetm.calls.get_drap_rtx (); 

  /* stack_realign_drap and drap_rtx must match.  */
  gcc_assert ((stack_realign_drap != 0) == (drap_rtx != NULL));

  /* Do nothing if NULL is returned, which means DRAP is not needed.  */
  if (NULL != drap_rtx)
    {
      crtl->args.internal_arg_pointer = drap_rtx;

      /* Call fixup_tail_calls to clean up REG_EQUIV note if DRAP is
         needed. */
      fixup_tail_calls ();
    }
}

/* Translate the intermediate representation contained in the CFG
   from GIMPLE trees to RTL.

   We do conversion per basic block and preserve/update the tree CFG.
   This implies we have to do some magic as the CFG can simultaneously
   consist of basic blocks containing RTL and GIMPLE trees.  This can
   confuse the CFG hooks, so be careful to not manipulate CFG during
   the expansion.  */

static unsigned int
gimple_expand_cfg (void)
{
  basic_block bb, init_block;
  sbitmap blocks;
  edge_iterator ei;
  edge e;

  /* Some backends want to know that we are expanding to RTL.  */
  currently_expanding_to_rtl = 1;

  rtl_profile_for_bb (ENTRY_BLOCK_PTR);

  insn_locators_alloc ();
  if (!DECL_BUILT_IN (current_function_decl))
    {
      /* Eventually, all FEs should explicitly set function_start_locus.  */
      if (cfun->function_start_locus == UNKNOWN_LOCATION)
       set_curr_insn_source_location
         (DECL_SOURCE_LOCATION (current_function_decl));
      else
       set_curr_insn_source_location (cfun->function_start_locus);
    }
  set_curr_insn_block (DECL_INITIAL (current_function_decl));
  prologue_locator = curr_insn_locator ();

  /* Make sure first insn is a note even if we don't want linenums.
     This makes sure the first insn will never be deleted.
     Also, final expects a note to appear there.  */
  emit_note (NOTE_INSN_DELETED);

  /* Mark arrays indexed with non-constant indices with TREE_ADDRESSABLE.  */
  discover_nonconstant_array_refs ();

  targetm.expand_to_rtl_hook ();
  crtl->stack_alignment_needed = STACK_BOUNDARY;
  crtl->max_used_stack_slot_alignment = STACK_BOUNDARY;
  crtl->stack_alignment_estimated = STACK_BOUNDARY;
  crtl->preferred_stack_boundary = STACK_BOUNDARY;
  cfun->cfg->max_jumptable_ents = 0;


  /* Expand the variables recorded during gimple lowering.  */
  expand_used_vars ();

  /* Honor stack protection warnings.  */
  if (warn_stack_protect)
    {
      if (cfun->calls_alloca)
	warning (OPT_Wstack_protector, 
		 "not protecting local variables: variable length buffer");
      if (has_short_buffer && !crtl->stack_protect_guard)
	warning (OPT_Wstack_protector, 
		 "not protecting function: no buffer at least %d bytes long",
		 (int) PARAM_VALUE (PARAM_SSP_BUFFER_SIZE));
    }

  /* Set up parameters and prepare for return, for the function.  */
  expand_function_start (current_function_decl);

  /* If this function is `main', emit a call to `__main'
     to run global initializers, etc.  */
  if (DECL_NAME (current_function_decl)
      && MAIN_NAME_P (DECL_NAME (current_function_decl))
      && DECL_FILE_SCOPE_P (current_function_decl))
    expand_main_function ();

  /* Initialize the stack_protect_guard field.  This must happen after the
     call to __main (if any) so that the external decl is initialized.  */
  if (crtl->stack_protect_guard)
    stack_protect_prologue ();

  /* Update stack boundary if needed.  */
  if (SUPPORTS_STACK_ALIGNMENT)
    {
      /* Call update_stack_boundary here to update incoming stack
	 boundary before TARGET_FUNCTION_OK_FOR_SIBCALL is called.
	 TARGET_FUNCTION_OK_FOR_SIBCALL needs to know the accurate
	 incoming stack alignment to check if it is OK to perform
	 sibcall optimization since sibcall optimization will only
	 align the outgoing stack to incoming stack boundary.  */
      if (targetm.calls.update_stack_boundary)
	targetm.calls.update_stack_boundary ();
      
      /* The incoming stack frame has to be aligned at least at
	 parm_stack_boundary.  */
      gcc_assert (crtl->parm_stack_boundary <= INCOMING_STACK_BOUNDARY);
    }

  /* Register rtl specific functions for cfg.  */
  rtl_register_cfg_hooks ();

  init_block = construct_init_block ();

  /* Clear EDGE_EXECUTABLE on the entry edge(s).  It is cleaned from the
     remaining edges in expand_gimple_basic_block.  */
  FOR_EACH_EDGE (e, ei, ENTRY_BLOCK_PTR->succs)
    e->flags &= ~EDGE_EXECUTABLE;

  lab_rtx_for_bb = pointer_map_create ();
  FOR_BB_BETWEEN (bb, init_block->next_bb, EXIT_BLOCK_PTR, next_bb)
    bb = expand_gimple_basic_block (bb);

  /* Expansion is used by optimization passes too, set maybe_hot_insn_p
     conservatively to true until they are all profile aware.  */
  pointer_map_destroy (lab_rtx_for_bb);
  free_histograms ();

  construct_exit_block ();
  set_curr_insn_block (DECL_INITIAL (current_function_decl));
  insn_locators_finalize ();

  /* We're done expanding trees to RTL.  */
  currently_expanding_to_rtl = 0;

  /* Convert tree EH labels to RTL EH labels and zap the tree EH table.  */
  convert_from_eh_region_ranges ();
  set_eh_throw_stmt_table (cfun, NULL);

  rebuild_jump_labels (get_insns ());
  find_exception_handler_labels ();

  blocks = sbitmap_alloc (last_basic_block);
  sbitmap_ones (blocks);
  find_many_sub_basic_blocks (blocks);
  purge_all_dead_edges ();
  sbitmap_free (blocks);

  compact_blocks ();

  expand_stack_alignment ();

#ifdef ENABLE_CHECKING
  verify_flow_info ();
#endif

  /* There's no need to defer outputting this function any more; we
     know we want to output it.  */
  DECL_DEFER_OUTPUT (current_function_decl) = 0;

  /* Now that we're done expanding trees to RTL, we shouldn't have any
     more CONCATs anywhere.  */
  generating_concat_p = 0;

  if (dump_file)
    {
      fprintf (dump_file,
	       "\n\n;;\n;; Full RTL generated for this function:\n;;\n");
      /* And the pass manager will dump RTL for us.  */
    }

  /* If we're emitting a nested function, make sure its parent gets
     emitted as well.  Doing otherwise confuses debug info.  */
  {
    tree parent;
    for (parent = DECL_CONTEXT (current_function_decl);
	 parent != NULL_TREE;
	 parent = get_containing_scope (parent))
      if (TREE_CODE (parent) == FUNCTION_DECL)
	TREE_SYMBOL_REFERENCED (DECL_ASSEMBLER_NAME (parent)) = 1;
  }

  /* We are now committed to emitting code for this function.  Do any
     preparation, such as emitting abstract debug info for the inline
     before it gets mangled by optimization.  */
  if (cgraph_function_possibly_inlined_p (current_function_decl))
    (*debug_hooks->outlining_inline_function) (current_function_decl);

  TREE_ASM_WRITTEN (current_function_decl) = 1;

  /* After expanding, the return labels are no longer needed. */
  return_label = NULL;
  naked_return_label = NULL;
  /* Tag the blocks with a depth number so that change_scope can find
     the common parent easily.  */
  set_block_levels (DECL_INITIAL (cfun->decl), 0);
  default_rtl_profile ();
  return 0;
}

struct rtl_opt_pass pass_expand =
{
 {
  RTL_PASS,
  "expand",				/* name */
  NULL,                                 /* gate */
  gimple_expand_cfg,			/* execute */
  NULL,                                 /* sub */
  NULL,                                 /* next */
  0,                                    /* static_pass_number */
  TV_EXPAND,				/* tv_id */
  /* ??? If TER is enabled, we actually receive GENERIC.  */
  PROP_gimple_leh | PROP_cfg,           /* properties_required */
  PROP_rtl,                             /* properties_provided */
  PROP_trees,				/* properties_destroyed */
  0,                                    /* todo_flags_start */
  TODO_dump_func,                       /* todo_flags_finish */
 }
};
