/* SSA Dominator optimizations for trees
   Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008
   Free Software Foundation, Inc.
   Contributed by Diego Novillo <dnovillo@redhat.com>

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
#include "flags.h"
#include "rtl.h"
#include "tm_p.h"
#include "ggc.h"
#include "basic-block.h"
#include "cfgloop.h"
#include "output.h"
#include "expr.h"
#include "function.h"
#include "diagnostic.h"
#include "timevar.h"
#include "tree-dump.h"
#include "tree-flow.h"
#include "domwalk.h"
#include "real.h"
#include "tree-pass.h"
#include "tree-ssa-propagate.h"
#include "langhooks.h"
#include "params.h"

/* This file implements optimizations on the dominator tree.  */

/* Representation of a "naked" right-hand-side expression, to be used
   in recording available expressions in the expression hash table.  */

enum expr_kind
{
  EXPR_SINGLE,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_CALL
};

struct hashable_expr
{
  tree type;
  enum expr_kind kind;
  union {
    struct { tree rhs; } single;
    struct { enum tree_code op;  tree opnd; } unary;
    struct { enum tree_code op;  tree opnd0; tree opnd1; } binary;
    struct { tree fn; bool pure; size_t nargs; tree *args; } call;
  } ops;
};

/* Structure for recording known values of a conditional expression
   at the exits from its block.  */

struct cond_equivalence
{
  struct hashable_expr cond;
  tree value;
};

/* Structure for recording edge equivalences as well as any pending
   edge redirections during the dominator optimizer.

   Computing and storing the edge equivalences instead of creating
   them on-demand can save significant amounts of time, particularly
   for pathological cases involving switch statements.  

   These structures live for a single iteration of the dominator
   optimizer in the edge's AUX field.  At the end of an iteration we
   free each of these structures and update the AUX field to point
   to any requested redirection target (the code for updating the
   CFG and SSA graph for edge redirection expects redirection edge
   targets to be in the AUX field for each edge.  */

struct edge_info
{
  /* If this edge creates a simple equivalence, the LHS and RHS of
     the equivalence will be stored here.  */
  tree lhs;
  tree rhs;

  /* Traversing an edge may also indicate one or more particular conditions
     are true or false.  The number of recorded conditions can vary, but
     can be determined by the condition's code.  So we have an array
     and its maximum index rather than use a varray.  */
  struct cond_equivalence *cond_equivalences;
  unsigned int max_cond_equivalences;
};

/* Hash table with expressions made available during the renaming process.
   When an assignment of the form X_i = EXPR is found, the statement is
   stored in this table.  If the same expression EXPR is later found on the
   RHS of another statement, it is replaced with X_i (thus performing
   global redundancy elimination).  Similarly as we pass through conditionals
   we record the conditional itself as having either a true or false value
   in this table.  */
static htab_t avail_exprs;

/* Stack of available expressions in AVAIL_EXPRs.  Each block pushes any
   expressions it enters into the hash table along with a marker entry
   (null).  When we finish processing the block, we pop off entries and
   remove the expressions from the global hash table until we hit the
   marker.  */
typedef struct expr_hash_elt * expr_hash_elt_t;
DEF_VEC_P(expr_hash_elt_t);
DEF_VEC_ALLOC_P(expr_hash_elt_t,heap);

static VEC(expr_hash_elt_t,heap) *avail_exprs_stack;

/* Stack of statements we need to rescan during finalization for newly
   exposed variables.

   Statement rescanning must occur after the current block's available
   expressions are removed from AVAIL_EXPRS.  Else we may change the
   hash code for an expression and be unable to find/remove it from
   AVAIL_EXPRS.  */
typedef gimple *gimple_p;
DEF_VEC_P(gimple_p);
DEF_VEC_ALLOC_P(gimple_p,heap);

static VEC(gimple_p,heap) *stmts_to_rescan;

/* Structure for entries in the expression hash table.  */

struct expr_hash_elt
{
  /* The value (lhs) of this expression.  */
  tree lhs;

  /* The expression (rhs) we want to record.  */
  struct hashable_expr expr;

  /* The stmt pointer if this element corresponds to a statement.  */
  gimple stmt;

  /* The hash value for RHS.  */
  hashval_t hash;

  /* A unique stamp, typically the address of the hash
     element itself, used in removing entries from the table.  */
  struct expr_hash_elt *stamp;
};

/* Stack of dest,src pairs that need to be restored during finalization.

   A NULL entry is used to mark the end of pairs which need to be
   restored during finalization of this block.  */
static VEC(tree,heap) *const_and_copies_stack;

/* Track whether or not we have changed the control flow graph.  */
static bool cfg_altered;

/* Bitmap of blocks that have had EH statements cleaned.  We should
   remove their dead edges eventually.  */
static bitmap need_eh_cleanup;

/* Statistics for dominator optimizations.  */
struct opt_stats_d
{
  long num_stmts;
  long num_exprs_considered;
  long num_re;
  long num_const_prop;
  long num_copy_prop;
};

static struct opt_stats_d opt_stats;

/* Local functions.  */
static void optimize_stmt (struct dom_walk_data *, 
			   basic_block,
			   gimple_stmt_iterator);
static tree lookup_avail_expr (gimple, bool);
static hashval_t avail_expr_hash (const void *);
static hashval_t real_avail_expr_hash (const void *);
static int avail_expr_eq (const void *, const void *);
static void htab_statistics (FILE *, htab_t);
static void record_cond (struct cond_equivalence *);
static void record_const_or_copy (tree, tree);
static void record_equality (tree, tree);
static void record_equivalences_from_phis (basic_block);
static void record_equivalences_from_incoming_edge (basic_block);
static bool eliminate_redundant_computations (gimple_stmt_iterator *);
static void record_equivalences_from_stmt (gimple, int);
static void dom_thread_across_edge (struct dom_walk_data *, edge);
static void dom_opt_finalize_block (struct dom_walk_data *, basic_block);
static void dom_opt_initialize_block (struct dom_walk_data *, basic_block);
static void propagate_to_outgoing_edges (struct dom_walk_data *, basic_block);
static void remove_local_expressions_from_table (void);
static void restore_vars_to_original_value (void);
static edge single_incoming_edge_ignoring_loop_edges (basic_block);


/* Given a statement STMT, initialize the hash table element pointed to
   by ELEMENT.  */

static void
initialize_hash_element (gimple stmt, tree lhs,
                         struct expr_hash_elt *element)
{
  enum gimple_code code = gimple_code (stmt);
  struct hashable_expr *expr = &element->expr;

  if (code == GIMPLE_ASSIGN)
    {
      enum tree_code subcode = gimple_assign_rhs_code (stmt);

      expr->type = NULL_TREE;
      
      switch (get_gimple_rhs_class (subcode))
        {
        case GIMPLE_SINGLE_RHS:
          expr->kind = EXPR_SINGLE;
          expr->ops.single.rhs = gimple_assign_rhs1 (stmt);
          break;
        case GIMPLE_UNARY_RHS:
          expr->kind = EXPR_UNARY;
	  expr->type = TREE_TYPE (gimple_assign_lhs (stmt));
          expr->ops.unary.op = subcode;
          expr->ops.unary.opnd = gimple_assign_rhs1 (stmt);
          break;
        case GIMPLE_BINARY_RHS:
          expr->kind = EXPR_BINARY;
	  expr->type = TREE_TYPE (gimple_assign_lhs (stmt));
          expr->ops.binary.op = subcode;
          expr->ops.binary.opnd0 = gimple_assign_rhs1 (stmt);
          expr->ops.binary.opnd1 = gimple_assign_rhs2 (stmt);
          break;
        default:
          gcc_unreachable ();
        }
    }
  else if (code == GIMPLE_COND)
    {
      expr->type = boolean_type_node;
      expr->kind = EXPR_BINARY;
      expr->ops.binary.op = gimple_cond_code (stmt);
      expr->ops.binary.opnd0 = gimple_cond_lhs (stmt);
      expr->ops.binary.opnd1 = gimple_cond_rhs (stmt);
    }
  else if (code == GIMPLE_CALL)
    {
      size_t nargs = gimple_call_num_args (stmt);
      size_t i;

      gcc_assert (gimple_call_lhs (stmt));

      expr->type = TREE_TYPE (gimple_call_lhs (stmt));
      expr->kind = EXPR_CALL;
      expr->ops.call.fn = gimple_call_fn (stmt);

      if (gimple_call_flags (stmt) & (ECF_CONST | ECF_PURE))
        expr->ops.call.pure = true;
      else 
        expr->ops.call.pure = false;

      expr->ops.call.nargs = nargs;
      expr->ops.call.args = (tree *) xcalloc (nargs, sizeof (tree));
      for (i = 0; i < nargs; i++)
        expr->ops.call.args[i] = gimple_call_arg (stmt, i);
    }
  else if (code == GIMPLE_SWITCH)
    {
      expr->type = TREE_TYPE (gimple_switch_index (stmt));
      expr->kind = EXPR_SINGLE;
      expr->ops.single.rhs = gimple_switch_index (stmt);
    }
  else if (code == GIMPLE_GOTO)
    {
      expr->type = TREE_TYPE (gimple_goto_dest (stmt));
      expr->kind = EXPR_SINGLE;
      expr->ops.single.rhs = gimple_goto_dest (stmt);
    }
  else
    gcc_unreachable ();

  element->lhs = lhs;
  element->stmt = stmt;
  element->hash = avail_expr_hash (element);
  element->stamp = element;
}

/* Given a conditional expression COND as a tree, initialize
   a hashable_expr expression EXPR.  The conditional must be a
   comparison or logical negation.  A constant or a variable is
   not permitted.  */

static void
initialize_expr_from_cond (tree cond, struct hashable_expr *expr)
{
  expr->type = boolean_type_node;
  
  if (COMPARISON_CLASS_P (cond))
    {
      expr->kind = EXPR_BINARY;
      expr->ops.binary.op = TREE_CODE (cond);
      expr->ops.binary.opnd0 = TREE_OPERAND (cond, 0);
      expr->ops.binary.opnd1 = TREE_OPERAND (cond, 1);
    }
  else if (TREE_CODE (cond) == TRUTH_NOT_EXPR)
    {
      expr->kind = EXPR_UNARY;
      expr->ops.unary.op = TRUTH_NOT_EXPR;
      expr->ops.unary.opnd = TREE_OPERAND (cond, 0);
    }
  else
    gcc_unreachable ();
}

/* Given a hashable_expr expression EXPR and an LHS,
   initialize the hash table element pointed to by ELEMENT.  */

static void
initialize_hash_element_from_expr (struct hashable_expr *expr,
                                   tree lhs,
                                   struct expr_hash_elt *element)
{
  element->expr = *expr;
  element->lhs = lhs;
  element->stmt = NULL;
  element->hash = avail_expr_hash (element);
  element->stamp = element;
}

/* Compare two hashable_expr structures for equivalence.
   They are considered equivalent when the the expressions
   they denote must necessarily be equal.  The logic is intended
   to follow that of operand_equal_p in fold-const.c  */

static bool
hashable_expr_equal_p (const struct hashable_expr *expr0,
                        const struct hashable_expr *expr1)
{
  tree type0 = expr0->type;
  tree type1 = expr1->type;

  /* If either type is NULL, there is nothing to check.  */
  if ((type0 == NULL_TREE) ^ (type1 == NULL_TREE))
    return false;

  /* If both types don't have the same signedness, precision, and mode,
     then we can't consider  them equal.  */
  if (type0 != type1
      && (TREE_CODE (type0) == ERROR_MARK
	  || TREE_CODE (type1) == ERROR_MARK
	  || TYPE_UNSIGNED (type0) != TYPE_UNSIGNED (type1)
	  || TYPE_PRECISION (type0) != TYPE_PRECISION (type1)
	  || TYPE_MODE (type0) != TYPE_MODE (type1)))
    return false;

  if (expr0->kind != expr1->kind)
    return false;

  switch (expr0->kind)
    {
    case EXPR_SINGLE:
      return operand_equal_p (expr0->ops.single.rhs,
                              expr1->ops.single.rhs, 0);

    case EXPR_UNARY:
      if (expr0->ops.unary.op != expr1->ops.unary.op)
        return false;

      if ((CONVERT_EXPR_CODE_P (expr0->ops.unary.op)
           || expr0->ops.unary.op == NON_LVALUE_EXPR)
          && TYPE_UNSIGNED (expr0->type) != TYPE_UNSIGNED (expr1->type))
        return false;

      return operand_equal_p (expr0->ops.unary.opnd,
                              expr1->ops.unary.opnd, 0);

    case EXPR_BINARY:
      {
        if (expr0->ops.binary.op != expr1->ops.binary.op)
          return false;

        if (operand_equal_p (expr0->ops.binary.opnd0,
                             expr1->ops.binary.opnd0, 0)
            && operand_equal_p (expr0->ops.binary.opnd1,
                                expr1->ops.binary.opnd1, 0))
          return true;

        /* For commutative ops, allow the other order.  */
        return (commutative_tree_code (expr0->ops.binary.op)
                && operand_equal_p (expr0->ops.binary.opnd0,
                                    expr1->ops.binary.opnd1, 0)
                && operand_equal_p (expr0->ops.binary.opnd1,
                                    expr1->ops.binary.opnd0, 0));
      }

    case EXPR_CALL:
      {
        size_t i;

        /* If the calls are to different functions, then they
           clearly cannot be equal.  */
        if (! operand_equal_p (expr0->ops.call.fn,
                               expr1->ops.call.fn, 0))
          return false;

        if (! expr0->ops.call.pure)
          return false;

        if (expr0->ops.call.nargs !=  expr1->ops.call.nargs)
          return false;

        for (i = 0; i < expr0->ops.call.nargs; i++)
          if (! operand_equal_p (expr0->ops.call.args[i],
                                 expr1->ops.call.args[i], 0))
            return false;

        return true;
      }
     
    default:
      gcc_unreachable ();
    }
}

/* Compute a hash value for a hashable_expr value EXPR and a
   previously accumulated hash value VAL.  If two hashable_expr
   values compare equal with hashable_expr_equal_p, they must
   hash to the same value, given an identical value of VAL.
   The logic is intended to follow iterative_hash_expr in tree.c.  */

static hashval_t
iterative_hash_hashable_expr (const struct hashable_expr *expr, hashval_t val)
{
  switch (expr->kind)
    {
    case EXPR_SINGLE:
      val = iterative_hash_expr (expr->ops.single.rhs, val);
      break;

    case EXPR_UNARY:
      val = iterative_hash_object (expr->ops.unary.op, val);

      /* Make sure to include signedness in the hash computation.
         Don't hash the type, that can lead to having nodes which
         compare equal according to operand_equal_p, but which
         have different hash codes.  */
      if (CONVERT_EXPR_CODE_P (expr->ops.unary.op)
          || expr->ops.unary.op == NON_LVALUE_EXPR)
        val += TYPE_UNSIGNED (expr->type);

      val = iterative_hash_expr (expr->ops.unary.opnd, val);
      break;

    case EXPR_BINARY:
      val = iterative_hash_object (expr->ops.binary.op, val);
      if (commutative_tree_code (expr->ops.binary.op))
          val = iterative_hash_exprs_commutative (expr->ops.binary.opnd0,
                                                  expr->ops.binary.opnd1, val);
      else
        {
          val = iterative_hash_expr (expr->ops.binary.opnd0, val);
          val = iterative_hash_expr (expr->ops.binary.opnd1, val);
        }
      break;

    case EXPR_CALL:
      {
        size_t i;
        enum tree_code code = CALL_EXPR;

        val = iterative_hash_object (code, val);
        val = iterative_hash_expr (expr->ops.call.fn, val);
        for (i = 0; i < expr->ops.call.nargs; i++)
          val = iterative_hash_expr (expr->ops.call.args[i], val);
      }
      break;
     
    default:
      gcc_unreachable ();
    }

  return val;
}

/* Print a diagnostic dump of an expression hash table entry.  */

static void
print_expr_hash_elt (FILE * stream, const struct expr_hash_elt *element)
{
  if (element->stmt)
    fprintf (stream, "STMT ");
  else
    fprintf (stream, "COND ");

  if (element->lhs)
    {
      print_generic_expr (stream, element->lhs, 0);
      fprintf (stream, " = ");
    }
  
  switch (element->expr.kind)
    {
      case EXPR_SINGLE:
        print_generic_expr (stream, element->expr.ops.single.rhs, 0);
        break;

      case EXPR_UNARY:
        fprintf (stream, "%s ", tree_code_name[element->expr.ops.unary.op]);
        print_generic_expr (stream, element->expr.ops.unary.opnd, 0);
        break;

      case EXPR_BINARY:
        print_generic_expr (stream, element->expr.ops.binary.opnd0, 0);
        fprintf (stream, " %s ", tree_code_name[element->expr.ops.binary.op]);
        print_generic_expr (stream, element->expr.ops.binary.opnd1, 0);
        break;

      case EXPR_CALL:
        {
          size_t i;
          size_t nargs = element->expr.ops.call.nargs;

          print_generic_expr (stream, element->expr.ops.call.fn, 0);
          fprintf (stream, " (");
          for (i = 0; i < nargs; i++)
            {
              print_generic_expr (stream, element->expr.ops.call.args[i], 0);
              if (i + 1 < nargs)
                fprintf (stream, ", ");
            }
          fprintf (stream, ")");
        }
        break;
    }
  fprintf (stream, "\n");

  if (element->stmt)
    {
      fprintf (stream, "          ");
      print_gimple_stmt (stream, element->stmt, 0, 0);
    }
}

/* Delete an expr_hash_elt and reclaim its storage.  */

static void
free_expr_hash_elt (void *elt)
{
  struct expr_hash_elt *element = ((struct expr_hash_elt *)elt);

  if (element->expr.kind == EXPR_CALL)
    free (element->expr.ops.call.args);

  free (element);
}

/* Allocate an EDGE_INFO for edge E and attach it to E.
   Return the new EDGE_INFO structure.  */

static struct edge_info *
allocate_edge_info (edge e)
{
  struct edge_info *edge_info;

  edge_info = XCNEW (struct edge_info);

  e->aux = edge_info;
  return edge_info;
}

/* Free all EDGE_INFO structures associated with edges in the CFG.
   If a particular edge can be threaded, copy the redirection
   target from the EDGE_INFO structure into the edge's AUX field
   as required by code to update the CFG and SSA graph for
   jump threading.  */

static void
free_all_edge_infos (void)
{
  basic_block bb;
  edge_iterator ei;
  edge e;

  FOR_EACH_BB (bb)
    {
      FOR_EACH_EDGE (e, ei, bb->preds)
        {
	 struct edge_info *edge_info = (struct edge_info *) e->aux;

	  if (edge_info)
	    {
	      if (edge_info->cond_equivalences)
		free (edge_info->cond_equivalences);
	      free (edge_info);
	      e->aux = NULL;
	    }
	}
    }
}

/* Jump threading, redundancy elimination and const/copy propagation. 

   This pass may expose new symbols that need to be renamed into SSA.  For
   every new symbol exposed, its corresponding bit will be set in
   VARS_TO_RENAME.  */

static unsigned int
tree_ssa_dominator_optimize (void)
{
  struct dom_walk_data walk_data;
  unsigned int i;

  memset (&opt_stats, 0, sizeof (opt_stats));

  /* Create our hash tables.  */
  avail_exprs = htab_create (1024, real_avail_expr_hash, avail_expr_eq, free_expr_hash_elt);
  avail_exprs_stack = VEC_alloc (expr_hash_elt_t, heap, 20);
  const_and_copies_stack = VEC_alloc (tree, heap, 20);
  stmts_to_rescan = VEC_alloc (gimple_p, heap, 20);
  need_eh_cleanup = BITMAP_ALLOC (NULL);

  /* Setup callbacks for the generic dominator tree walker.  */
  walk_data.walk_stmts_backward = false;
  walk_data.dom_direction = CDI_DOMINATORS;
  walk_data.initialize_block_local_data = NULL;
  walk_data.before_dom_children_before_stmts = dom_opt_initialize_block;
  walk_data.before_dom_children_walk_stmts = optimize_stmt;
  walk_data.before_dom_children_after_stmts = propagate_to_outgoing_edges;
  walk_data.after_dom_children_before_stmts = NULL;
  walk_data.after_dom_children_walk_stmts = NULL;
  walk_data.after_dom_children_after_stmts = dom_opt_finalize_block;
  /* Right now we only attach a dummy COND_EXPR to the global data pointer.
     When we attach more stuff we'll need to fill this out with a real
     structure.  */
  walk_data.global_data = NULL;
  walk_data.block_local_data_size = 0;
  walk_data.interesting_blocks = NULL;

  /* Now initialize the dominator walker.  */
  init_walk_dominator_tree (&walk_data);

  calculate_dominance_info (CDI_DOMINATORS);
  cfg_altered = false;

  /* We need to know loop structures in order to avoid destroying them
     in jump threading.  Note that we still can e.g. thread through loop
     headers to an exit edge, or through loop header to the loop body, assuming
     that we update the loop info.  */
  loop_optimizer_init (LOOPS_HAVE_SIMPLE_LATCHES);

  /* We need accurate information regarding back edges in the CFG
     for jump threading; this may include back edges that are not part of
     a single loop.  */
  mark_dfs_back_edges ();
      
  /* Recursively walk the dominator tree optimizing statements.  */
  walk_dominator_tree (&walk_data, ENTRY_BLOCK_PTR);

  {
    gimple_stmt_iterator gsi;
    basic_block bb;
    FOR_EACH_BB (bb)
      {for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	  update_stmt_if_modified (gsi_stmt (gsi));
      }
  }

  /* If we exposed any new variables, go ahead and put them into
     SSA form now, before we handle jump threading.  This simplifies
     interactions between rewriting of _DECL nodes into SSA form
     and rewriting SSA_NAME nodes into SSA form after block
     duplication and CFG manipulation.  */
  update_ssa (TODO_update_ssa);

  free_all_edge_infos ();

  /* Thread jumps, creating duplicate blocks as needed.  */
  cfg_altered |= thread_through_all_blocks (first_pass_instance);

  if (cfg_altered)
    free_dominance_info (CDI_DOMINATORS);

  /* Removal of statements may make some EH edges dead.  Purge
     such edges from the CFG as needed.  */
  if (!bitmap_empty_p (need_eh_cleanup))
    {
      unsigned i;
      bitmap_iterator bi;

      /* Jump threading may have created forwarder blocks from blocks
	 needing EH cleanup; the new successor of these blocks, which
	 has inherited from the original block, needs the cleanup.  */
      EXECUTE_IF_SET_IN_BITMAP (need_eh_cleanup, 0, i, bi)
	{
	  basic_block bb = BASIC_BLOCK (i);
	  if (single_succ_p (bb) == 1
	      && (single_succ_edge (bb)->flags & EDGE_EH) == 0)
	    {
	      bitmap_clear_bit (need_eh_cleanup, i);
	      bitmap_set_bit (need_eh_cleanup, single_succ (bb)->index);
	    }
	}

      gimple_purge_all_dead_eh_edges (need_eh_cleanup);
      bitmap_zero (need_eh_cleanup);
    }

  /* Finally, remove everything except invariants in SSA_NAME_VALUE.

     Long term we will be able to let everything in SSA_NAME_VALUE
     persist.  However, for now, we know this is the safe thing to do.  */
  for (i = 0; i < num_ssa_names; i++)
   {
      tree name = ssa_name (i);
      tree value;

      if (!name)
        continue;

      value = SSA_NAME_VALUE (name);
      if (value && !is_gimple_min_invariant (value))
	SSA_NAME_VALUE (name) = NULL;
    }

  statistics_counter_event (cfun, "Redundant expressions eliminated",
			    opt_stats.num_re);
  statistics_counter_event (cfun, "Constants propagated",
			    opt_stats.num_const_prop);
  statistics_counter_event (cfun, "Copies propagated",
			    opt_stats.num_copy_prop);

  /* Debugging dumps.  */
  if (dump_file && (dump_flags & TDF_STATS))
    dump_dominator_optimization_stats (dump_file);

  loop_optimizer_finalize ();

  /* Delete our main hashtable.  */
  htab_delete (avail_exprs);

  /* And finalize the dominator walker.  */
  fini_walk_dominator_tree (&walk_data);

  /* Free asserted bitmaps and stacks.  */
  BITMAP_FREE (need_eh_cleanup);
  
  VEC_free (expr_hash_elt_t, heap, avail_exprs_stack);
  VEC_free (tree, heap, const_and_copies_stack);
  VEC_free (gimple_p, heap, stmts_to_rescan);
  
  return 0;
}

static bool
gate_dominator (void)
{
  return flag_tree_dom != 0;
}

struct gimple_opt_pass pass_dominator = 
{
 {
  GIMPLE_PASS,
  "dom",				/* name */
  gate_dominator,			/* gate */
  tree_ssa_dominator_optimize,		/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  TV_TREE_SSA_DOMINATOR_OPTS,		/* tv_id */
  PROP_cfg | PROP_ssa | PROP_alias,	/* properties_required */
  0,					/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_dump_func
    | TODO_update_ssa
    | TODO_cleanup_cfg
    | TODO_verify_ssa			/* todo_flags_finish */
 }
};


/* Given a conditional statement CONDSTMT, convert the
   condition to a canonical form.  */

static void
canonicalize_comparison (gimple condstmt)
{
  tree op0;
  tree op1;
  enum tree_code code;

  gcc_assert (gimple_code (condstmt) == GIMPLE_COND);

  op0 = gimple_cond_lhs (condstmt);
  op1 = gimple_cond_rhs (condstmt);

  code = gimple_cond_code (condstmt);

  /* If it would be profitable to swap the operands, then do so to
     canonicalize the statement, enabling better optimization.

     By placing canonicalization of such expressions here we
     transparently keep statements in canonical form, even
     when the statement is modified.  */
  if (tree_swap_operands_p (op0, op1, false))
    {
      /* For relationals we need to swap the operands
	 and change the code.  */
      if (code == LT_EXPR
	  || code == GT_EXPR
	  || code == LE_EXPR
	  || code == GE_EXPR)
	{
          code = swap_tree_comparison (code);

          gimple_cond_set_code (condstmt, code);
          gimple_cond_set_lhs (condstmt, op1);
          gimple_cond_set_rhs (condstmt, op0);

          update_stmt (condstmt);
	}
    }
}

/* Initialize local stacks for this optimizer and record equivalences
   upon entry to BB.  Equivalences can come from the edge traversed to
   reach BB or they may come from PHI nodes at the start of BB.  */

static void
dom_opt_initialize_block (struct dom_walk_data *walk_data ATTRIBUTE_UNUSED,
			  basic_block bb)
{
  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "\n\nOptimizing block #%d\n\n", bb->index);

  /* Push a marker on the stacks of local information so that we know how
     far to unwind when we finalize this block.  */
  VEC_safe_push (expr_hash_elt_t, heap, avail_exprs_stack, NULL);
  VEC_safe_push (tree, heap, const_and_copies_stack, NULL_TREE);

  record_equivalences_from_incoming_edge (bb);

  /* PHI nodes can create equivalences too.  */
  record_equivalences_from_phis (bb);
}

/* Remove all the expressions in LOCALS from TABLE, stopping when there are
   LIMIT entries left in LOCALs.  */

static void
remove_local_expressions_from_table (void)
{
  /* Remove all the expressions made available in this block.  */
  while (VEC_length (expr_hash_elt_t, avail_exprs_stack) > 0)
    {
      struct expr_hash_elt element;
      expr_hash_elt_t victim = VEC_pop (expr_hash_elt_t, avail_exprs_stack);

      if (victim == NULL)
	break;

      element = *victim;

      /* This must precede the actual removal from the hash table,
         as ELEMENT and the table entry may share a call argument
         vector which will be freed during removal.  */
      if (dump_file && (dump_flags & TDF_DETAILS))
        {
          fprintf (dump_file, "<<<< ");
          print_expr_hash_elt (dump_file, &element);
        }

      htab_remove_elt_with_hash (avail_exprs, &element, element.hash);
    }
}

/* Use the source/dest pairs in CONST_AND_COPIES_STACK to restore
   CONST_AND_COPIES to its original state, stopping when we hit a
   NULL marker.  */

static void
restore_vars_to_original_value (void)
{
  while (VEC_length (tree, const_and_copies_stack) > 0)
    {
      tree prev_value, dest;

      dest = VEC_pop (tree, const_and_copies_stack);

      if (dest == NULL)
	break;

      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "<<<< COPY ");
	  print_generic_expr (dump_file, dest, 0);
	  fprintf (dump_file, " = ");
	  print_generic_expr (dump_file, SSA_NAME_VALUE (dest), 0);
	  fprintf (dump_file, "\n");
	}

      prev_value = VEC_pop (tree, const_and_copies_stack);
      SSA_NAME_VALUE (dest) =  prev_value;
    }
}

/* A trivial wrapper so that we can present the generic jump
   threading code with a simple API for simplifying statements.  */
static tree
simplify_stmt_for_jump_threading (gimple stmt,
				  gimple within_stmt ATTRIBUTE_UNUSED)
{
  return lookup_avail_expr (stmt, false);
}

/* Wrapper for common code to attempt to thread an edge.  For example,
   it handles lazily building the dummy condition and the bookkeeping
   when jump threading is successful.  */

static void
dom_thread_across_edge (struct dom_walk_data *walk_data, edge e)
{
  if (! walk_data->global_data)
  {
    gimple dummy_cond =
        gimple_build_cond (NE_EXPR,
                           integer_zero_node, integer_zero_node,
                           NULL, NULL);
    walk_data->global_data = dummy_cond;
  }

  thread_across_edge ((gimple) walk_data->global_data, e, false,
		      &const_and_copies_stack,
		      simplify_stmt_for_jump_threading);
}

/* We have finished processing the dominator children of BB, perform
   any finalization actions in preparation for leaving this node in
   the dominator tree.  */

static void
dom_opt_finalize_block (struct dom_walk_data *walk_data, basic_block bb)
{
  gimple last;

  /* If we have an outgoing edge to a block with multiple incoming and
     outgoing edges, then we may be able to thread the edge, i.e., we
     may be able to statically determine which of the outgoing edges
     will be traversed when the incoming edge from BB is traversed.  */
  if (single_succ_p (bb)
      && (single_succ_edge (bb)->flags & EDGE_ABNORMAL) == 0
      && potentially_threadable_block (single_succ (bb)))
    {
      dom_thread_across_edge (walk_data, single_succ_edge (bb));
    }
  else if ((last = last_stmt (bb))
	   && gimple_code (last) == GIMPLE_COND
	   && EDGE_COUNT (bb->succs) == 2
	   && (EDGE_SUCC (bb, 0)->flags & EDGE_ABNORMAL) == 0
	   && (EDGE_SUCC (bb, 1)->flags & EDGE_ABNORMAL) == 0)
    {
      edge true_edge, false_edge;

      extract_true_false_edges_from_block (bb, &true_edge, &false_edge);

      /* Only try to thread the edge if it reaches a target block with
	 more than one predecessor and more than one successor.  */
      if (potentially_threadable_block (true_edge->dest))
	{
	  struct edge_info *edge_info;
	  unsigned int i;

	  /* Push a marker onto the available expression stack so that we
	     unwind any expressions related to the TRUE arm before processing
	     the false arm below.  */
          VEC_safe_push (expr_hash_elt_t, heap, avail_exprs_stack, NULL);
	  VEC_safe_push (tree, heap, const_and_copies_stack, NULL_TREE);

	  edge_info = (struct edge_info *) true_edge->aux;

	  /* If we have info associated with this edge, record it into
	     our equivalence tables.  */
	  if (edge_info)
	    {
	      struct cond_equivalence *cond_equivalences = edge_info->cond_equivalences;
	      tree lhs = edge_info->lhs;
	      tree rhs = edge_info->rhs;

	      /* If we have a simple NAME = VALUE equivalence, record it.  */
	      if (lhs && TREE_CODE (lhs) == SSA_NAME)
		record_const_or_copy (lhs, rhs);

	      /* If we have 0 = COND or 1 = COND equivalences, record them
		 into our expression hash tables.  */
	      if (cond_equivalences)
		for (i = 0; i < edge_info->max_cond_equivalences; i++)
                  record_cond (&cond_equivalences[i]);
	    }

	  dom_thread_across_edge (walk_data, true_edge);

	  /* And restore the various tables to their state before
	     we threaded this edge.  */
	  remove_local_expressions_from_table ();
	}

      /* Similarly for the ELSE arm.  */
      if (potentially_threadable_block (false_edge->dest))
	{
	  struct edge_info *edge_info;
	  unsigned int i;

	  VEC_safe_push (tree, heap, const_and_copies_stack, NULL_TREE);
	  edge_info = (struct edge_info *) false_edge->aux;

	  /* If we have info associated with this edge, record it into
	     our equivalence tables.  */
	  if (edge_info)
	    {
	      struct cond_equivalence *cond_equivalences = edge_info->cond_equivalences;
	      tree lhs = edge_info->lhs;
	      tree rhs = edge_info->rhs;

	      /* If we have a simple NAME = VALUE equivalence, record it.  */
	      if (lhs && TREE_CODE (lhs) == SSA_NAME)
		record_const_or_copy (lhs, rhs);

	      /* If we have 0 = COND or 1 = COND equivalences, record them
		 into our expression hash tables.  */
	      if (cond_equivalences)
		for (i = 0; i < edge_info->max_cond_equivalences; i++)
                  record_cond (&cond_equivalences[i]);
	    }

	  /* Now thread the edge.  */
	  dom_thread_across_edge (walk_data, false_edge);

	  /* No need to remove local expressions from our tables
	     or restore vars to their original value as that will
	     be done immediately below.  */
	}
    }

  remove_local_expressions_from_table ();
  restore_vars_to_original_value ();

  /* If we queued any statements to rescan in this block, then
     go ahead and rescan them now.  */
  while (VEC_length (gimple_p, stmts_to_rescan) > 0)
    {
      gimple *stmt_p = VEC_last (gimple_p, stmts_to_rescan);
      gimple stmt = *stmt_p;
      basic_block stmt_bb = gimple_bb (stmt);

      if (stmt_bb != bb)
	break;

      VEC_pop (gimple_p, stmts_to_rescan);
      pop_stmt_changes (stmt_p);
    }
}

/* PHI nodes can create equivalences too.

   Ignoring any alternatives which are the same as the result, if
   all the alternatives are equal, then the PHI node creates an
   equivalence.  */

static void
record_equivalences_from_phis (basic_block bb)
{
  gimple_stmt_iterator gsi;
  
  for (gsi = gsi_start_phis (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gimple phi = gsi_stmt (gsi);

      tree lhs = gimple_phi_result (phi);
      tree rhs = NULL;
      size_t i;

      for (i = 0; i < gimple_phi_num_args (phi); i++)
	{
	  tree t = gimple_phi_arg_def (phi, i);

	  /* Ignore alternatives which are the same as our LHS.  Since
	     LHS is a PHI_RESULT, it is known to be a SSA_NAME, so we
	     can simply compare pointers.  */
	  if (lhs == t)
	    continue;

	  /* If we have not processed an alternative yet, then set
	     RHS to this alternative.  */
	  if (rhs == NULL)
	    rhs = t;
	  /* If we have processed an alternative (stored in RHS), then
	     see if it is equal to this one.  If it isn't, then stop
	     the search.  */
	  else if (! operand_equal_for_phi_arg_p (rhs, t))
	    break;
	}

      /* If we had no interesting alternatives, then all the RHS alternatives
	 must have been the same as LHS.  */
      if (!rhs)
	rhs = lhs;

      /* If we managed to iterate through each PHI alternative without
	 breaking out of the loop, then we have a PHI which may create
	 a useful equivalence.  We do not need to record unwind data for
	 this, since this is a true assignment and not an equivalence
	 inferred from a comparison.  All uses of this ssa name are dominated
	 by this assignment, so unwinding just costs time and space.  */
      if (i == gimple_phi_num_args (phi) && may_propagate_copy (lhs, rhs))
	SSA_NAME_VALUE (lhs) = rhs;
    }
}

/* Ignoring loop backedges, if BB has precisely one incoming edge then
   return that edge.  Otherwise return NULL.  */
static edge
single_incoming_edge_ignoring_loop_edges (basic_block bb)
{
  edge retval = NULL;
  edge e;
  edge_iterator ei;

  FOR_EACH_EDGE (e, ei, bb->preds)
    {
      /* A loop back edge can be identified by the destination of
	 the edge dominating the source of the edge.  */
      if (dominated_by_p (CDI_DOMINATORS, e->src, e->dest))
	continue;

      /* If we have already seen a non-loop edge, then we must have
	 multiple incoming non-loop edges and thus we return NULL.  */
      if (retval)
	return NULL;

      /* This is the first non-loop incoming edge we have found.  Record
	 it.  */
      retval = e;
    }

  return retval;
}

/* Record any equivalences created by the incoming edge to BB.  If BB
   has more than one incoming edge, then no equivalence is created.  */

static void
record_equivalences_from_incoming_edge (basic_block bb)
{
  edge e;
  basic_block parent;
  struct edge_info *edge_info;

  /* If our parent block ended with a control statement, then we may be
     able to record some equivalences based on which outgoing edge from
     the parent was followed.  */
  parent = get_immediate_dominator (CDI_DOMINATORS, bb);

  e = single_incoming_edge_ignoring_loop_edges (bb);

  /* If we had a single incoming edge from our parent block, then enter
     any data associated with the edge into our tables.  */
  if (e && e->src == parent)
    {
      unsigned int i;

      edge_info = (struct edge_info *) e->aux;

      if (edge_info)
	{
	  tree lhs = edge_info->lhs;
	  tree rhs = edge_info->rhs;
	  struct cond_equivalence *cond_equivalences = edge_info->cond_equivalences;

	  if (lhs)
	    record_equality (lhs, rhs);

	  if (cond_equivalences)
            for (i = 0; i < edge_info->max_cond_equivalences; i++)
              record_cond (&cond_equivalences[i]);
	}
    }
}

/* Dump SSA statistics on FILE.  */

void
dump_dominator_optimization_stats (FILE *file)
{
  fprintf (file, "Total number of statements:                   %6ld\n\n",
	   opt_stats.num_stmts);
  fprintf (file, "Exprs considered for dominator optimizations: %6ld\n",
           opt_stats.num_exprs_considered);

  fprintf (file, "\nHash table statistics:\n");

  fprintf (file, "    avail_exprs: ");
  htab_statistics (file, avail_exprs);
}


/* Dump SSA statistics on stderr.  */

void
debug_dominator_optimization_stats (void)
{
  dump_dominator_optimization_stats (stderr);
}


/* Dump statistics for the hash table HTAB.  */

static void
htab_statistics (FILE *file, htab_t htab)
{
  fprintf (file, "size %ld, %ld elements, %f collision/search ratio\n",
	   (long) htab_size (htab),
	   (long) htab_elements (htab),
	   htab_collisions (htab));
}


/* Enter condition equivalence into the expression hash table.
   This indicates that a conditional expression has a known
   boolean value.  */

static void
record_cond (struct cond_equivalence *p)
{
  struct expr_hash_elt *element = XCNEW (struct expr_hash_elt);
  void **slot;

  initialize_hash_element_from_expr (&p->cond, p->value, element);

  slot = htab_find_slot_with_hash (avail_exprs, (void *)element,
				   element->hash, INSERT);
  if (*slot == NULL)
    {
      *slot = (void *) element;

      if (dump_file && (dump_flags & TDF_DETAILS))
        {
          fprintf (dump_file, "1>>> ");
          print_expr_hash_elt (dump_file, element);
        }

      VEC_safe_push (expr_hash_elt_t, heap, avail_exprs_stack, element);
    }
  else
    free (element);
}

/* Build a cond_equivalence record indicating that the comparison
   CODE holds between operands OP0 and OP1.  */
   
static void
build_and_record_new_cond (enum tree_code code,
                           tree op0, tree op1,
                           struct cond_equivalence *p)
{
  struct hashable_expr *cond = &p->cond;

  gcc_assert (TREE_CODE_CLASS (code) == tcc_comparison);

  cond->type = boolean_type_node;
  cond->kind = EXPR_BINARY;
  cond->ops.binary.op = code;
  cond->ops.binary.opnd0 = op0;
  cond->ops.binary.opnd1 = op1;

  p->value = boolean_true_node;
}

/* Record that COND is true and INVERTED is false into the edge information
   structure.  Also record that any conditions dominated by COND are true
   as well.

   For example, if a < b is true, then a <= b must also be true.  */

static void
record_conditions (struct edge_info *edge_info, tree cond, tree inverted)
{
  tree op0, op1;

  if (!COMPARISON_CLASS_P (cond))
    return;

  op0 = TREE_OPERAND (cond, 0);
  op1 = TREE_OPERAND (cond, 1);

  switch (TREE_CODE (cond))
    {
    case LT_EXPR:
    case GT_EXPR:
      if (FLOAT_TYPE_P (TREE_TYPE (op0)))
	{
	  edge_info->max_cond_equivalences = 6;
	  edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 6);
	  build_and_record_new_cond (ORDERED_EXPR, op0, op1,
				     &edge_info->cond_equivalences[4]);
	  build_and_record_new_cond (LTGT_EXPR, op0, op1,
				     &edge_info->cond_equivalences[5]);
	}
      else
        {
          edge_info->max_cond_equivalences = 4;
	  edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 4);
	}

      build_and_record_new_cond ((TREE_CODE (cond) == LT_EXPR
				  ? LE_EXPR : GE_EXPR),
				 op0, op1, &edge_info->cond_equivalences[2]);
      build_and_record_new_cond (NE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[3]);
      break;

    case GE_EXPR:
    case LE_EXPR:
      if (FLOAT_TYPE_P (TREE_TYPE (op0)))
	{
	  edge_info->max_cond_equivalences = 3;
	  edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 3);
	  build_and_record_new_cond (ORDERED_EXPR, op0, op1,
				     &edge_info->cond_equivalences[2]);
	}
      else
	{
	  edge_info->max_cond_equivalences = 2;
	  edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 2);
	}
      break;

    case EQ_EXPR:
      if (FLOAT_TYPE_P (TREE_TYPE (op0)))
	{
	  edge_info->max_cond_equivalences = 5;
	  edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 5);
	  build_and_record_new_cond (ORDERED_EXPR, op0, op1,
				     &edge_info->cond_equivalences[4]);
	}
      else
	{
	  edge_info->max_cond_equivalences = 4;
	  edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 4);
	}
      build_and_record_new_cond (LE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[2]);
      build_and_record_new_cond (GE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[3]);
      break;

    case UNORDERED_EXPR:
      edge_info->max_cond_equivalences = 8;
      edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 8);
      build_and_record_new_cond (NE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[2]);
      build_and_record_new_cond (UNLE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[3]);
      build_and_record_new_cond (UNGE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[4]);
      build_and_record_new_cond (UNEQ_EXPR, op0, op1,
				 &edge_info->cond_equivalences[5]);
      build_and_record_new_cond (UNLT_EXPR, op0, op1,
				 &edge_info->cond_equivalences[6]);
      build_and_record_new_cond (UNGT_EXPR, op0, op1,
				 &edge_info->cond_equivalences[7]);
      break;

    case UNLT_EXPR:
    case UNGT_EXPR:
      edge_info->max_cond_equivalences = 4;
      edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 4);
      build_and_record_new_cond ((TREE_CODE (cond) == UNLT_EXPR
				  ? UNLE_EXPR : UNGE_EXPR),
				 op0, op1, &edge_info->cond_equivalences[2]);
      build_and_record_new_cond (NE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[3]);
      break;

    case UNEQ_EXPR:
      edge_info->max_cond_equivalences = 4;
      edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 4);
      build_and_record_new_cond (UNLE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[2]);
      build_and_record_new_cond (UNGE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[3]);
      break;

    case LTGT_EXPR:
      edge_info->max_cond_equivalences = 4;
      edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 4);
      build_and_record_new_cond (NE_EXPR, op0, op1,
				 &edge_info->cond_equivalences[2]);
      build_and_record_new_cond (ORDERED_EXPR, op0, op1,
				 &edge_info->cond_equivalences[3]);
      break;

    default:
      edge_info->max_cond_equivalences = 2;
      edge_info->cond_equivalences = XNEWVEC (struct cond_equivalence, 2);
      break;
    }

  /* Now store the original true and false conditions into the first
     two slots.  */
  initialize_expr_from_cond (cond, &edge_info->cond_equivalences[0].cond);
  edge_info->cond_equivalences[0].value = boolean_true_node;

  /* It is possible for INVERTED to be the negation of a comparison,
     and not a valid RHS or GIMPLE_COND condition.  This happens because
     invert_truthvalue may return such an expression when asked to invert
     a floating-point comparison.  These comparisons are not assumed to
     obey the trichotomy law.  */
  initialize_expr_from_cond (inverted, &edge_info->cond_equivalences[1].cond);
  edge_info->cond_equivalences[1].value = boolean_false_node;
}

/* A helper function for record_const_or_copy and record_equality.
   Do the work of recording the value and undo info.  */

static void
record_const_or_copy_1 (tree x, tree y, tree prev_x)
{
  SSA_NAME_VALUE (x) = y;

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "0>>> COPY ");
      print_generic_expr (dump_file, x, 0);
      fprintf (dump_file, " = ");
      print_generic_expr (dump_file, y, 0);
      fprintf (dump_file, "\n");
    }

  VEC_reserve (tree, heap, const_and_copies_stack, 2);
  VEC_quick_push (tree, const_and_copies_stack, prev_x);
  VEC_quick_push (tree, const_and_copies_stack, x);
}

/* Return the loop depth of the basic block of the defining statement of X.
   This number should not be treated as absolutely correct because the loop
   information may not be completely up-to-date when dom runs.  However, it
   will be relatively correct, and as more passes are taught to keep loop info
   up to date, the result will become more and more accurate.  */

int
loop_depth_of_name (tree x)
{
  gimple defstmt;
  basic_block defbb;

  /* If it's not an SSA_NAME, we have no clue where the definition is.  */
  if (TREE_CODE (x) != SSA_NAME)
    return 0;

  /* Otherwise return the loop depth of the defining statement's bb.
     Note that there may not actually be a bb for this statement, if the
     ssa_name is live on entry.  */
  defstmt = SSA_NAME_DEF_STMT (x);
  defbb = gimple_bb (defstmt);
  if (!defbb)
    return 0;

  return defbb->loop_depth;
}

/* Record that X is equal to Y in const_and_copies.  Record undo
   information in the block-local vector.  */

static void
record_const_or_copy (tree x, tree y)
{
  tree prev_x = SSA_NAME_VALUE (x);

  gcc_assert (TREE_CODE (x) == SSA_NAME);

  if (TREE_CODE (y) == SSA_NAME)
    {
      tree tmp = SSA_NAME_VALUE (y);
      if (tmp)
	y = tmp;
    }

  record_const_or_copy_1 (x, y, prev_x);
}

/* Similarly, but assume that X and Y are the two operands of an EQ_EXPR.
   This constrains the cases in which we may treat this as assignment.  */

static void
record_equality (tree x, tree y)
{
  tree prev_x = NULL, prev_y = NULL;

  if (TREE_CODE (x) == SSA_NAME)
    prev_x = SSA_NAME_VALUE (x);
  if (TREE_CODE (y) == SSA_NAME)
    prev_y = SSA_NAME_VALUE (y);

  /* If one of the previous values is invariant, or invariant in more loops
     (by depth), then use that.
     Otherwise it doesn't matter which value we choose, just so
     long as we canonicalize on one value.  */
  if (is_gimple_min_invariant (y))
    ;
  else if (is_gimple_min_invariant (x)
	   || (loop_depth_of_name (x) <= loop_depth_of_name (y)))
    prev_x = x, x = y, y = prev_x, prev_x = prev_y;
  else if (prev_x && is_gimple_min_invariant (prev_x))
    x = y, y = prev_x, prev_x = prev_y;
  else if (prev_y)
    y = prev_y;

  /* After the swapping, we must have one SSA_NAME.  */
  if (TREE_CODE (x) != SSA_NAME)
    return;

  /* For IEEE, -0.0 == 0.0, so we don't necessarily know the sign of a
     variable compared against zero.  If we're honoring signed zeros,
     then we cannot record this value unless we know that the value is
     nonzero.  */
  if (HONOR_SIGNED_ZEROS (TYPE_MODE (TREE_TYPE (x)))
      && (TREE_CODE (y) != REAL_CST
	  || REAL_VALUES_EQUAL (dconst0, TREE_REAL_CST (y))))
    return;

  record_const_or_copy_1 (x, y, prev_x);
}

/* Returns true when STMT is a simple iv increment.  It detects the
   following situation:
   
   i_1 = phi (..., i_2)
   i_2 = i_1 +/- ...  */

static bool
simple_iv_increment_p (gimple stmt)
{
  tree lhs, preinc;
  gimple phi;
  size_t i;

  if (gimple_code (stmt) != GIMPLE_ASSIGN)
    return false;

  lhs = gimple_assign_lhs (stmt);
  if (TREE_CODE (lhs) != SSA_NAME)
    return false;

  if (gimple_assign_rhs_code (stmt) != PLUS_EXPR
      && gimple_assign_rhs_code (stmt) != MINUS_EXPR)
    return false;

  preinc = gimple_assign_rhs1 (stmt);

  if (TREE_CODE (preinc) != SSA_NAME)
    return false;

  phi = SSA_NAME_DEF_STMT (preinc);
  if (gimple_code (phi) != GIMPLE_PHI)
    return false;

  for (i = 0; i < gimple_phi_num_args (phi); i++)
    if (gimple_phi_arg_def (phi, i) == lhs)
      return true;

  return false;
}

/* CONST_AND_COPIES is a table which maps an SSA_NAME to the current
   known value for that SSA_NAME (or NULL if no value is known).  

   Propagate values from CONST_AND_COPIES into the PHI nodes of the
   successors of BB.  */

static void
cprop_into_successor_phis (basic_block bb)
{
  edge e;
  edge_iterator ei;

  FOR_EACH_EDGE (e, ei, bb->succs)
    {
      int indx;
      gimple_stmt_iterator gsi;

      /* If this is an abnormal edge, then we do not want to copy propagate
	 into the PHI alternative associated with this edge.  */
      if (e->flags & EDGE_ABNORMAL)
	continue;

      gsi = gsi_start_phis (e->dest);
      if (gsi_end_p (gsi))
	continue;

      indx = e->dest_idx;
      for ( ; !gsi_end_p (gsi); gsi_next (&gsi))
	{
	  tree new_val;
	  use_operand_p orig_p;
	  tree orig_val;
          gimple phi = gsi_stmt (gsi);

	  /* The alternative may be associated with a constant, so verify
	     it is an SSA_NAME before doing anything with it.  */
	  orig_p = gimple_phi_arg_imm_use_ptr (phi, indx);
	  orig_val = get_use_from_ptr (orig_p);
	  if (TREE_CODE (orig_val) != SSA_NAME)
	    continue;

	  /* If we have *ORIG_P in our constant/copy table, then replace
	     ORIG_P with its value in our constant/copy table.  */
	  new_val = SSA_NAME_VALUE (orig_val);
	  if (new_val
	      && new_val != orig_val
	      && (TREE_CODE (new_val) == SSA_NAME
		  || is_gimple_min_invariant (new_val))
	      && may_propagate_copy (orig_val, new_val))
	    propagate_value (orig_p, new_val);
	}
    }
}

/* We have finished optimizing BB, record any information implied by
   taking a specific outgoing edge from BB.  */

static void
record_edge_info (basic_block bb)
{
  gimple_stmt_iterator gsi = gsi_last_bb (bb);
  struct edge_info *edge_info;

  if (! gsi_end_p (gsi))
    {
      gimple stmt = gsi_stmt (gsi);

      if (gimple_code (stmt) == GIMPLE_SWITCH)
	{
	  tree index = gimple_switch_index (stmt);

	  if (TREE_CODE (index) == SSA_NAME)
	    {
	      int i;
              int n_labels = gimple_switch_num_labels (stmt);
	      tree *info = XCNEWVEC (tree, last_basic_block);
	      edge e;
	      edge_iterator ei;

	      for (i = 0; i < n_labels; i++)
		{
		  tree label = gimple_switch_label (stmt, i);
		  basic_block target_bb = label_to_block (CASE_LABEL (label));
		  if (CASE_HIGH (label)
		      || !CASE_LOW (label)
		      || info[target_bb->index])
		    info[target_bb->index] = error_mark_node;
		  else
		    info[target_bb->index] = label;
		}

	      FOR_EACH_EDGE (e, ei, bb->succs)
		{
		  basic_block target_bb = e->dest;
		  tree label = info[target_bb->index];

		  if (label != NULL && label != error_mark_node)
		    {
		      tree x = fold_convert (TREE_TYPE (index), CASE_LOW (label));
		      edge_info = allocate_edge_info (e);
		      edge_info->lhs = index;
		      edge_info->rhs = x;
		    }
		}
	      free (info);
	    }
	}

      /* A COND_EXPR may create equivalences too.  */
      if (gimple_code (stmt) == GIMPLE_COND)
	{
	  edge true_edge;
	  edge false_edge;

          tree op0 = gimple_cond_lhs (stmt);
          tree op1 = gimple_cond_rhs (stmt);
          enum tree_code code = gimple_cond_code (stmt);

	  extract_true_false_edges_from_block (bb, &true_edge, &false_edge);

          /* Special case comparing booleans against a constant as we
             know the value of OP0 on both arms of the branch.  i.e., we
             can record an equivalence for OP0 rather than COND.  */
          if ((code == EQ_EXPR || code == NE_EXPR)
              && TREE_CODE (op0) == SSA_NAME
              && TREE_CODE (TREE_TYPE (op0)) == BOOLEAN_TYPE
              && is_gimple_min_invariant (op1))
            {
              if (code == EQ_EXPR)
                {
                  edge_info = allocate_edge_info (true_edge);
                  edge_info->lhs = op0;
                  edge_info->rhs = (integer_zerop (op1)
                                    ? boolean_false_node
                                    : boolean_true_node);

                  edge_info = allocate_edge_info (false_edge);
                  edge_info->lhs = op0;
                  edge_info->rhs = (integer_zerop (op1)
                                    ? boolean_true_node
                                    : boolean_false_node);
                }
              else
                {
                  edge_info = allocate_edge_info (true_edge);
                  edge_info->lhs = op0;
                  edge_info->rhs = (integer_zerop (op1)
                                    ? boolean_true_node
                                    : boolean_false_node);

                  edge_info = allocate_edge_info (false_edge);
                  edge_info->lhs = op0;
                  edge_info->rhs = (integer_zerop (op1)
                                    ? boolean_false_node
                                    : boolean_true_node);
                }
            }
          else if (is_gimple_min_invariant (op0)
                   && (TREE_CODE (op1) == SSA_NAME
                       || is_gimple_min_invariant (op1)))
            {
              tree cond = build2 (code, boolean_type_node, op0, op1);
              tree inverted = invert_truthvalue (cond);
              struct edge_info *edge_info;

              edge_info = allocate_edge_info (true_edge);
              record_conditions (edge_info, cond, inverted);

              if (code == EQ_EXPR)
                {
                  edge_info->lhs = op1;
                  edge_info->rhs = op0;
                }

              edge_info = allocate_edge_info (false_edge);
              record_conditions (edge_info, inverted, cond);

              if (code == NE_EXPR)
                {
                  edge_info->lhs = op1;
                  edge_info->rhs = op0;
                }
            }

          else if (TREE_CODE (op0) == SSA_NAME
                   && (is_gimple_min_invariant (op1)
                       || TREE_CODE (op1) == SSA_NAME))
            {
              tree cond = build2 (code, boolean_type_node, op0, op1);
              tree inverted = invert_truthvalue (cond);
              struct edge_info *edge_info;

              edge_info = allocate_edge_info (true_edge);
              record_conditions (edge_info, cond, inverted);

              if (code == EQ_EXPR)
                {
                  edge_info->lhs = op0;
                  edge_info->rhs = op1;
                }

              edge_info = allocate_edge_info (false_edge);
              record_conditions (edge_info, inverted, cond);

              if (TREE_CODE (cond) == NE_EXPR)
                {
                  edge_info->lhs = op0;
                  edge_info->rhs = op1;
                }
            }
        }

      /* ??? TRUTH_NOT_EXPR can create an equivalence too.  */
    }
}

/* Propagate information from BB to its outgoing edges.

   This can include equivalence information implied by control statements
   at the end of BB and const/copy propagation into PHIs in BB's
   successor blocks.  */

static void
propagate_to_outgoing_edges (struct dom_walk_data *walk_data ATTRIBUTE_UNUSED,
			     basic_block bb)
{
  record_edge_info (bb);
  cprop_into_successor_phis (bb);
}

/* Search for redundant computations in STMT.  If any are found, then
   replace them with the variable holding the result of the computation.

   If safe, record this expression into the available expression hash
   table.  */

static bool
eliminate_redundant_computations (gimple_stmt_iterator* gsi)
{
  tree expr_type;
  tree cached_lhs;
  bool insert = true;
  bool retval = false;
  bool assigns_var_p = false;

  gimple stmt = gsi_stmt (*gsi);

  tree def = gimple_get_lhs (stmt);

  /* Certain expressions on the RHS can be optimized away, but can not
     themselves be entered into the hash tables.  */
  if (! def
      || TREE_CODE (def) != SSA_NAME
      || SSA_NAME_OCCURS_IN_ABNORMAL_PHI (def)
      || !ZERO_SSA_OPERANDS (stmt, SSA_OP_VDEF)
      /* Do not record equivalences for increments of ivs.  This would create
	 overlapping live ranges for a very questionable gain.  */
      || simple_iv_increment_p (stmt))
    insert = false;

  /* Check if the expression has been computed before.  */
  cached_lhs = lookup_avail_expr (stmt, insert);

  opt_stats.num_exprs_considered++;

  /* Get the type of the expression we are trying to optimize.  */
  if (is_gimple_assign (stmt))
    {
      expr_type = TREE_TYPE (gimple_assign_lhs (stmt));
      assigns_var_p = true;
    }
  else if (gimple_code (stmt) == GIMPLE_COND)
    expr_type = boolean_type_node;
  else if (is_gimple_call (stmt))
    {
      gcc_assert (gimple_call_lhs (stmt));
      expr_type = TREE_TYPE (gimple_call_lhs (stmt));
      assigns_var_p = true;
    }
  else if (gimple_code (stmt) == GIMPLE_SWITCH)
    expr_type = TREE_TYPE (gimple_switch_index (stmt));
  else
    gcc_unreachable ();

  if (!cached_lhs)
    return false;

  /* It is safe to ignore types here since we have already done
     type checking in the hashing and equality routines.  In fact
     type checking here merely gets in the way of constant
     propagation.  Also, make sure that it is safe to propagate
     CACHED_LHS into the expression in STMT.  */
  if ((TREE_CODE (cached_lhs) != SSA_NAME
       && (assigns_var_p
           || useless_type_conversion_p (expr_type, TREE_TYPE (cached_lhs))))
      || may_propagate_copy_into_stmt (stmt, cached_lhs))
  {
#if defined ENABLE_CHECKING
      gcc_assert (TREE_CODE (cached_lhs) == SSA_NAME
		  || is_gimple_min_invariant (cached_lhs));
#endif

      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "  Replaced redundant expr '");
	  print_gimple_expr (dump_file, stmt, 0, dump_flags);
	  fprintf (dump_file, "' with '");
	  print_generic_expr (dump_file, cached_lhs, dump_flags);
          fprintf (dump_file, "'\n");
	}

      opt_stats.num_re++;

      if (TREE_CODE (cached_lhs) == ADDR_EXPR
	  || (POINTER_TYPE_P (expr_type)
	      && is_gimple_min_invariant (cached_lhs)))
	retval = true;
      
      if (assigns_var_p
	  && !useless_type_conversion_p (expr_type, TREE_TYPE (cached_lhs)))
	cached_lhs = fold_convert (expr_type, cached_lhs);

      propagate_tree_value_into_stmt (gsi, cached_lhs);

      /* Since it is always necessary to mark the result as modified,
         perhaps we should move this into propagate_tree_value_into_stmt
         itself.  */
      gimple_set_modified (gsi_stmt (*gsi), true);
  }
  return retval;
}

/* Return true if statement GS is an assignment that peforms a useless
   type conversion.  It is is intended to be a tuples analog of function
   tree_ssa_useless_type_conversion.  */

static bool
gimple_assign_unary_useless_conversion_p (gimple gs)
{
  if (is_gimple_assign (gs)
      && (CONVERT_EXPR_CODE_P (gimple_assign_rhs_code (gs))
          || gimple_assign_rhs_code (gs) == VIEW_CONVERT_EXPR
          || gimple_assign_rhs_code (gs) == NON_LVALUE_EXPR))
    {
      tree lhs_type = TREE_TYPE (gimple_assign_lhs (gs));
      tree rhs_type = TREE_TYPE (gimple_assign_rhs1 (gs));
      return useless_type_conversion_p (lhs_type, rhs_type);
    }
  
  return false;
}

/* STMT, a GIMPLE_ASSIGN, may create certain equivalences, in either
   the available expressions table or the const_and_copies table.
   Detect and record those equivalences.  */
/* We handle only very simple copy equivalences here.  The heavy
   lifing is done by eliminate_redundant_computations.  */

static void
record_equivalences_from_stmt (gimple stmt, int may_optimize_p)
{
  tree lhs;
  enum tree_code lhs_code;

  gcc_assert (is_gimple_assign (stmt));

  lhs = gimple_assign_lhs (stmt);
  lhs_code = TREE_CODE (lhs);

  if (lhs_code == SSA_NAME
      && (gimple_assign_single_p (stmt)
          || gimple_assign_unary_useless_conversion_p (stmt)))
    {
      tree rhs = gimple_assign_rhs1 (stmt);
               
      /* Strip away any useless type conversions.  */
      STRIP_USELESS_TYPE_CONVERSION (rhs);

      /* If the RHS of the assignment is a constant or another variable that
	 may be propagated, register it in the CONST_AND_COPIES table.  We
	 do not need to record unwind data for this, since this is a true
	 assignment and not an equivalence inferred from a comparison.  All
	 uses of this ssa name are dominated by this assignment, so unwinding
	 just costs time and space.  */
      if (may_optimize_p
	  && (TREE_CODE (rhs) == SSA_NAME
	      || is_gimple_min_invariant (rhs)))
      {
	if (dump_file && (dump_flags & TDF_DETAILS))
	  {
	    fprintf (dump_file, "==== ASGN ");
	    print_generic_expr (dump_file, lhs, 0);
	    fprintf (dump_file, " = ");
	    print_generic_expr (dump_file, rhs, 0);
	    fprintf (dump_file, "\n");
	  }

	SSA_NAME_VALUE (lhs) = rhs;
      }
    }

  /* A memory store, even an aliased store, creates a useful
     equivalence.  By exchanging the LHS and RHS, creating suitable
     vops and recording the result in the available expression table,
     we may be able to expose more redundant loads.  */
  if (!gimple_has_volatile_ops (stmt)
      && gimple_references_memory_p (stmt)
      && gimple_assign_single_p (stmt)
      && (TREE_CODE (gimple_assign_rhs1 (stmt)) == SSA_NAME
	  || is_gimple_min_invariant (gimple_assign_rhs1 (stmt)))
      && !is_gimple_reg (lhs))
    {
      tree rhs = gimple_assign_rhs1 (stmt);
      gimple new_stmt;

      /* Build a new statement with the RHS and LHS exchanged.  */
      if (TREE_CODE (rhs) == SSA_NAME)
        {
          /* NOTE tuples.  The call to gimple_build_assign below replaced
             a call to build_gimple_modify_stmt, which did not set the
             SSA_NAME_DEF_STMT on the LHS of the assignment.  Doing so
             may cause an SSA validation failure, as the LHS may be a
             default-initialized name and should have no definition.  I'm
             a bit dubious of this, as the artificial statement that we
             generate here may in fact be ill-formed, but it is simply
             used as an internal device in this pass, and never becomes
             part of the CFG.  */
          gimple defstmt = SSA_NAME_DEF_STMT (rhs);
          new_stmt = gimple_build_assign (rhs, lhs);
          SSA_NAME_DEF_STMT (rhs) = defstmt;
        }
      else
        new_stmt = gimple_build_assign (rhs, lhs);

      create_ssa_artificial_load_stmt (new_stmt, stmt, true);

      /* Finally enter the statement into the available expression
	 table.  */
      lookup_avail_expr (new_stmt, true);
    }
}

/* Replace *OP_P in STMT with any known equivalent value for *OP_P from
   CONST_AND_COPIES.  */

static bool
cprop_operand (gimple stmt, use_operand_p op_p)
{
  bool may_have_exposed_new_symbols = false;
  tree val;
  tree op = USE_FROM_PTR (op_p);

  /* If the operand has a known constant value or it is known to be a
     copy of some other variable, use the value or copy stored in
     CONST_AND_COPIES.  */
  val = SSA_NAME_VALUE (op);
  if (val && val != op)
    {
      /* Do not change the base variable in the virtual operand
	 tables.  That would make it impossible to reconstruct
	 the renamed virtual operand if we later modify this
	 statement.  Also only allow the new value to be an SSA_NAME
	 for propagation into virtual operands.  */
      if (!is_gimple_reg (op)
	  && (TREE_CODE (val) != SSA_NAME
	      || is_gimple_reg (val)
	      || get_virtual_var (val) != get_virtual_var (op)))
	return false;

      /* Do not replace hard register operands in asm statements.  */
      if (gimple_code (stmt) == GIMPLE_ASM
	  && !may_propagate_copy_into_asm (op))
	return false;

      /* Certain operands are not allowed to be copy propagated due
	 to their interaction with exception handling and some GCC
	 extensions.  */
      if (!may_propagate_copy (op, val))
	return false;

      /* Do not propagate addresses that point to volatiles into memory
	 stmts without volatile operands.  */
      if (POINTER_TYPE_P (TREE_TYPE (val))
	  && TYPE_VOLATILE (TREE_TYPE (TREE_TYPE (val)))
	  && gimple_has_mem_ops (stmt)
	  && !gimple_has_volatile_ops (stmt))
	return false;

      /* Do not propagate copies if the propagated value is at a deeper loop
	 depth than the propagatee.  Otherwise, this may move loop variant
	 variables outside of their loops and prevent coalescing
	 opportunities.  If the value was loop invariant, it will be hoisted
	 by LICM and exposed for copy propagation.  */
      if (loop_depth_of_name (val) > loop_depth_of_name (op))
	return false;

      /* Dump details.  */
      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "  Replaced '");
	  print_generic_expr (dump_file, op, dump_flags);
	  fprintf (dump_file, "' with %s '",
		   (TREE_CODE (val) != SSA_NAME ? "constant" : "variable"));
	  print_generic_expr (dump_file, val, dump_flags);
	  fprintf (dump_file, "'\n");
	}

      /* If VAL is an ADDR_EXPR or a constant of pointer type, note
	 that we may have exposed a new symbol for SSA renaming.  */
      if (TREE_CODE (val) == ADDR_EXPR
	  || (POINTER_TYPE_P (TREE_TYPE (op))
	      && is_gimple_min_invariant (val)))
	may_have_exposed_new_symbols = true;

      if (TREE_CODE (val) != SSA_NAME)
	opt_stats.num_const_prop++;
      else
	opt_stats.num_copy_prop++;

      propagate_value (op_p, val);

      /* And note that we modified this statement.  This is now
	 safe, even if we changed virtual operands since we will
	 rescan the statement and rewrite its operands again.  */
      gimple_set_modified (stmt, true);
    }
  return may_have_exposed_new_symbols;
}

/* CONST_AND_COPIES is a table which maps an SSA_NAME to the current
   known value for that SSA_NAME (or NULL if no value is known).  

   Propagate values from CONST_AND_COPIES into the uses, vuses and
   vdef_ops of STMT.  */

static bool
cprop_into_stmt (gimple stmt)
{
  bool may_have_exposed_new_symbols = false;
  use_operand_p op_p;
  ssa_op_iter iter;

  FOR_EACH_SSA_USE_OPERAND (op_p, stmt, iter, SSA_OP_ALL_USES)
    {
      if (TREE_CODE (USE_FROM_PTR (op_p)) == SSA_NAME)
	may_have_exposed_new_symbols |= cprop_operand (stmt, op_p);
    }

  return may_have_exposed_new_symbols;
}

/* Optimize the statement pointed to by iterator SI.
   
   We try to perform some simplistic global redundancy elimination and
   constant propagation:

   1- To detect global redundancy, we keep track of expressions that have
      been computed in this block and its dominators.  If we find that the
      same expression is computed more than once, we eliminate repeated
      computations by using the target of the first one.

   2- Constant values and copy assignments.  This is used to do very
      simplistic constant and copy propagation.  When a constant or copy
      assignment is found, we map the value on the RHS of the assignment to
      the variable in the LHS in the CONST_AND_COPIES table.  */

static void
optimize_stmt (struct dom_walk_data *walk_data ATTRIBUTE_UNUSED,
	       basic_block bb, gimple_stmt_iterator si)
{
  gimple stmt, old_stmt;
  bool may_optimize_p;
  bool may_have_exposed_new_symbols;
  bool modified_p = false;

  old_stmt = stmt = gsi_stmt (si);
  
  if (gimple_code (stmt) == GIMPLE_COND)
    canonicalize_comparison (stmt);
  
  update_stmt_if_modified (stmt);
  opt_stats.num_stmts++;
  push_stmt_changes (gsi_stmt_ptr (&si));

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Optimizing statement ");
      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
    }

  /* Const/copy propagate into USES, VUSES and the RHS of VDEFs.  */
  may_have_exposed_new_symbols = cprop_into_stmt (stmt);

  /* If the statement has been modified with constant replacements,
     fold its RHS before checking for redundant computations.  */
  if (gimple_modified_p (stmt))
    {
      tree rhs = NULL;

      /* Try to fold the statement making sure that STMT is kept
	 up to date.  */
      if (fold_stmt (&si))
	{
	  stmt = gsi_stmt (si);

	  if (dump_file && (dump_flags & TDF_DETAILS))
	    {
	      fprintf (dump_file, "  Folded to: ");
	      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
	    }
	}

      /* We only need to consider cases that can yield a gimple operand.  */
      if (gimple_assign_single_p (stmt))
        rhs = gimple_assign_rhs1 (stmt);
      else if (gimple_code (stmt) == GIMPLE_GOTO)
        rhs = gimple_goto_dest (stmt);
      else if (gimple_code (stmt) == GIMPLE_SWITCH)
        /* This should never be an ADDR_EXPR.  */
        rhs = gimple_switch_index (stmt);

      if (rhs && TREE_CODE (rhs) == ADDR_EXPR)
        recompute_tree_invariant_for_addr_expr (rhs);

      /* Constant/copy propagation above may change the set of 
	 virtual operands associated with this statement.  Folding
	 may remove the need for some virtual operands.

	 Indicate we will need to rescan and rewrite the statement.  */
      may_have_exposed_new_symbols = true;
      /* Indicate that maybe_clean_or_replace_eh_stmt needs to be called,
	 even if fold_stmt updated the stmt already and thus cleared
	 gimple_modified_p flag on it.  */
      modified_p = true;
    }

  /* Check for redundant computations.  Do this optimization only
     for assignments that have no volatile ops and conditionals.  */
  may_optimize_p = (!gimple_has_volatile_ops (stmt)
                    && ((is_gimple_assign (stmt)
                         && !gimple_rhs_has_side_effects (stmt))
                        || (is_gimple_call (stmt)
                            && gimple_call_lhs (stmt) != NULL_TREE
                            && !gimple_rhs_has_side_effects (stmt))
                        || gimple_code (stmt) == GIMPLE_COND
                        || gimple_code (stmt) == GIMPLE_SWITCH));

  if (may_optimize_p)
    {
      may_have_exposed_new_symbols |= eliminate_redundant_computations (&si);
      stmt = gsi_stmt (si);
    }

  /* Record any additional equivalences created by this statement.  */
  if (is_gimple_assign (stmt))
    record_equivalences_from_stmt (stmt, may_optimize_p);

  /* If STMT is a COND_EXPR and it was modified, then we may know
     where it goes.  If that is the case, then mark the CFG as altered.

     This will cause us to later call remove_unreachable_blocks and
     cleanup_tree_cfg when it is safe to do so.  It is not safe to 
     clean things up here since removal of edges and such can trigger
     the removal of PHI nodes, which in turn can release SSA_NAMEs to
     the manager.

     That's all fine and good, except that once SSA_NAMEs are released
     to the manager, we must not call create_ssa_name until all references
     to released SSA_NAMEs have been eliminated.

     All references to the deleted SSA_NAMEs can not be eliminated until
     we remove unreachable blocks.

     We can not remove unreachable blocks until after we have completed
     any queued jump threading.

     We can not complete any queued jump threads until we have taken
     appropriate variables out of SSA form.  Taking variables out of
     SSA form can call create_ssa_name and thus we lose.

     Ultimately I suspect we're going to need to change the interface
     into the SSA_NAME manager.  */
  if (gimple_modified_p (stmt) || modified_p)
    {
      tree val = NULL;

      if (gimple_code (stmt) == GIMPLE_COND)
        val = fold_binary (gimple_cond_code (stmt), boolean_type_node,
                           gimple_cond_lhs (stmt),  gimple_cond_rhs (stmt));
      else if (gimple_code (stmt) == GIMPLE_SWITCH)
	val = gimple_switch_index (stmt);

      if (val && TREE_CODE (val) == INTEGER_CST && find_taken_edge (bb, val))
	cfg_altered = true;

      /* If we simplified a statement in such a way as to be shown that it
	 cannot trap, update the eh information and the cfg to match.  */
      if (maybe_clean_or_replace_eh_stmt (old_stmt, stmt))
	{
	  bitmap_set_bit (need_eh_cleanup, bb->index);
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    fprintf (dump_file, "  Flagged to clear EH edges.\n");
	}
    }

  if (may_have_exposed_new_symbols)
    {
      /* Queue the statement to be re-scanned after all the
	 AVAIL_EXPRS have been processed.  The change buffer stack for
	 all the pushed statements will be processed when this queue
	 is emptied.  */
      VEC_safe_push (gimple_p, heap, stmts_to_rescan, gsi_stmt_ptr (&si));
    }
  else
    {
      /* Otherwise, just discard the recently pushed change buffer.  If
	 not, the STMTS_TO_RESCAN queue will get out of synch with the
	 change buffer stack.  */
      discard_stmt_changes (gsi_stmt_ptr (&si));
    }
}

/* Search for an existing instance of STMT in the AVAIL_EXPRS table.
   If found, return its LHS. Otherwise insert STMT in the table and
   return NULL_TREE.

   Also, when an expression is first inserted in the  table, it is also
   is also added to AVAIL_EXPRS_STACK, so that it can be removed when
   we finish processing this block and its children.  */

static tree
lookup_avail_expr (gimple stmt, bool insert)
{
  void **slot;
  tree lhs;
  tree temp;
  struct expr_hash_elt *element = XNEW (struct expr_hash_elt);

  /* Get LHS of assignment or call, else NULL_TREE.  */
  lhs = gimple_get_lhs (stmt);

  initialize_hash_element (stmt, lhs, element);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "LKUP ");
      print_expr_hash_elt (dump_file, element);
    }

  /* Don't bother remembering constant assignments and copy operations.
     Constants and copy operations are handled by the constant/copy propagator
     in optimize_stmt.  */
  if (element->expr.kind == EXPR_SINGLE
      && (TREE_CODE (element->expr.ops.single.rhs) == SSA_NAME
          || is_gimple_min_invariant (element->expr.ops.single.rhs)))
    {
      free (element);
      return NULL_TREE;
    }

  /* Finally try to find the expression in the main expression hash table.  */
  slot = htab_find_slot_with_hash (avail_exprs, element, element->hash,
				   (insert ? INSERT : NO_INSERT));
  if (slot == NULL)
    {
      free (element);
      return NULL_TREE;  
    }

  if (*slot == NULL)
    {
      *slot = (void *) element;

      if (dump_file && (dump_flags & TDF_DETAILS))
        {
          fprintf (dump_file, "2>>> ");
          print_expr_hash_elt (dump_file, element);
        }

      VEC_safe_push (expr_hash_elt_t, heap, avail_exprs_stack, element);
      return NULL_TREE;
    }

  /* Extract the LHS of the assignment so that it can be used as the current
     definition of another variable.  */
  lhs = ((struct expr_hash_elt *)*slot)->lhs;

  /* See if the LHS appears in the CONST_AND_COPIES table.  If it does, then
     use the value from the const_and_copies table.  */
  if (TREE_CODE (lhs) == SSA_NAME)
    {
      temp = SSA_NAME_VALUE (lhs);
      if (temp)
	lhs = temp;
    }

  free (element);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "FIND: ");
      print_generic_expr (dump_file, lhs, 0);
      fprintf (dump_file, "\n");
    }

  return lhs;
}

/* Hashing and equality functions for AVAIL_EXPRS.  We compute a value number
   for expressions using the code of the expression and the SSA numbers of
   its operands.  */

static hashval_t
avail_expr_hash (const void *p)
{
  gimple stmt = ((const struct expr_hash_elt *)p)->stmt;
  const struct hashable_expr *expr = &((const struct expr_hash_elt *)p)->expr;
  tree vuse;
  ssa_op_iter iter;
  hashval_t val = 0;

  val = iterative_hash_hashable_expr (expr, val);

  /* If the hash table entry is not associated with a statement, then we
     can just hash the expression and not worry about virtual operands
     and such.  */
  if (!stmt)
    return val;

  /* Add the SSA version numbers of every vuse operand.  This is important
     because compound variables like arrays are not renamed in the
     operands.  Rather, the rename is done on the virtual variable
     representing all the elements of the array.  */
  FOR_EACH_SSA_TREE_OPERAND (vuse, stmt, iter, SSA_OP_VUSE)
    val = iterative_hash_expr (vuse, val);

  return val;
}

static hashval_t
real_avail_expr_hash (const void *p)
{
  return ((const struct expr_hash_elt *)p)->hash;
}

static int
avail_expr_eq (const void *p1, const void *p2)
{
  gimple stmt1 = ((const struct expr_hash_elt *)p1)->stmt;
  const struct hashable_expr *expr1 = &((const struct expr_hash_elt *)p1)->expr;
  const struct expr_hash_elt *stamp1 = ((const struct expr_hash_elt *)p1)->stamp;
  gimple stmt2 = ((const struct expr_hash_elt *)p2)->stmt;
  const struct hashable_expr *expr2 = &((const struct expr_hash_elt *)p2)->expr;
  const struct expr_hash_elt *stamp2 = ((const struct expr_hash_elt *)p2)->stamp;

  /* This case should apply only when removing entries from the table.  */
  if (stamp1 == stamp2)
    return true;

  /* FIXME tuples:
     We add stmts to a hash table and them modify them. To detect the case
     that we modify a stmt and then search for it, we assume that the hash
     is always modified by that change.
     We have to fully check why this doesn't happen on trunk or rewrite
     this in a more  reliable (and easier to understand) way. */
  if (((const struct expr_hash_elt *)p1)->hash
      != ((const struct expr_hash_elt *)p2)->hash)
    return false;

  /* In case of a collision, both RHS have to be identical and have the
     same VUSE operands.  */
  if (hashable_expr_equal_p (expr1, expr2)
      && types_compatible_p (expr1->type, expr2->type))
    {
      /* Note that STMT1 and/or STMT2 may be NULL.  */
      bool ret = compare_ssa_operands_equal (stmt1, stmt2, SSA_OP_VUSE);
      return ret;
    }

  return false;
}

/* PHI-ONLY copy and constant propagation.  This pass is meant to clean
   up degenerate PHIs created by or exposed by jump threading.  */

/* Given PHI, return its RHS if the PHI is a degenerate, otherwise return
   NULL.  */

static tree
degenerate_phi_result (gimple phi)
{
  tree lhs = gimple_phi_result (phi);
  tree val = NULL;
  size_t i;

  /* Ignoring arguments which are the same as LHS, if all the remaining
     arguments are the same, then the PHI is a degenerate and has the
     value of that common argument.  */
  for (i = 0; i < gimple_phi_num_args (phi); i++)
    {
      tree arg = gimple_phi_arg_def (phi, i);

      if (arg == lhs)
	continue;
      else if (!arg)
	break;
      else if (!val)
	val = arg;
      else if (!operand_equal_p (arg, val, 0))
	break;
    }
  return (i == gimple_phi_num_args (phi) ? val : NULL);
}

/* Given a statement STMT, which is either a PHI node or an assignment,
   remove it from the IL.  */

static void
remove_stmt_or_phi (gimple stmt)
{
  gimple_stmt_iterator gsi = gsi_for_stmt (stmt);

  if (gimple_code (stmt) == GIMPLE_PHI)
    remove_phi_node (&gsi, true);
  else
    {
      gsi_remove (&gsi, true);
      release_defs (stmt);
    }
}

/* Given a statement STMT, which is either a PHI node or an assignment,
   return the "rhs" of the node, in the case of a non-degenerate
   phi, NULL is returned.  */

static tree
get_rhs_or_phi_arg (gimple stmt)
{
  if (gimple_code (stmt) == GIMPLE_PHI)
    return degenerate_phi_result (stmt);
  else if (gimple_assign_single_p (stmt))
    return gimple_assign_rhs1 (stmt);
  else
    gcc_unreachable ();
}


/* Given a statement STMT, which is either a PHI node or an assignment,
   return the "lhs" of the node.  */

static tree
get_lhs_or_phi_result (gimple stmt)
{
  if (gimple_code (stmt) == GIMPLE_PHI)
    return gimple_phi_result (stmt);
  else if (is_gimple_assign (stmt))
    return gimple_assign_lhs (stmt);
  else
    gcc_unreachable ();
}

/* Propagate RHS into all uses of LHS (when possible).

   RHS and LHS are derived from STMT, which is passed in solely so
   that we can remove it if propagation is successful.

   When propagating into a PHI node or into a statement which turns
   into a trivial copy or constant initialization, set the
   appropriate bit in INTERESTING_NAMEs so that we will visit those
   nodes as well in an effort to pick up secondary optimization
   opportunities.  */

static void 
propagate_rhs_into_lhs (gimple stmt, tree lhs, tree rhs, bitmap interesting_names)
{
  /* First verify that propagation is valid and isn't going to move a
     loop variant variable outside its loop.  */
  if (! SSA_NAME_OCCURS_IN_ABNORMAL_PHI (lhs)
      && (TREE_CODE (rhs) != SSA_NAME
	  || ! SSA_NAME_OCCURS_IN_ABNORMAL_PHI (rhs))
      && may_propagate_copy (lhs, rhs)
      && loop_depth_of_name (lhs) >= loop_depth_of_name (rhs))
    {
      use_operand_p use_p;
      imm_use_iterator iter;
      gimple use_stmt;
      bool all = true;

      /* Dump details.  */
      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "  Replacing '");
	  print_generic_expr (dump_file, lhs, dump_flags);
	  fprintf (dump_file, "' with %s '",
	           (TREE_CODE (rhs) != SSA_NAME ? "constant" : "variable"));
		   print_generic_expr (dump_file, rhs, dump_flags);
	  fprintf (dump_file, "'\n");
	}

      /* Walk over every use of LHS and try to replace the use with RHS. 
	 At this point the only reason why such a propagation would not
	 be successful would be if the use occurs in an ASM_EXPR.  */
      FOR_EACH_IMM_USE_STMT (use_stmt, iter, lhs)
	{
	
	  /* It's not always safe to propagate into an ASM_EXPR.  */
	  if (gimple_code (use_stmt) == GIMPLE_ASM
              && ! may_propagate_copy_into_asm (lhs))
	    {
	      all = false;
	      continue;
	    }

	  /* Dump details.  */
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    {
	      fprintf (dump_file, "    Original statement:");
	      print_gimple_stmt (dump_file, use_stmt, 0, dump_flags);
	    }

	  push_stmt_changes (&use_stmt);

	  /* Propagate the RHS into this use of the LHS.  */
	  FOR_EACH_IMM_USE_ON_STMT (use_p, iter)
	    propagate_value (use_p, rhs);

	  /* Special cases to avoid useless calls into the folding
	     routines, operand scanning, etc.

	     First, propagation into a PHI may cause the PHI to become
	     a degenerate, so mark the PHI as interesting.  No other
	     actions are necessary.

	     Second, if we're propagating a virtual operand and the
	     propagation does not change the underlying _DECL node for
	     the virtual operand, then no further actions are necessary.  */
	  if (gimple_code (use_stmt) == GIMPLE_PHI
	      || (! is_gimple_reg (lhs)
		  && TREE_CODE (rhs) == SSA_NAME
		  && SSA_NAME_VAR (lhs) == SSA_NAME_VAR (rhs)))
	    {
	      /* Dump details.  */
	      if (dump_file && (dump_flags & TDF_DETAILS))
		{
		  fprintf (dump_file, "    Updated statement:");
		  print_gimple_stmt (dump_file, use_stmt, 0, dump_flags);
		}

	      /* Propagation into a PHI may expose new degenerate PHIs,
		 so mark the result of the PHI as interesting.  */
	      if (gimple_code (use_stmt) == GIMPLE_PHI)
		{
		  tree result = get_lhs_or_phi_result (use_stmt);
		  bitmap_set_bit (interesting_names, SSA_NAME_VERSION (result));
		}

	      discard_stmt_changes (&use_stmt);
	      continue;
	    }

	  /* From this point onward we are propagating into a 
	     real statement.  Folding may (or may not) be possible,
	     we may expose new operands, expose dead EH edges,
	     etc.  */
          /* NOTE tuples. In the tuples world, fold_stmt_inplace
             cannot fold a call that simplifies to a constant,
             because the GIMPLE_CALL must be replaced by a
             GIMPLE_ASSIGN, and there is no way to effect such a
             transformation in-place.  We might want to consider
             using the more general fold_stmt here.  */
	  fold_stmt_inplace (use_stmt);

	  /* Sometimes propagation can expose new operands to the
	     renamer.  Note this will call update_stmt at the 
	     appropriate time.  */
	  pop_stmt_changes (&use_stmt);

	  /* Dump details.  */
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    {
	      fprintf (dump_file, "    Updated statement:");
	      print_gimple_stmt (dump_file, use_stmt, 0, dump_flags);
	    }

	  /* If we replaced a variable index with a constant, then
	     we would need to update the invariant flag for ADDR_EXPRs.  */
          if (gimple_assign_single_p (use_stmt)
              && TREE_CODE (gimple_assign_rhs1 (use_stmt)) == ADDR_EXPR)
	    recompute_tree_invariant_for_addr_expr
                (gimple_assign_rhs1 (use_stmt));

	  /* If we cleaned up EH information from the statement,
	     mark its containing block as needing EH cleanups.  */
	  if (maybe_clean_or_replace_eh_stmt (use_stmt, use_stmt))
	    {
	      bitmap_set_bit (need_eh_cleanup, gimple_bb (use_stmt)->index);
	      if (dump_file && (dump_flags & TDF_DETAILS))
		fprintf (dump_file, "  Flagged to clear EH edges.\n");
	    }

	  /* Propagation may expose new trivial copy/constant propagation
	     opportunities.  */
          if (gimple_assign_single_p (use_stmt)
              && TREE_CODE (gimple_assign_lhs (use_stmt)) == SSA_NAME
              && (TREE_CODE (gimple_assign_rhs1 (use_stmt)) == SSA_NAME
                  || is_gimple_min_invariant (gimple_assign_rhs1 (use_stmt))))
            {
	      tree result = get_lhs_or_phi_result (use_stmt);
	      bitmap_set_bit (interesting_names, SSA_NAME_VERSION (result));
	    }

	  /* Propagation into these nodes may make certain edges in
	     the CFG unexecutable.  We want to identify them as PHI nodes
	     at the destination of those unexecutable edges may become
	     degenerates.  */
	  else if (gimple_code (use_stmt) == GIMPLE_COND
		   || gimple_code (use_stmt) == GIMPLE_SWITCH
		   || gimple_code (use_stmt) == GIMPLE_GOTO)
            {
	      tree val;

	      if (gimple_code (use_stmt) == GIMPLE_COND)
                val = fold_binary (gimple_cond_code (use_stmt),
                                   boolean_type_node,
                                   gimple_cond_lhs (use_stmt),
                                   gimple_cond_rhs (use_stmt));
              else if (gimple_code (use_stmt) == GIMPLE_SWITCH)
		val = gimple_switch_index (use_stmt);
	      else
		val = gimple_goto_dest  (use_stmt);

	      if (val && is_gimple_min_invariant (val))
		{
		  basic_block bb = gimple_bb (use_stmt);
		  edge te = find_taken_edge (bb, val);
		  edge_iterator ei;
		  edge e;
		  gimple_stmt_iterator gsi, psi;

		  /* Remove all outgoing edges except TE.  */
		  for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei));)
		    {
		      if (e != te)
			{
			  /* Mark all the PHI nodes at the destination of
			     the unexecutable edge as interesting.  */
                          for (psi = gsi_start_phis (e->dest);
                               !gsi_end_p (psi);
                               gsi_next (&psi))
                            {
                              gimple phi = gsi_stmt (psi);

			      tree result = gimple_phi_result (phi);
			      int version = SSA_NAME_VERSION (result);

			      bitmap_set_bit (interesting_names, version);
			    }

			  te->probability += e->probability;

			  te->count += e->count;
			  remove_edge (e);
			  cfg_altered = true;
			}
		      else
			ei_next (&ei);
		    }

		  gsi = gsi_last_bb (gimple_bb (use_stmt));
		  gsi_remove (&gsi, true);

		  /* And fixup the flags on the single remaining edge.  */
		  te->flags &= ~(EDGE_TRUE_VALUE | EDGE_FALSE_VALUE);
		  te->flags &= ~EDGE_ABNORMAL;
		  te->flags |= EDGE_FALLTHRU;
		  if (te->probability > REG_BR_PROB_BASE)
		    te->probability = REG_BR_PROB_BASE;
	        }
	    }
	}

      /* Ensure there is nothing else to do. */ 
      gcc_assert (!all || has_zero_uses (lhs));

      /* If we were able to propagate away all uses of LHS, then
	 we can remove STMT.  */
      if (all)
	remove_stmt_or_phi (stmt);
    }
}

/* STMT is either a PHI node (potentially a degenerate PHI node) or
   a statement that is a trivial copy or constant initialization.

   Attempt to eliminate T by propagating its RHS into all uses of
   its LHS.  This may in turn set new bits in INTERESTING_NAMES
   for nodes we want to revisit later.

   All exit paths should clear INTERESTING_NAMES for the result
   of STMT.  */

static void
eliminate_const_or_copy (gimple stmt, bitmap interesting_names)
{
  tree lhs = get_lhs_or_phi_result (stmt);
  tree rhs;
  int version = SSA_NAME_VERSION (lhs);

  /* If the LHS of this statement or PHI has no uses, then we can
     just eliminate it.  This can occur if, for example, the PHI
     was created by block duplication due to threading and its only
     use was in the conditional at the end of the block which was
     deleted.  */
  if (has_zero_uses (lhs))
    {
      bitmap_clear_bit (interesting_names, version);
      remove_stmt_or_phi (stmt);
      return;
    }

  /* Get the RHS of the assignment or PHI node if the PHI is a
     degenerate.  */
  rhs = get_rhs_or_phi_arg (stmt);
  if (!rhs)
    {
      bitmap_clear_bit (interesting_names, version);
      return;
    }

  propagate_rhs_into_lhs (stmt, lhs, rhs, interesting_names);

  /* Note that STMT may well have been deleted by now, so do
     not access it, instead use the saved version # to clear
     T's entry in the worklist.  */
  bitmap_clear_bit (interesting_names, version);
}

/* The first phase in degenerate PHI elimination.

   Eliminate the degenerate PHIs in BB, then recurse on the
   dominator children of BB.  */

static void
eliminate_degenerate_phis_1 (basic_block bb, bitmap interesting_names)
{
  gimple_stmt_iterator gsi;
  basic_block son;

  for (gsi = gsi_start_phis (bb); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gimple phi = gsi_stmt (gsi);

      eliminate_const_or_copy (phi, interesting_names);
    }

  /* Recurse into the dominator children of BB.  */
  for (son = first_dom_son (CDI_DOMINATORS, bb);
       son;
       son = next_dom_son (CDI_DOMINATORS, son))
    eliminate_degenerate_phis_1 (son, interesting_names);
}


/* A very simple pass to eliminate degenerate PHI nodes from the
   IL.  This is meant to be fast enough to be able to be run several
   times in the optimization pipeline.

   Certain optimizations, particularly those which duplicate blocks
   or remove edges from the CFG can create or expose PHIs which are
   trivial copies or constant initializations.

   While we could pick up these optimizations in DOM or with the
   combination of copy-prop and CCP, those solutions are far too
   heavy-weight for our needs.

   This implementation has two phases so that we can efficiently
   eliminate the first order degenerate PHIs and second order
   degenerate PHIs.

   The first phase performs a dominator walk to identify and eliminate
   the vast majority of the degenerate PHIs.  When a degenerate PHI
   is identified and eliminated any affected statements or PHIs
   are put on a worklist.

   The second phase eliminates degenerate PHIs and trivial copies
   or constant initializations using the worklist.  This is how we
   pick up the secondary optimization opportunities with minimal
   cost.  */

static unsigned int
eliminate_degenerate_phis (void)
{
  bitmap interesting_names;
  bitmap interesting_names1;

  /* Bitmap of blocks which need EH information updated.  We can not
     update it on-the-fly as doing so invalidates the dominator tree.  */
  need_eh_cleanup = BITMAP_ALLOC (NULL);

  /* INTERESTING_NAMES is effectively our worklist, indexed by
     SSA_NAME_VERSION.

     A set bit indicates that the statement or PHI node which
     defines the SSA_NAME should be (re)examined to determine if
     it has become a degenerate PHI or trivial const/copy propagation
     opportunity. 

     Experiments have show we generally get better compilation
     time behavior with bitmaps rather than sbitmaps.  */
  interesting_names = BITMAP_ALLOC (NULL);
  interesting_names1 = BITMAP_ALLOC (NULL);

  calculate_dominance_info (CDI_DOMINATORS);
  cfg_altered = false;

  /* First phase.  Eliminate degenerate PHIs via a dominator
     walk of the CFG.

     Experiments have indicated that we generally get better
     compile-time behavior by visiting blocks in the first
     phase in dominator order.  Presumably this is because walking
     in dominator order leaves fewer PHIs for later examination
     by the worklist phase.  */
  eliminate_degenerate_phis_1 (ENTRY_BLOCK_PTR, interesting_names);

  /* Second phase.  Eliminate second order degenerate PHIs as well
     as trivial copies or constant initializations identified by
     the first phase or this phase.  Basically we keep iterating
     until our set of INTERESTING_NAMEs is empty.   */
  while (!bitmap_empty_p (interesting_names))
    {
      unsigned int i;
      bitmap_iterator bi;

      /* EXECUTE_IF_SET_IN_BITMAP does not like its bitmap
	 changed during the loop.  Copy it to another bitmap and
	 use that.  */
      bitmap_copy (interesting_names1, interesting_names);

      EXECUTE_IF_SET_IN_BITMAP (interesting_names1, 0, i, bi)
	{
	  tree name = ssa_name (i);

	  /* Ignore SSA_NAMEs that have been released because
	     their defining statement was deleted (unreachable).  */
	  if (name)
	    eliminate_const_or_copy (SSA_NAME_DEF_STMT (ssa_name (i)),
				     interesting_names);
	}
    }

  if (cfg_altered)
    free_dominance_info (CDI_DOMINATORS);

  /* Propagation of const and copies may make some EH edges dead.  Purge
     such edges from the CFG as needed.  */
  if (!bitmap_empty_p (need_eh_cleanup))
    {
      gimple_purge_all_dead_eh_edges (need_eh_cleanup);
      BITMAP_FREE (need_eh_cleanup);
    }

  BITMAP_FREE (interesting_names);
  BITMAP_FREE (interesting_names1);
  return 0;
}

struct gimple_opt_pass pass_phi_only_cprop =
{
 {
  GIMPLE_PASS,
  "phicprop",                           /* name */
  gate_dominator,                       /* gate */
  eliminate_degenerate_phis,            /* execute */
  NULL,                                 /* sub */
  NULL,                                 /* next */
  0,                                    /* static_pass_number */
  TV_TREE_PHI_CPROP,                    /* tv_id */
  PROP_cfg | PROP_ssa | PROP_alias,     /* properties_required */
  0,                                    /* properties_provided */
  0,		                        /* properties_destroyed */
  0,                                    /* todo_flags_start */
  TODO_cleanup_cfg
    | TODO_dump_func 
    | TODO_ggc_collect
    | TODO_verify_ssa
    | TODO_verify_stmts
    | TODO_update_ssa			/* todo_flags_finish */
 }
};
