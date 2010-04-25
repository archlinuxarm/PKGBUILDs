/* Generic SSA value propagation engine.
   Copyright (C) 2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
   Contributed by Diego Novillo <dnovillo@redhat.com>

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "flags.h"
#include "rtl.h"
#include "tm_p.h"
#include "ggc.h"
#include "basic-block.h"
#include "output.h"
#include "expr.h"
#include "function.h"
#include "diagnostic.h"
#include "timevar.h"
#include "tree-dump.h"
#include "tree-flow.h"
#include "tree-pass.h"
#include "tree-ssa-propagate.h"
#include "langhooks.h"
#include "varray.h"
#include "vec.h"
#include "value-prof.h"
#include "gimple.h"

/* This file implements a generic value propagation engine based on
   the same propagation used by the SSA-CCP algorithm [1].

   Propagation is performed by simulating the execution of every
   statement that produces the value being propagated.  Simulation
   proceeds as follows:

   1- Initially, all edges of the CFG are marked not executable and
      the CFG worklist is seeded with all the statements in the entry
      basic block (block 0).

   2- Every statement S is simulated with a call to the call-back
      function SSA_PROP_VISIT_STMT.  This evaluation may produce 3
      results:

      	SSA_PROP_NOT_INTERESTING: Statement S produces nothing of
	    interest and does not affect any of the work lists.

	SSA_PROP_VARYING: The value produced by S cannot be determined
	    at compile time.  Further simulation of S is not required.
	    If S is a conditional jump, all the outgoing edges for the
	    block are considered executable and added to the work
	    list.

	SSA_PROP_INTERESTING: S produces a value that can be computed
	    at compile time.  Its result can be propagated into the
	    statements that feed from S.  Furthermore, if S is a
	    conditional jump, only the edge known to be taken is added
	    to the work list.  Edges that are known not to execute are
	    never simulated.

   3- PHI nodes are simulated with a call to SSA_PROP_VISIT_PHI.  The
      return value from SSA_PROP_VISIT_PHI has the same semantics as
      described in #2.

   4- Three work lists are kept.  Statements are only added to these
      lists if they produce one of SSA_PROP_INTERESTING or
      SSA_PROP_VARYING.

   	CFG_BLOCKS contains the list of blocks to be simulated.
	    Blocks are added to this list if their incoming edges are
	    found executable.

	VARYING_SSA_EDGES contains the list of statements that feed
	    from statements that produce an SSA_PROP_VARYING result.
	    These are simulated first to speed up processing.

	INTERESTING_SSA_EDGES contains the list of statements that
	    feed from statements that produce an SSA_PROP_INTERESTING
	    result.

   5- Simulation terminates when all three work lists are drained.

   Before calling ssa_propagate, it is important to clear
   prop_simulate_again_p for all the statements in the program that
   should be simulated.  This initialization allows an implementation
   to specify which statements should never be simulated.

   It is also important to compute def-use information before calling
   ssa_propagate.

   References:

     [1] Constant propagation with conditional branches,
         Wegman and Zadeck, ACM TOPLAS 13(2):181-210.

     [2] Building an Optimizing Compiler,
	 Robert Morgan, Butterworth-Heinemann, 1998, Section 8.9.

     [3] Advanced Compiler Design and Implementation,
	 Steven Muchnick, Morgan Kaufmann, 1997, Section 12.6  */

/* Function pointers used to parameterize the propagation engine.  */
static ssa_prop_visit_stmt_fn ssa_prop_visit_stmt;
static ssa_prop_visit_phi_fn ssa_prop_visit_phi;

/* Keep track of statements that have been added to one of the SSA
   edges worklists.  This flag is used to avoid visiting statements
   unnecessarily when draining an SSA edge worklist.  If while
   simulating a basic block, we find a statement with
   STMT_IN_SSA_EDGE_WORKLIST set, we clear it to prevent SSA edge
   processing from visiting it again.

   NOTE: users of the propagation engine are not allowed to use
   the GF_PLF_1 flag.  */
#define STMT_IN_SSA_EDGE_WORKLIST	GF_PLF_1

/* A bitmap to keep track of executable blocks in the CFG.  */
static sbitmap executable_blocks;

/* Array of control flow edges on the worklist.  */
static VEC(basic_block,heap) *cfg_blocks;

static unsigned int cfg_blocks_num = 0;
static int cfg_blocks_tail;
static int cfg_blocks_head;

static sbitmap bb_in_list;

/* Worklist of SSA edges which will need reexamination as their
   definition has changed.  SSA edges are def-use edges in the SSA
   web.  For each D-U edge, we store the target statement or PHI node
   U.  */
static GTY(()) VEC(gimple,gc) *interesting_ssa_edges;

/* Identical to INTERESTING_SSA_EDGES.  For performance reasons, the
   list of SSA edges is split into two.  One contains all SSA edges
   who need to be reexamined because their lattice value changed to
   varying (this worklist), and the other contains all other SSA edges
   to be reexamined (INTERESTING_SSA_EDGES).

   Since most values in the program are VARYING, the ideal situation
   is to move them to that lattice value as quickly as possible.
   Thus, it doesn't make sense to process any other type of lattice
   value until all VARYING values are propagated fully, which is one
   thing using the VARYING worklist achieves.  In addition, if we
   don't use a separate worklist for VARYING edges, we end up with
   situations where lattice values move from
   UNDEFINED->INTERESTING->VARYING instead of UNDEFINED->VARYING.  */
static GTY(()) VEC(gimple,gc) *varying_ssa_edges;


/* Return true if the block worklist empty.  */

static inline bool
cfg_blocks_empty_p (void)
{
  return (cfg_blocks_num == 0);
}


/* Add a basic block to the worklist.  The block must not be already
   in the worklist, and it must not be the ENTRY or EXIT block.  */

static void 
cfg_blocks_add (basic_block bb)
{
  bool head = false;

  gcc_assert (bb != ENTRY_BLOCK_PTR && bb != EXIT_BLOCK_PTR);
  gcc_assert (!TEST_BIT (bb_in_list, bb->index));

  if (cfg_blocks_empty_p ())
    {
      cfg_blocks_tail = cfg_blocks_head = 0;
      cfg_blocks_num = 1;
    }
  else
    {
      cfg_blocks_num++;
      if (cfg_blocks_num > VEC_length (basic_block, cfg_blocks))
	{
	  /* We have to grow the array now.  Adjust to queue to occupy
	     the full space of the original array.  We do not need to
	     initialize the newly allocated portion of the array
	     because we keep track of CFG_BLOCKS_HEAD and
	     CFG_BLOCKS_HEAD.  */
	  cfg_blocks_tail = VEC_length (basic_block, cfg_blocks);
	  cfg_blocks_head = 0;
	  VEC_safe_grow (basic_block, heap, cfg_blocks, 2 * cfg_blocks_tail);
	}
      /* Minor optimization: we prefer to see blocks with more
	 predecessors later, because there is more of a chance that
	 the incoming edges will be executable.  */
      else if (EDGE_COUNT (bb->preds)
	       >= EDGE_COUNT (VEC_index (basic_block, cfg_blocks,
					 cfg_blocks_head)->preds))
	cfg_blocks_tail = ((cfg_blocks_tail + 1)
			   % VEC_length (basic_block, cfg_blocks));
      else
	{
	  if (cfg_blocks_head == 0)
	    cfg_blocks_head = VEC_length (basic_block, cfg_blocks);
	  --cfg_blocks_head;
	  head = true;
	}
    }

  VEC_replace (basic_block, cfg_blocks,
	       head ? cfg_blocks_head : cfg_blocks_tail,
	       bb);
  SET_BIT (bb_in_list, bb->index);
}


/* Remove a block from the worklist.  */

static basic_block
cfg_blocks_get (void)
{
  basic_block bb;

  bb = VEC_index (basic_block, cfg_blocks, cfg_blocks_head);

  gcc_assert (!cfg_blocks_empty_p ());
  gcc_assert (bb);

  cfg_blocks_head = ((cfg_blocks_head + 1)
		     % VEC_length (basic_block, cfg_blocks));
  --cfg_blocks_num;
  RESET_BIT (bb_in_list, bb->index);

  return bb;
}


/* We have just defined a new value for VAR.  If IS_VARYING is true,
   add all immediate uses of VAR to VARYING_SSA_EDGES, otherwise add
   them to INTERESTING_SSA_EDGES.  */

static void
add_ssa_edge (tree var, bool is_varying)
{
  imm_use_iterator iter;
  use_operand_p use_p;

  FOR_EACH_IMM_USE_FAST (use_p, iter, var)
    {
      gimple use_stmt = USE_STMT (use_p);

      if (prop_simulate_again_p (use_stmt)
	  && !gimple_plf (use_stmt, STMT_IN_SSA_EDGE_WORKLIST))
	{
	  gimple_set_plf (use_stmt, STMT_IN_SSA_EDGE_WORKLIST, true);
	  if (is_varying)
	    VEC_safe_push (gimple, gc, varying_ssa_edges, use_stmt);
	  else
	    VEC_safe_push (gimple, gc, interesting_ssa_edges, use_stmt);
	}
    }
}


/* Add edge E to the control flow worklist.  */

static void
add_control_edge (edge e)
{
  basic_block bb = e->dest;
  if (bb == EXIT_BLOCK_PTR)
    return;

  /* If the edge had already been executed, skip it.  */
  if (e->flags & EDGE_EXECUTABLE)
    return;

  e->flags |= EDGE_EXECUTABLE;

  /* If the block is already in the list, we're done.  */
  if (TEST_BIT (bb_in_list, bb->index))
    return;

  cfg_blocks_add (bb);

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "Adding Destination of edge (%d -> %d) to worklist\n\n",
	e->src->index, e->dest->index);
}


/* Simulate the execution of STMT and update the work lists accordingly.  */

static void
simulate_stmt (gimple stmt)
{
  enum ssa_prop_result val = SSA_PROP_NOT_INTERESTING;
  edge taken_edge = NULL;
  tree output_name = NULL_TREE;

  /* Don't bother visiting statements that are already
     considered varying by the propagator.  */
  if (!prop_simulate_again_p (stmt))
    return;

  if (gimple_code (stmt) == GIMPLE_PHI)
    {
      val = ssa_prop_visit_phi (stmt);
      output_name = gimple_phi_result (stmt);
    }
  else
    val = ssa_prop_visit_stmt (stmt, &taken_edge, &output_name);

  if (val == SSA_PROP_VARYING)
    {
      prop_set_simulate_again (stmt, false);

      /* If the statement produced a new varying value, add the SSA
	 edges coming out of OUTPUT_NAME.  */
      if (output_name)
	add_ssa_edge (output_name, true);

      /* If STMT transfers control out of its basic block, add
	 all outgoing edges to the work list.  */
      if (stmt_ends_bb_p (stmt))
	{
	  edge e;
	  edge_iterator ei;
	  basic_block bb = gimple_bb (stmt);
	  FOR_EACH_EDGE (e, ei, bb->succs)
	    add_control_edge (e);
	}
    }
  else if (val == SSA_PROP_INTERESTING)
    {
      /* If the statement produced new value, add the SSA edges coming
	 out of OUTPUT_NAME.  */
      if (output_name)
	add_ssa_edge (output_name, false);

      /* If we know which edge is going to be taken out of this block,
	 add it to the CFG work list.  */
      if (taken_edge)
	add_control_edge (taken_edge);
    }
}

/* Process an SSA edge worklist.  WORKLIST is the SSA edge worklist to
   drain.  This pops statements off the given WORKLIST and processes
   them until there are no more statements on WORKLIST.
   We take a pointer to WORKLIST because it may be reallocated when an
   SSA edge is added to it in simulate_stmt.  */

static void
process_ssa_edge_worklist (VEC(gimple,gc) **worklist)
{
  /* Drain the entire worklist.  */
  while (VEC_length (gimple, *worklist) > 0)
    {
      basic_block bb;

      /* Pull the statement to simulate off the worklist.  */
      gimple stmt = VEC_pop (gimple, *worklist);

      /* If this statement was already visited by simulate_block, then
	 we don't need to visit it again here.  */
      if (!gimple_plf (stmt, STMT_IN_SSA_EDGE_WORKLIST))
	continue;

      /* STMT is no longer in a worklist.  */
      gimple_set_plf (stmt, STMT_IN_SSA_EDGE_WORKLIST, false);

      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "\nSimulating statement (from ssa_edges): ");
	  print_gimple_stmt (dump_file, stmt, 0, dump_flags);
	}

      bb = gimple_bb (stmt);

      /* PHI nodes are always visited, regardless of whether or not
	 the destination block is executable.  Otherwise, visit the
	 statement only if its block is marked executable.  */
      if (gimple_code (stmt) == GIMPLE_PHI
	  || TEST_BIT (executable_blocks, bb->index))
	simulate_stmt (stmt);
    }
}


/* Simulate the execution of BLOCK.  Evaluate the statement associated
   with each variable reference inside the block.  */

static void
simulate_block (basic_block block)
{
  gimple_stmt_iterator gsi;

  /* There is nothing to do for the exit block.  */
  if (block == EXIT_BLOCK_PTR)
    return;

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "\nSimulating block %d\n", block->index);

  /* Always simulate PHI nodes, even if we have simulated this block
     before.  */
  for (gsi = gsi_start_phis (block); !gsi_end_p (gsi); gsi_next (&gsi))
    simulate_stmt (gsi_stmt (gsi));

  /* If this is the first time we've simulated this block, then we
     must simulate each of its statements.  */
  if (!TEST_BIT (executable_blocks, block->index))
    {
      gimple_stmt_iterator j;
      unsigned int normal_edge_count;
      edge e, normal_edge;
      edge_iterator ei;

      /* Note that we have simulated this block.  */
      SET_BIT (executable_blocks, block->index);

      for (j = gsi_start_bb (block); !gsi_end_p (j); gsi_next (&j))
	{
	  gimple stmt = gsi_stmt (j);

	  /* If this statement is already in the worklist then
	     "cancel" it.  The reevaluation implied by the worklist
	     entry will produce the same value we generate here and
	     thus reevaluating it again from the worklist is
	     pointless.  */
	  if (gimple_plf (stmt, STMT_IN_SSA_EDGE_WORKLIST))
	    gimple_set_plf (stmt, STMT_IN_SSA_EDGE_WORKLIST, false);

	  simulate_stmt (stmt);
	}

      /* We can not predict when abnormal edges will be executed, so
	 once a block is considered executable, we consider any
	 outgoing abnormal edges as executable.

	 At the same time, if this block has only one successor that is
	 reached by non-abnormal edges, then add that successor to the
	 worklist.  */
      normal_edge_count = 0;
      normal_edge = NULL;
      FOR_EACH_EDGE (e, ei, block->succs)
	{
	  if (e->flags & EDGE_ABNORMAL)
	    add_control_edge (e);
	  else
	    {
	      normal_edge_count++;
	      normal_edge = e;
	    }
	}

      if (normal_edge_count == 1)
	add_control_edge (normal_edge);
    }
}


/* Initialize local data structures and work lists.  */

static void
ssa_prop_init (void)
{
  edge e;
  edge_iterator ei;
  basic_block bb;
  size_t i;

  /* Worklists of SSA edges.  */
  interesting_ssa_edges = VEC_alloc (gimple, gc, 20);
  varying_ssa_edges = VEC_alloc (gimple, gc, 20);

  executable_blocks = sbitmap_alloc (last_basic_block);
  sbitmap_zero (executable_blocks);

  bb_in_list = sbitmap_alloc (last_basic_block);
  sbitmap_zero (bb_in_list);

  if (dump_file && (dump_flags & TDF_DETAILS))
    dump_immediate_uses (dump_file);

  cfg_blocks = VEC_alloc (basic_block, heap, 20);
  VEC_safe_grow (basic_block, heap, cfg_blocks, 20);

  /* Initialize the values for every SSA_NAME.  */
  for (i = 1; i < num_ssa_names; i++)
    if (ssa_name (i))
      SSA_NAME_VALUE (ssa_name (i)) = NULL_TREE;

  /* Initially assume that every edge in the CFG is not executable.
     (including the edges coming out of ENTRY_BLOCK_PTR).  */
  FOR_ALL_BB (bb)
    {
      gimple_stmt_iterator si;

      for (si = gsi_start_bb (bb); !gsi_end_p (si); gsi_next (&si))
	gimple_set_plf (gsi_stmt (si), STMT_IN_SSA_EDGE_WORKLIST, false);
    
      for (si = gsi_start_phis (bb); !gsi_end_p (si); gsi_next (&si))
	gimple_set_plf (gsi_stmt (si), STMT_IN_SSA_EDGE_WORKLIST, false);

      FOR_EACH_EDGE (e, ei, bb->succs)
	e->flags &= ~EDGE_EXECUTABLE;
    }

  /* Seed the algorithm by adding the successors of the entry block to the
     edge worklist.  */
  FOR_EACH_EDGE (e, ei, ENTRY_BLOCK_PTR->succs)
    add_control_edge (e);
}


/* Free allocated storage.  */

static void
ssa_prop_fini (void)
{
  VEC_free (gimple, gc, interesting_ssa_edges);
  VEC_free (gimple, gc, varying_ssa_edges);
  VEC_free (basic_block, heap, cfg_blocks);
  cfg_blocks = NULL;
  sbitmap_free (bb_in_list);
  sbitmap_free (executable_blocks);
}


/* Return true if EXPR is an acceptable right-hand-side for a
   GIMPLE assignment.  We validate the entire tree, not just
   the root node, thus catching expressions that embed complex
   operands that are not permitted in GIMPLE.  This function
   is needed because the folding routines in fold-const.c
   may return such expressions in some cases, e.g., an array
   access with an embedded index addition.  It may make more
   sense to have folding routines that are sensitive to the
   constraints on GIMPLE operands, rather than abandoning any
   any attempt to fold if the usual folding turns out to be too
   aggressive.  */

bool
valid_gimple_rhs_p (tree expr)
{
  enum tree_code code = TREE_CODE (expr);

  switch (TREE_CODE_CLASS (code))
    {
    case tcc_declaration:
      if (!is_gimple_variable (expr))
	return false;
      break;

    case tcc_constant:
      /* All constants are ok.  */
      break;

    case tcc_binary:
    case tcc_comparison:
      if (!is_gimple_val (TREE_OPERAND (expr, 0))
	  || !is_gimple_val (TREE_OPERAND (expr, 1)))
	return false;
      break;

    case tcc_unary:
      if (!is_gimple_val (TREE_OPERAND (expr, 0)))
	return false;
      break;

    case tcc_expression:
      switch (code)
        {
        case ADDR_EXPR:
          {
	    tree t;
	    if (is_gimple_min_invariant (expr))
	      return true;
            t = TREE_OPERAND (expr, 0);
            while (handled_component_p (t))
              {
                /* ??? More checks needed, see the GIMPLE verifier.  */
                if ((TREE_CODE (t) == ARRAY_REF
                     || TREE_CODE (t) == ARRAY_RANGE_REF)
                    && !is_gimple_val (TREE_OPERAND (t, 1)))
                  return false;
                t = TREE_OPERAND (t, 0);
              }
            if (!is_gimple_id (t))
              return false;
          }
          break;

	case TRUTH_NOT_EXPR:
	  if (!is_gimple_val (TREE_OPERAND (expr, 0)))
	    return false;
	  break;

	case TRUTH_AND_EXPR:
	case TRUTH_XOR_EXPR:
	case TRUTH_OR_EXPR:
	  if (!is_gimple_val (TREE_OPERAND (expr, 0))
	      || !is_gimple_val (TREE_OPERAND (expr, 1)))
	    return false;
	  break;

	case EXC_PTR_EXPR:
	case FILTER_EXPR:
	  break;

	default:
	  return false;
	}
      break;

    case tcc_vl_exp:
      return false;

    case tcc_exceptional:
      if (code != SSA_NAME)
        return false;
      break;

    default:
      return false;
    }

  return true;
}


/* Return true if EXPR is a CALL_EXPR suitable for representation
   as a single GIMPLE_CALL statement.  If the arguments require
   further gimplification, return false.  */

bool
valid_gimple_call_p (tree expr)
{
  unsigned i, nargs;

  if (TREE_CODE (expr) != CALL_EXPR)
    return false;

  nargs = call_expr_nargs (expr);
  for (i = 0; i < nargs; i++)
    if (! is_gimple_operand (CALL_EXPR_ARG (expr, i)))
      return false;

  return true;
}


/* Make SSA names defined by OLD_STMT point to NEW_STMT
   as their defining statement.  */

void
move_ssa_defining_stmt_for_defs (gimple new_stmt, gimple old_stmt)
{
  tree var;
  ssa_op_iter iter;

  if (gimple_in_ssa_p (cfun))
    {
      /* Make defined SSA_NAMEs point to the new
         statement as their definition.  */
      FOR_EACH_SSA_TREE_OPERAND (var, old_stmt, iter, SSA_OP_ALL_DEFS)
        {
          if (TREE_CODE (var) == SSA_NAME)
            SSA_NAME_DEF_STMT (var) = new_stmt;
        }
    }
}


/* Update a GIMPLE_CALL statement at iterator *SI_P to reflect the
   value of EXPR, which is expected to be the result of folding the
   call.  This can only be done if EXPR is a CALL_EXPR with valid
   GIMPLE operands as arguments, or if it is a suitable RHS expression
   for a GIMPLE_ASSIGN.  More complex expressions will require
   gimplification, which will introduce addtional statements.  In this
   event, no update is performed, and the function returns false.
   Note that we cannot mutate a GIMPLE_CALL in-place, so we always
   replace the statement at *SI_P with an entirely new statement.
   The new statement need not be a call, e.g., if the original call
   folded to a constant.  */

bool
update_call_from_tree (gimple_stmt_iterator *si_p, tree expr)
{
  tree lhs;

  gimple stmt = gsi_stmt (*si_p);

  gcc_assert (is_gimple_call (stmt));

  lhs = gimple_call_lhs (stmt);

  if (valid_gimple_call_p (expr))
    {
      /* The call has simplified to another call.  */
      tree fn = CALL_EXPR_FN (expr);
      unsigned i;
      unsigned nargs = call_expr_nargs (expr);
      VEC(tree, heap) *args = NULL;
      gimple new_stmt;

      if (nargs > 0)
        {
          args = VEC_alloc (tree, heap, nargs);
          VEC_safe_grow (tree, heap, args, nargs);
      
          for (i = 0; i < nargs; i++)
            VEC_replace (tree, args, i, CALL_EXPR_ARG (expr, i));
        }

      new_stmt = gimple_build_call_vec (fn, args);
      gimple_call_set_lhs (new_stmt, lhs);
      copy_virtual_operands (new_stmt, stmt);
      move_ssa_defining_stmt_for_defs (new_stmt, stmt);
      gimple_set_location (new_stmt, gimple_location (stmt));
      gsi_replace (si_p, new_stmt, false);
      VEC_free (tree, heap, args);

      return true;
    }
  else if (valid_gimple_rhs_p (expr))
    {
      gimple new_stmt;

      /* The call has simplified to an expression
         that cannot be represented as a GIMPLE_CALL. */
      if (lhs)
        {
          /* A value is expected.
             Introduce a new GIMPLE_ASSIGN statement.  */
          STRIP_USELESS_TYPE_CONVERSION (expr);
          new_stmt = gimple_build_assign (lhs, expr);
          copy_virtual_operands (new_stmt, stmt);
          move_ssa_defining_stmt_for_defs (new_stmt, stmt);
        }
      else if (!TREE_SIDE_EFFECTS (expr))
        {
          /* No value is expected, and EXPR has no effect.
             Replace it with an empty statement.  */
          new_stmt = gimple_build_nop ();
        }
      else
        {
          /* No value is expected, but EXPR has an effect,
             e.g., it could be a reference to a volatile
             variable.  Create an assignment statement
             with a dummy (unused) lhs variable.  */
          STRIP_USELESS_TYPE_CONVERSION (expr);
          lhs = create_tmp_var (TREE_TYPE (expr), NULL);
          new_stmt = gimple_build_assign (lhs, expr);
          add_referenced_var (lhs);
          lhs = make_ssa_name (lhs, new_stmt);
          gimple_assign_set_lhs (new_stmt, lhs);
          copy_virtual_operands (new_stmt, stmt);
          move_ssa_defining_stmt_for_defs (new_stmt, stmt);
        }
      gimple_set_location (new_stmt, gimple_location (stmt));
      gsi_replace (si_p, new_stmt, false);
      return true;
    }
  else
    /* The call simplified to an expression that is
       not a valid GIMPLE RHS.  */
    return false;
}


/* Entry point to the propagation engine.

   VISIT_STMT is called for every statement visited.
   VISIT_PHI is called for every PHI node visited.  */

void
ssa_propagate (ssa_prop_visit_stmt_fn visit_stmt,
	       ssa_prop_visit_phi_fn visit_phi)
{
  ssa_prop_visit_stmt = visit_stmt;
  ssa_prop_visit_phi = visit_phi;

  ssa_prop_init ();

  /* Iterate until the worklists are empty.  */
  while (!cfg_blocks_empty_p () 
	 || VEC_length (gimple, interesting_ssa_edges) > 0
	 || VEC_length (gimple, varying_ssa_edges) > 0)
    {
      if (!cfg_blocks_empty_p ())
	{
	  /* Pull the next block to simulate off the worklist.  */
	  basic_block dest_block = cfg_blocks_get ();
	  simulate_block (dest_block);
	}

      /* In order to move things to varying as quickly as
	 possible,process the VARYING_SSA_EDGES worklist first.  */
      process_ssa_edge_worklist (&varying_ssa_edges);

      /* Now process the INTERESTING_SSA_EDGES worklist.  */
      process_ssa_edge_worklist (&interesting_ssa_edges);
    }

  ssa_prop_fini ();
}


/* Return true if STMT is of the form 'LHS = mem_ref', where 'mem_ref'
   is a non-volatile pointer dereference, a structure reference or a
   reference to a single _DECL.  Ignore volatile memory references
   because they are not interesting for the optimizers.  */

bool
stmt_makes_single_load (gimple stmt)
{
  tree rhs;

  if (gimple_code (stmt) != GIMPLE_ASSIGN)
    return false;

  /* Only a GIMPLE_SINGLE_RHS assignment may have a
     declaration or reference as its RHS.  */
  if (get_gimple_rhs_class (gimple_assign_rhs_code (stmt))
      != GIMPLE_SINGLE_RHS)
    return false;

  if (ZERO_SSA_OPERANDS (stmt, SSA_OP_VDEF|SSA_OP_VUSE))
    return false;

  rhs = gimple_assign_rhs1 (stmt);

  return (!TREE_THIS_VOLATILE (rhs)
	  && (DECL_P (rhs)
	      || REFERENCE_CLASS_P (rhs)));
}


/* Return true if STMT is of the form 'mem_ref = RHS', where 'mem_ref'
   is a non-volatile pointer dereference, a structure reference or a
   reference to a single _DECL.  Ignore volatile memory references
   because they are not interesting for the optimizers.  */

bool
stmt_makes_single_store (gimple stmt)
{
  tree lhs;

  if (gimple_code (stmt) != GIMPLE_ASSIGN
      && gimple_code (stmt) != GIMPLE_CALL)
    return false;

  if (ZERO_SSA_OPERANDS (stmt, SSA_OP_VDEF))
    return false;

  lhs = gimple_get_lhs (stmt);

  /* A call statement may have a null LHS.  */
  if (!lhs)
    return false;

  return (!TREE_THIS_VOLATILE (lhs)
          && (DECL_P (lhs)
	      || REFERENCE_CLASS_P (lhs)));
}


/* Propagation statistics.  */
struct prop_stats_d
{
  long num_const_prop;
  long num_copy_prop;
  long num_pred_folded;
  long num_dce;
};

static struct prop_stats_d prop_stats;

/* Replace USE references in statement STMT with the values stored in
   PROP_VALUE. Return true if at least one reference was replaced.  */

static bool
replace_uses_in (gimple stmt, prop_value_t *prop_value)
{
  bool replaced = false;
  use_operand_p use;
  ssa_op_iter iter;

  FOR_EACH_SSA_USE_OPERAND (use, stmt, iter, SSA_OP_USE)
    {
      tree tuse = USE_FROM_PTR (use);
      tree val = prop_value[SSA_NAME_VERSION (tuse)].value;

      if (val == tuse || val == NULL_TREE)
	continue;

      if (gimple_code (stmt) == GIMPLE_ASM
	  && !may_propagate_copy_into_asm (tuse))
	continue;

      if (!may_propagate_copy (tuse, val))
	continue;

      if (TREE_CODE (val) != SSA_NAME)
	prop_stats.num_const_prop++;
      else
	prop_stats.num_copy_prop++;

      propagate_value (use, val);

      replaced = true;
    }

  return replaced;
}


/* Replace propagated values into all the arguments for PHI using the
   values from PROP_VALUE.  */

static void
replace_phi_args_in (gimple phi, prop_value_t *prop_value)
{
  size_t i;
  bool replaced = false;

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Folding PHI node: ");
      print_gimple_stmt (dump_file, phi, 0, TDF_SLIM);
    }

  for (i = 0; i < gimple_phi_num_args (phi); i++)
    {
      tree arg = gimple_phi_arg_def (phi, i);

      if (TREE_CODE (arg) == SSA_NAME)
	{
	  tree val = prop_value[SSA_NAME_VERSION (arg)].value;

	  if (val && val != arg && may_propagate_copy (arg, val))
	    {
	      if (TREE_CODE (val) != SSA_NAME)
		prop_stats.num_const_prop++;
	      else
		prop_stats.num_copy_prop++;

	      propagate_value (PHI_ARG_DEF_PTR (phi, i), val);
	      replaced = true;

	      /* If we propagated a copy and this argument flows
		 through an abnormal edge, update the replacement
		 accordingly.  */
	      if (TREE_CODE (val) == SSA_NAME
		  && gimple_phi_arg_edge (phi, i)->flags & EDGE_ABNORMAL)
		SSA_NAME_OCCURS_IN_ABNORMAL_PHI (val) = 1;
	    }
	}
    }
  
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      if (!replaced)
	fprintf (dump_file, "No folding possible\n");
      else
	{
	  fprintf (dump_file, "Folded into: ");
	  print_gimple_stmt (dump_file, phi, 0, TDF_SLIM);
	  fprintf (dump_file, "\n");
	}
    }
}


/* If the statement pointed by SI has a predicate whose value can be
   computed using the value range information computed by VRP, compute
   its value and return true.  Otherwise, return false.  */

static bool
fold_predicate_in (gimple_stmt_iterator *si)
{
  bool assignment_p = false;
  tree val;
  gimple stmt = gsi_stmt (*si);

  if (is_gimple_assign (stmt)
      && TREE_CODE_CLASS (gimple_assign_rhs_code (stmt)) == tcc_comparison)
    {
      assignment_p = true;
      val = vrp_evaluate_conditional (gimple_assign_rhs_code (stmt),
				      gimple_assign_rhs1 (stmt),
				      gimple_assign_rhs2 (stmt),
				      stmt);
    }
  else if (gimple_code (stmt) == GIMPLE_COND)
    val = vrp_evaluate_conditional (gimple_cond_code (stmt),
				    gimple_cond_lhs (stmt),
				    gimple_cond_rhs (stmt),
				    stmt);
  else
    return false;


  if (val)
    {
      if (assignment_p)
        val = fold_convert (gimple_expr_type (stmt), val);
      
      if (dump_file)
	{
	  fprintf (dump_file, "Folding predicate ");
	  print_gimple_expr (dump_file, stmt, 0, 0);
	  fprintf (dump_file, " to ");
	  print_generic_expr (dump_file, val, 0);
	  fprintf (dump_file, "\n");
	}

      prop_stats.num_pred_folded++;

      if (is_gimple_assign (stmt))
	gimple_assign_set_rhs_from_tree (si, val);
      else
	{
	  gcc_assert (gimple_code (stmt) == GIMPLE_COND);
	  if (integer_zerop (val))
	    gimple_cond_make_false (stmt);
	  else if (integer_onep (val))
	    gimple_cond_make_true (stmt);
	  else
	    gcc_unreachable ();
	}

      return true;
    }

  return false;
}


/* Perform final substitution and folding of propagated values.

   PROP_VALUE[I] contains the single value that should be substituted
   at every use of SSA name N_I.  If PROP_VALUE is NULL, no values are
   substituted.

   If USE_RANGES_P is true, statements that contain predicate
   expressions are evaluated with a call to vrp_evaluate_conditional.
   This will only give meaningful results when called from tree-vrp.c
   (the information used by vrp_evaluate_conditional is built by the
   VRP pass).  

   Return TRUE when something changed.  */

bool
substitute_and_fold (prop_value_t *prop_value, bool use_ranges_p)
{
  basic_block bb;
  bool something_changed = false;

  if (prop_value == NULL && !use_ranges_p)
    return false;

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "\nSubstituting values and folding statements\n\n");

  memset (&prop_stats, 0, sizeof (prop_stats));

  /* Substitute values in every statement of every basic block.  */
  FOR_EACH_BB (bb)
    {
      gimple_stmt_iterator i;

      /* Propagate known values into PHI nodes.  */
      if (prop_value)
	for (i = gsi_start_phis (bb); !gsi_end_p (i); gsi_next (&i))
	  replace_phi_args_in (gsi_stmt (i), prop_value);

      /* Propagate known values into stmts.  Do a backward walk to expose
	 more trivially deletable stmts.  */
      for (i = gsi_last_bb (bb); !gsi_end_p (i);)
	{
          bool did_replace;
	  gimple stmt = gsi_stmt (i);
	  gimple old_stmt;
	  enum gimple_code code = gimple_code (stmt);

	  /* Ignore ASSERT_EXPRs.  They are used by VRP to generate
	     range information for names and they are discarded
	     afterwards.  */

	  if (code == GIMPLE_ASSIGN
	      && TREE_CODE (gimple_assign_rhs1 (stmt)) == ASSERT_EXPR)
	    {
	      gsi_prev (&i);
	      continue;
	    }

	  /* No point propagating into a stmt whose result is not used,
	     but instead we might be able to remove a trivially dead stmt.  */
	  if (gimple_get_lhs (stmt)
	      && TREE_CODE (gimple_get_lhs (stmt)) == SSA_NAME
	      && has_zero_uses (gimple_get_lhs (stmt))
	      && !stmt_could_throw_p (stmt)
	      && !gimple_has_side_effects (stmt))
	    {
	      gimple_stmt_iterator i2;

	      if (dump_file && dump_flags & TDF_DETAILS)
		{
		  fprintf (dump_file, "Removing dead stmt ");
		  print_gimple_stmt (dump_file, stmt, 0, 0);
		  fprintf (dump_file, "\n");
		}
	      prop_stats.num_dce++;
	      gsi_prev (&i);
	      i2 = gsi_for_stmt (stmt);
	      gsi_remove (&i2, true);
	      release_defs (stmt);
	      continue;
	    }

	  /* Record the state of the statement before replacements.  */
	  push_stmt_changes (gsi_stmt_ptr (&i));

	  /* Replace the statement with its folded version and mark it
	     folded.  */
	  did_replace = false;
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    {
	      fprintf (dump_file, "Folding statement: ");
	      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
	    }

	  /* If we have range information, see if we can fold
	     predicate expressions.  */
	  if (use_ranges_p)
	    {
	      did_replace = fold_predicate_in (&i);
	      /* fold_predicate_in should not have reallocated STMT.  */
	      gcc_assert (gsi_stmt (i) == stmt);
	    }

	  /* Only replace real uses if we couldn't fold the
	     statement using value range information.  */
	  if (prop_value
	      && !did_replace)
	    did_replace |= replace_uses_in (stmt, prop_value);

	  /* If we made a replacement, fold the statement.  */

	  old_stmt = stmt;
	  if (did_replace)
	    fold_stmt (&i);

	  /* Some statements may be simplified using ranges.  For
	     example, division may be replaced by shifts, modulo
	     replaced with bitwise and, etc.   Do this after 
	     substituting constants, folding, etc so that we're
	     presented with a fully propagated, canonicalized
	     statement.  */
	  if (use_ranges_p)
	    did_replace |= simplify_stmt_using_ranges (&i);

	  /* Now cleanup.  */
	  if (did_replace)
	    {
	      stmt = gsi_stmt (i);

              /* If we cleaned up EH information from the statement,
                 remove EH edges.  */
	      if (maybe_clean_or_replace_eh_stmt (old_stmt, stmt))
		gimple_purge_dead_eh_edges (bb);

              if (is_gimple_assign (stmt)
                  && (get_gimple_rhs_class (gimple_assign_rhs_code (stmt))
                      == GIMPLE_SINGLE_RHS))
              {
                tree rhs = gimple_assign_rhs1 (stmt);
                
                if (TREE_CODE (rhs) == ADDR_EXPR)
                  recompute_tree_invariant_for_addr_expr (rhs);
              }

	      /* Determine what needs to be done to update the SSA form.  */
	      pop_stmt_changes (gsi_stmt_ptr (&i));
	      something_changed = true;
	    }
	  else
	    {
	      /* The statement was not modified, discard the change buffer.  */
	      discard_stmt_changes (gsi_stmt_ptr (&i));
	    }

	  if (dump_file && (dump_flags & TDF_DETAILS))
	    {
	      if (did_replace)
		{
		  fprintf (dump_file, "Folded into: ");
		  print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
		  fprintf (dump_file, "\n");
		}
	      else
		fprintf (dump_file, "Not folded\n");
	    }

	  gsi_prev (&i);
	}
    }

  statistics_counter_event (cfun, "Constants propagated",
			    prop_stats.num_const_prop);
  statistics_counter_event (cfun, "Copies propagated",
			    prop_stats.num_copy_prop);
  statistics_counter_event (cfun, "Predicates folded",
			    prop_stats.num_pred_folded);
  statistics_counter_event (cfun, "Statements deleted",
			    prop_stats.num_dce);
  return something_changed;
}

#include "gt-tree-ssa-propagate.h"
