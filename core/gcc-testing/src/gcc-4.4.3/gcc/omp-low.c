/* Lowering pass for OpenMP directives.  Converts OpenMP directives
   into explicit calls to the runtime library (libgomp) and data
   marshalling to implement data sharing and copying clauses.
   Contributed by Diego Novillo <dnovillo@redhat.com>

   Copyright (C) 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.

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

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "rtl.h"
#include "gimple.h"
#include "tree-iterator.h"
#include "tree-inline.h"
#include "langhooks.h"
#include "diagnostic.h"
#include "tree-flow.h"
#include "timevar.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "toplev.h"
#include "tree-pass.h"
#include "ggc.h"
#include "except.h"
#include "splay-tree.h"
#include "optabs.h"
#include "cfgloop.h"


/* Lowering of OpenMP parallel and workshare constructs proceeds in two 
   phases.  The first phase scans the function looking for OMP statements
   and then for variables that must be replaced to satisfy data sharing
   clauses.  The second phase expands code for the constructs, as well as
   re-gimplifying things when variables have been replaced with complex
   expressions.

   Final code generation is done by pass_expand_omp.  The flowgraph is
   scanned for parallel regions which are then moved to a new
   function, to be invoked by the thread library.  */

/* Context structure.  Used to store information about each parallel
   directive in the code.  */

typedef struct omp_context
{
  /* This field must be at the beginning, as we do "inheritance": Some
     callback functions for tree-inline.c (e.g., omp_copy_decl)
     receive a copy_body_data pointer that is up-casted to an
     omp_context pointer.  */
  copy_body_data cb;

  /* The tree of contexts corresponding to the encountered constructs.  */
  struct omp_context *outer;
  gimple stmt;

  /* Map variables to fields in a structure that allows communication 
     between sending and receiving threads.  */
  splay_tree field_map;
  tree record_type;
  tree sender_decl;
  tree receiver_decl;

  /* These are used just by task contexts, if task firstprivate fn is
     needed.  srecord_type is used to communicate from the thread
     that encountered the task construct to task firstprivate fn,
     record_type is allocated by GOMP_task, initialized by task firstprivate
     fn and passed to the task body fn.  */
  splay_tree sfield_map;
  tree srecord_type;

  /* A chain of variables to add to the top-level block surrounding the
     construct.  In the case of a parallel, this is in the child function.  */
  tree block_vars;

  /* What to do with variables with implicitly determined sharing
     attributes.  */
  enum omp_clause_default_kind default_kind;

  /* Nesting depth of this context.  Used to beautify error messages re
     invalid gotos.  The outermost ctx is depth 1, with depth 0 being
     reserved for the main body of the function.  */
  int depth;

  /* True if this parallel directive is nested within another.  */
  bool is_nested;
} omp_context;


struct omp_for_data_loop
{
  tree v, n1, n2, step;
  enum tree_code cond_code;
};

/* A structure describing the main elements of a parallel loop.  */

struct omp_for_data
{
  struct omp_for_data_loop loop;
  tree chunk_size;
  gimple for_stmt;
  tree pre, iter_type;
  int collapse;
  bool have_nowait, have_ordered;
  enum omp_clause_schedule_kind sched_kind;
  struct omp_for_data_loop *loops;
};


static splay_tree all_contexts;
static int taskreg_nesting_level;
struct omp_region *root_omp_region;
static bitmap task_shared_vars;

static void scan_omp (gimple_seq, omp_context *);
static tree scan_omp_1_op (tree *, int *, void *);

#define WALK_SUBSTMTS  \
    case GIMPLE_BIND: \
    case GIMPLE_TRY: \
    case GIMPLE_CATCH: \
    case GIMPLE_EH_FILTER: \
      /* The sub-statements for these should be walked.  */ \
      *handled_ops_p = false; \
      break;

/* Convenience function for calling scan_omp_1_op on tree operands.  */

static inline tree
scan_omp_op (tree *tp, omp_context *ctx)
{
  struct walk_stmt_info wi;

  memset (&wi, 0, sizeof (wi));
  wi.info = ctx;
  wi.want_locations = true;

  return walk_tree (tp, scan_omp_1_op, &wi, NULL);
}

static void lower_omp (gimple_seq, omp_context *);
static tree lookup_decl_in_outer_ctx (tree, omp_context *);
static tree maybe_lookup_decl_in_outer_ctx (tree, omp_context *);

/* Find an OpenMP clause of type KIND within CLAUSES.  */

tree
find_omp_clause (tree clauses, enum omp_clause_code kind)
{
  for (; clauses ; clauses = OMP_CLAUSE_CHAIN (clauses))
    if (OMP_CLAUSE_CODE (clauses) == kind)
      return clauses;

  return NULL_TREE;
}

/* Return true if CTX is for an omp parallel.  */

static inline bool
is_parallel_ctx (omp_context *ctx)
{
  return gimple_code (ctx->stmt) == GIMPLE_OMP_PARALLEL;
}


/* Return true if CTX is for an omp task.  */

static inline bool
is_task_ctx (omp_context *ctx)
{
  return gimple_code (ctx->stmt) == GIMPLE_OMP_TASK;
}


/* Return true if CTX is for an omp parallel or omp task.  */

static inline bool
is_taskreg_ctx (omp_context *ctx)
{
  return gimple_code (ctx->stmt) == GIMPLE_OMP_PARALLEL
	 || gimple_code (ctx->stmt) == GIMPLE_OMP_TASK;
}


/* Return true if REGION is a combined parallel+workshare region.  */

static inline bool
is_combined_parallel (struct omp_region *region)
{
  return region->is_combined_parallel;
}


/* Extract the header elements of parallel loop FOR_STMT and store
   them into *FD.  */

static void
extract_omp_for_data (gimple for_stmt, struct omp_for_data *fd,
		      struct omp_for_data_loop *loops)
{
  tree t, var, *collapse_iter, *collapse_count;
  tree count = NULL_TREE, iter_type = long_integer_type_node;
  struct omp_for_data_loop *loop;
  int i;
  struct omp_for_data_loop dummy_loop;

  fd->for_stmt = for_stmt;
  fd->pre = NULL;
  fd->collapse = gimple_omp_for_collapse (for_stmt);
  if (fd->collapse > 1)
    fd->loops = loops;
  else
    fd->loops = &fd->loop;

  fd->have_nowait = fd->have_ordered = false;
  fd->sched_kind = OMP_CLAUSE_SCHEDULE_STATIC;
  fd->chunk_size = NULL_TREE;
  collapse_iter = NULL;
  collapse_count = NULL;

  for (t = gimple_omp_for_clauses (for_stmt); t ; t = OMP_CLAUSE_CHAIN (t))
    switch (OMP_CLAUSE_CODE (t))
      {
      case OMP_CLAUSE_NOWAIT:
	fd->have_nowait = true;
	break;
      case OMP_CLAUSE_ORDERED:
	fd->have_ordered = true;
	break;
      case OMP_CLAUSE_SCHEDULE:
	fd->sched_kind = OMP_CLAUSE_SCHEDULE_KIND (t);
	fd->chunk_size = OMP_CLAUSE_SCHEDULE_CHUNK_EXPR (t);
	break;
      case OMP_CLAUSE_COLLAPSE:
	if (fd->collapse > 1)
	  {
	    collapse_iter = &OMP_CLAUSE_COLLAPSE_ITERVAR (t);
	    collapse_count = &OMP_CLAUSE_COLLAPSE_COUNT (t);
	  }
      default:
	break;
      }

  /* FIXME: for now map schedule(auto) to schedule(static).
     There should be analysis to determine whether all iterations
     are approximately the same amount of work (then schedule(static)
     is best) or if it varies (then schedule(dynamic,N) is better).  */
  if (fd->sched_kind == OMP_CLAUSE_SCHEDULE_AUTO)
    {
      fd->sched_kind = OMP_CLAUSE_SCHEDULE_STATIC;
      gcc_assert (fd->chunk_size == NULL);
    }
  gcc_assert (fd->collapse == 1 || collapse_iter != NULL);
  if (fd->sched_kind == OMP_CLAUSE_SCHEDULE_RUNTIME)
    gcc_assert (fd->chunk_size == NULL);
  else if (fd->chunk_size == NULL)
    {
      /* We only need to compute a default chunk size for ordered
	 static loops and dynamic loops.  */
      if (fd->sched_kind != OMP_CLAUSE_SCHEDULE_STATIC
	  || fd->have_ordered
	  || fd->collapse > 1)
	fd->chunk_size = (fd->sched_kind == OMP_CLAUSE_SCHEDULE_STATIC)
			 ? integer_zero_node : integer_one_node;
    }

  for (i = 0; i < fd->collapse; i++)
    {
      if (fd->collapse == 1)
	loop = &fd->loop;
      else if (loops != NULL)
	loop = loops + i;
      else
	loop = &dummy_loop;

      
      loop->v = gimple_omp_for_index (for_stmt, i);
      gcc_assert (SSA_VAR_P (loop->v));
      gcc_assert (TREE_CODE (TREE_TYPE (loop->v)) == INTEGER_TYPE
		  || TREE_CODE (TREE_TYPE (loop->v)) == POINTER_TYPE);
      var = TREE_CODE (loop->v) == SSA_NAME ? SSA_NAME_VAR (loop->v) : loop->v;
      loop->n1 = gimple_omp_for_initial (for_stmt, i);

      loop->cond_code = gimple_omp_for_cond (for_stmt, i);
      loop->n2 = gimple_omp_for_final (for_stmt, i);
      switch (loop->cond_code)
	{
	case LT_EXPR:
	case GT_EXPR:
	  break;
	case LE_EXPR:
	  if (POINTER_TYPE_P (TREE_TYPE (loop->n2)))
	    loop->n2 = fold_build2 (POINTER_PLUS_EXPR, TREE_TYPE (loop->n2),
				    loop->n2, size_one_node);
	  else
	    loop->n2 = fold_build2 (PLUS_EXPR, TREE_TYPE (loop->n2), loop->n2,
				    build_int_cst (TREE_TYPE (loop->n2), 1));
	  loop->cond_code = LT_EXPR;
	  break;
	case GE_EXPR:
	  if (POINTER_TYPE_P (TREE_TYPE (loop->n2)))
	    loop->n2 = fold_build2 (POINTER_PLUS_EXPR, TREE_TYPE (loop->n2),
				    loop->n2, size_int (-1));
	  else
	    loop->n2 = fold_build2 (MINUS_EXPR, TREE_TYPE (loop->n2), loop->n2,
				    build_int_cst (TREE_TYPE (loop->n2), 1));
	  loop->cond_code = GT_EXPR;
	  break;
	default:
	  gcc_unreachable ();
	}

      t = gimple_omp_for_incr (for_stmt, i);
      gcc_assert (TREE_OPERAND (t, 0) == var);
      switch (TREE_CODE (t))
	{
	case PLUS_EXPR:
	case POINTER_PLUS_EXPR:
	  loop->step = TREE_OPERAND (t, 1);
	  break;
	case MINUS_EXPR:
	  loop->step = TREE_OPERAND (t, 1);
	  loop->step = fold_build1 (NEGATE_EXPR, TREE_TYPE (loop->step),
				    loop->step);
	  break;
	default:
	  gcc_unreachable ();
	}

      if (iter_type != long_long_unsigned_type_node)
	{
	  if (POINTER_TYPE_P (TREE_TYPE (loop->v)))
	    iter_type = long_long_unsigned_type_node;
	  else if (TYPE_UNSIGNED (TREE_TYPE (loop->v))
		   && TYPE_PRECISION (TREE_TYPE (loop->v))
		      >= TYPE_PRECISION (iter_type))
	    {
	      tree n;

	      if (loop->cond_code == LT_EXPR)
		n = fold_build2 (PLUS_EXPR, TREE_TYPE (loop->v),
				 loop->n2, loop->step);
	      else
		n = loop->n1;
	      if (TREE_CODE (n) != INTEGER_CST
		  || tree_int_cst_lt (TYPE_MAX_VALUE (iter_type), n))
		iter_type = long_long_unsigned_type_node;
	    }
	  else if (TYPE_PRECISION (TREE_TYPE (loop->v))
		   > TYPE_PRECISION (iter_type))
	    {
	      tree n1, n2;

	      if (loop->cond_code == LT_EXPR)
		{
		  n1 = loop->n1;
		  n2 = fold_build2 (PLUS_EXPR, TREE_TYPE (loop->v),
				    loop->n2, loop->step);
		}
	      else
		{
		  n1 = fold_build2 (MINUS_EXPR, TREE_TYPE (loop->v),
				    loop->n2, loop->step);
		  n2 = loop->n1;
		}
	      if (TREE_CODE (n1) != INTEGER_CST
		  || TREE_CODE (n2) != INTEGER_CST
		  || !tree_int_cst_lt (TYPE_MIN_VALUE (iter_type), n1)
		  || !tree_int_cst_lt (n2, TYPE_MAX_VALUE (iter_type)))
		iter_type = long_long_unsigned_type_node;
	    }
	}

      if (collapse_count && *collapse_count == NULL)
	{
	  if ((i == 0 || count != NULL_TREE)
	      && TREE_CODE (TREE_TYPE (loop->v)) == INTEGER_TYPE
	      && TREE_CONSTANT (loop->n1)
	      && TREE_CONSTANT (loop->n2)
	      && TREE_CODE (loop->step) == INTEGER_CST)
	    {
	      tree itype = TREE_TYPE (loop->v);

	      if (POINTER_TYPE_P (itype))
		itype
		  = lang_hooks.types.type_for_size (TYPE_PRECISION (itype), 0);
	      t = build_int_cst (itype, (loop->cond_code == LT_EXPR ? -1 : 1));
	      t = fold_build2 (PLUS_EXPR, itype,
			       fold_convert (itype, loop->step), t);
	      t = fold_build2 (PLUS_EXPR, itype, t,
			       fold_convert (itype, loop->n2));
	      t = fold_build2 (MINUS_EXPR, itype, t,
			       fold_convert (itype, loop->n1));
	      if (TYPE_UNSIGNED (itype) && loop->cond_code == GT_EXPR)
		t = fold_build2 (TRUNC_DIV_EXPR, itype,
				 fold_build1 (NEGATE_EXPR, itype, t),
				 fold_build1 (NEGATE_EXPR, itype,
					      fold_convert (itype,
							    loop->step)));
	      else
		t = fold_build2 (TRUNC_DIV_EXPR, itype, t,
				 fold_convert (itype, loop->step));
	      t = fold_convert (long_long_unsigned_type_node, t);
	      if (count != NULL_TREE)
		count = fold_build2 (MULT_EXPR, long_long_unsigned_type_node,
				     count, t);
	      else
		count = t;
	      if (TREE_CODE (count) != INTEGER_CST)
		count = NULL_TREE;
	    }
	  else
	    count = NULL_TREE;
	}
    }

  if (count)
    {
      if (!tree_int_cst_lt (count, TYPE_MAX_VALUE (long_integer_type_node)))
	iter_type = long_long_unsigned_type_node;
      else
	iter_type = long_integer_type_node;
    }
  else if (collapse_iter && *collapse_iter != NULL)
    iter_type = TREE_TYPE (*collapse_iter);
  fd->iter_type = iter_type;
  if (collapse_iter && *collapse_iter == NULL)
    *collapse_iter = create_tmp_var (iter_type, ".iter");
  if (collapse_count && *collapse_count == NULL)
    {
      if (count)
	*collapse_count = fold_convert (iter_type, count);
      else
	*collapse_count = create_tmp_var (iter_type, ".count");
    }

  if (fd->collapse > 1)
    {
      fd->loop.v = *collapse_iter;
      fd->loop.n1 = build_int_cst (TREE_TYPE (fd->loop.v), 0);
      fd->loop.n2 = *collapse_count;
      fd->loop.step = build_int_cst (TREE_TYPE (fd->loop.v), 1);
      fd->loop.cond_code = LT_EXPR;
    }
}


/* Given two blocks PAR_ENTRY_BB and WS_ENTRY_BB such that WS_ENTRY_BB
   is the immediate dominator of PAR_ENTRY_BB, return true if there
   are no data dependencies that would prevent expanding the parallel
   directive at PAR_ENTRY_BB as a combined parallel+workshare region.

   When expanding a combined parallel+workshare region, the call to
   the child function may need additional arguments in the case of
   GIMPLE_OMP_FOR regions.  In some cases, these arguments are
   computed out of variables passed in from the parent to the child
   via 'struct .omp_data_s'.  For instance:

	#pragma omp parallel for schedule (guided, i * 4)
	for (j ...)

   Is lowered into:

   	# BLOCK 2 (PAR_ENTRY_BB)
	.omp_data_o.i = i;
	#pragma omp parallel [child fn: bar.omp_fn.0 ( ..., D.1598)
	
	# BLOCK 3 (WS_ENTRY_BB)
	.omp_data_i = &.omp_data_o;
	D.1667 = .omp_data_i->i;
	D.1598 = D.1667 * 4;
	#pragma omp for schedule (guided, D.1598)

   When we outline the parallel region, the call to the child function
   'bar.omp_fn.0' will need the value D.1598 in its argument list, but
   that value is computed *after* the call site.  So, in principle we
   cannot do the transformation.

   To see whether the code in WS_ENTRY_BB blocks the combined
   parallel+workshare call, we collect all the variables used in the
   GIMPLE_OMP_FOR header check whether they appear on the LHS of any
   statement in WS_ENTRY_BB.  If so, then we cannot emit the combined
   call.

   FIXME.  If we had the SSA form built at this point, we could merely
   hoist the code in block 3 into block 2 and be done with it.  But at
   this point we don't have dataflow information and though we could
   hack something up here, it is really not worth the aggravation.  */

static bool
workshare_safe_to_combine_p (basic_block par_entry_bb, basic_block ws_entry_bb)
{
  struct omp_for_data fd;
  gimple par_stmt, ws_stmt;

  par_stmt = last_stmt (par_entry_bb);
  ws_stmt = last_stmt (ws_entry_bb);

  if (gimple_code (ws_stmt) == GIMPLE_OMP_SECTIONS)
    return true;

  gcc_assert (gimple_code (ws_stmt) == GIMPLE_OMP_FOR);

  extract_omp_for_data (ws_stmt, &fd, NULL);

  if (fd.collapse > 1 && TREE_CODE (fd.loop.n2) != INTEGER_CST)
    return false;
  if (fd.iter_type != long_integer_type_node)
    return false;

  /* FIXME.  We give up too easily here.  If any of these arguments
     are not constants, they will likely involve variables that have
     been mapped into fields of .omp_data_s for sharing with the child
     function.  With appropriate data flow, it would be possible to
     see through this.  */
  if (!is_gimple_min_invariant (fd.loop.n1)
      || !is_gimple_min_invariant (fd.loop.n2)
      || !is_gimple_min_invariant (fd.loop.step)
      || (fd.chunk_size && !is_gimple_min_invariant (fd.chunk_size)))
    return false;

  return true;
}


/* Collect additional arguments needed to emit a combined
   parallel+workshare call.  WS_STMT is the workshare directive being
   expanded.  */

static tree
get_ws_args_for (gimple ws_stmt)
{
  tree t;

  if (gimple_code (ws_stmt) == GIMPLE_OMP_FOR)
    {
      struct omp_for_data fd;
      tree ws_args;

      extract_omp_for_data (ws_stmt, &fd, NULL);

      ws_args = NULL_TREE;
      if (fd.chunk_size)
	{
	  t = fold_convert (long_integer_type_node, fd.chunk_size);
	  ws_args = tree_cons (NULL, t, ws_args);
	}

      t = fold_convert (long_integer_type_node, fd.loop.step);
      ws_args = tree_cons (NULL, t, ws_args);

      t = fold_convert (long_integer_type_node, fd.loop.n2);
      ws_args = tree_cons (NULL, t, ws_args);

      t = fold_convert (long_integer_type_node, fd.loop.n1);
      ws_args = tree_cons (NULL, t, ws_args);

      return ws_args;
    }
  else if (gimple_code (ws_stmt) == GIMPLE_OMP_SECTIONS)
    {
      /* Number of sections is equal to the number of edges from the
	 GIMPLE_OMP_SECTIONS_SWITCH statement, except for the one to
	 the exit of the sections region.  */
      basic_block bb = single_succ (gimple_bb (ws_stmt));
      t = build_int_cst (unsigned_type_node, EDGE_COUNT (bb->succs) - 1);
      t = tree_cons (NULL, t, NULL);
      return t;
    }

  gcc_unreachable ();
}


/* Discover whether REGION is a combined parallel+workshare region.  */

static void
determine_parallel_type (struct omp_region *region)
{
  basic_block par_entry_bb, par_exit_bb;
  basic_block ws_entry_bb, ws_exit_bb;

  if (region == NULL || region->inner == NULL
      || region->exit == NULL || region->inner->exit == NULL
      || region->inner->cont == NULL)
    return;

  /* We only support parallel+for and parallel+sections.  */
  if (region->type != GIMPLE_OMP_PARALLEL
      || (region->inner->type != GIMPLE_OMP_FOR
	  && region->inner->type != GIMPLE_OMP_SECTIONS))
    return;

  /* Check for perfect nesting PAR_ENTRY_BB -> WS_ENTRY_BB and
     WS_EXIT_BB -> PAR_EXIT_BB.  */
  par_entry_bb = region->entry;
  par_exit_bb = region->exit;
  ws_entry_bb = region->inner->entry;
  ws_exit_bb = region->inner->exit;

  if (single_succ (par_entry_bb) == ws_entry_bb
      && single_succ (ws_exit_bb) == par_exit_bb
      && workshare_safe_to_combine_p (par_entry_bb, ws_entry_bb)
      && (gimple_omp_parallel_combined_p (last_stmt (par_entry_bb))
	  || (last_and_only_stmt (ws_entry_bb)
	      && last_and_only_stmt (par_exit_bb))))
    {
      gimple ws_stmt = last_stmt (ws_entry_bb);

      if (region->inner->type == GIMPLE_OMP_FOR)
	{
	  /* If this is a combined parallel loop, we need to determine
	     whether or not to use the combined library calls.  There
	     are two cases where we do not apply the transformation:
	     static loops and any kind of ordered loop.  In the first
	     case, we already open code the loop so there is no need
	     to do anything else.  In the latter case, the combined
	     parallel loop call would still need extra synchronization
	     to implement ordered semantics, so there would not be any
	     gain in using the combined call.  */
	  tree clauses = gimple_omp_for_clauses (ws_stmt);
	  tree c = find_omp_clause (clauses, OMP_CLAUSE_SCHEDULE);
	  if (c == NULL
	      || OMP_CLAUSE_SCHEDULE_KIND (c) == OMP_CLAUSE_SCHEDULE_STATIC
	      || find_omp_clause (clauses, OMP_CLAUSE_ORDERED))
	    {
	      region->is_combined_parallel = false;
	      region->inner->is_combined_parallel = false;
	      return;
	    }
	}

      region->is_combined_parallel = true;
      region->inner->is_combined_parallel = true;
      region->ws_args = get_ws_args_for (ws_stmt);
    }
}


/* Return true if EXPR is variable sized.  */

static inline bool
is_variable_sized (const_tree expr)
{
  return !TREE_CONSTANT (TYPE_SIZE_UNIT (TREE_TYPE (expr)));
}

/* Return true if DECL is a reference type.  */

static inline bool
is_reference (tree decl)
{
  return lang_hooks.decls.omp_privatize_by_reference (decl);
}

/* Lookup variables in the decl or field splay trees.  The "maybe" form
   allows for the variable form to not have been entered, otherwise we
   assert that the variable must have been entered.  */

static inline tree
lookup_decl (tree var, omp_context *ctx)
{
  tree *n;
  n = (tree *) pointer_map_contains (ctx->cb.decl_map, var);
  return *n;
}

static inline tree
maybe_lookup_decl (const_tree var, omp_context *ctx)
{
  tree *n;
  n = (tree *) pointer_map_contains (ctx->cb.decl_map, var);
  return n ? *n : NULL_TREE;
}

static inline tree
lookup_field (tree var, omp_context *ctx)
{
  splay_tree_node n;
  n = splay_tree_lookup (ctx->field_map, (splay_tree_key) var);
  return (tree) n->value;
}

static inline tree
lookup_sfield (tree var, omp_context *ctx)
{
  splay_tree_node n;
  n = splay_tree_lookup (ctx->sfield_map
			 ? ctx->sfield_map : ctx->field_map,
			 (splay_tree_key) var);
  return (tree) n->value;
}

static inline tree
maybe_lookup_field (tree var, omp_context *ctx)
{
  splay_tree_node n;
  n = splay_tree_lookup (ctx->field_map, (splay_tree_key) var);
  return n ? (tree) n->value : NULL_TREE;
}

/* Return true if DECL should be copied by pointer.  SHARED_CTX is
   the parallel context if DECL is to be shared.  */

static bool
use_pointer_for_field (tree decl, omp_context *shared_ctx)
{
  if (AGGREGATE_TYPE_P (TREE_TYPE (decl)))
    return true;

  /* We can only use copy-in/copy-out semantics for shared variables
     when we know the value is not accessible from an outer scope.  */
  if (shared_ctx)
    {
      /* ??? Trivially accessible from anywhere.  But why would we even
	 be passing an address in this case?  Should we simply assert
	 this to be false, or should we have a cleanup pass that removes
	 these from the list of mappings?  */
      if (TREE_STATIC (decl) || DECL_EXTERNAL (decl))
	return true;

      /* For variables with DECL_HAS_VALUE_EXPR_P set, we cannot tell
	 without analyzing the expression whether or not its location
	 is accessible to anyone else.  In the case of nested parallel
	 regions it certainly may be.  */
      if (TREE_CODE (decl) != RESULT_DECL && DECL_HAS_VALUE_EXPR_P (decl))
	return true;

      /* Do not use copy-in/copy-out for variables that have their
	 address taken.  */
      if (TREE_ADDRESSABLE (decl))
	return true;

      /* Disallow copy-in/out in nested parallel if
	 decl is shared in outer parallel, otherwise
	 each thread could store the shared variable
	 in its own copy-in location, making the
	 variable no longer really shared.  */
      if (!TREE_READONLY (decl) && shared_ctx->is_nested)
	{
	  omp_context *up;

	  for (up = shared_ctx->outer; up; up = up->outer)
	    if (is_taskreg_ctx (up) && maybe_lookup_decl (decl, up))
	      break;

	  if (up)
	    {
	      tree c;

	      for (c = gimple_omp_taskreg_clauses (up->stmt);
		   c; c = OMP_CLAUSE_CHAIN (c))
		if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_SHARED
		    && OMP_CLAUSE_DECL (c) == decl)
		  break;

	      if (c)
		return true;
	    }
	}

      /* For tasks avoid using copy-in/out, unless they are readonly
	 (in which case just copy-in is used).  As tasks can be
	 deferred or executed in different thread, when GOMP_task
	 returns, the task hasn't necessarily terminated.  */
      if (!TREE_READONLY (decl) && is_task_ctx (shared_ctx))
	{
	  tree outer = maybe_lookup_decl_in_outer_ctx (decl, shared_ctx);
	  if (is_gimple_reg (outer))
	    {
	      /* Taking address of OUTER in lower_send_shared_vars
		 might need regimplification of everything that uses the
		 variable.  */
	      if (!task_shared_vars)
		task_shared_vars = BITMAP_ALLOC (NULL);
	      bitmap_set_bit (task_shared_vars, DECL_UID (outer));
	      TREE_ADDRESSABLE (outer) = 1;
	    }
	  return true;
	}
    }

  return false;
}

/* Create a new VAR_DECL and copy information from VAR to it.  */

tree
copy_var_decl (tree var, tree name, tree type)
{
  tree copy = build_decl (VAR_DECL, name, type);

  TREE_ADDRESSABLE (copy) = TREE_ADDRESSABLE (var);
  TREE_THIS_VOLATILE (copy) = TREE_THIS_VOLATILE (var);
  DECL_GIMPLE_REG_P (copy) = DECL_GIMPLE_REG_P (var);
  DECL_NO_TBAA_P (copy) = DECL_NO_TBAA_P (var);
  DECL_ARTIFICIAL (copy) = DECL_ARTIFICIAL (var);
  DECL_IGNORED_P (copy) = DECL_IGNORED_P (var);
  DECL_CONTEXT (copy) = DECL_CONTEXT (var);
  DECL_SOURCE_LOCATION (copy) = DECL_SOURCE_LOCATION (var);
  TREE_USED (copy) = 1;
  DECL_SEEN_IN_BIND_EXPR_P (copy) = 1;

  return copy;
}

/* Construct a new automatic decl similar to VAR.  */

static tree
omp_copy_decl_2 (tree var, tree name, tree type, omp_context *ctx)
{
  tree copy = copy_var_decl (var, name, type);

  DECL_CONTEXT (copy) = current_function_decl;
  TREE_CHAIN (copy) = ctx->block_vars;
  ctx->block_vars = copy;

  return copy;
}

static tree
omp_copy_decl_1 (tree var, omp_context *ctx)
{
  return omp_copy_decl_2 (var, DECL_NAME (var), TREE_TYPE (var), ctx);
}

/* Build tree nodes to access the field for VAR on the receiver side.  */

static tree
build_receiver_ref (tree var, bool by_ref, omp_context *ctx)
{
  tree x, field = lookup_field (var, ctx);

  /* If the receiver record type was remapped in the child function,
     remap the field into the new record type.  */
  x = maybe_lookup_field (field, ctx);
  if (x != NULL)
    field = x;

  x = build_fold_indirect_ref (ctx->receiver_decl);
  x = build3 (COMPONENT_REF, TREE_TYPE (field), x, field, NULL);
  if (by_ref)
    x = build_fold_indirect_ref (x);

  return x;
}

/* Build tree nodes to access VAR in the scope outer to CTX.  In the case
   of a parallel, this is a component reference; for workshare constructs
   this is some variable.  */

static tree
build_outer_var_ref (tree var, omp_context *ctx)
{
  tree x;

  if (is_global_var (maybe_lookup_decl_in_outer_ctx (var, ctx)))
    x = var;
  else if (is_variable_sized (var))
    {
      x = TREE_OPERAND (DECL_VALUE_EXPR (var), 0);
      x = build_outer_var_ref (x, ctx);
      x = build_fold_indirect_ref (x);
    }
  else if (is_taskreg_ctx (ctx))
    {
      bool by_ref = use_pointer_for_field (var, NULL);
      x = build_receiver_ref (var, by_ref, ctx);
    }
  else if (ctx->outer)
    x = lookup_decl (var, ctx->outer);
  else if (is_reference (var))
    /* This can happen with orphaned constructs.  If var is reference, it is
       possible it is shared and as such valid.  */
    x = var;
  else
    gcc_unreachable ();

  if (is_reference (var))
    x = build_fold_indirect_ref (x);

  return x;
}

/* Build tree nodes to access the field for VAR on the sender side.  */

static tree
build_sender_ref (tree var, omp_context *ctx)
{
  tree field = lookup_sfield (var, ctx);
  return build3 (COMPONENT_REF, TREE_TYPE (field),
		 ctx->sender_decl, field, NULL);
}

/* Add a new field for VAR inside the structure CTX->SENDER_DECL.  */

static void
install_var_field (tree var, bool by_ref, int mask, omp_context *ctx)
{
  tree field, type, sfield = NULL_TREE;

  gcc_assert ((mask & 1) == 0
	      || !splay_tree_lookup (ctx->field_map, (splay_tree_key) var));
  gcc_assert ((mask & 2) == 0 || !ctx->sfield_map
	      || !splay_tree_lookup (ctx->sfield_map, (splay_tree_key) var));

  type = TREE_TYPE (var);
  if (by_ref)
    type = build_pointer_type (type);
  else if ((mask & 3) == 1 && is_reference (var))
    type = TREE_TYPE (type);

  field = build_decl (FIELD_DECL, DECL_NAME (var), type);

  /* Remember what variable this field was created for.  This does have a
     side effect of making dwarf2out ignore this member, so for helpful
     debugging we clear it later in delete_omp_context.  */
  DECL_ABSTRACT_ORIGIN (field) = var;
  if (type == TREE_TYPE (var))
    {
      DECL_ALIGN (field) = DECL_ALIGN (var);
      DECL_USER_ALIGN (field) = DECL_USER_ALIGN (var);
      TREE_THIS_VOLATILE (field) = TREE_THIS_VOLATILE (var);
    }
  else
    DECL_ALIGN (field) = TYPE_ALIGN (type);

  if ((mask & 3) == 3)
    {
      insert_field_into_struct (ctx->record_type, field);
      if (ctx->srecord_type)
	{
	  sfield = build_decl (FIELD_DECL, DECL_NAME (var), type);
	  DECL_ABSTRACT_ORIGIN (sfield) = var;
	  DECL_ALIGN (sfield) = DECL_ALIGN (field);
	  DECL_USER_ALIGN (sfield) = DECL_USER_ALIGN (field);
	  TREE_THIS_VOLATILE (sfield) = TREE_THIS_VOLATILE (field);
	  insert_field_into_struct (ctx->srecord_type, sfield);
	}
    }
  else
    {
      if (ctx->srecord_type == NULL_TREE)
	{
	  tree t;

	  ctx->srecord_type = lang_hooks.types.make_type (RECORD_TYPE);
	  ctx->sfield_map = splay_tree_new (splay_tree_compare_pointers, 0, 0);
	  for (t = TYPE_FIELDS (ctx->record_type); t ; t = TREE_CHAIN (t))
	    {
	      sfield = build_decl (FIELD_DECL, DECL_NAME (t), TREE_TYPE (t));
	      DECL_ABSTRACT_ORIGIN (sfield) = DECL_ABSTRACT_ORIGIN (t);
	      insert_field_into_struct (ctx->srecord_type, sfield);
	      splay_tree_insert (ctx->sfield_map,
				 (splay_tree_key) DECL_ABSTRACT_ORIGIN (t),
				 (splay_tree_value) sfield);
	    }
	}
      sfield = field;
      insert_field_into_struct ((mask & 1) ? ctx->record_type
				: ctx->srecord_type, field);
    }

  if (mask & 1)
    splay_tree_insert (ctx->field_map, (splay_tree_key) var,
		       (splay_tree_value) field);
  if ((mask & 2) && ctx->sfield_map)
    splay_tree_insert (ctx->sfield_map, (splay_tree_key) var,
		       (splay_tree_value) sfield);
}

static tree
install_var_local (tree var, omp_context *ctx)
{
  tree new_var = omp_copy_decl_1 (var, ctx);
  insert_decl_map (&ctx->cb, var, new_var);
  return new_var;
}

/* Adjust the replacement for DECL in CTX for the new context.  This means
   copying the DECL_VALUE_EXPR, and fixing up the type.  */

static void
fixup_remapped_decl (tree decl, omp_context *ctx, bool private_debug)
{
  tree new_decl, size;

  new_decl = lookup_decl (decl, ctx);

  TREE_TYPE (new_decl) = remap_type (TREE_TYPE (decl), &ctx->cb);

  if ((!TREE_CONSTANT (DECL_SIZE (new_decl)) || private_debug)
      && DECL_HAS_VALUE_EXPR_P (decl))
    {
      tree ve = DECL_VALUE_EXPR (decl);
      walk_tree (&ve, copy_tree_body_r, &ctx->cb, NULL);
      SET_DECL_VALUE_EXPR (new_decl, ve);
      DECL_HAS_VALUE_EXPR_P (new_decl) = 1;
    }

  if (!TREE_CONSTANT (DECL_SIZE (new_decl)))
    {
      size = remap_decl (DECL_SIZE (decl), &ctx->cb);
      if (size == error_mark_node)
	size = TYPE_SIZE (TREE_TYPE (new_decl));
      DECL_SIZE (new_decl) = size;

      size = remap_decl (DECL_SIZE_UNIT (decl), &ctx->cb);
      if (size == error_mark_node)
	size = TYPE_SIZE_UNIT (TREE_TYPE (new_decl));
      DECL_SIZE_UNIT (new_decl) = size;
    }
}

/* The callback for remap_decl.  Search all containing contexts for a
   mapping of the variable; this avoids having to duplicate the splay
   tree ahead of time.  We know a mapping doesn't already exist in the
   given context.  Create new mappings to implement default semantics.  */

static tree
omp_copy_decl (tree var, copy_body_data *cb)
{
  omp_context *ctx = (omp_context *) cb;
  tree new_var;

  if (TREE_CODE (var) == LABEL_DECL)
    {
      new_var = create_artificial_label ();
      DECL_CONTEXT (new_var) = current_function_decl;
      insert_decl_map (&ctx->cb, var, new_var);
      return new_var;
    }

  while (!is_taskreg_ctx (ctx))
    {
      ctx = ctx->outer;
      if (ctx == NULL)
	return var;
      new_var = maybe_lookup_decl (var, ctx);
      if (new_var)
	return new_var;
    }

  if (is_global_var (var) || decl_function_context (var) != ctx->cb.src_fn)
    return var;

  return error_mark_node;
}


/* Return the parallel region associated with STMT.  */

/* Debugging dumps for parallel regions.  */
void dump_omp_region (FILE *, struct omp_region *, int);
void debug_omp_region (struct omp_region *);
void debug_all_omp_regions (void);

/* Dump the parallel region tree rooted at REGION.  */

void
dump_omp_region (FILE *file, struct omp_region *region, int indent)
{
  fprintf (file, "%*sbb %d: %s\n", indent, "", region->entry->index,
	   gimple_code_name[region->type]);

  if (region->inner)
    dump_omp_region (file, region->inner, indent + 4);

  if (region->cont)
    {
      fprintf (file, "%*sbb %d: GIMPLE_OMP_CONTINUE\n", indent, "",
	       region->cont->index);
    }
    
  if (region->exit)
    fprintf (file, "%*sbb %d: GIMPLE_OMP_RETURN\n", indent, "",
	     region->exit->index);
  else
    fprintf (file, "%*s[no exit marker]\n", indent, "");

  if (region->next)
    dump_omp_region (file, region->next, indent);
}

void
debug_omp_region (struct omp_region *region)
{
  dump_omp_region (stderr, region, 0);
}

void
debug_all_omp_regions (void)
{
  dump_omp_region (stderr, root_omp_region, 0);
}


/* Create a new parallel region starting at STMT inside region PARENT.  */

struct omp_region *
new_omp_region (basic_block bb, enum gimple_code type,
		struct omp_region *parent)
{
  struct omp_region *region = XCNEW (struct omp_region);

  region->outer = parent;
  region->entry = bb;
  region->type = type;

  if (parent)
    {
      /* This is a nested region.  Add it to the list of inner
	 regions in PARENT.  */
      region->next = parent->inner;
      parent->inner = region;
    }
  else
    {
      /* This is a toplevel region.  Add it to the list of toplevel
	 regions in ROOT_OMP_REGION.  */
      region->next = root_omp_region;
      root_omp_region = region;
    }

  return region;
}

/* Release the memory associated with the region tree rooted at REGION.  */

static void
free_omp_region_1 (struct omp_region *region)
{
  struct omp_region *i, *n;

  for (i = region->inner; i ; i = n)
    {
      n = i->next;
      free_omp_region_1 (i);
    }

  free (region);
}

/* Release the memory for the entire omp region tree.  */

void
free_omp_regions (void)
{
  struct omp_region *r, *n;
  for (r = root_omp_region; r ; r = n)
    {
      n = r->next;
      free_omp_region_1 (r);
    }
  root_omp_region = NULL;
}


/* Create a new context, with OUTER_CTX being the surrounding context.  */

static omp_context *
new_omp_context (gimple stmt, omp_context *outer_ctx)
{
  omp_context *ctx = XCNEW (omp_context);

  splay_tree_insert (all_contexts, (splay_tree_key) stmt,
		     (splay_tree_value) ctx);
  ctx->stmt = stmt;

  if (outer_ctx)
    {
      ctx->outer = outer_ctx;
      ctx->cb = outer_ctx->cb;
      ctx->cb.block = NULL;
      ctx->depth = outer_ctx->depth + 1;
    }
  else
    {
      ctx->cb.src_fn = current_function_decl;
      ctx->cb.dst_fn = current_function_decl;
      ctx->cb.src_node = cgraph_node (current_function_decl);
      ctx->cb.dst_node = ctx->cb.src_node;
      ctx->cb.src_cfun = cfun;
      ctx->cb.copy_decl = omp_copy_decl;
      ctx->cb.eh_region = -1;
      ctx->cb.transform_call_graph_edges = CB_CGE_MOVE;
      ctx->depth = 1;
    }

  ctx->cb.decl_map = pointer_map_create ();

  return ctx;
}

static gimple_seq maybe_catch_exception (gimple_seq);

/* Finalize task copyfn.  */

static void
finalize_task_copyfn (gimple task_stmt)
{
  struct function *child_cfun;
  tree child_fn, old_fn;
  gimple_seq seq, new_seq;
  gimple bind;

  child_fn = gimple_omp_task_copy_fn (task_stmt);
  if (child_fn == NULL_TREE)
    return;

  child_cfun = DECL_STRUCT_FUNCTION (child_fn);

  /* Inform the callgraph about the new function.  */
  DECL_STRUCT_FUNCTION (child_fn)->curr_properties
    = cfun->curr_properties;

  old_fn = current_function_decl;
  push_cfun (child_cfun);
  current_function_decl = child_fn;
  bind = gimplify_body (&DECL_SAVED_TREE (child_fn), child_fn, false);
  seq = gimple_seq_alloc ();
  gimple_seq_add_stmt (&seq, bind);
  new_seq = maybe_catch_exception (seq);
  if (new_seq != seq)
    {
      bind = gimple_build_bind (NULL, new_seq, NULL);
      seq = gimple_seq_alloc ();
      gimple_seq_add_stmt (&seq, bind);
    }
  gimple_set_body (child_fn, seq);
  pop_cfun ();
  current_function_decl = old_fn;

  cgraph_add_new_function (child_fn, false);
}

/* Destroy a omp_context data structures.  Called through the splay tree
   value delete callback.  */

static void
delete_omp_context (splay_tree_value value)
{
  omp_context *ctx = (omp_context *) value;

  pointer_map_destroy (ctx->cb.decl_map);

  if (ctx->field_map)
    splay_tree_delete (ctx->field_map);
  if (ctx->sfield_map)
    splay_tree_delete (ctx->sfield_map);

  /* We hijacked DECL_ABSTRACT_ORIGIN earlier.  We need to clear it before
     it produces corrupt debug information.  */
  if (ctx->record_type)
    {
      tree t;
      for (t = TYPE_FIELDS (ctx->record_type); t ; t = TREE_CHAIN (t))
	DECL_ABSTRACT_ORIGIN (t) = NULL;
    }
  if (ctx->srecord_type)
    {
      tree t;
      for (t = TYPE_FIELDS (ctx->srecord_type); t ; t = TREE_CHAIN (t))
	DECL_ABSTRACT_ORIGIN (t) = NULL;
    }

  if (is_task_ctx (ctx))
    finalize_task_copyfn (ctx->stmt);

  XDELETE (ctx);
}

/* Fix up RECEIVER_DECL with a type that has been remapped to the child
   context.  */

static void
fixup_child_record_type (omp_context *ctx)
{
  tree f, type = ctx->record_type;

  /* ??? It isn't sufficient to just call remap_type here, because
     variably_modified_type_p doesn't work the way we expect for
     record types.  Testing each field for whether it needs remapping
     and creating a new record by hand works, however.  */
  for (f = TYPE_FIELDS (type); f ; f = TREE_CHAIN (f))
    if (variably_modified_type_p (TREE_TYPE (f), ctx->cb.src_fn))
      break;
  if (f)
    {
      tree name, new_fields = NULL;

      type = lang_hooks.types.make_type (RECORD_TYPE);
      name = DECL_NAME (TYPE_NAME (ctx->record_type));
      name = build_decl (TYPE_DECL, name, type);
      TYPE_NAME (type) = name;

      for (f = TYPE_FIELDS (ctx->record_type); f ; f = TREE_CHAIN (f))
	{
	  tree new_f = copy_node (f);
	  DECL_CONTEXT (new_f) = type;
	  TREE_TYPE (new_f) = remap_type (TREE_TYPE (f), &ctx->cb);
	  TREE_CHAIN (new_f) = new_fields;
	  walk_tree (&DECL_SIZE (new_f), copy_tree_body_r, &ctx->cb, NULL);
	  walk_tree (&DECL_SIZE_UNIT (new_f), copy_tree_body_r,
		     &ctx->cb, NULL);
	  walk_tree (&DECL_FIELD_OFFSET (new_f), copy_tree_body_r,
		     &ctx->cb, NULL);
	  new_fields = new_f;

	  /* Arrange to be able to look up the receiver field
	     given the sender field.  */
	  splay_tree_insert (ctx->field_map, (splay_tree_key) f,
			     (splay_tree_value) new_f);
	}
      TYPE_FIELDS (type) = nreverse (new_fields);
      layout_type (type);
    }

  TREE_TYPE (ctx->receiver_decl) = build_pointer_type (type);
}

/* Instantiate decls as necessary in CTX to satisfy the data sharing
   specified by CLAUSES.  */

static void
scan_sharing_clauses (tree clauses, omp_context *ctx)
{
  tree c, decl;
  bool scan_array_reductions = false;

  for (c = clauses; c; c = OMP_CLAUSE_CHAIN (c))
    {
      bool by_ref;

      switch (OMP_CLAUSE_CODE (c))
	{
	case OMP_CLAUSE_PRIVATE:
	  decl = OMP_CLAUSE_DECL (c);
	  if (OMP_CLAUSE_PRIVATE_OUTER_REF (c))
	    goto do_private;
	  else if (!is_variable_sized (decl))
	    install_var_local (decl, ctx);
	  break;

	case OMP_CLAUSE_SHARED:
	  gcc_assert (is_taskreg_ctx (ctx));
	  decl = OMP_CLAUSE_DECL (c);
	  gcc_assert (!COMPLETE_TYPE_P (TREE_TYPE (decl))
		      || !is_variable_sized (decl));
	  /* Global variables don't need to be copied,
	     the receiver side will use them directly.  */
	  if (is_global_var (maybe_lookup_decl_in_outer_ctx (decl, ctx)))
	    break;
	  by_ref = use_pointer_for_field (decl, ctx);
	  if (! TREE_READONLY (decl)
	      || TREE_ADDRESSABLE (decl)
	      || by_ref
	      || is_reference (decl))
	    {
	      install_var_field (decl, by_ref, 3, ctx);
	      install_var_local (decl, ctx);
	      break;
	    }
	  /* We don't need to copy const scalar vars back.  */
	  OMP_CLAUSE_SET_CODE (c, OMP_CLAUSE_FIRSTPRIVATE);
	  goto do_private;

	case OMP_CLAUSE_LASTPRIVATE:
	  /* Let the corresponding firstprivate clause create
	     the variable.  */
	  if (OMP_CLAUSE_LASTPRIVATE_FIRSTPRIVATE (c))
	    break;
	  /* FALLTHRU */

	case OMP_CLAUSE_FIRSTPRIVATE:
	case OMP_CLAUSE_REDUCTION:
	  decl = OMP_CLAUSE_DECL (c);
	do_private:
	  if (is_variable_sized (decl))
	    {
	      if (is_task_ctx (ctx))
		install_var_field (decl, false, 1, ctx);
	      break;
	    }
	  else if (is_taskreg_ctx (ctx))
	    {
	      bool global
		= is_global_var (maybe_lookup_decl_in_outer_ctx (decl, ctx));
	      by_ref = use_pointer_for_field (decl, NULL);

	      if (is_task_ctx (ctx)
		  && (global || by_ref || is_reference (decl)))
		{
		  install_var_field (decl, false, 1, ctx);
		  if (!global)
		    install_var_field (decl, by_ref, 2, ctx);
		}
	      else if (!global)
		install_var_field (decl, by_ref, 3, ctx);
	    }
	  install_var_local (decl, ctx);
	  break;

	case OMP_CLAUSE_COPYPRIVATE:
	  if (ctx->outer)
	    scan_omp_op (&OMP_CLAUSE_DECL (c), ctx->outer);
	  /* FALLTHRU */

	case OMP_CLAUSE_COPYIN:
	  decl = OMP_CLAUSE_DECL (c);
	  by_ref = use_pointer_for_field (decl, NULL);
	  install_var_field (decl, by_ref, 3, ctx);
	  break;

	case OMP_CLAUSE_DEFAULT:
	  ctx->default_kind = OMP_CLAUSE_DEFAULT_KIND (c);
	  break;

	case OMP_CLAUSE_IF:
	case OMP_CLAUSE_NUM_THREADS:
	case OMP_CLAUSE_SCHEDULE:
	  if (ctx->outer)
	    scan_omp_op (&OMP_CLAUSE_OPERAND (c, 0), ctx->outer);
	  break;

	case OMP_CLAUSE_NOWAIT:
	case OMP_CLAUSE_ORDERED:
	case OMP_CLAUSE_COLLAPSE:
	case OMP_CLAUSE_UNTIED:
	  break;

	default:
	  gcc_unreachable ();
	}
    }

  for (c = clauses; c; c = OMP_CLAUSE_CHAIN (c))
    {
      switch (OMP_CLAUSE_CODE (c))
	{
	case OMP_CLAUSE_LASTPRIVATE:
	  /* Let the corresponding firstprivate clause create
	     the variable.  */
	  if (OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ (c))
	    scan_array_reductions = true;
	  if (OMP_CLAUSE_LASTPRIVATE_FIRSTPRIVATE (c))
	    break;
	  /* FALLTHRU */

	case OMP_CLAUSE_PRIVATE:
	case OMP_CLAUSE_FIRSTPRIVATE:
	case OMP_CLAUSE_REDUCTION:
	  decl = OMP_CLAUSE_DECL (c);
	  if (is_variable_sized (decl))
	    install_var_local (decl, ctx);
	  fixup_remapped_decl (decl, ctx,
			       OMP_CLAUSE_CODE (c) == OMP_CLAUSE_PRIVATE
			       && OMP_CLAUSE_PRIVATE_DEBUG (c));
	  if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_REDUCTION
	      && OMP_CLAUSE_REDUCTION_PLACEHOLDER (c))
	    scan_array_reductions = true;
	  break;

	case OMP_CLAUSE_SHARED:
	  decl = OMP_CLAUSE_DECL (c);
	  if (! is_global_var (maybe_lookup_decl_in_outer_ctx (decl, ctx)))
	    fixup_remapped_decl (decl, ctx, false);
	  break;

	case OMP_CLAUSE_COPYPRIVATE:
	case OMP_CLAUSE_COPYIN:
	case OMP_CLAUSE_DEFAULT:
	case OMP_CLAUSE_IF:
	case OMP_CLAUSE_NUM_THREADS:
	case OMP_CLAUSE_SCHEDULE:
	case OMP_CLAUSE_NOWAIT:
	case OMP_CLAUSE_ORDERED:
	case OMP_CLAUSE_COLLAPSE:
	case OMP_CLAUSE_UNTIED:
	  break;

	default:
	  gcc_unreachable ();
	}
    }

  if (scan_array_reductions)
    for (c = clauses; c; c = OMP_CLAUSE_CHAIN (c))
      if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_REDUCTION
	  && OMP_CLAUSE_REDUCTION_PLACEHOLDER (c))
	{
	  scan_omp (OMP_CLAUSE_REDUCTION_GIMPLE_INIT (c), ctx);
	  scan_omp (OMP_CLAUSE_REDUCTION_GIMPLE_MERGE (c), ctx);
	}
      else if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_LASTPRIVATE
	       && OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ (c))
	scan_omp (OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ (c), ctx);
}

/* Create a new name for omp child function.  Returns an identifier.  */

static GTY(()) unsigned int tmp_ompfn_id_num;

static tree
create_omp_child_function_name (bool task_copy)
{
  tree name = DECL_ASSEMBLER_NAME (current_function_decl);
  size_t len = IDENTIFIER_LENGTH (name);
  char *tmp_name, *prefix;
  const char *suffix;

  suffix = task_copy ? "_omp_cpyfn" : "_omp_fn";
  prefix = XALLOCAVEC (char, len + strlen (suffix) + 1);
  memcpy (prefix, IDENTIFIER_POINTER (name), len);
  strcpy (prefix + len, suffix);
#ifndef NO_DOT_IN_LABEL
  prefix[len] = '.';
#elif !defined NO_DOLLAR_IN_LABEL
  prefix[len] = '$';
#endif
  ASM_FORMAT_PRIVATE_NAME (tmp_name, prefix, tmp_ompfn_id_num++);
  return get_identifier (tmp_name);
}

/* Build a decl for the omp child function.  It'll not contain a body
   yet, just the bare decl.  */

static void
create_omp_child_function (omp_context *ctx, bool task_copy)
{
  tree decl, type, name, t;

  name = create_omp_child_function_name (task_copy);
  if (task_copy)
    type = build_function_type_list (void_type_node, ptr_type_node,
				     ptr_type_node, NULL_TREE);
  else
    type = build_function_type_list (void_type_node, ptr_type_node, NULL_TREE);

  decl = build_decl (FUNCTION_DECL, name, type);
  decl = lang_hooks.decls.pushdecl (decl);

  if (!task_copy)
    ctx->cb.dst_fn = decl;
  else
    gimple_omp_task_set_copy_fn (ctx->stmt, decl);

  TREE_STATIC (decl) = 1;
  TREE_USED (decl) = 1;
  DECL_ARTIFICIAL (decl) = 1;
  DECL_IGNORED_P (decl) = 0;
  TREE_PUBLIC (decl) = 0;
  DECL_UNINLINABLE (decl) = 1;
  DECL_EXTERNAL (decl) = 0;
  DECL_CONTEXT (decl) = NULL_TREE;
  DECL_INITIAL (decl) = make_node (BLOCK);

  t = build_decl (RESULT_DECL, NULL_TREE, void_type_node);
  DECL_ARTIFICIAL (t) = 1;
  DECL_IGNORED_P (t) = 1;
  DECL_RESULT (decl) = t;

  t = build_decl (PARM_DECL, get_identifier (".omp_data_i"), ptr_type_node);
  DECL_ARTIFICIAL (t) = 1;
  DECL_ARG_TYPE (t) = ptr_type_node;
  DECL_CONTEXT (t) = current_function_decl;
  TREE_USED (t) = 1;
  DECL_ARGUMENTS (decl) = t;
  if (!task_copy)
    ctx->receiver_decl = t;
  else
    {
      t = build_decl (PARM_DECL, get_identifier (".omp_data_o"),
		      ptr_type_node);
      DECL_ARTIFICIAL (t) = 1;
      DECL_ARG_TYPE (t) = ptr_type_node;
      DECL_CONTEXT (t) = current_function_decl;
      TREE_USED (t) = 1;
      TREE_CHAIN (t) = DECL_ARGUMENTS (decl);
      DECL_ARGUMENTS (decl) = t;
    }

  /* Allocate memory for the function structure.  The call to 
     allocate_struct_function clobbers CFUN, so we need to restore
     it afterward.  */
  push_struct_function (decl);
  DECL_SOURCE_LOCATION (decl) = gimple_location (ctx->stmt);
  cfun->function_end_locus = gimple_location (ctx->stmt);
  pop_cfun ();
}


/* Scan an OpenMP parallel directive.  */

static void
scan_omp_parallel (gimple_stmt_iterator *gsi, omp_context *outer_ctx)
{
  omp_context *ctx;
  tree name;
  gimple stmt = gsi_stmt (*gsi);

  /* Ignore parallel directives with empty bodies, unless there
     are copyin clauses.  */
  if (optimize > 0
      && empty_body_p (gimple_omp_body (stmt))
      && find_omp_clause (gimple_omp_parallel_clauses (stmt),
			  OMP_CLAUSE_COPYIN) == NULL)
    {
      gsi_replace (gsi, gimple_build_nop (), false);
      return;
    }

  ctx = new_omp_context (stmt, outer_ctx);
  if (taskreg_nesting_level > 1)
    ctx->is_nested = true;
  ctx->field_map = splay_tree_new (splay_tree_compare_pointers, 0, 0);
  ctx->default_kind = OMP_CLAUSE_DEFAULT_SHARED;
  ctx->record_type = lang_hooks.types.make_type (RECORD_TYPE);
  name = create_tmp_var_name (".omp_data_s");
  name = build_decl (TYPE_DECL, name, ctx->record_type);
  TYPE_NAME (ctx->record_type) = name;
  create_omp_child_function (ctx, false);
  gimple_omp_parallel_set_child_fn (stmt, ctx->cb.dst_fn);

  scan_sharing_clauses (gimple_omp_parallel_clauses (stmt), ctx);
  scan_omp (gimple_omp_body (stmt), ctx);

  if (TYPE_FIELDS (ctx->record_type) == NULL)
    ctx->record_type = ctx->receiver_decl = NULL;
  else
    {
      layout_type (ctx->record_type);
      fixup_child_record_type (ctx);
    }
}

/* Scan an OpenMP task directive.  */

static void
scan_omp_task (gimple_stmt_iterator *gsi, omp_context *outer_ctx)
{
  omp_context *ctx;
  tree name, t;
  gimple stmt = gsi_stmt (*gsi);

  /* Ignore task directives with empty bodies.  */
  if (optimize > 0
      && empty_body_p (gimple_omp_body (stmt)))
    {
      gsi_replace (gsi, gimple_build_nop (), false);
      return;
    }

  ctx = new_omp_context (stmt, outer_ctx);
  if (taskreg_nesting_level > 1)
    ctx->is_nested = true;
  ctx->field_map = splay_tree_new (splay_tree_compare_pointers, 0, 0);
  ctx->default_kind = OMP_CLAUSE_DEFAULT_SHARED;
  ctx->record_type = lang_hooks.types.make_type (RECORD_TYPE);
  name = create_tmp_var_name (".omp_data_s");
  name = build_decl (TYPE_DECL, name, ctx->record_type);
  TYPE_NAME (ctx->record_type) = name;
  create_omp_child_function (ctx, false);
  gimple_omp_task_set_child_fn (stmt, ctx->cb.dst_fn);

  scan_sharing_clauses (gimple_omp_task_clauses (stmt), ctx);

  if (ctx->srecord_type)
    {
      name = create_tmp_var_name (".omp_data_a");
      name = build_decl (TYPE_DECL, name, ctx->srecord_type);
      TYPE_NAME (ctx->srecord_type) = name;
      create_omp_child_function (ctx, true);
    }

  scan_omp (gimple_omp_body (stmt), ctx);

  if (TYPE_FIELDS (ctx->record_type) == NULL)
    {
      ctx->record_type = ctx->receiver_decl = NULL;
      t = build_int_cst (long_integer_type_node, 0);
      gimple_omp_task_set_arg_size (stmt, t);
      t = build_int_cst (long_integer_type_node, 1);
      gimple_omp_task_set_arg_align (stmt, t);
    }
  else
    {
      tree *p, vla_fields = NULL_TREE, *q = &vla_fields;
      /* Move VLA fields to the end.  */
      p = &TYPE_FIELDS (ctx->record_type);
      while (*p)
	if (!TYPE_SIZE_UNIT (TREE_TYPE (*p))
	    || ! TREE_CONSTANT (TYPE_SIZE_UNIT (TREE_TYPE (*p))))
	  {
	    *q = *p;
	    *p = TREE_CHAIN (*p);
	    TREE_CHAIN (*q) = NULL_TREE;
	    q = &TREE_CHAIN (*q);
	  }
	else
	  p = &TREE_CHAIN (*p);
      *p = vla_fields;
      layout_type (ctx->record_type);
      fixup_child_record_type (ctx);
      if (ctx->srecord_type)
	layout_type (ctx->srecord_type);
      t = fold_convert (long_integer_type_node,
			TYPE_SIZE_UNIT (ctx->record_type));
      gimple_omp_task_set_arg_size (stmt, t);
      t = build_int_cst (long_integer_type_node,
			 TYPE_ALIGN_UNIT (ctx->record_type));
      gimple_omp_task_set_arg_align (stmt, t);
    }
}


/* Scan an OpenMP loop directive.  */

static void
scan_omp_for (gimple stmt, omp_context *outer_ctx)
{
  omp_context *ctx;
  size_t i;

  ctx = new_omp_context (stmt, outer_ctx);

  scan_sharing_clauses (gimple_omp_for_clauses (stmt), ctx);

  scan_omp (gimple_omp_for_pre_body (stmt), ctx);
  for (i = 0; i < gimple_omp_for_collapse (stmt); i++)
    {
      scan_omp_op (gimple_omp_for_index_ptr (stmt, i), ctx);
      scan_omp_op (gimple_omp_for_initial_ptr (stmt, i), ctx);
      scan_omp_op (gimple_omp_for_final_ptr (stmt, i), ctx);
      scan_omp_op (gimple_omp_for_incr_ptr (stmt, i), ctx);
    }
  scan_omp (gimple_omp_body (stmt), ctx);
}

/* Scan an OpenMP sections directive.  */

static void
scan_omp_sections (gimple stmt, omp_context *outer_ctx)
{
  omp_context *ctx;

  ctx = new_omp_context (stmt, outer_ctx);
  scan_sharing_clauses (gimple_omp_sections_clauses (stmt), ctx);
  scan_omp (gimple_omp_body (stmt), ctx);
}

/* Scan an OpenMP single directive.  */

static void
scan_omp_single (gimple stmt, omp_context *outer_ctx)
{
  omp_context *ctx;
  tree name;

  ctx = new_omp_context (stmt, outer_ctx);
  ctx->field_map = splay_tree_new (splay_tree_compare_pointers, 0, 0);
  ctx->record_type = lang_hooks.types.make_type (RECORD_TYPE);
  name = create_tmp_var_name (".omp_copy_s");
  name = build_decl (TYPE_DECL, name, ctx->record_type);
  TYPE_NAME (ctx->record_type) = name;

  scan_sharing_clauses (gimple_omp_single_clauses (stmt), ctx);
  scan_omp (gimple_omp_body (stmt), ctx);

  if (TYPE_FIELDS (ctx->record_type) == NULL)
    ctx->record_type = NULL;
  else
    layout_type (ctx->record_type);
}


/* Check OpenMP nesting restrictions.  */
static void
check_omp_nesting_restrictions (gimple  stmt, omp_context *ctx)
{
  switch (gimple_code (stmt))
    {
    case GIMPLE_OMP_FOR:
    case GIMPLE_OMP_SECTIONS:
    case GIMPLE_OMP_SINGLE:
    case GIMPLE_CALL:
      for (; ctx != NULL; ctx = ctx->outer)
	switch (gimple_code (ctx->stmt))
	  {
	  case GIMPLE_OMP_FOR:
	  case GIMPLE_OMP_SECTIONS:
	  case GIMPLE_OMP_SINGLE:
	  case GIMPLE_OMP_ORDERED:
	  case GIMPLE_OMP_MASTER:
	  case GIMPLE_OMP_TASK:
	    if (is_gimple_call (stmt))
	      {
		warning (0, "barrier region may not be closely nested inside "
			    "of work-sharing, critical, ordered, master or "
			    "explicit task region");
		return;
	      }
	    warning (0, "work-sharing region may not be closely nested inside "
			"of work-sharing, critical, ordered, master or explicit "
			"task region");
	    return;
	  case GIMPLE_OMP_PARALLEL:
	    return;
	  default:
	    break;
	  }
      break;
    case GIMPLE_OMP_MASTER:
      for (; ctx != NULL; ctx = ctx->outer)
	switch (gimple_code (ctx->stmt))
	  {
	  case GIMPLE_OMP_FOR:
	  case GIMPLE_OMP_SECTIONS:
	  case GIMPLE_OMP_SINGLE:
	  case GIMPLE_OMP_TASK:
	    warning (0, "master region may not be closely nested inside "
			"of work-sharing or explicit task region");
	    return;
	  case GIMPLE_OMP_PARALLEL:
	    return;
	  default:
	    break;
	  }
      break;
    case GIMPLE_OMP_ORDERED:
      for (; ctx != NULL; ctx = ctx->outer)
	switch (gimple_code (ctx->stmt))
	  {
	  case GIMPLE_OMP_CRITICAL:
	  case GIMPLE_OMP_TASK:
	    warning (0, "ordered region may not be closely nested inside "
			"of critical or explicit task region");
	    return;
	  case GIMPLE_OMP_FOR:
	    if (find_omp_clause (gimple_omp_for_clauses (ctx->stmt),
				 OMP_CLAUSE_ORDERED) == NULL)
	      warning (0, "ordered region must be closely nested inside "
			  "a loop region with an ordered clause");
	    return;
	  case GIMPLE_OMP_PARALLEL:
	    return;
	  default:
	    break;
	  }
      break;
    case GIMPLE_OMP_CRITICAL:
      for (; ctx != NULL; ctx = ctx->outer)
	if (gimple_code (ctx->stmt) == GIMPLE_OMP_CRITICAL
	    && (gimple_omp_critical_name (stmt)
		== gimple_omp_critical_name (ctx->stmt)))
	  {
	    warning (0, "critical region may not be nested inside a critical "
			"region with the same name");
	    return;
	  }
      break;
    default:
      break;
    }
}


/* Helper function scan_omp.

   Callback for walk_tree or operators in walk_gimple_stmt used to
   scan for OpenMP directives in TP.  */

static tree
scan_omp_1_op (tree *tp, int *walk_subtrees, void *data)
{
  struct walk_stmt_info *wi = (struct walk_stmt_info *) data;
  omp_context *ctx = (omp_context *) wi->info;
  tree t = *tp;

  switch (TREE_CODE (t))
    {
    case VAR_DECL:
    case PARM_DECL:
    case LABEL_DECL:
    case RESULT_DECL:
      if (ctx)
	*tp = remap_decl (t, &ctx->cb);
      break;

    default:
      if (ctx && TYPE_P (t))
	*tp = remap_type (t, &ctx->cb);
      else if (!DECL_P (t))
	*walk_subtrees = 1;
      break;
    }

  return NULL_TREE;
}


/* Helper function for scan_omp.

   Callback for walk_gimple_stmt used to scan for OpenMP directives in
   the current statement in GSI.  */

static tree
scan_omp_1_stmt (gimple_stmt_iterator *gsi, bool *handled_ops_p,
		 struct walk_stmt_info *wi)
{
  gimple stmt = gsi_stmt (*gsi);
  omp_context *ctx = (omp_context *) wi->info;

  if (gimple_has_location (stmt))
    input_location = gimple_location (stmt);

  /* Check the OpenMP nesting restrictions.  */
  if (ctx != NULL)
    {
      if (is_gimple_omp (stmt))
	check_omp_nesting_restrictions (stmt, ctx);
      else if (is_gimple_call (stmt))
	{
	  tree fndecl = gimple_call_fndecl (stmt);
	  if (fndecl && DECL_BUILT_IN_CLASS (fndecl) == BUILT_IN_NORMAL
	      && DECL_FUNCTION_CODE (fndecl) == BUILT_IN_GOMP_BARRIER)
	    check_omp_nesting_restrictions (stmt, ctx);
	}
    }

  *handled_ops_p = true;

  switch (gimple_code (stmt))
    {
    case GIMPLE_OMP_PARALLEL:
      taskreg_nesting_level++;
      scan_omp_parallel (gsi, ctx);
      taskreg_nesting_level--;
      break;

    case GIMPLE_OMP_TASK:
      taskreg_nesting_level++;
      scan_omp_task (gsi, ctx);
      taskreg_nesting_level--;
      break;

    case GIMPLE_OMP_FOR:
      scan_omp_for (stmt, ctx);
      break;

    case GIMPLE_OMP_SECTIONS:
      scan_omp_sections (stmt, ctx);
      break;

    case GIMPLE_OMP_SINGLE:
      scan_omp_single (stmt, ctx);
      break;

    case GIMPLE_OMP_SECTION:
    case GIMPLE_OMP_MASTER:
    case GIMPLE_OMP_ORDERED:
    case GIMPLE_OMP_CRITICAL:
      ctx = new_omp_context (stmt, ctx);
      scan_omp (gimple_omp_body (stmt), ctx);
      break;

    case GIMPLE_BIND:
      {
	tree var;

	*handled_ops_p = false;
	if (ctx)
	  for (var = gimple_bind_vars (stmt); var ; var = TREE_CHAIN (var))
	    insert_decl_map (&ctx->cb, var, var);
      }
      break;
    default:
      *handled_ops_p = false;
      break;
    }

  return NULL_TREE;
}


/* Scan all the statements starting at the current statement.  CTX
   contains context information about the OpenMP directives and
   clauses found during the scan.  */

static void
scan_omp (gimple_seq body, omp_context *ctx)
{
  location_t saved_location;
  struct walk_stmt_info wi;

  memset (&wi, 0, sizeof (wi));
  wi.info = ctx;
  wi.want_locations = true;

  saved_location = input_location;
  walk_gimple_seq (body, scan_omp_1_stmt, scan_omp_1_op, &wi);
  input_location = saved_location;
}

/* Re-gimplification and code generation routines.  */

/* Build a call to GOMP_barrier.  */

static tree
build_omp_barrier (void)
{
  return build_call_expr (built_in_decls[BUILT_IN_GOMP_BARRIER], 0);
}

/* If a context was created for STMT when it was scanned, return it.  */

static omp_context *
maybe_lookup_ctx (gimple stmt)
{
  splay_tree_node n;
  n = splay_tree_lookup (all_contexts, (splay_tree_key) stmt);
  return n ? (omp_context *) n->value : NULL;
}


/* Find the mapping for DECL in CTX or the immediately enclosing
   context that has a mapping for DECL.

   If CTX is a nested parallel directive, we may have to use the decl
   mappings created in CTX's parent context.  Suppose that we have the
   following parallel nesting (variable UIDs showed for clarity):

	iD.1562 = 0;
     	#omp parallel shared(iD.1562)		-> outer parallel
	  iD.1562 = iD.1562 + 1;

	  #omp parallel shared (iD.1562)	-> inner parallel
	     iD.1562 = iD.1562 - 1;

   Each parallel structure will create a distinct .omp_data_s structure
   for copying iD.1562 in/out of the directive:

  	outer parallel		.omp_data_s.1.i -> iD.1562
	inner parallel		.omp_data_s.2.i -> iD.1562

   A shared variable mapping will produce a copy-out operation before
   the parallel directive and a copy-in operation after it.  So, in
   this case we would have:

  	iD.1562 = 0;
	.omp_data_o.1.i = iD.1562;
	#omp parallel shared(iD.1562)		-> outer parallel
	  .omp_data_i.1 = &.omp_data_o.1
	  .omp_data_i.1->i = .omp_data_i.1->i + 1;

	  .omp_data_o.2.i = iD.1562;		-> **
	  #omp parallel shared(iD.1562)		-> inner parallel
	    .omp_data_i.2 = &.omp_data_o.2
	    .omp_data_i.2->i = .omp_data_i.2->i - 1;


    ** This is a problem.  The symbol iD.1562 cannot be referenced
       inside the body of the outer parallel region.  But since we are
       emitting this copy operation while expanding the inner parallel
       directive, we need to access the CTX structure of the outer
       parallel directive to get the correct mapping:

	  .omp_data_o.2.i = .omp_data_i.1->i

    Since there may be other workshare or parallel directives enclosing
    the parallel directive, it may be necessary to walk up the context
    parent chain.  This is not a problem in general because nested
    parallelism happens only rarely.  */

static tree
lookup_decl_in_outer_ctx (tree decl, omp_context *ctx)
{
  tree t;
  omp_context *up;

  for (up = ctx->outer, t = NULL; up && t == NULL; up = up->outer)
    t = maybe_lookup_decl (decl, up);

  gcc_assert (!ctx->is_nested || t || is_global_var (decl));

  return t ? t : decl;
}


/* Similar to lookup_decl_in_outer_ctx, but return DECL if not found
   in outer contexts.  */

static tree
maybe_lookup_decl_in_outer_ctx (tree decl, omp_context *ctx)
{
  tree t = NULL;
  omp_context *up;

  for (up = ctx->outer, t = NULL; up && t == NULL; up = up->outer)
    t = maybe_lookup_decl (decl, up);

  return t ? t : decl;
}


/* Construct the initialization value for reduction CLAUSE.  */

tree
omp_reduction_init (tree clause, tree type)
{
  switch (OMP_CLAUSE_REDUCTION_CODE (clause))
    {
    case PLUS_EXPR:
    case MINUS_EXPR:
    case BIT_IOR_EXPR:
    case BIT_XOR_EXPR:
    case TRUTH_OR_EXPR:
    case TRUTH_ORIF_EXPR:
    case TRUTH_XOR_EXPR:
    case NE_EXPR:
      return fold_convert (type, integer_zero_node);

    case MULT_EXPR:
    case TRUTH_AND_EXPR:
    case TRUTH_ANDIF_EXPR:
    case EQ_EXPR:
      return fold_convert (type, integer_one_node);

    case BIT_AND_EXPR:
      return fold_convert (type, integer_minus_one_node);

    case MAX_EXPR:
      if (SCALAR_FLOAT_TYPE_P (type))
	{
	  REAL_VALUE_TYPE max, min;
	  if (HONOR_INFINITIES (TYPE_MODE (type)))
	    {
	      real_inf (&max);
	      real_arithmetic (&min, NEGATE_EXPR, &max, NULL);
	    }
	  else
	    real_maxval (&min, 1, TYPE_MODE (type));
	  return build_real (type, min);
	}
      else
	{
	  gcc_assert (INTEGRAL_TYPE_P (type));
	  return TYPE_MIN_VALUE (type);
	}

    case MIN_EXPR:
      if (SCALAR_FLOAT_TYPE_P (type))
	{
	  REAL_VALUE_TYPE max;
	  if (HONOR_INFINITIES (TYPE_MODE (type)))
	    real_inf (&max);
	  else
	    real_maxval (&max, 0, TYPE_MODE (type));
	  return build_real (type, max);
	}
      else
	{
	  gcc_assert (INTEGRAL_TYPE_P (type));
	  return TYPE_MAX_VALUE (type);
	}

    default:
      gcc_unreachable ();
    }
}

/* Generate code to implement the input clauses, FIRSTPRIVATE and COPYIN,
   from the receiver (aka child) side and initializers for REFERENCE_TYPE
   private variables.  Initialization statements go in ILIST, while calls
   to destructors go in DLIST.  */

static void
lower_rec_input_clauses (tree clauses, gimple_seq *ilist, gimple_seq *dlist,
			 omp_context *ctx)
{
  gimple_stmt_iterator diter;
  tree c, dtor, copyin_seq, x, ptr;
  bool copyin_by_ref = false;
  bool lastprivate_firstprivate = false;
  int pass;

  *dlist = gimple_seq_alloc ();
  diter = gsi_start (*dlist);
  copyin_seq = NULL;

  /* Do all the fixed sized types in the first pass, and the variable sized
     types in the second pass.  This makes sure that the scalar arguments to
     the variable sized types are processed before we use them in the 
     variable sized operations.  */
  for (pass = 0; pass < 2; ++pass)
    {
      for (c = clauses; c ; c = OMP_CLAUSE_CHAIN (c))
	{
	  enum omp_clause_code c_kind = OMP_CLAUSE_CODE (c);
	  tree var, new_var;
	  bool by_ref;

	  switch (c_kind)
	    {
	    case OMP_CLAUSE_PRIVATE:
	      if (OMP_CLAUSE_PRIVATE_DEBUG (c))
		continue;
	      break;
	    case OMP_CLAUSE_SHARED:
	      if (maybe_lookup_decl (OMP_CLAUSE_DECL (c), ctx) == NULL)
		{
		  gcc_assert (is_global_var (OMP_CLAUSE_DECL (c)));
		  continue;
		}
	    case OMP_CLAUSE_FIRSTPRIVATE:
	    case OMP_CLAUSE_COPYIN:
	    case OMP_CLAUSE_REDUCTION:
	      break;
	    case OMP_CLAUSE_LASTPRIVATE:
	      if (OMP_CLAUSE_LASTPRIVATE_FIRSTPRIVATE (c))
		{
		  lastprivate_firstprivate = true;
		  if (pass != 0)
		    continue;
		}
	      break;
	    default:
	      continue;
	    }

	  new_var = var = OMP_CLAUSE_DECL (c);
	  if (c_kind != OMP_CLAUSE_COPYIN)
	    new_var = lookup_decl (var, ctx);

	  if (c_kind == OMP_CLAUSE_SHARED || c_kind == OMP_CLAUSE_COPYIN)
	    {
	      if (pass != 0)
		continue;
	    }
	  else if (is_variable_sized (var))
	    {
	      /* For variable sized types, we need to allocate the
		 actual storage here.  Call alloca and store the
		 result in the pointer decl that we created elsewhere.  */
	      if (pass == 0)
		continue;

	      if (c_kind != OMP_CLAUSE_FIRSTPRIVATE || !is_task_ctx (ctx))
		{
		  gimple stmt;
		  tree tmp;

		  ptr = DECL_VALUE_EXPR (new_var);
		  gcc_assert (TREE_CODE (ptr) == INDIRECT_REF);
		  ptr = TREE_OPERAND (ptr, 0);
		  gcc_assert (DECL_P (ptr));
		  x = TYPE_SIZE_UNIT (TREE_TYPE (new_var));

		  /* void *tmp = __builtin_alloca */
		  stmt
		    = gimple_build_call (built_in_decls[BUILT_IN_ALLOCA], 1, x);
		  tmp = create_tmp_var_raw (ptr_type_node, NULL);
		  gimple_add_tmp_var (tmp);
		  gimple_call_set_lhs (stmt, tmp);

		  gimple_seq_add_stmt (ilist, stmt);

		  x = fold_convert (TREE_TYPE (ptr), tmp);
		  gimplify_assign (ptr, x, ilist);
		}
	    }
	  else if (is_reference (var))
	    {
	      /* For references that are being privatized for Fortran,
		 allocate new backing storage for the new pointer
		 variable.  This allows us to avoid changing all the
		 code that expects a pointer to something that expects
		 a direct variable.  Note that this doesn't apply to
		 C++, since reference types are disallowed in data
		 sharing clauses there, except for NRV optimized
		 return values.  */
	      if (pass == 0)
		continue;

	      x = TYPE_SIZE_UNIT (TREE_TYPE (TREE_TYPE (new_var)));
	      if (c_kind == OMP_CLAUSE_FIRSTPRIVATE && is_task_ctx (ctx))
		{
		  x = build_receiver_ref (var, false, ctx);
		  x = build_fold_addr_expr (x);
		}
	      else if (TREE_CONSTANT (x))
		{
		  const char *name = NULL;
		  if (DECL_NAME (var))
		    name = IDENTIFIER_POINTER (DECL_NAME (new_var));

		  x = create_tmp_var_raw (TREE_TYPE (TREE_TYPE (new_var)),
					  name);
		  gimple_add_tmp_var (x);
		  x = build_fold_addr_expr_with_type (x, TREE_TYPE (new_var));
		}
	      else
		{
		  x = build_call_expr (built_in_decls[BUILT_IN_ALLOCA], 1, x);
		  x = fold_convert (TREE_TYPE (new_var), x);
		}

	      gimplify_assign (new_var, x, ilist);

	      new_var = build_fold_indirect_ref (new_var);
	    }
	  else if (c_kind == OMP_CLAUSE_REDUCTION
		   && OMP_CLAUSE_REDUCTION_PLACEHOLDER (c))
	    {
	      if (pass == 0)
		continue;
	    }
	  else if (pass != 0)
	    continue;

	  switch (OMP_CLAUSE_CODE (c))
	    {
	    case OMP_CLAUSE_SHARED:
	      /* Shared global vars are just accessed directly.  */
	      if (is_global_var (new_var))
		break;
	      /* Set up the DECL_VALUE_EXPR for shared variables now.  This
		 needs to be delayed until after fixup_child_record_type so
		 that we get the correct type during the dereference.  */
	      by_ref = use_pointer_for_field (var, ctx);
	      x = build_receiver_ref (var, by_ref, ctx);
	      SET_DECL_VALUE_EXPR (new_var, x);
	      DECL_HAS_VALUE_EXPR_P (new_var) = 1;

	      /* ??? If VAR is not passed by reference, and the variable
		 hasn't been initialized yet, then we'll get a warning for
		 the store into the omp_data_s structure.  Ideally, we'd be
		 able to notice this and not store anything at all, but 
		 we're generating code too early.  Suppress the warning.  */
	      if (!by_ref)
		TREE_NO_WARNING (var) = 1;
	      break;

	    case OMP_CLAUSE_LASTPRIVATE:
	      if (OMP_CLAUSE_LASTPRIVATE_FIRSTPRIVATE (c))
		break;
	      /* FALLTHRU */

	    case OMP_CLAUSE_PRIVATE:
	      if (OMP_CLAUSE_CODE (c) != OMP_CLAUSE_PRIVATE)
		x = build_outer_var_ref (var, ctx);
	      else if (OMP_CLAUSE_PRIVATE_OUTER_REF (c))
		{
		  if (is_task_ctx (ctx))
		    x = build_receiver_ref (var, false, ctx);
		  else
		    x = build_outer_var_ref (var, ctx);
		}
	      else
		x = NULL;
	      x = lang_hooks.decls.omp_clause_default_ctor (c, new_var, x);
	      if (x)
		gimplify_and_add (x, ilist);
	      /* FALLTHRU */

	    do_dtor:
	      x = lang_hooks.decls.omp_clause_dtor (c, new_var);
	      if (x)
		{
		  gimple_seq tseq = NULL;

		  dtor = x;
		  gimplify_stmt (&dtor, &tseq);
		  gsi_insert_seq_before (&diter, tseq, GSI_SAME_STMT);
		}
	      break;

	    case OMP_CLAUSE_FIRSTPRIVATE:
	      if (is_task_ctx (ctx))
		{
		  if (is_reference (var) || is_variable_sized (var))
		    goto do_dtor;
		  else if (is_global_var (maybe_lookup_decl_in_outer_ctx (var,
									  ctx))
			   || use_pointer_for_field (var, NULL))
		    {
		      x = build_receiver_ref (var, false, ctx);
		      SET_DECL_VALUE_EXPR (new_var, x);
		      DECL_HAS_VALUE_EXPR_P (new_var) = 1;
		      goto do_dtor;
		    }
		}
	      x = build_outer_var_ref (var, ctx);
	      x = lang_hooks.decls.omp_clause_copy_ctor (c, new_var, x);
	      gimplify_and_add (x, ilist);
	      goto do_dtor;
	      break;

	    case OMP_CLAUSE_COPYIN:
	      by_ref = use_pointer_for_field (var, NULL);
	      x = build_receiver_ref (var, by_ref, ctx);
	      x = lang_hooks.decls.omp_clause_assign_op (c, new_var, x);
	      append_to_statement_list (x, &copyin_seq);
	      copyin_by_ref |= by_ref;
	      break;

	    case OMP_CLAUSE_REDUCTION:
	      if (OMP_CLAUSE_REDUCTION_PLACEHOLDER (c))
		{
		  tree placeholder = OMP_CLAUSE_REDUCTION_PLACEHOLDER (c);
		  x = build_outer_var_ref (var, ctx);

		  if (is_reference (var))
		    x = build_fold_addr_expr (x);
		  SET_DECL_VALUE_EXPR (placeholder, x);
		  DECL_HAS_VALUE_EXPR_P (placeholder) = 1;
		  lower_omp (OMP_CLAUSE_REDUCTION_GIMPLE_INIT (c), ctx);
		  gimple_seq_add_seq (ilist,
				      OMP_CLAUSE_REDUCTION_GIMPLE_INIT (c));
		  OMP_CLAUSE_REDUCTION_GIMPLE_INIT (c) = NULL;
		  DECL_HAS_VALUE_EXPR_P (placeholder) = 0;
		}
	      else
		{
		  x = omp_reduction_init (c, TREE_TYPE (new_var));
		  gcc_assert (TREE_CODE (TREE_TYPE (new_var)) != ARRAY_TYPE);
		  gimplify_assign (new_var, x, ilist);
		}
	      break;

	    default:
	      gcc_unreachable ();
	    }
	}
    }

  /* The copyin sequence is not to be executed by the main thread, since
     that would result in self-copies.  Perhaps not visible to scalars,
     but it certainly is to C++ operator=.  */
  if (copyin_seq)
    {
      x = build_call_expr (built_in_decls[BUILT_IN_OMP_GET_THREAD_NUM], 0);
      x = build2 (NE_EXPR, boolean_type_node, x,
		  build_int_cst (TREE_TYPE (x), 0));
      x = build3 (COND_EXPR, void_type_node, x, copyin_seq, NULL);
      gimplify_and_add (x, ilist);
    }

  /* If any copyin variable is passed by reference, we must ensure the
     master thread doesn't modify it before it is copied over in all
     threads.  Similarly for variables in both firstprivate and
     lastprivate clauses we need to ensure the lastprivate copying
     happens after firstprivate copying in all threads.  */
  if (copyin_by_ref || lastprivate_firstprivate)
    gimplify_and_add (build_omp_barrier (), ilist);
}


/* Generate code to implement the LASTPRIVATE clauses.  This is used for
   both parallel and workshare constructs.  PREDICATE may be NULL if it's
   always true.   */

static void
lower_lastprivate_clauses (tree clauses, tree predicate, gimple_seq *stmt_list,
			    omp_context *ctx)
{
  tree x, c, label = NULL;
  bool par_clauses = false;

  /* Early exit if there are no lastprivate clauses.  */
  clauses = find_omp_clause (clauses, OMP_CLAUSE_LASTPRIVATE);
  if (clauses == NULL)
    {
      /* If this was a workshare clause, see if it had been combined
	 with its parallel.  In that case, look for the clauses on the
	 parallel statement itself.  */
      if (is_parallel_ctx (ctx))
	return;

      ctx = ctx->outer;
      if (ctx == NULL || !is_parallel_ctx (ctx))
	return;

      clauses = find_omp_clause (gimple_omp_parallel_clauses (ctx->stmt),
				 OMP_CLAUSE_LASTPRIVATE);
      if (clauses == NULL)
	return;
      par_clauses = true;
    }

  if (predicate)
    {
      gimple stmt;
      tree label_true, arm1, arm2;

      label = create_artificial_label ();
      label_true = create_artificial_label ();
      arm1 = TREE_OPERAND (predicate, 0);
      arm2 = TREE_OPERAND (predicate, 1);
      gimplify_expr (&arm1, stmt_list, NULL, is_gimple_val, fb_rvalue);
      gimplify_expr (&arm2, stmt_list, NULL, is_gimple_val, fb_rvalue);
      stmt = gimple_build_cond (TREE_CODE (predicate), arm1, arm2,
				label_true, label);
      gimple_seq_add_stmt (stmt_list, stmt);
      gimple_seq_add_stmt (stmt_list, gimple_build_label (label_true));
    }

  for (c = clauses; c ;)
    {
      tree var, new_var;

      if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_LASTPRIVATE)
	{
	  var = OMP_CLAUSE_DECL (c);
	  new_var = lookup_decl (var, ctx);

	  if (OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ (c))
	    {
	      lower_omp (OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ (c), ctx);
	      gimple_seq_add_seq (stmt_list,
				  OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ (c));
	    }
	  OMP_CLAUSE_LASTPRIVATE_GIMPLE_SEQ (c) = NULL;

	  x = build_outer_var_ref (var, ctx);
	  if (is_reference (var))
	    new_var = build_fold_indirect_ref (new_var);
	  x = lang_hooks.decls.omp_clause_assign_op (c, x, new_var);
	  gimplify_and_add (x, stmt_list);
	}
      c = OMP_CLAUSE_CHAIN (c);
      if (c == NULL && !par_clauses)
	{
	  /* If this was a workshare clause, see if it had been combined
	     with its parallel.  In that case, continue looking for the
	     clauses also on the parallel statement itself.  */
	  if (is_parallel_ctx (ctx))
	    break;

	  ctx = ctx->outer;
	  if (ctx == NULL || !is_parallel_ctx (ctx))
	    break;

	  c = find_omp_clause (gimple_omp_parallel_clauses (ctx->stmt),
			       OMP_CLAUSE_LASTPRIVATE);
	  par_clauses = true;
	}
    }

  if (label)
    gimple_seq_add_stmt (stmt_list, gimple_build_label (label));
}


/* Generate code to implement the REDUCTION clauses.  */

static void
lower_reduction_clauses (tree clauses, gimple_seq *stmt_seqp, omp_context *ctx)
{
  gimple_seq sub_seq = NULL;
  gimple stmt;
  tree x, c;
  int count = 0;

  /* First see if there is exactly one reduction clause.  Use OMP_ATOMIC
     update in that case, otherwise use a lock.  */
  for (c = clauses; c && count < 2; c = OMP_CLAUSE_CHAIN (c))
    if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_REDUCTION)
      {
	if (OMP_CLAUSE_REDUCTION_PLACEHOLDER (c))
	  {
	    /* Never use OMP_ATOMIC for array reductions.  */
	    count = -1;
	    break;
	  }
	count++;
      }

  if (count == 0)
    return;

  for (c = clauses; c ; c = OMP_CLAUSE_CHAIN (c))
    {
      tree var, ref, new_var;
      enum tree_code code;

      if (OMP_CLAUSE_CODE (c) != OMP_CLAUSE_REDUCTION)
	continue;

      var = OMP_CLAUSE_DECL (c);
      new_var = lookup_decl (var, ctx);
      if (is_reference (var))
	new_var = build_fold_indirect_ref (new_var);
      ref = build_outer_var_ref (var, ctx);
      code = OMP_CLAUSE_REDUCTION_CODE (c);

      /* reduction(-:var) sums up the partial results, so it acts
	 identically to reduction(+:var).  */
      if (code == MINUS_EXPR)
        code = PLUS_EXPR;

      if (count == 1)
	{
	  tree addr = build_fold_addr_expr (ref);

	  addr = save_expr (addr);
	  ref = build1 (INDIRECT_REF, TREE_TYPE (TREE_TYPE (addr)), addr);
	  x = fold_build2 (code, TREE_TYPE (ref), ref, new_var);
	  x = build2 (OMP_ATOMIC, void_type_node, addr, x);
	  gimplify_and_add (x, stmt_seqp);
	  return;
	}

      if (OMP_CLAUSE_REDUCTION_PLACEHOLDER (c))
	{
	  tree placeholder = OMP_CLAUSE_REDUCTION_PLACEHOLDER (c);

	  if (is_reference (var))
	    ref = build_fold_addr_expr (ref);
	  SET_DECL_VALUE_EXPR (placeholder, ref);
	  DECL_HAS_VALUE_EXPR_P (placeholder) = 1;
	  lower_omp (OMP_CLAUSE_REDUCTION_GIMPLE_MERGE (c), ctx);
	  gimple_seq_add_seq (&sub_seq, OMP_CLAUSE_REDUCTION_GIMPLE_MERGE (c));
	  OMP_CLAUSE_REDUCTION_GIMPLE_MERGE (c) = NULL;
	  OMP_CLAUSE_REDUCTION_PLACEHOLDER (c) = NULL;
	}
      else
	{
	  x = build2 (code, TREE_TYPE (ref), ref, new_var);
	  ref = build_outer_var_ref (var, ctx);
	  gimplify_assign (ref, x, &sub_seq);
	}
    }

  stmt = gimple_build_call (built_in_decls[BUILT_IN_GOMP_ATOMIC_START], 0);
  gimple_seq_add_stmt (stmt_seqp, stmt);

  gimple_seq_add_seq (stmt_seqp, sub_seq);

  stmt = gimple_build_call (built_in_decls[BUILT_IN_GOMP_ATOMIC_END], 0);
  gimple_seq_add_stmt (stmt_seqp, stmt);
}


/* Generate code to implement the COPYPRIVATE clauses.  */

static void
lower_copyprivate_clauses (tree clauses, gimple_seq *slist, gimple_seq *rlist,
			    omp_context *ctx)
{
  tree c;

  for (c = clauses; c ; c = OMP_CLAUSE_CHAIN (c))
    {
      tree var, ref, x;
      bool by_ref;

      if (OMP_CLAUSE_CODE (c) != OMP_CLAUSE_COPYPRIVATE)
	continue;

      var = OMP_CLAUSE_DECL (c);
      by_ref = use_pointer_for_field (var, NULL);

      ref = build_sender_ref (var, ctx);
      x = lookup_decl_in_outer_ctx (var, ctx);
      x = by_ref ? build_fold_addr_expr (x) : x;
      gimplify_assign (ref, x, slist);

      ref = build_receiver_ref (var, by_ref, ctx);
      if (is_reference (var))
	{
	  ref = build_fold_indirect_ref (ref);
	  var = build_fold_indirect_ref (var);
	}
      x = lang_hooks.decls.omp_clause_assign_op (c, var, ref);
      gimplify_and_add (x, rlist);
    }
}


/* Generate code to implement the clauses, FIRSTPRIVATE, COPYIN, LASTPRIVATE,
   and REDUCTION from the sender (aka parent) side.  */

static void
lower_send_clauses (tree clauses, gimple_seq *ilist, gimple_seq *olist,
    		    omp_context *ctx)
{
  tree c;

  for (c = clauses; c ; c = OMP_CLAUSE_CHAIN (c))
    {
      tree val, ref, x, var;
      bool by_ref, do_in = false, do_out = false;

      switch (OMP_CLAUSE_CODE (c))
	{
	case OMP_CLAUSE_PRIVATE:
	  if (OMP_CLAUSE_PRIVATE_OUTER_REF (c))
	    break;
	  continue;
	case OMP_CLAUSE_FIRSTPRIVATE:
	case OMP_CLAUSE_COPYIN:
	case OMP_CLAUSE_LASTPRIVATE:
	case OMP_CLAUSE_REDUCTION:
	  break;
	default:
	  continue;
	}

      val = OMP_CLAUSE_DECL (c);
      var = lookup_decl_in_outer_ctx (val, ctx);

      if (OMP_CLAUSE_CODE (c) != OMP_CLAUSE_COPYIN
	  && is_global_var (var))
	continue;
      if (is_variable_sized (val))
	continue;
      by_ref = use_pointer_for_field (val, NULL);

      switch (OMP_CLAUSE_CODE (c))
	{
	case OMP_CLAUSE_PRIVATE:
	case OMP_CLAUSE_FIRSTPRIVATE:
	case OMP_CLAUSE_COPYIN:
	  do_in = true;
	  break;

	case OMP_CLAUSE_LASTPRIVATE:
	  if (by_ref || is_reference (val))
	    {
	      if (OMP_CLAUSE_LASTPRIVATE_FIRSTPRIVATE (c))
		continue;
	      do_in = true;
	    }
	  else
	    {
	      do_out = true;
	      if (lang_hooks.decls.omp_private_outer_ref (val))
		do_in = true;
	    }
	  break;

	case OMP_CLAUSE_REDUCTION:
	  do_in = true;
	  do_out = !(by_ref || is_reference (val));
	  break;

	default:
	  gcc_unreachable ();
	}

      if (do_in)
	{
	  ref = build_sender_ref (val, ctx);
	  x = by_ref ? build_fold_addr_expr (var) : var;
	  gimplify_assign (ref, x, ilist);
	  if (is_task_ctx (ctx))
	    DECL_ABSTRACT_ORIGIN (TREE_OPERAND (ref, 1)) = NULL;
	}

      if (do_out)
	{
	  ref = build_sender_ref (val, ctx);
	  gimplify_assign (var, ref, olist);
	}
    }
}

/* Generate code to implement SHARED from the sender (aka parent)
   side.  This is trickier, since GIMPLE_OMP_PARALLEL_CLAUSES doesn't
   list things that got automatically shared.  */

static void
lower_send_shared_vars (gimple_seq *ilist, gimple_seq *olist, omp_context *ctx)
{
  tree var, ovar, nvar, f, x, record_type;

  if (ctx->record_type == NULL)
    return;

  record_type = ctx->srecord_type ? ctx->srecord_type : ctx->record_type;
  for (f = TYPE_FIELDS (record_type); f ; f = TREE_CHAIN (f))
    {
      ovar = DECL_ABSTRACT_ORIGIN (f);
      nvar = maybe_lookup_decl (ovar, ctx);
      if (!nvar || !DECL_HAS_VALUE_EXPR_P (nvar))
	continue;

      /* If CTX is a nested parallel directive.  Find the immediately
	 enclosing parallel or workshare construct that contains a
	 mapping for OVAR.  */
      var = lookup_decl_in_outer_ctx (ovar, ctx);

      if (use_pointer_for_field (ovar, ctx))
	{
	  x = build_sender_ref (ovar, ctx);
	  var = build_fold_addr_expr (var);
	  gimplify_assign (x, var, ilist);
	}
      else
	{
	  x = build_sender_ref (ovar, ctx);
	  gimplify_assign (x, var, ilist);

	  if (!TREE_READONLY (var)
	      /* We don't need to receive a new reference to a result
	         or parm decl.  In fact we may not store to it as we will
		 invalidate any pending RSO and generate wrong gimple
		 during inlining.  */
	      && !((TREE_CODE (var) == RESULT_DECL
		    || TREE_CODE (var) == PARM_DECL)
		   && DECL_BY_REFERENCE (var)))
	    {
	      x = build_sender_ref (ovar, ctx);
	      gimplify_assign (var, x, olist);
	    }
	}
    }
}


/* A convenience function to build an empty GIMPLE_COND with just the
   condition.  */

static gimple
gimple_build_cond_empty (tree cond)
{
  enum tree_code pred_code;
  tree lhs, rhs;

  gimple_cond_get_ops_from_tree (cond, &pred_code, &lhs, &rhs);
  return gimple_build_cond (pred_code, lhs, rhs, NULL_TREE, NULL_TREE);
}


/* Build the function calls to GOMP_parallel_start etc to actually 
   generate the parallel operation.  REGION is the parallel region
   being expanded.  BB is the block where to insert the code.  WS_ARGS
   will be set if this is a call to a combined parallel+workshare
   construct, it contains the list of additional arguments needed by
   the workshare construct.  */

static void
expand_parallel_call (struct omp_region *region, basic_block bb,
		      gimple entry_stmt, tree ws_args)
{
  tree t, t1, t2, val, cond, c, clauses;
  gimple_stmt_iterator gsi;
  gimple stmt;
  int start_ix;

  clauses = gimple_omp_parallel_clauses (entry_stmt);

  /* Determine what flavor of GOMP_parallel_start we will be
     emitting.  */
  start_ix = BUILT_IN_GOMP_PARALLEL_START;
  if (is_combined_parallel (region))
    {
      switch (region->inner->type)
	{
	case GIMPLE_OMP_FOR:
	  gcc_assert (region->inner->sched_kind != OMP_CLAUSE_SCHEDULE_AUTO);
	  start_ix = BUILT_IN_GOMP_PARALLEL_LOOP_STATIC_START
		     + (region->inner->sched_kind
			== OMP_CLAUSE_SCHEDULE_RUNTIME
			? 3 : region->inner->sched_kind);
	  break;
	case GIMPLE_OMP_SECTIONS:
	  start_ix = BUILT_IN_GOMP_PARALLEL_SECTIONS_START;
	  break;
	default:
	  gcc_unreachable ();
	}
    }

  /* By default, the value of NUM_THREADS is zero (selected at run time)
     and there is no conditional.  */
  cond = NULL_TREE;
  val = build_int_cst (unsigned_type_node, 0);

  c = find_omp_clause (clauses, OMP_CLAUSE_IF);
  if (c)
    cond = OMP_CLAUSE_IF_EXPR (c);

  c = find_omp_clause (clauses, OMP_CLAUSE_NUM_THREADS);
  if (c)
    val = OMP_CLAUSE_NUM_THREADS_EXPR (c);

  /* Ensure 'val' is of the correct type.  */
  val = fold_convert (unsigned_type_node, val);

  /* If we found the clause 'if (cond)', build either
     (cond != 0) or (cond ? val : 1u).  */
  if (cond)
    {
      gimple_stmt_iterator gsi;

      cond = gimple_boolify (cond);

      if (integer_zerop (val))
	val = fold_build2 (EQ_EXPR, unsigned_type_node, cond,
			   build_int_cst (TREE_TYPE (cond), 0));
      else
	{
	  basic_block cond_bb, then_bb, else_bb;
	  edge e, e_then, e_else;
	  tree tmp_then, tmp_else, tmp_join, tmp_var;

	  tmp_var = create_tmp_var (TREE_TYPE (val), NULL);
	  if (gimple_in_ssa_p (cfun))
	    {
	      tmp_then = make_ssa_name (tmp_var, NULL);
	      tmp_else = make_ssa_name (tmp_var, NULL);
	      tmp_join = make_ssa_name (tmp_var, NULL);
	    }
	  else
	    {
	      tmp_then = tmp_var;
	      tmp_else = tmp_var;
	      tmp_join = tmp_var;
	    }

	  e = split_block (bb, NULL);
	  cond_bb = e->src;
	  bb = e->dest;
	  remove_edge (e);

	  then_bb = create_empty_bb (cond_bb);
	  else_bb = create_empty_bb (then_bb);
	  set_immediate_dominator (CDI_DOMINATORS, then_bb, cond_bb);
	  set_immediate_dominator (CDI_DOMINATORS, else_bb, cond_bb);

	  stmt = gimple_build_cond_empty (cond);
	  gsi = gsi_start_bb (cond_bb);
	  gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);

	  gsi = gsi_start_bb (then_bb);
	  stmt = gimple_build_assign (tmp_then, val);
	  gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);

	  gsi = gsi_start_bb (else_bb);
	  stmt = gimple_build_assign
	    	   (tmp_else, build_int_cst (unsigned_type_node, 1));
	  gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);

	  make_edge (cond_bb, then_bb, EDGE_TRUE_VALUE);
	  make_edge (cond_bb, else_bb, EDGE_FALSE_VALUE);
	  e_then = make_edge (then_bb, bb, EDGE_FALLTHRU);
	  e_else = make_edge (else_bb, bb, EDGE_FALLTHRU);

	  if (gimple_in_ssa_p (cfun))
	    {
	      gimple phi = create_phi_node (tmp_join, bb);
	      SSA_NAME_DEF_STMT (tmp_join) = phi;
	      add_phi_arg (phi, tmp_then, e_then);
	      add_phi_arg (phi, tmp_else, e_else);
	    }

	  val = tmp_join;
	}

      gsi = gsi_start_bb (bb);
      val = force_gimple_operand_gsi (&gsi, val, true, NULL_TREE,
				      false, GSI_CONTINUE_LINKING);
    }

  gsi = gsi_last_bb (bb);
  t = gimple_omp_parallel_data_arg (entry_stmt);
  if (t == NULL)
    t1 = null_pointer_node;
  else
    t1 = build_fold_addr_expr (t);
  t2 = build_fold_addr_expr (gimple_omp_parallel_child_fn (entry_stmt));

  if (ws_args)
    {
      tree args = tree_cons (NULL, t2,
			     tree_cons (NULL, t1,
					tree_cons (NULL, val, ws_args)));
      t = build_function_call_expr (built_in_decls[start_ix], args);
    }
  else
    t = build_call_expr (built_in_decls[start_ix], 3, t2, t1, val);

  force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
			    false, GSI_CONTINUE_LINKING);

  t = gimple_omp_parallel_data_arg (entry_stmt);
  if (t == NULL)
    t = null_pointer_node;
  else
    t = build_fold_addr_expr (t);
  t = build_call_expr (gimple_omp_parallel_child_fn (entry_stmt), 1, t);
  force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
			    false, GSI_CONTINUE_LINKING);

  t = build_call_expr (built_in_decls[BUILT_IN_GOMP_PARALLEL_END], 0);
  force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
			    false, GSI_CONTINUE_LINKING);
}


/* Build the function call to GOMP_task to actually
   generate the task operation.  BB is the block where to insert the code.  */

static void
expand_task_call (basic_block bb, gimple entry_stmt)
{
  tree t, t1, t2, t3, flags, cond, c, clauses;
  gimple_stmt_iterator gsi;

  clauses = gimple_omp_task_clauses (entry_stmt);

  c = find_omp_clause (clauses, OMP_CLAUSE_IF);
  if (c)
    cond = gimple_boolify (OMP_CLAUSE_IF_EXPR (c));
  else
    cond = boolean_true_node;

  c = find_omp_clause (clauses, OMP_CLAUSE_UNTIED);
  flags = build_int_cst (unsigned_type_node, (c ? 1 : 0));

  gsi = gsi_last_bb (bb);
  t = gimple_omp_task_data_arg (entry_stmt);
  if (t == NULL)
    t2 = null_pointer_node;
  else
    t2 = build_fold_addr_expr (t);
  t1 = build_fold_addr_expr (gimple_omp_task_child_fn (entry_stmt));
  t = gimple_omp_task_copy_fn (entry_stmt);
  if (t == NULL)
    t3 = null_pointer_node;
  else
    t3 = build_fold_addr_expr (t);

  t = build_call_expr (built_in_decls[BUILT_IN_GOMP_TASK], 7, t1, t2, t3,
		       gimple_omp_task_arg_size (entry_stmt),
		       gimple_omp_task_arg_align (entry_stmt), cond, flags);

  force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
			    false, GSI_CONTINUE_LINKING);
}


/* If exceptions are enabled, wrap the statements in BODY in a MUST_NOT_THROW
   catch handler and return it.  This prevents programs from violating the
   structured block semantics with throws.  */

static gimple_seq
maybe_catch_exception (gimple_seq body)
{
  gimple f, t;

  if (!flag_exceptions)
    return body;

  if (lang_protect_cleanup_actions)
    t = lang_protect_cleanup_actions ();
  else
    t = gimple_build_call (built_in_decls[BUILT_IN_TRAP], 0);

  f = gimple_build_eh_filter (NULL, gimple_seq_alloc_with_stmt (t));
  gimple_eh_filter_set_must_not_throw (f, true);

  t = gimple_build_try (body, gimple_seq_alloc_with_stmt (f),
      			GIMPLE_TRY_CATCH);

 return gimple_seq_alloc_with_stmt (t);
}

/* Chain all the DECLs in LIST by their TREE_CHAIN fields.  */

static tree
list2chain (tree list)
{
  tree t;

  for (t = list; t; t = TREE_CHAIN (t))
    {
      tree var = TREE_VALUE (t);
      if (TREE_CHAIN (t))
	TREE_CHAIN (var) = TREE_VALUE (TREE_CHAIN (t));
      else
	TREE_CHAIN (var) = NULL_TREE;
    }

  return list ? TREE_VALUE (list) : NULL_TREE;
}


/* Remove barriers in REGION->EXIT's block.  Note that this is only
   valid for GIMPLE_OMP_PARALLEL regions.  Since the end of a parallel region
   is an implicit barrier, any workshare inside the GIMPLE_OMP_PARALLEL that
   left a barrier at the end of the GIMPLE_OMP_PARALLEL region can now be
   removed.  */

static void
remove_exit_barrier (struct omp_region *region)
{
  gimple_stmt_iterator gsi;
  basic_block exit_bb;
  edge_iterator ei;
  edge e;
  gimple stmt;
  int any_addressable_vars = -1;

  exit_bb = region->exit;

  /* If the parallel region doesn't return, we don't have REGION->EXIT
     block at all.  */
  if (! exit_bb)
    return;

  /* The last insn in the block will be the parallel's GIMPLE_OMP_RETURN.  The
     workshare's GIMPLE_OMP_RETURN will be in a preceding block.  The kinds of
     statements that can appear in between are extremely limited -- no
     memory operations at all.  Here, we allow nothing at all, so the
     only thing we allow to precede this GIMPLE_OMP_RETURN is a label.  */
  gsi = gsi_last_bb (exit_bb);
  gcc_assert (gimple_code (gsi_stmt (gsi)) == GIMPLE_OMP_RETURN);
  gsi_prev (&gsi);
  if (!gsi_end_p (gsi) && gimple_code (gsi_stmt (gsi)) != GIMPLE_LABEL)
    return;

  FOR_EACH_EDGE (e, ei, exit_bb->preds)
    {
      gsi = gsi_last_bb (e->src);
      if (gsi_end_p (gsi))
	continue;
      stmt = gsi_stmt (gsi);
      if (gimple_code (stmt) == GIMPLE_OMP_RETURN
	  && !gimple_omp_return_nowait_p (stmt))
	{
	  /* OpenMP 3.0 tasks unfortunately prevent this optimization
	     in many cases.  If there could be tasks queued, the barrier
	     might be needed to let the tasks run before some local
	     variable of the parallel that the task uses as shared
	     runs out of scope.  The task can be spawned either
	     from within current function (this would be easy to check)
	     or from some function it calls and gets passed an address
	     of such a variable.  */
	  if (any_addressable_vars < 0)
	    {
	      gimple parallel_stmt = last_stmt (region->entry);
	      tree child_fun = gimple_omp_parallel_child_fn (parallel_stmt);
	      tree local_decls = DECL_STRUCT_FUNCTION (child_fun)->local_decls;
	      tree block;

	      any_addressable_vars = 0;
	      for (; local_decls; local_decls = TREE_CHAIN (local_decls))
		if (TREE_ADDRESSABLE (TREE_VALUE (local_decls)))
		  {
		    any_addressable_vars = 1;
		    break;
		  }
	      for (block = gimple_block (stmt);
		   !any_addressable_vars
		   && block
		   && TREE_CODE (block) == BLOCK;
		   block = BLOCK_SUPERCONTEXT (block))
		{
		  for (local_decls = BLOCK_VARS (block);
		       local_decls;
		       local_decls = TREE_CHAIN (local_decls))
		    if (TREE_ADDRESSABLE (local_decls))
		      {
			any_addressable_vars = 1;
			break;
		      }
		  if (block == gimple_block (parallel_stmt))
		    break;
		}
	    }
	  if (!any_addressable_vars)
	    gimple_omp_return_set_nowait (stmt);
	}
    }
}

static void
remove_exit_barriers (struct omp_region *region)
{
  if (region->type == GIMPLE_OMP_PARALLEL)
    remove_exit_barrier (region);

  if (region->inner)
    {
      region = region->inner;
      remove_exit_barriers (region);
      while (region->next)
	{
	  region = region->next;
	  remove_exit_barriers (region);
	}
    }
}

/* Optimize omp_get_thread_num () and omp_get_num_threads ()
   calls.  These can't be declared as const functions, but
   within one parallel body they are constant, so they can be
   transformed there into __builtin_omp_get_{thread_num,num_threads} ()
   which are declared const.  Similarly for task body, except
   that in untied task omp_get_thread_num () can change at any task
   scheduling point.  */

static void
optimize_omp_library_calls (gimple entry_stmt)
{
  basic_block bb;
  gimple_stmt_iterator gsi;
  tree thr_num_id
    = DECL_ASSEMBLER_NAME (built_in_decls [BUILT_IN_OMP_GET_THREAD_NUM]);
  tree num_thr_id
    = DECL_ASSEMBLER_NAME (built_in_decls [BUILT_IN_OMP_GET_NUM_THREADS]);
  bool untied_task = (gimple_code (entry_stmt) == GIMPLE_OMP_TASK
		      && find_omp_clause (gimple_omp_task_clauses (entry_stmt),
					  OMP_CLAUSE_UNTIED) != NULL);

  FOR_EACH_BB (bb)
    for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
      {
	gimple call = gsi_stmt (gsi);
	tree decl;

	if (is_gimple_call (call)
	    && (decl = gimple_call_fndecl (call))
	    && DECL_EXTERNAL (decl)
	    && TREE_PUBLIC (decl)
	    && DECL_INITIAL (decl) == NULL)
	  {
	    tree built_in;

	    if (DECL_NAME (decl) == thr_num_id)
	      {
		/* In #pragma omp task untied omp_get_thread_num () can change
		   during the execution of the task region.  */
		if (untied_task)
		  continue;
		built_in = built_in_decls [BUILT_IN_OMP_GET_THREAD_NUM];
	      }
	    else if (DECL_NAME (decl) == num_thr_id)
	      built_in = built_in_decls [BUILT_IN_OMP_GET_NUM_THREADS];
	    else
	      continue;

	    if (DECL_ASSEMBLER_NAME (decl) != DECL_ASSEMBLER_NAME (built_in)
		|| gimple_call_num_args (call) != 0)
	      continue;

	    if (flag_exceptions && !TREE_NOTHROW (decl))
	      continue;

	    if (TREE_CODE (TREE_TYPE (decl)) != FUNCTION_TYPE
		|| TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (decl)))
		   != TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (built_in))))
	      continue;

	    gimple_call_set_fndecl (call, built_in);
	  }
      }
}

/* Expand the OpenMP parallel or task directive starting at REGION.  */

static void
expand_omp_taskreg (struct omp_region *region)
{
  basic_block entry_bb, exit_bb, new_bb;
  struct function *child_cfun;
  tree child_fn, block, t, ws_args, *tp;
  gimple_stmt_iterator gsi;
  gimple entry_stmt, stmt;
  edge e;

  entry_stmt = last_stmt (region->entry);
  child_fn = gimple_omp_taskreg_child_fn (entry_stmt);
  child_cfun = DECL_STRUCT_FUNCTION (child_fn);
  /* If this function has been already instrumented, make sure
     the child function isn't instrumented again.  */
  child_cfun->after_tree_profile = cfun->after_tree_profile;

  entry_bb = region->entry;
  exit_bb = region->exit;

  if (is_combined_parallel (region))
    ws_args = region->ws_args;
  else
    ws_args = NULL_TREE;

  if (child_cfun->cfg)
    {
      /* Due to inlining, it may happen that we have already outlined
	 the region, in which case all we need to do is make the
	 sub-graph unreachable and emit the parallel call.  */
      edge entry_succ_e, exit_succ_e;
      gimple_stmt_iterator gsi;

      entry_succ_e = single_succ_edge (entry_bb);

      gsi = gsi_last_bb (entry_bb);
      gcc_assert (gimple_code (gsi_stmt (gsi)) == GIMPLE_OMP_PARALLEL
		  || gimple_code (gsi_stmt (gsi)) == GIMPLE_OMP_TASK);
      gsi_remove (&gsi, true);

      new_bb = entry_bb;
      if (exit_bb)
	{
	  exit_succ_e = single_succ_edge (exit_bb);
	  make_edge (new_bb, exit_succ_e->dest, EDGE_FALLTHRU);
	}
      remove_edge_and_dominated_blocks (entry_succ_e);
    }
  else
    {
      /* If the parallel region needs data sent from the parent
	 function, then the very first statement (except possible
	 tree profile counter updates) of the parallel body
	 is a copy assignment .OMP_DATA_I = &.OMP_DATA_O.  Since
	 &.OMP_DATA_O is passed as an argument to the child function,
	 we need to replace it with the argument as seen by the child
	 function.

	 In most cases, this will end up being the identity assignment
	 .OMP_DATA_I = .OMP_DATA_I.  However, if the parallel body had
	 a function call that has been inlined, the original PARM_DECL
	 .OMP_DATA_I may have been converted into a different local
	 variable.  In which case, we need to keep the assignment.  */
      if (gimple_omp_taskreg_data_arg (entry_stmt))
	{
	  basic_block entry_succ_bb = single_succ (entry_bb);
	  gimple_stmt_iterator gsi;
	  tree arg, narg;
	  gimple parcopy_stmt = NULL;

	  for (gsi = gsi_start_bb (entry_succ_bb); ; gsi_next (&gsi))
	    {
	      gimple stmt;

	      gcc_assert (!gsi_end_p (gsi));
	      stmt = gsi_stmt (gsi);
	      if (gimple_code (stmt) != GIMPLE_ASSIGN)
		continue;

	      if (gimple_num_ops (stmt) == 2)
		{
		  tree arg = gimple_assign_rhs1 (stmt);

		  /* We're ignore the subcode because we're
		     effectively doing a STRIP_NOPS.  */

		  if (TREE_CODE (arg) == ADDR_EXPR
		      && TREE_OPERAND (arg, 0)
		        == gimple_omp_taskreg_data_arg (entry_stmt))
		    {
		      parcopy_stmt = stmt;
		      break;
		    }
		}
	    }

	  gcc_assert (parcopy_stmt != NULL);
	  arg = DECL_ARGUMENTS (child_fn);

	  if (!gimple_in_ssa_p (cfun))
	    {
	      if (gimple_assign_lhs (parcopy_stmt) == arg)
		gsi_remove (&gsi, true);
	      else
		{
	          /* ?? Is setting the subcode really necessary ??  */
		  gimple_omp_set_subcode (parcopy_stmt, TREE_CODE (arg));
		  gimple_assign_set_rhs1 (parcopy_stmt, arg);
		}
	    }
	  else
	    {
	      /* If we are in ssa form, we must load the value from the default
		 definition of the argument.  That should not be defined now,
		 since the argument is not used uninitialized.  */
	      gcc_assert (gimple_default_def (cfun, arg) == NULL);
	      narg = make_ssa_name (arg, gimple_build_nop ());
	      set_default_def (arg, narg);
	      /* ?? Is setting the subcode really necessary ??  */
	      gimple_omp_set_subcode (parcopy_stmt, TREE_CODE (narg));
	      gimple_assign_set_rhs1 (parcopy_stmt, narg);
	      update_stmt (parcopy_stmt);
	    }
	}

      /* Declare local variables needed in CHILD_CFUN.  */
      block = DECL_INITIAL (child_fn);
      BLOCK_VARS (block) = list2chain (child_cfun->local_decls);
      /* The gimplifier could record temporaries in parallel/task block
	 rather than in containing function's local_decls chain,
	 which would mean cgraph missed finalizing them.  Do it now.  */
      for (t = BLOCK_VARS (block); t; t = TREE_CHAIN (t))
	if (TREE_CODE (t) == VAR_DECL
	    && TREE_STATIC (t)
	    && !DECL_EXTERNAL (t))
	  varpool_finalize_decl (t);
      DECL_SAVED_TREE (child_fn) = NULL;
      gimple_set_body (child_fn, bb_seq (single_succ (entry_bb)));
      TREE_USED (block) = 1;

      /* Reset DECL_CONTEXT on function arguments.  */
      for (t = DECL_ARGUMENTS (child_fn); t; t = TREE_CHAIN (t))
	DECL_CONTEXT (t) = child_fn;

      /* Split ENTRY_BB at GIMPLE_OMP_PARALLEL or GIMPLE_OMP_TASK,
	 so that it can be moved to the child function.  */
      gsi = gsi_last_bb (entry_bb);
      stmt = gsi_stmt (gsi);
      gcc_assert (stmt && (gimple_code (stmt) == GIMPLE_OMP_PARALLEL
			   || gimple_code (stmt) == GIMPLE_OMP_TASK));
      gsi_remove (&gsi, true);
      e = split_block (entry_bb, stmt);
      entry_bb = e->dest;
      single_succ_edge (entry_bb)->flags = EDGE_FALLTHRU;

      /* Convert GIMPLE_OMP_RETURN into a RETURN_EXPR.  */
      if (exit_bb)
	{
	  gsi = gsi_last_bb (exit_bb);
	  gcc_assert (!gsi_end_p (gsi)
		      && gimple_code (gsi_stmt (gsi)) == GIMPLE_OMP_RETURN);
	  stmt = gimple_build_return (NULL);
	  gsi_insert_after (&gsi, stmt, GSI_SAME_STMT);
	  gsi_remove (&gsi, true);
	}

      /* Move the parallel region into CHILD_CFUN.  */
 
      if (gimple_in_ssa_p (cfun))
	{
	  push_cfun (child_cfun);
	  init_tree_ssa (child_cfun);
	  init_ssa_operands ();
	  cfun->gimple_df->in_ssa_p = true;
	  pop_cfun ();
	  block = NULL_TREE;
	}
      else
	block = gimple_block (entry_stmt);

      new_bb = move_sese_region_to_fn (child_cfun, entry_bb, exit_bb, block);
      if (exit_bb)
	single_succ_edge (new_bb)->flags = EDGE_FALLTHRU;

      /* Remove non-local VAR_DECLs from child_cfun->local_decls list.  */
      for (tp = &child_cfun->local_decls; *tp; )
	if (DECL_CONTEXT (TREE_VALUE (*tp)) != cfun->decl)
	  tp = &TREE_CHAIN (*tp);
	else
	  *tp = TREE_CHAIN (*tp);

      /* Inform the callgraph about the new function.  */
      DECL_STRUCT_FUNCTION (child_fn)->curr_properties
	= cfun->curr_properties;
      cgraph_add_new_function (child_fn, true);

      /* Fix the callgraph edges for child_cfun.  Those for cfun will be
	 fixed in a following pass.  */
      push_cfun (child_cfun);
      if (optimize)
	optimize_omp_library_calls (entry_stmt);
      rebuild_cgraph_edges ();

      /* Some EH regions might become dead, see PR34608.  If
	 pass_cleanup_cfg isn't the first pass to happen with the
	 new child, these dead EH edges might cause problems.
	 Clean them up now.  */
      if (flag_exceptions)
	{
	  basic_block bb;
	  tree save_current = current_function_decl;
	  bool changed = false;

	  current_function_decl = child_fn;
	  FOR_EACH_BB (bb)
	    changed |= gimple_purge_dead_eh_edges (bb);
	  if (changed)
	    cleanup_tree_cfg ();
	  current_function_decl = save_current;
	}
      pop_cfun ();
    }
  
  /* Emit a library call to launch the children threads.  */
  if (gimple_code (entry_stmt) == GIMPLE_OMP_PARALLEL)
    expand_parallel_call (region, new_bb, entry_stmt, ws_args);
  else
    expand_task_call (new_bb, entry_stmt);
  update_ssa (TODO_update_ssa_only_virtuals);
}


/* A subroutine of expand_omp_for.  Generate code for a parallel
   loop with any schedule.  Given parameters:

	for (V = N1; V cond N2; V += STEP) BODY;

   where COND is "<" or ">", we generate pseudocode

	more = GOMP_loop_foo_start (N1, N2, STEP, CHUNK, &istart0, &iend0);
	if (more) goto L0; else goto L3;
    L0:
	V = istart0;
	iend = iend0;
    L1:
	BODY;
	V += STEP;
	if (V cond iend) goto L1; else goto L2;
    L2:
	if (GOMP_loop_foo_next (&istart0, &iend0)) goto L0; else goto L3;
    L3:

    If this is a combined omp parallel loop, instead of the call to
    GOMP_loop_foo_start, we call GOMP_loop_foo_next.

    For collapsed loops, given parameters:
      collapse(3)
      for (V1 = N11; V1 cond1 N12; V1 += STEP1)
	for (V2 = N21; V2 cond2 N22; V2 += STEP2)
	  for (V3 = N31; V3 cond3 N32; V3 += STEP3)
	    BODY;

    we generate pseudocode

	if (cond3 is <)
	  adj = STEP3 - 1;
	else
	  adj = STEP3 + 1;
	count3 = (adj + N32 - N31) / STEP3;
	if (cond2 is <)
	  adj = STEP2 - 1;
	else
	  adj = STEP2 + 1;
	count2 = (adj + N22 - N21) / STEP2;
	if (cond1 is <)
	  adj = STEP1 - 1;
	else
	  adj = STEP1 + 1;
	count1 = (adj + N12 - N11) / STEP1;
	count = count1 * count2 * count3;
	more = GOMP_loop_foo_start (0, count, 1, CHUNK, &istart0, &iend0);
	if (more) goto L0; else goto L3;
    L0:
	V = istart0;
	T = V;
	V3 = N31 + (T % count3) * STEP3;
	T = T / count3;
	V2 = N21 + (T % count2) * STEP2;
	T = T / count2;
	V1 = N11 + T * STEP1;
	iend = iend0;
    L1:
	BODY;
	V += 1;
	if (V < iend) goto L10; else goto L2;
    L10:
	V3 += STEP3;
	if (V3 cond3 N32) goto L1; else goto L11;
    L11:
	V3 = N31;
	V2 += STEP2;
	if (V2 cond2 N22) goto L1; else goto L12;
    L12:
	V2 = N21;
	V1 += STEP1;
	goto L1;
    L2:
	if (GOMP_loop_foo_next (&istart0, &iend0)) goto L0; else goto L3;
    L3:

      */

static void
expand_omp_for_generic (struct omp_region *region,
			struct omp_for_data *fd,
			enum built_in_function start_fn,
			enum built_in_function next_fn)
{
  tree type, istart0, iend0, iend;
  tree t, vmain, vback, bias = NULL_TREE;
  basic_block entry_bb, cont_bb, exit_bb, l0_bb, l1_bb, collapse_bb;
  basic_block l2_bb = NULL, l3_bb = NULL;
  gimple_stmt_iterator gsi;
  gimple stmt;
  bool in_combined_parallel = is_combined_parallel (region);
  bool broken_loop = region->cont == NULL;
  edge e, ne;
  tree *counts = NULL;
  int i;

  gcc_assert (!broken_loop || !in_combined_parallel);
  gcc_assert (fd->iter_type == long_integer_type_node
	      || !in_combined_parallel);

  type = TREE_TYPE (fd->loop.v);
  istart0 = create_tmp_var (fd->iter_type, ".istart0");
  iend0 = create_tmp_var (fd->iter_type, ".iend0");
  TREE_ADDRESSABLE (istart0) = 1;
  TREE_ADDRESSABLE (iend0) = 1;
  if (gimple_in_ssa_p (cfun))
    {
      add_referenced_var (istart0);
      add_referenced_var (iend0);
    }

  /* See if we need to bias by LLONG_MIN.  */
  if (fd->iter_type == long_long_unsigned_type_node
      && TREE_CODE (type) == INTEGER_TYPE
      && !TYPE_UNSIGNED (type))
    {
      tree n1, n2;

      if (fd->loop.cond_code == LT_EXPR)
	{
	  n1 = fd->loop.n1;
	  n2 = fold_build2 (PLUS_EXPR, type, fd->loop.n2, fd->loop.step);
	}
      else
	{
	  n1 = fold_build2 (MINUS_EXPR, type, fd->loop.n2, fd->loop.step);
	  n2 = fd->loop.n1;
	}
      if (TREE_CODE (n1) != INTEGER_CST
	  || TREE_CODE (n2) != INTEGER_CST
	  || ((tree_int_cst_sgn (n1) < 0) ^ (tree_int_cst_sgn (n2) < 0)))
	bias = fold_convert (fd->iter_type, TYPE_MIN_VALUE (type));
    }

  entry_bb = region->entry;
  cont_bb = region->cont;
  collapse_bb = NULL;
  gcc_assert (EDGE_COUNT (entry_bb->succs) == 2);
  gcc_assert (broken_loop
	      || BRANCH_EDGE (entry_bb)->dest == FALLTHRU_EDGE (cont_bb)->dest);
  l0_bb = split_edge (FALLTHRU_EDGE (entry_bb));
  l1_bb = single_succ (l0_bb);
  if (!broken_loop)
    {
      l2_bb = create_empty_bb (cont_bb);
      gcc_assert (BRANCH_EDGE (cont_bb)->dest == l1_bb);
      gcc_assert (EDGE_COUNT (cont_bb->succs) == 2);
    }
  else
    l2_bb = NULL;
  l3_bb = BRANCH_EDGE (entry_bb)->dest;
  exit_bb = region->exit;

  gsi = gsi_last_bb (entry_bb);

  gcc_assert (gimple_code (gsi_stmt (gsi)) == GIMPLE_OMP_FOR);
  if (fd->collapse > 1)
    {
      /* collapsed loops need work for expansion in SSA form.  */
      gcc_assert (!gimple_in_ssa_p (cfun));
      counts = (tree *) alloca (fd->collapse * sizeof (tree));
      for (i = 0; i < fd->collapse; i++)
	{
	  tree itype = TREE_TYPE (fd->loops[i].v);

	  if (POINTER_TYPE_P (itype))
	    itype = lang_hooks.types.type_for_size (TYPE_PRECISION (itype), 0);
	  t = build_int_cst (itype, (fd->loops[i].cond_code == LT_EXPR
				     ? -1 : 1));
	  t = fold_build2 (PLUS_EXPR, itype,
			   fold_convert (itype, fd->loops[i].step), t);
	  t = fold_build2 (PLUS_EXPR, itype, t,
			   fold_convert (itype, fd->loops[i].n2));
	  t = fold_build2 (MINUS_EXPR, itype, t,
			   fold_convert (itype, fd->loops[i].n1));
	  if (TYPE_UNSIGNED (itype) && fd->loops[i].cond_code == GT_EXPR)
	    t = fold_build2 (TRUNC_DIV_EXPR, itype,
			     fold_build1 (NEGATE_EXPR, itype, t),
			     fold_build1 (NEGATE_EXPR, itype,
					  fold_convert (itype,
							fd->loops[i].step)));
	  else
	    t = fold_build2 (TRUNC_DIV_EXPR, itype, t,
			     fold_convert (itype, fd->loops[i].step));
	  t = fold_convert (type, t);
	  if (TREE_CODE (t) == INTEGER_CST)
	    counts[i] = t;
	  else
	    {
	      counts[i] = create_tmp_var (type, ".count");
	      t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
					    true, GSI_SAME_STMT);
	      stmt = gimple_build_assign (counts[i], t);
	      gsi_insert_before (&gsi, stmt, GSI_SAME_STMT);
	    }
	  if (SSA_VAR_P (fd->loop.n2))
	    {
	      if (i == 0)
		t = counts[0];
	      else
		{
		  t = fold_build2 (MULT_EXPR, type, fd->loop.n2, counts[i]);
		  t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
						true, GSI_SAME_STMT);
		}
	      stmt = gimple_build_assign (fd->loop.n2, t);
	      gsi_insert_before (&gsi, stmt, GSI_SAME_STMT);
	    }
	}
    }
  if (in_combined_parallel)
    {
      /* In a combined parallel loop, emit a call to
	 GOMP_loop_foo_next.  */
      t = build_call_expr (built_in_decls[next_fn], 2,
			   build_fold_addr_expr (istart0),
			   build_fold_addr_expr (iend0));
    }
  else
    {
      tree t0, t1, t2, t3, t4;
      /* If this is not a combined parallel loop, emit a call to
	 GOMP_loop_foo_start in ENTRY_BB.  */
      t4 = build_fold_addr_expr (iend0);
      t3 = build_fold_addr_expr (istart0);
      t2 = fold_convert (fd->iter_type, fd->loop.step);
      if (POINTER_TYPE_P (type)
	  && TYPE_PRECISION (type) != TYPE_PRECISION (fd->iter_type))
	{
	  /* Avoid casting pointers to integer of a different size.  */
	  tree itype
	    = lang_hooks.types.type_for_size (TYPE_PRECISION (type), 0);
	  t1 = fold_convert (fd->iter_type, fold_convert (itype, fd->loop.n2));
	  t0 = fold_convert (fd->iter_type, fold_convert (itype, fd->loop.n1));
	}
      else
	{
	  t1 = fold_convert (fd->iter_type, fd->loop.n2);
	  t0 = fold_convert (fd->iter_type, fd->loop.n1);
	}
      if (bias)
	{
	  t1 = fold_build2 (PLUS_EXPR, fd->iter_type, t1, bias);
	  t0 = fold_build2 (PLUS_EXPR, fd->iter_type, t0, bias);
	}
      if (fd->iter_type == long_integer_type_node)
	{
	  if (fd->chunk_size)
	    {
	      t = fold_convert (fd->iter_type, fd->chunk_size);
	      t = build_call_expr (built_in_decls[start_fn], 6,
				   t0, t1, t2, t, t3, t4);
	    }
	  else
	    t = build_call_expr (built_in_decls[start_fn], 5,
				 t0, t1, t2, t3, t4);
	}
      else
	{
	  tree t5;
	  tree c_bool_type;

	  /* The GOMP_loop_ull_*start functions have additional boolean
	     argument, true for < loops and false for > loops.
	     In Fortran, the C bool type can be different from
	     boolean_type_node.  */
	  c_bool_type = TREE_TYPE (TREE_TYPE (built_in_decls[start_fn]));
	  t5 = build_int_cst (c_bool_type,
			      fd->loop.cond_code == LT_EXPR ? 1 : 0);
	  if (fd->chunk_size)
	    {
	      t = fold_convert (fd->iter_type, fd->chunk_size);
	      t = build_call_expr (built_in_decls[start_fn], 7,
				   t5, t0, t1, t2, t, t3, t4);
	    }
	  else
	    t = build_call_expr (built_in_decls[start_fn], 6,
				 t5, t0, t1, t2, t3, t4);
	}
    }
  if (TREE_TYPE (t) != boolean_type_node)
    t = fold_build2 (NE_EXPR, boolean_type_node,
		     t, build_int_cst (TREE_TYPE (t), 0));
  t = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
			       	true, GSI_SAME_STMT);
  gsi_insert_after (&gsi, gimple_build_cond_empty (t), GSI_SAME_STMT);

  /* Remove the GIMPLE_OMP_FOR statement.  */
  gsi_remove (&gsi, true);

  /* Iteration setup for sequential loop goes in L0_BB.  */
  gsi = gsi_start_bb (l0_bb);
  if (bias)
    t = fold_convert (type, fold_build2 (MINUS_EXPR, fd->iter_type,
					 istart0, bias));
  else
    t = fold_convert (type, istart0);
  t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
				false, GSI_CONTINUE_LINKING);
  stmt = gimple_build_assign (fd->loop.v, t);
  gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);

  if (bias)
    t = fold_convert (type, fold_build2 (MINUS_EXPR, fd->iter_type,
					 iend0, bias));
  else
    t = fold_convert (type, iend0);
  iend = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
				   false, GSI_CONTINUE_LINKING);
  if (fd->collapse > 1)
    {
      tree tem = create_tmp_var (type, ".tem");

      stmt = gimple_build_assign (tem, fd->loop.v);
      gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);
      for (i = fd->collapse - 1; i >= 0; i--)
	{
	  tree vtype = TREE_TYPE (fd->loops[i].v), itype;
	  itype = vtype;
	  if (POINTER_TYPE_P (vtype))
	    itype = lang_hooks.types.type_for_size (TYPE_PRECISION (vtype), 0);
	  t = fold_build2 (TRUNC_MOD_EXPR, type, tem, counts[i]);
	  t = fold_convert (itype, t);
	  t = fold_build2 (MULT_EXPR, itype, t, fd->loops[i].step);
	  if (POINTER_TYPE_P (vtype))
	    t = fold_build2 (POINTER_PLUS_EXPR, vtype,
			     fd->loops[i].n1, fold_convert (sizetype, t));
	  else
	    t = fold_build2 (PLUS_EXPR, itype, fd->loops[i].n1, t);
	  t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
					false, GSI_CONTINUE_LINKING);
	  stmt = gimple_build_assign (fd->loops[i].v, t);
	  gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);
	  if (i != 0)
	    {
	      t = fold_build2 (TRUNC_DIV_EXPR, type, tem, counts[i]);
	      t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
					    false, GSI_CONTINUE_LINKING);
	      stmt = gimple_build_assign (tem, t);
	      gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);
	    }
	}
    }

  if (!broken_loop)
    {
      /* Code to control the increment and predicate for the sequential
	 loop goes in the CONT_BB.  */
      gsi = gsi_last_bb (cont_bb);
      stmt = gsi_stmt (gsi);
      gcc_assert (gimple_code (stmt) == GIMPLE_OMP_CONTINUE);
      vmain = gimple_omp_continue_control_use (stmt);
      vback = gimple_omp_continue_control_def (stmt);

      if (POINTER_TYPE_P (type))
	t = fold_build2 (POINTER_PLUS_EXPR, type, vmain,
			 fold_convert (sizetype, fd->loop.step));
      else
	t = fold_build2 (PLUS_EXPR, type, vmain, fd->loop.step);
      t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
				    true, GSI_SAME_STMT);
      stmt = gimple_build_assign (vback, t);
      gsi_insert_before (&gsi, stmt, GSI_SAME_STMT);

      t = build2 (fd->loop.cond_code, boolean_type_node, vback, iend);
      stmt = gimple_build_cond_empty (t);
      gsi_insert_before (&gsi, stmt, GSI_SAME_STMT);

      /* Remove GIMPLE_OMP_CONTINUE.  */
      gsi_remove (&gsi, true);

      if (fd->collapse > 1)
	{
	  basic_block last_bb, bb;

	  last_bb = cont_bb;
	  for (i = fd->collapse - 1; i >= 0; i--)
	    {
	      tree vtype = TREE_TYPE (fd->loops[i].v);

	      bb = create_empty_bb (last_bb);
	      gsi = gsi_start_bb (bb);

	      if (i < fd->collapse - 1)
		{
		  e = make_edge (last_bb, bb, EDGE_FALSE_VALUE);
		  e->probability = REG_BR_PROB_BASE / 8;

		  t = fd->loops[i + 1].n1;
		  t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
					        false, GSI_CONTINUE_LINKING);
		  stmt = gimple_build_assign (fd->loops[i + 1].v, t);
		  gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);
		}
	      else
		collapse_bb = bb;

	      set_immediate_dominator (CDI_DOMINATORS, bb, last_bb);

	      if (POINTER_TYPE_P (vtype))
		t = fold_build2 (POINTER_PLUS_EXPR, vtype,
				 fd->loops[i].v,
				 fold_convert (sizetype, fd->loops[i].step));
	      else
		t = fold_build2 (PLUS_EXPR, vtype, fd->loops[i].v,
				 fd->loops[i].step);
	      t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
					    false, GSI_CONTINUE_LINKING);
	      stmt = gimple_build_assign (fd->loops[i].v, t);
	      gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);

	      if (i > 0)
		{
		  t = fd->loops[i].n2;
		  t = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
						false, GSI_CONTINUE_LINKING);
		  t = fold_build2 (fd->loops[i].cond_code, boolean_type_node,
				   fd->loops[i].v, t);
		  stmt = gimple_build_cond_empty (t);
		  gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);
		  e = make_edge (bb, l1_bb, EDGE_TRUE_VALUE);
		  e->probability = REG_BR_PROB_BASE * 7 / 8;
		}
	      else
		make_edge (bb, l1_bb, EDGE_FALLTHRU);
	      last_bb = bb;
	    }
	}

      /* Emit code to get the next parallel iteration in L2_BB.  */
      gsi = gsi_start_bb (l2_bb);

      t = build_call_expr (built_in_decls[next_fn], 2,
			   build_fold_addr_expr (istart0),
			   build_fold_addr_expr (iend0));
      t = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
				    false, GSI_CONTINUE_LINKING);
      if (TREE_TYPE (t) != boolean_type_node)
	t = fold_build2 (NE_EXPR, boolean_type_node,
			 t, build_int_cst (TREE_TYPE (t), 0));
      stmt = gimple_build_cond_empty (t);
      gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);
    }

  /* Add the loop cleanup function.  */
  gsi = gsi_last_bb (exit_bb);
  if (gimple_omp_return_nowait_p (gsi_stmt (gsi)))
    t = built_in_decls[BUILT_IN_GOMP_LOOP_END_NOWAIT];
  else
    t = built_in_decls[BUILT_IN_GOMP_LOOP_END];
  stmt = gimple_build_call (t, 0);
  gsi_insert_after (&gsi, stmt, GSI_SAME_STMT);
  gsi_remove (&gsi, true);

  /* Connect the new blocks.  */
  find_edge (entry_bb, l0_bb)->flags = EDGE_TRUE_VALUE;
  find_edge (entry_bb, l3_bb)->flags = EDGE_FALSE_VALUE;

  if (!broken_loop)
    {
      gimple_seq phis;

      e = find_edge (cont_bb, l3_bb);
      ne = make_edge (l2_bb, l3_bb, EDGE_FALSE_VALUE);

      phis = phi_nodes (l3_bb);
      for (gsi = gsi_start (phis); !gsi_end_p (gsi); gsi_next (&gsi))
	{
	  gimple phi = gsi_stmt (gsi);
	  SET_USE (PHI_ARG_DEF_PTR_FROM_EDGE (phi, ne),
		   PHI_ARG_DEF_FROM_EDGE (phi, e));
	}
      remove_edge (e);

      make_edge (cont_bb, l2_bb, EDGE_FALSE_VALUE);
      if (fd->collapse > 1)
	{
	  e = find_edge (cont_bb, l1_bb);
	  remove_edge (e);
	  e = make_edge (cont_bb, collapse_bb, EDGE_TRUE_VALUE);
	}
      else
	{
	  e = find_edge (cont_bb, l1_bb);
	  e->flags = EDGE_TRUE_VALUE;
	}
      e->probability = REG_BR_PROB_BASE * 7 / 8;
      find_edge (cont_bb, l2_bb)->probability = REG_BR_PROB_BASE / 8;
      make_edge (l2_bb, l0_bb, EDGE_TRUE_VALUE);

      set_immediate_dominator (CDI_DOMINATORS, l2_bb,
			       recompute_dominator (CDI_DOMINATORS, l2_bb));
      set_immediate_dominator (CDI_DOMINATORS, l3_bb,
			       recompute_dominator (CDI_DOMINATORS, l3_bb));
      set_immediate_dominator (CDI_DOMINATORS, l0_bb,
			       recompute_dominator (CDI_DOMINATORS, l0_bb));
      set_immediate_dominator (CDI_DOMINATORS, l1_bb,
			       recompute_dominator (CDI_DOMINATORS, l1_bb));
    }
}


/* A subroutine of expand_omp_for.  Generate code for a parallel
   loop with static schedule and no specified chunk size.  Given
   parameters:

	for (V = N1; V cond N2; V += STEP) BODY;

   where COND is "<" or ">", we generate pseudocode

	if (cond is <)
	  adj = STEP - 1;
	else
	  adj = STEP + 1;
	if ((__typeof (V)) -1 > 0 && cond is >)
	  n = -(adj + N2 - N1) / -STEP;
	else
	  n = (adj + N2 - N1) / STEP;
	q = n / nthreads;
	q += (q * nthreads != n);
	s0 = q * threadid;
	e0 = min(s0 + q, n);
	V = s0 * STEP + N1;
	if (s0 >= e0) goto L2; else goto L0;
    L0:
	e = e0 * STEP + N1;
    L1:
	BODY;
	V += STEP;
	if (V cond e) goto L1;
    L2:
*/

static void
expand_omp_for_static_nochunk (struct omp_region *region,
			       struct omp_for_data *fd)
{
  tree n, q, s0, e0, e, t, nthreads, threadid;
  tree type, itype, vmain, vback;
  basic_block entry_bb, exit_bb, seq_start_bb, body_bb, cont_bb;
  basic_block fin_bb;
  gimple_stmt_iterator gsi;
  gimple stmt;

  itype = type = TREE_TYPE (fd->loop.v);
  if (POINTER_TYPE_P (type))
    itype = lang_hooks.types.type_for_size (TYPE_PRECISION (type), 0);

  entry_bb = region->entry;
  cont_bb = region->cont;
  gcc_assert (EDGE_COUNT (entry_bb->succs) == 2);
  gcc_assert (BRANCH_EDGE (entry_bb)->dest == FALLTHRU_EDGE (cont_bb)->dest);
  seq_start_bb = split_edge (FALLTHRU_EDGE (entry_bb));
  body_bb = single_succ (seq_start_bb);
  gcc_assert (BRANCH_EDGE (cont_bb)->dest == body_bb);
  gcc_assert (EDGE_COUNT (cont_bb->succs) == 2);
  fin_bb = FALLTHRU_EDGE (cont_bb)->dest;
  exit_bb = region->exit;

  /* Iteration space partitioning goes in ENTRY_BB.  */
  gsi = gsi_last_bb (entry_bb);
  gcc_assert (gimple_code (gsi_stmt (gsi)) == GIMPLE_OMP_FOR);

  t = build_call_expr (built_in_decls[BUILT_IN_OMP_GET_NUM_THREADS], 0);
  t = fold_convert (itype, t);
  nthreads = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
				       true, GSI_SAME_STMT);
  
  t = build_call_expr (built_in_decls[BUILT_IN_OMP_GET_THREAD_NUM], 0);
  t = fold_convert (itype, t);
  threadid = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
				       true, GSI_SAME_STMT);

  fd->loop.n1
    = force_gimple_operand_gsi (&gsi, fold_convert (type, fd->loop.n1),
				true, NULL_TREE, true, GSI_SAME_STMT);
  fd->loop.n2
    = force_gimple_operand_gsi (&gsi, fold_convert (itype, fd->loop.n2),
				true, NULL_TREE, true, GSI_SAME_STMT);
  fd->loop.step
    = force_gimple_operand_gsi (&gsi, fold_convert (itype, fd->loop.step),
				true, NULL_TREE, true, GSI_SAME_STMT);

  t = build_int_cst (itype, (fd->loop.cond_code == LT_EXPR ? -1 : 1));
  t = fold_build2 (PLUS_EXPR, itype, fd->loop.step, t);
  t = fold_build2 (PLUS_EXPR, itype, t, fd->loop.n2);
  t = fold_build2 (MINUS_EXPR, itype, t, fold_convert (itype, fd->loop.n1));
  if (TYPE_UNSIGNED (itype) && fd->loop.cond_code == GT_EXPR)
    t = fold_build2 (TRUNC_DIV_EXPR, itype,
		     fold_build1 (NEGATE_EXPR, itype, t),
		     fold_build1 (NEGATE_EXPR, itype, fd->loop.step));
  else
    t = fold_build2 (TRUNC_DIV_EXPR, itype, t, fd->loop.step);
  t = fold_convert (itype, t);
  n = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE, true, GSI_SAME_STMT);

  t = fold_build2 (TRUNC_DIV_EXPR, itype, n, nthreads);
  q = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE, true, GSI_SAME_STMT);

  t = fold_build2 (MULT_EXPR, itype, q, nthreads);
  t = fold_build2 (NE_EXPR, itype, t, n);
  t = fold_build2 (PLUS_EXPR, itype, q, t);
  q = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE, true, GSI_SAME_STMT);

  t = build2 (MULT_EXPR, itype, q, threadid);
  s0 = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE, true, GSI_SAME_STMT);

  t = fold_build2 (PLUS_EXPR, itype, s0, q);
  t = fold_build2 (MIN_EXPR, itype, t, n);
  e0 = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE, true, GSI_SAME_STMT);

  t = build2 (GE_EXPR, boolean_type_node, s0, e0);
  gsi_insert_before (&gsi, gimple_build_cond_empty (t), GSI_SAME_STMT);

  /* Remove the GIMPLE_OMP_FOR statement.  */
  gsi_remove (&gsi, true);

  /* Setup code for sequential iteration goes in SEQ_START_BB.  */
  gsi = gsi_start_bb (seq_start_bb);

  t = fold_convert (itype, s0);
  t = fold_build2 (MULT_EXPR, itype, t, fd->loop.step);
  if (POINTER_TYPE_P (type))
    t = fold_build2 (POINTER_PLUS_EXPR, type, fd->loop.n1,
		     fold_convert (sizetype, t));
  else
    t = fold_build2 (PLUS_EXPR, type, t, fd->loop.n1);
  t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
				false, GSI_CONTINUE_LINKING);
  stmt = gimple_build_assign (fd->loop.v, t);
  gsi_insert_after (&gsi, stmt, GSI_CONTINUE_LINKING);
 
  t = fold_convert (itype, e0);
  t = fold_build2 (MULT_EXPR, itype, t, fd->loop.step);
  if (POINTER_TYPE_P (type))
    t = fold_build2 (POINTER_PLUS_EXPR, type, fd->loop.n1,
		     fold_convert (sizetype, t));
  else
    t = fold_build2 (PLUS_EXPR, type, t, fd->loop.n1);
  e = force_gimple_operand_gsi (&gsi, t, true, NULL_TREE,
				false, GSI_CONTINUE_LINKING);

  /* The code controlling the sequential loop replaces the
     GIMPLE_OMP_CONTINUE.  */
  gsi = gsi_last_bb (cont_bb);
  stmt = gsi_stmt (gsi);
  gcc_assert (gimple_code (stmt) == GIMPLE_OMP_CONTINUE);
  vmain = gimple_omp_continue_control_use (stmt);
  vback = gimple_omp_continue_control_def (stmt);

  if (POINTER_TYPE_P (type))
    t = fold_build2 (POINTER_PLUS_EXPR, type, vmain,
		     fold_convert (sizetype, fd->loop.step));
  else
    t = fold_build2 (PLUS_EXPR, type, vmain, fd->loop.step);
  t = force_gimple_operand_gsi (&gsi, t, false, NULL_TREE,
				true, GSI_SAME_STMT);
  stmt = gimple_build_assign (vback, t);
  gsi_insert_before (&gsi, stmt, GSI_SAME_STMT);

  t = build2 (fd->loop.cond_code, boolean_type_node, vback, e);
  gsi_insert_before (&gsi, gimple_build_cond_empty (t), GSI_SAME_STMT);

  /* Remove the GIMPLE_OMP_CONTINUE statement.  */
  gsi_remove (&gsi, true);

  /* Replace the GIMPLE_OMP_RETURN with a barrier, or nothing.  */
  gsi = gsi_last_bb (exit_bb);
  if (!gimple_omp_return_nowait_p (gsi_stmt (gsi)))
    force_gimple_operand_gsi (&gsi, build_omp_barrier (), false, NULL_TREE,
			      false, GSI_SAME_STMT);
  gsi_remove (&gsi, true);

  /* Connect all the blocks.  */
  find_edge (entry_bb, seq_start_bb)->flags = EDGE_FALSE_VALUE;
  find_edge (entry_bb, fin_bb)->flags = EDGE_TRUE_VALUE;

  find_edge (cont_bb, body_bb)->flags = EDGE_TRUE_VALUE;
  find_edge (cont_bb, fin_bb)->flags = EDGE_FALSE_VALUE;
 
  set_immediate_dominator (CDI_DOMINATORS, seq_start_bb, entry_bb);
  set_immediate_dominator (CDI_DOMINATORS, body_bb,
			   recompute_dominator (CDI_DOMINATORS, body_bb));
  set_immediate_dominator (CDI_DOMINATORS, fin_bb,
			   recompute_dominator (CDI_DOMINATORS, fin_bb));
}


/* A subroutine of expand_omp_for.  Generate code for a parallel
   loop with static schedule and a specified chunk size.  Given
   parameters:

	for (V = N1; V cond N2; V += STEP) BODY;

   where COND is "<" or ">", we generate pseudocode

	if (cond is <)
	  adj = STEP - 1;
	else
	  adj = STEP + 1;
	if ((__typeof (V)) -1 > 0 && cond is >)
	  n = -(adj + N2 - N1) / -STEP;
	else
	  n = (adj + N2 - N1) / STEP;
	trip = 0;
	V = threadid * CHUNK * STEP + N1;  -- this extra definition of V is
					      here so that V is defined
					      if the loop is not entered
    L0:
	s0 = (trip * nthreads + threadid) * CHUNK;
	e0 = min(s0 + CHUNK, n);
	if (s0 < n) goto L1; else goto L4;
    L1:
	V = s0 * STEP + N1;
	e = e0 * STEP + N1;
    L2:
	BODY;
	V += STEP;
	if (V cond e) goto L2; else goto L3;
    L3:
	trip += 1;
	goto L0;
    L4:
*/

static void
expand_omp_for_static_chunk (struct omp_region *region, struct omp_for_data *fd)
{
  tree n, s0, e0, e, t;
  tree trip_var, trip_init, trip_main, trip_back, nthreads, threadid;
  tree type, itype, v_main, v_back, v_extra;
  basic_block entry_bb, exit_bb, body_bb, seq_start_bb, iter_part_bb;
  basic_block trip_update_bb, cont_bb, fin_bb;
  gimple_stmt_iterator si;
  gimple stmt;
  edge se;

  itype = type = TREE_TYPE (fd->loop.v);
  if (POINTER_TYPE_P (type))
    itype = lang_hooks.types.type_for_size (TYPE_PRECISION (type), 0);

  entry_bb = region->entry;
  se = split_block (entry_bb, last_stmt (entry_bb));
  entry_bb = se->src;
  iter_part_bb = se->dest;
  cont_bb = region->cont;
  gcc_assert (EDGE_COUNT (iter_part_bb->succs) == 2);
  gcc_assert (BRANCH_EDGE (iter_part_bb)->dest
	      == FALLTHRU_EDGE (cont_bb)->dest);
  seq_start_bb = split_edge (FALLTHRU_EDGE (iter_part_bb));
  body_bb = single_succ (seq_start_bb);
  gcc_assert (BRANCH_EDGE (cont_bb)->dest == body_bb);
  gcc_assert (EDGE_COUNT (cont_bb->succs) == 2);
  fin_bb = FALLTHRU_EDGE (cont_bb)->dest;
  trip_update_bb = split_edge (FALLTHRU_EDGE (cont_bb));
  exit_bb = region->exit;

  /* Trip and adjustment setup goes in ENTRY_BB.  */
  si = gsi_last_bb (entry_bb);
  gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_FOR);

  t = build_call_expr (built_in_decls[BUILT_IN_OMP_GET_NUM_THREADS], 0);
  t = fold_convert (itype, t);
  nthreads = force_gimple_operand_gsi (&si, t, true, NULL_TREE,
				       true, GSI_SAME_STMT);
  
  t = build_call_expr (built_in_decls[BUILT_IN_OMP_GET_THREAD_NUM], 0);
  t = fold_convert (itype, t);
  threadid = force_gimple_operand_gsi (&si, t, true, NULL_TREE,
				       true, GSI_SAME_STMT);

  fd->loop.n1
    = force_gimple_operand_gsi (&si, fold_convert (type, fd->loop.n1),
				true, NULL_TREE, true, GSI_SAME_STMT);
  fd->loop.n2
    = force_gimple_operand_gsi (&si, fold_convert (itype, fd->loop.n2),
				true, NULL_TREE, true, GSI_SAME_STMT);
  fd->loop.step
    = force_gimple_operand_gsi (&si, fold_convert (itype, fd->loop.step),
				true, NULL_TREE, true, GSI_SAME_STMT);
  fd->chunk_size
    = force_gimple_operand_gsi (&si, fold_convert (itype, fd->chunk_size),
				true, NULL_TREE, true, GSI_SAME_STMT);

  t = build_int_cst (itype, (fd->loop.cond_code == LT_EXPR ? -1 : 1));
  t = fold_build2 (PLUS_EXPR, itype, fd->loop.step, t);
  t = fold_build2 (PLUS_EXPR, itype, t, fd->loop.n2);
  t = fold_build2 (MINUS_EXPR, itype, t, fold_convert (itype, fd->loop.n1));
  if (TYPE_UNSIGNED (itype) && fd->loop.cond_code == GT_EXPR)
    t = fold_build2 (TRUNC_DIV_EXPR, itype,
		     fold_build1 (NEGATE_EXPR, itype, t),
		     fold_build1 (NEGATE_EXPR, itype, fd->loop.step));
  else
    t = fold_build2 (TRUNC_DIV_EXPR, itype, t, fd->loop.step);
  t = fold_convert (itype, t);
  n = force_gimple_operand_gsi (&si, t, true, NULL_TREE,
				true, GSI_SAME_STMT);

  trip_var = create_tmp_var (itype, ".trip");
  if (gimple_in_ssa_p (cfun))
    {
      add_referenced_var (trip_var);
      trip_init = make_ssa_name (trip_var, NULL);
      trip_main = make_ssa_name (trip_var, NULL);
      trip_back = make_ssa_name (trip_var, NULL);
    }
  else
    {
      trip_init = trip_var;
      trip_main = trip_var;
      trip_back = trip_var;
    }

  stmt = gimple_build_assign (trip_init, build_int_cst (itype, 0));
  gsi_insert_before (&si, stmt, GSI_SAME_STMT);

  t = fold_build2 (MULT_EXPR, itype, threadid, fd->chunk_size);
  t = fold_build2 (MULT_EXPR, itype, t, fd->loop.step);
  if (POINTER_TYPE_P (type))
    t = fold_build2 (POINTER_PLUS_EXPR, type, fd->loop.n1,
		     fold_convert (sizetype, t));
  else
    t = fold_build2 (PLUS_EXPR, type, t, fd->loop.n1);
  v_extra = force_gimple_operand_gsi (&si, t, true, NULL_TREE,
				      true, GSI_SAME_STMT);

  /* Remove the GIMPLE_OMP_FOR.  */
  gsi_remove (&si, true);

  /* Iteration space partitioning goes in ITER_PART_BB.  */
  si = gsi_last_bb (iter_part_bb);

  t = fold_build2 (MULT_EXPR, itype, trip_main, nthreads);
  t = fold_build2 (PLUS_EXPR, itype, t, threadid);
  t = fold_build2 (MULT_EXPR, itype, t, fd->chunk_size);
  s0 = force_gimple_operand_gsi (&si, t, true, NULL_TREE,
				 false, GSI_CONTINUE_LINKING);

  t = fold_build2 (PLUS_EXPR, itype, s0, fd->chunk_size);
  t = fold_build2 (MIN_EXPR, itype, t, n);
  e0 = force_gimple_operand_gsi (&si, t, true, NULL_TREE,
				 false, GSI_CONTINUE_LINKING);

  t = build2 (LT_EXPR, boolean_type_node, s0, n);
  gsi_insert_after (&si, gimple_build_cond_empty (t), GSI_CONTINUE_LINKING);

  /* Setup code for sequential iteration goes in SEQ_START_BB.  */
  si = gsi_start_bb (seq_start_bb);

  t = fold_convert (itype, s0);
  t = fold_build2 (MULT_EXPR, itype, t, fd->loop.step);
  if (POINTER_TYPE_P (type))
    t = fold_build2 (POINTER_PLUS_EXPR, type, fd->loop.n1,
		     fold_convert (sizetype, t));
  else
    t = fold_build2 (PLUS_EXPR, type, t, fd->loop.n1);
  t = force_gimple_operand_gsi (&si, t, false, NULL_TREE,
				false, GSI_CONTINUE_LINKING);
  stmt = gimple_build_assign (fd->loop.v, t);
  gsi_insert_after (&si, stmt, GSI_CONTINUE_LINKING);

  t = fold_convert (itype, e0);
  t = fold_build2 (MULT_EXPR, itype, t, fd->loop.step);
  if (POINTER_TYPE_P (type))
    t = fold_build2 (POINTER_PLUS_EXPR, type, fd->loop.n1,
		     fold_convert (sizetype, t));
  else
    t = fold_build2 (PLUS_EXPR, type, t, fd->loop.n1);
  e = force_gimple_operand_gsi (&si, t, true, NULL_TREE,
				false, GSI_CONTINUE_LINKING);

  /* The code controlling the sequential loop goes in CONT_BB,
     replacing the GIMPLE_OMP_CONTINUE.  */
  si = gsi_last_bb (cont_bb);
  stmt = gsi_stmt (si);
  gcc_assert (gimple_code (stmt) == GIMPLE_OMP_CONTINUE);
  v_main = gimple_omp_continue_control_use (stmt);
  v_back = gimple_omp_continue_control_def (stmt);

  if (POINTER_TYPE_P (type))
    t = fold_build2 (POINTER_PLUS_EXPR, type, v_main,
		     fold_convert (sizetype, fd->loop.step));
  else
    t = fold_build2 (PLUS_EXPR, type, v_main, fd->loop.step);
  stmt = gimple_build_assign (v_back, t);
  gsi_insert_before (&si, stmt, GSI_SAME_STMT);

  t = build2 (fd->loop.cond_code, boolean_type_node, v_back, e);
  gsi_insert_before (&si, gimple_build_cond_empty (t), GSI_SAME_STMT);
  
  /* Remove GIMPLE_OMP_CONTINUE.  */
  gsi_remove (&si, true);

  /* Trip update code goes into TRIP_UPDATE_BB.  */
  si = gsi_start_bb (trip_update_bb);

  t = build_int_cst (itype, 1);
  t = build2 (PLUS_EXPR, itype, trip_main, t);
  stmt = gimple_build_assign (trip_back, t);
  gsi_insert_after (&si, stmt, GSI_CONTINUE_LINKING);

  /* Replace the GIMPLE_OMP_RETURN with a barrier, or nothing.  */
  si = gsi_last_bb (exit_bb);
  if (!gimple_omp_return_nowait_p (gsi_stmt (si)))
    force_gimple_operand_gsi (&si, build_omp_barrier (), false, NULL_TREE,
			      false, GSI_SAME_STMT);
  gsi_remove (&si, true);

  /* Connect the new blocks.  */
  find_edge (iter_part_bb, seq_start_bb)->flags = EDGE_TRUE_VALUE;
  find_edge (iter_part_bb, fin_bb)->flags = EDGE_FALSE_VALUE;

  find_edge (cont_bb, body_bb)->flags = EDGE_TRUE_VALUE;
  find_edge (cont_bb, trip_update_bb)->flags = EDGE_FALSE_VALUE;

  redirect_edge_and_branch (single_succ_edge (trip_update_bb), iter_part_bb);

  if (gimple_in_ssa_p (cfun))
    {
      gimple_stmt_iterator psi;
      gimple phi;
      edge re, ene;
      edge_var_map_vector head;
      edge_var_map *vm;
      size_t i;

      /* When we redirect the edge from trip_update_bb to iter_part_bb, we
	 remove arguments of the phi nodes in fin_bb.  We need to create
	 appropriate phi nodes in iter_part_bb instead.  */
      se = single_pred_edge (fin_bb);
      re = single_succ_edge (trip_update_bb);
      head = redirect_edge_var_map_vector (re);
      ene = single_succ_edge (entry_bb);

      psi = gsi_start_phis (fin_bb);
      for (i = 0; !gsi_end_p (psi) && VEC_iterate (edge_var_map, head, i, vm);
	   gsi_next (&psi), ++i)
	{
	  gimple nphi;

	  phi = gsi_stmt (psi);
	  t = gimple_phi_result (phi);
	  gcc_assert (t == redirect_edge_var_map_result (vm));
	  nphi = create_phi_node (t, iter_part_bb);
	  SSA_NAME_DEF_STMT (t) = nphi;

	  t = PHI_ARG_DEF_FROM_EDGE (phi, se);
	  /* A special case -- fd->loop.v is not yet computed in
	     iter_part_bb, we need to use v_extra instead.  */
	  if (t == fd->loop.v)
	    t = v_extra;
	  add_phi_arg (nphi, t, ene);
	  add_phi_arg (nphi, redirect_edge_var_map_def (vm), re);
	}
      gcc_assert (!gsi_end_p (psi) && i == VEC_length (edge_var_map, head));
      redirect_edge_var_map_clear (re);
      while (1)
	{
	  psi = gsi_start_phis (fin_bb);
	  if (gsi_end_p (psi))
	    break;
	  remove_phi_node (&psi, false);
	}

      /* Make phi node for trip.  */
      phi = create_phi_node (trip_main, iter_part_bb);
      SSA_NAME_DEF_STMT (trip_main) = phi;
      add_phi_arg (phi, trip_back, single_succ_edge (trip_update_bb));
      add_phi_arg (phi, trip_init, single_succ_edge (entry_bb));
    }

  set_immediate_dominator (CDI_DOMINATORS, trip_update_bb, cont_bb);
  set_immediate_dominator (CDI_DOMINATORS, iter_part_bb,
			   recompute_dominator (CDI_DOMINATORS, iter_part_bb));
  set_immediate_dominator (CDI_DOMINATORS, fin_bb,
			   recompute_dominator (CDI_DOMINATORS, fin_bb));
  set_immediate_dominator (CDI_DOMINATORS, seq_start_bb,
			   recompute_dominator (CDI_DOMINATORS, seq_start_bb));
  set_immediate_dominator (CDI_DOMINATORS, body_bb,
			   recompute_dominator (CDI_DOMINATORS, body_bb));
}


/* Expand the OpenMP loop defined by REGION.  */

static void
expand_omp_for (struct omp_region *region)
{
  struct omp_for_data fd;
  struct omp_for_data_loop *loops;

  loops
    = (struct omp_for_data_loop *)
      alloca (gimple_omp_for_collapse (last_stmt (region->entry))
	      * sizeof (struct omp_for_data_loop));
  extract_omp_for_data (last_stmt (region->entry), &fd, loops);
  region->sched_kind = fd.sched_kind;

  gcc_assert (EDGE_COUNT (region->entry->succs) == 2);
  BRANCH_EDGE (region->entry)->flags &= ~EDGE_ABNORMAL;
  FALLTHRU_EDGE (region->entry)->flags &= ~EDGE_ABNORMAL;
  if (region->cont)
    {
      gcc_assert (EDGE_COUNT (region->cont->succs) == 2);
      BRANCH_EDGE (region->cont)->flags &= ~EDGE_ABNORMAL;
      FALLTHRU_EDGE (region->cont)->flags &= ~EDGE_ABNORMAL;
    }

  if (fd.sched_kind == OMP_CLAUSE_SCHEDULE_STATIC
      && !fd.have_ordered
      && fd.collapse == 1
      && region->cont != NULL)
    {
      if (fd.chunk_size == NULL)
	expand_omp_for_static_nochunk (region, &fd);
      else
	expand_omp_for_static_chunk (region, &fd);
    }
  else
    {
      int fn_index, start_ix, next_ix;

      gcc_assert (fd.sched_kind != OMP_CLAUSE_SCHEDULE_AUTO);
      fn_index = (fd.sched_kind == OMP_CLAUSE_SCHEDULE_RUNTIME)
		  ? 3 : fd.sched_kind;
      fn_index += fd.have_ordered * 4;
      start_ix = BUILT_IN_GOMP_LOOP_STATIC_START + fn_index;
      next_ix = BUILT_IN_GOMP_LOOP_STATIC_NEXT + fn_index;
      if (fd.iter_type == long_long_unsigned_type_node)
	{
	  start_ix += BUILT_IN_GOMP_LOOP_ULL_STATIC_START
		      - BUILT_IN_GOMP_LOOP_STATIC_START;
	  next_ix += BUILT_IN_GOMP_LOOP_ULL_STATIC_NEXT
		     - BUILT_IN_GOMP_LOOP_STATIC_NEXT;
	}
      expand_omp_for_generic (region, &fd, start_ix, next_ix);
    }

  update_ssa (TODO_update_ssa_only_virtuals);
}


/* Expand code for an OpenMP sections directive.  In pseudo code, we generate

	v = GOMP_sections_start (n);
    L0:
	switch (v)
	  {
	  case 0:
	    goto L2;
	  case 1:
	    section 1;
	    goto L1;
	  case 2:
	    ...
	  case n:
	    ...
	  default:
	    abort ();
	  }
    L1:
	v = GOMP_sections_next ();
	goto L0;
    L2:
	reduction;

    If this is a combined parallel sections, replace the call to
    GOMP_sections_start with call to GOMP_sections_next.  */

static void
expand_omp_sections (struct omp_region *region)
{
  tree t, u, vin = NULL, vmain, vnext, l1, l2;
  VEC (tree,heap) *label_vec;
  unsigned len;
  basic_block entry_bb, l0_bb, l1_bb, l2_bb, default_bb;
  gimple_stmt_iterator si, switch_si;
  gimple sections_stmt, stmt, cont;
  edge_iterator ei;
  edge e;
  struct omp_region *inner;
  unsigned i, casei;
  bool exit_reachable = region->cont != NULL;

  gcc_assert (exit_reachable == (region->exit != NULL));
  entry_bb = region->entry;
  l0_bb = single_succ (entry_bb);
  l1_bb = region->cont;
  l2_bb = region->exit;
  if (exit_reachable)
    {
      if (single_pred (l2_bb) == l0_bb)
	l2 = gimple_block_label (l2_bb);
      else
	{
	  /* This can happen if there are reductions.  */
	  len = EDGE_COUNT (l0_bb->succs);
	  gcc_assert (len > 0);
	  e = EDGE_SUCC (l0_bb, len - 1);
	  si = gsi_last_bb (e->dest);
	  l2 = NULL_TREE;
	  if (gsi_end_p (si)
	      || gimple_code (gsi_stmt (si)) != GIMPLE_OMP_SECTION)
	    l2 = gimple_block_label (e->dest);
	  else
	    FOR_EACH_EDGE (e, ei, l0_bb->succs)
	      {
		si = gsi_last_bb (e->dest);
		if (gsi_end_p (si)
		    || gimple_code (gsi_stmt (si)) != GIMPLE_OMP_SECTION)
		  {
		    l2 = gimple_block_label (e->dest);
		    break;
		  }
	      }
	}
      default_bb = create_empty_bb (l1_bb->prev_bb);
      l1 = gimple_block_label (l1_bb);
    }
  else
    {
      default_bb = create_empty_bb (l0_bb);
      l1 = NULL_TREE;
      l2 = gimple_block_label (default_bb);
    }

  /* We will build a switch() with enough cases for all the
     GIMPLE_OMP_SECTION regions, a '0' case to handle the end of more work
     and a default case to abort if something goes wrong.  */
  len = EDGE_COUNT (l0_bb->succs);

  /* Use VEC_quick_push on label_vec throughout, since we know the size
     in advance.  */
  label_vec = VEC_alloc (tree, heap, len);

  /* The call to GOMP_sections_start goes in ENTRY_BB, replacing the
     GIMPLE_OMP_SECTIONS statement.  */
  si = gsi_last_bb (entry_bb);
  sections_stmt = gsi_stmt (si);
  gcc_assert (gimple_code (sections_stmt) == GIMPLE_OMP_SECTIONS);
  vin = gimple_omp_sections_control (sections_stmt);
  if (!is_combined_parallel (region))
    {
      /* If we are not inside a combined parallel+sections region,
	 call GOMP_sections_start.  */
      t = build_int_cst (unsigned_type_node,
			 exit_reachable ? len - 1 : len);
      u = built_in_decls[BUILT_IN_GOMP_SECTIONS_START];
      stmt = gimple_build_call (u, 1, t);
    }
  else
    {
      /* Otherwise, call GOMP_sections_next.  */
      u = built_in_decls[BUILT_IN_GOMP_SECTIONS_NEXT];
      stmt = gimple_build_call (u, 0);
    }
  gimple_call_set_lhs (stmt, vin);
  gsi_insert_after (&si, stmt, GSI_SAME_STMT);
  gsi_remove (&si, true);

  /* The switch() statement replacing GIMPLE_OMP_SECTIONS_SWITCH goes in
     L0_BB.  */
  switch_si = gsi_last_bb (l0_bb);
  gcc_assert (gimple_code (gsi_stmt (switch_si)) == GIMPLE_OMP_SECTIONS_SWITCH);
  if (exit_reachable)
    {
      cont = last_stmt (l1_bb);
      gcc_assert (gimple_code (cont) == GIMPLE_OMP_CONTINUE);
      vmain = gimple_omp_continue_control_use (cont);
      vnext = gimple_omp_continue_control_def (cont);
    }
  else
    {
      vmain = vin;
      vnext = NULL_TREE;
    }

  i = 0;
  if (exit_reachable)
    {
      t = build3 (CASE_LABEL_EXPR, void_type_node,
		  build_int_cst (unsigned_type_node, 0), NULL, l2);
      VEC_quick_push (tree, label_vec, t);
      i++;
    }

  /* Convert each GIMPLE_OMP_SECTION into a CASE_LABEL_EXPR.  */
  for (inner = region->inner, casei = 1;
       inner;
       inner = inner->next, i++, casei++)
    {
      basic_block s_entry_bb, s_exit_bb;

      /* Skip optional reduction region.  */
      if (inner->type == GIMPLE_OMP_ATOMIC_LOAD)
	{
	  --i;
	  --casei;
	  continue;
	}

      s_entry_bb = inner->entry;
      s_exit_bb = inner->exit;

      t = gimple_block_label (s_entry_bb);
      u = build_int_cst (unsigned_type_node, casei);
      u = build3 (CASE_LABEL_EXPR, void_type_node, u, NULL, t);
      VEC_quick_push (tree, label_vec, u);

      si = gsi_last_bb (s_entry_bb);
      gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_SECTION);
      gcc_assert (i < len || gimple_omp_section_last_p (gsi_stmt (si)));
      gsi_remove (&si, true);
      single_succ_edge (s_entry_bb)->flags = EDGE_FALLTHRU;

      if (s_exit_bb == NULL)
	continue;

      si = gsi_last_bb (s_exit_bb);
      gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_RETURN);
      gsi_remove (&si, true);

      single_succ_edge (s_exit_bb)->flags = EDGE_FALLTHRU;
    }

  /* Error handling code goes in DEFAULT_BB.  */
  t = gimple_block_label (default_bb);
  u = build3 (CASE_LABEL_EXPR, void_type_node, NULL, NULL, t);
  make_edge (l0_bb, default_bb, 0);

  stmt = gimple_build_switch_vec (vmain, u, label_vec);
  gsi_insert_after (&switch_si, stmt, GSI_SAME_STMT);
  gsi_remove (&switch_si, true);
  VEC_free (tree, heap, label_vec);

  si = gsi_start_bb (default_bb);
  stmt = gimple_build_call (built_in_decls[BUILT_IN_TRAP], 0);
  gsi_insert_after (&si, stmt, GSI_CONTINUE_LINKING);

  if (exit_reachable)
    {
      /* Code to get the next section goes in L1_BB.  */
      si = gsi_last_bb (l1_bb);
      gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_CONTINUE);

      stmt = gimple_build_call (built_in_decls[BUILT_IN_GOMP_SECTIONS_NEXT], 0);
      gimple_call_set_lhs (stmt, vnext);
      gsi_insert_after (&si, stmt, GSI_SAME_STMT);
      gsi_remove (&si, true);

      single_succ_edge (l1_bb)->flags = EDGE_FALLTHRU;

      /* Cleanup function replaces GIMPLE_OMP_RETURN in EXIT_BB.  */
      si = gsi_last_bb (l2_bb);
      if (gimple_omp_return_nowait_p (gsi_stmt (si)))
	t = built_in_decls[BUILT_IN_GOMP_SECTIONS_END_NOWAIT];
      else
	t = built_in_decls[BUILT_IN_GOMP_SECTIONS_END];
      stmt = gimple_build_call (t, 0);
      gsi_insert_after (&si, stmt, GSI_SAME_STMT);
      gsi_remove (&si, true);
    }

  set_immediate_dominator (CDI_DOMINATORS, default_bb, l0_bb);
}


/* Expand code for an OpenMP single directive.  We've already expanded
   much of the code, here we simply place the GOMP_barrier call.  */

static void
expand_omp_single (struct omp_region *region)
{
  basic_block entry_bb, exit_bb;
  gimple_stmt_iterator si;
  bool need_barrier = false;

  entry_bb = region->entry;
  exit_bb = region->exit;

  si = gsi_last_bb (entry_bb);
  /* The terminal barrier at the end of a GOMP_single_copy sequence cannot
     be removed.  We need to ensure that the thread that entered the single
     does not exit before the data is copied out by the other threads.  */
  if (find_omp_clause (gimple_omp_single_clauses (gsi_stmt (si)),
		       OMP_CLAUSE_COPYPRIVATE))
    need_barrier = true;
  gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_SINGLE);
  gsi_remove (&si, true);
  single_succ_edge (entry_bb)->flags = EDGE_FALLTHRU;

  si = gsi_last_bb (exit_bb);
  if (!gimple_omp_return_nowait_p (gsi_stmt (si)) || need_barrier)
    force_gimple_operand_gsi (&si, build_omp_barrier (), false, NULL_TREE,
			      false, GSI_SAME_STMT);
  gsi_remove (&si, true);
  single_succ_edge (exit_bb)->flags = EDGE_FALLTHRU;
}


/* Generic expansion for OpenMP synchronization directives: master,
   ordered and critical.  All we need to do here is remove the entry
   and exit markers for REGION.  */

static void
expand_omp_synch (struct omp_region *region)
{
  basic_block entry_bb, exit_bb;
  gimple_stmt_iterator si;

  entry_bb = region->entry;
  exit_bb = region->exit;

  si = gsi_last_bb (entry_bb);
  gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_SINGLE
	      || gimple_code (gsi_stmt (si)) == GIMPLE_OMP_MASTER
	      || gimple_code (gsi_stmt (si)) == GIMPLE_OMP_ORDERED
	      || gimple_code (gsi_stmt (si)) == GIMPLE_OMP_CRITICAL);
  gsi_remove (&si, true);
  single_succ_edge (entry_bb)->flags = EDGE_FALLTHRU;

  if (exit_bb)
    {
      si = gsi_last_bb (exit_bb);
      gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_RETURN);
      gsi_remove (&si, true);
      single_succ_edge (exit_bb)->flags = EDGE_FALLTHRU;
    }
}

/* A subroutine of expand_omp_atomic.  Attempt to implement the atomic
   operation as a __sync_fetch_and_op builtin.  INDEX is log2 of the
   size of the data type, and thus usable to find the index of the builtin
   decl.  Returns false if the expression is not of the proper form.  */

static bool
expand_omp_atomic_fetch_op (basic_block load_bb,
			    tree addr, tree loaded_val,
			    tree stored_val, int index)
{
  enum built_in_function base;
  tree decl, itype, call;
  enum insn_code *optab;
  tree rhs;
  basic_block store_bb = single_succ (load_bb);
  gimple_stmt_iterator gsi;
  gimple stmt;

  /* We expect to find the following sequences:
   
   load_bb:
       GIMPLE_OMP_ATOMIC_LOAD (tmp, mem)

   store_bb:
       val = tmp OP something; (or: something OP tmp)
       GIMPLE_OMP_STORE (val) 

  ???FIXME: Allow a more flexible sequence.  
  Perhaps use data flow to pick the statements.
  
  */

  gsi = gsi_after_labels (store_bb);
  stmt = gsi_stmt (gsi);
  if (!is_gimple_assign (stmt))
    return false;
  gsi_next (&gsi);
  if (gimple_code (gsi_stmt (gsi)) != GIMPLE_OMP_ATOMIC_STORE)
    return false;

  if (!operand_equal_p (gimple_assign_lhs (stmt), stored_val, 0))
    return false;

  /* Check for one of the supported fetch-op operations.  */
  switch (gimple_assign_rhs_code (stmt))
    {
    case PLUS_EXPR:
    case POINTER_PLUS_EXPR:
      base = BUILT_IN_FETCH_AND_ADD_N;
      optab = sync_add_optab;
      break;
    case MINUS_EXPR:
      base = BUILT_IN_FETCH_AND_SUB_N;
      optab = sync_add_optab;
      break;
    case BIT_AND_EXPR:
      base = BUILT_IN_FETCH_AND_AND_N;
      optab = sync_and_optab;
      break;
    case BIT_IOR_EXPR:
      base = BUILT_IN_FETCH_AND_OR_N;
      optab = sync_ior_optab;
      break;
    case BIT_XOR_EXPR:
      base = BUILT_IN_FETCH_AND_XOR_N;
      optab = sync_xor_optab;
      break;
    default:
      return false;
    }
  /* Make sure the expression is of the proper form.  */
  if (operand_equal_p (gimple_assign_rhs1 (stmt), loaded_val, 0))
    rhs = gimple_assign_rhs2 (stmt);
  else if (commutative_tree_code (gimple_assign_rhs_code (stmt))
	   && operand_equal_p (gimple_assign_rhs2 (stmt), loaded_val, 0))
    rhs = gimple_assign_rhs1 (stmt);
  else
    return false;

  decl = built_in_decls[base + index + 1];
  itype = TREE_TYPE (TREE_TYPE (decl));

  if (optab[TYPE_MODE (itype)] == CODE_FOR_nothing)
    return false;

  gsi = gsi_last_bb (load_bb);
  gcc_assert (gimple_code (gsi_stmt (gsi)) == GIMPLE_OMP_ATOMIC_LOAD);
  call = build_call_expr (decl, 2, addr, fold_convert (itype, rhs));
  call = fold_convert (void_type_node, call);
  force_gimple_operand_gsi (&gsi, call, true, NULL_TREE, true, GSI_SAME_STMT);
  gsi_remove (&gsi, true);

  gsi = gsi_last_bb (store_bb);
  gcc_assert (gimple_code (gsi_stmt (gsi)) == GIMPLE_OMP_ATOMIC_STORE);
  gsi_remove (&gsi, true);
  gsi = gsi_last_bb (store_bb);
  gsi_remove (&gsi, true);

  if (gimple_in_ssa_p (cfun))
    update_ssa (TODO_update_ssa_no_phi);

  return true;
}

/* A subroutine of expand_omp_atomic.  Implement the atomic operation as:

      oldval = *addr;
      repeat:
        newval = rhs;	 // with oldval replacing *addr in rhs
	oldval = __sync_val_compare_and_swap (addr, oldval, newval);
	if (oldval != newval)
	  goto repeat;

   INDEX is log2 of the size of the data type, and thus usable to find the
   index of the builtin decl.  */

static bool
expand_omp_atomic_pipeline (basic_block load_bb, basic_block store_bb,
			    tree addr, tree loaded_val, tree stored_val,
			    int index)
{
  tree loadedi, storedi, initial, new_storedi, old_vali;
  tree type, itype, cmpxchg, iaddr;
  gimple_stmt_iterator si;
  basic_block loop_header = single_succ (load_bb);
  gimple phi, stmt;
  edge e;

  cmpxchg = built_in_decls[BUILT_IN_VAL_COMPARE_AND_SWAP_N + index + 1];
  type = TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (addr)));
  itype = TREE_TYPE (TREE_TYPE (cmpxchg));

  if (sync_compare_and_swap[TYPE_MODE (itype)] == CODE_FOR_nothing)
    return false;

  /* Load the initial value, replacing the GIMPLE_OMP_ATOMIC_LOAD.  */
  si = gsi_last_bb (load_bb);
  gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_ATOMIC_LOAD);

  /* For floating-point values, we'll need to view-convert them to integers
     so that we can perform the atomic compare and swap.  Simplify the
     following code by always setting up the "i"ntegral variables.  */
  if (!INTEGRAL_TYPE_P (type) && !POINTER_TYPE_P (type))
    {
      tree iaddr_val;

      iaddr = create_tmp_var (build_pointer_type (itype), NULL);
      iaddr_val
	= force_gimple_operand_gsi (&si,
				    fold_convert (TREE_TYPE (iaddr), addr),
				    false, NULL_TREE, true, GSI_SAME_STMT);
      stmt = gimple_build_assign (iaddr, iaddr_val);
      gsi_insert_before (&si, stmt, GSI_SAME_STMT);
      DECL_NO_TBAA_P (iaddr) = 1;
      DECL_POINTER_ALIAS_SET (iaddr) = 0;
      loadedi = create_tmp_var (itype, NULL);
      if (gimple_in_ssa_p (cfun))
	{
	  add_referenced_var (iaddr);
	  add_referenced_var (loadedi);
	  loadedi = make_ssa_name (loadedi, NULL);
	}
    }
  else
    {
      iaddr = addr;
      loadedi = loaded_val;
    }

  initial = force_gimple_operand_gsi (&si, build_fold_indirect_ref (iaddr),
				      true, NULL_TREE, true, GSI_SAME_STMT);

  /* Move the value to the LOADEDI temporary.  */
  if (gimple_in_ssa_p (cfun))
    {
      gcc_assert (gimple_seq_empty_p (phi_nodes (loop_header)));
      phi = create_phi_node (loadedi, loop_header);
      SSA_NAME_DEF_STMT (loadedi) = phi;
      SET_USE (PHI_ARG_DEF_PTR_FROM_EDGE (phi, single_succ_edge (load_bb)),
	       initial);
    }
  else
    gsi_insert_before (&si,
		       gimple_build_assign (loadedi, initial),
		       GSI_SAME_STMT);
  if (loadedi != loaded_val)
    {
      gimple_stmt_iterator gsi2;
      tree x;

      x = build1 (VIEW_CONVERT_EXPR, type, loadedi);
      gsi2 = gsi_start_bb (loop_header);
      if (gimple_in_ssa_p (cfun))
	{
	  gimple stmt;
	  x = force_gimple_operand_gsi (&gsi2, x, true, NULL_TREE,
					true, GSI_SAME_STMT);
	  stmt = gimple_build_assign (loaded_val, x);
	  gsi_insert_before (&gsi2, stmt, GSI_SAME_STMT);
	}
      else
	{
	  x = build2 (MODIFY_EXPR, TREE_TYPE (loaded_val), loaded_val, x);
	  force_gimple_operand_gsi (&gsi2, x, true, NULL_TREE,
				    true, GSI_SAME_STMT);
	}
    }
  gsi_remove (&si, true);

  si = gsi_last_bb (store_bb);
  gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_ATOMIC_STORE);

  if (iaddr == addr)
    storedi = stored_val;
  else
    storedi =
      force_gimple_operand_gsi (&si,
				build1 (VIEW_CONVERT_EXPR, itype,
					stored_val), true, NULL_TREE, true,
				GSI_SAME_STMT);

  /* Build the compare&swap statement.  */
  new_storedi = build_call_expr (cmpxchg, 3, iaddr, loadedi, storedi);
  new_storedi = force_gimple_operand_gsi (&si,
					  fold_convert (itype, new_storedi),
					  true, NULL_TREE,
					  true, GSI_SAME_STMT);

  if (gimple_in_ssa_p (cfun))
    old_vali = loadedi;
  else
    {
      old_vali = create_tmp_var (itype, NULL);
      if (gimple_in_ssa_p (cfun))
	add_referenced_var (old_vali);
      stmt = gimple_build_assign (old_vali, loadedi);
      gsi_insert_before (&si, stmt, GSI_SAME_STMT);

      stmt = gimple_build_assign (loadedi, new_storedi);
      gsi_insert_before (&si, stmt, GSI_SAME_STMT);
    }

  /* Note that we always perform the comparison as an integer, even for
     floating point.  This allows the atomic operation to properly 
     succeed even with NaNs and -0.0.  */
  stmt = gimple_build_cond_empty
           (build2 (NE_EXPR, boolean_type_node,
		    new_storedi, old_vali));
  gsi_insert_before (&si, stmt, GSI_SAME_STMT);

  /* Update cfg.  */
  e = single_succ_edge (store_bb);
  e->flags &= ~EDGE_FALLTHRU;
  e->flags |= EDGE_FALSE_VALUE;

  e = make_edge (store_bb, loop_header, EDGE_TRUE_VALUE);

  /* Copy the new value to loadedi (we already did that before the condition
     if we are not in SSA).  */
  if (gimple_in_ssa_p (cfun))
    {
      phi = gimple_seq_first_stmt (phi_nodes (loop_header));
      SET_USE (PHI_ARG_DEF_PTR_FROM_EDGE (phi, e), new_storedi);
    }

  /* Remove GIMPLE_OMP_ATOMIC_STORE.  */
  gsi_remove (&si, true);

  if (gimple_in_ssa_p (cfun))
    update_ssa (TODO_update_ssa_no_phi);

  return true;
}

/* A subroutine of expand_omp_atomic.  Implement the atomic operation as:

		 		  GOMP_atomic_start ();
		 		  *addr = rhs;
		 		  GOMP_atomic_end ();

   The result is not globally atomic, but works so long as all parallel
   references are within #pragma omp atomic directives.  According to
   responses received from omp@openmp.org, appears to be within spec.
   Which makes sense, since that's how several other compilers handle
   this situation as well.  
   LOADED_VAL and ADDR are the operands of GIMPLE_OMP_ATOMIC_LOAD we're
   expanding.  STORED_VAL is the operand of the matching
   GIMPLE_OMP_ATOMIC_STORE.

   We replace 
   GIMPLE_OMP_ATOMIC_LOAD (loaded_val, addr) with  
   loaded_val = *addr;

   and replace
   GIMPLE_OMP_ATOMIC_ATORE (stored_val)  with
   *addr = stored_val;  
*/

static bool
expand_omp_atomic_mutex (basic_block load_bb, basic_block store_bb,
			 tree addr, tree loaded_val, tree stored_val)
{
  gimple_stmt_iterator si;
  gimple stmt;
  tree t;

  si = gsi_last_bb (load_bb);
  gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_ATOMIC_LOAD);

  t = built_in_decls[BUILT_IN_GOMP_ATOMIC_START];
  t = build_function_call_expr (t, 0);
  force_gimple_operand_gsi (&si, t, true, NULL_TREE, true, GSI_SAME_STMT);

  stmt = gimple_build_assign (loaded_val, build_fold_indirect_ref (addr));
  gsi_insert_before (&si, stmt, GSI_SAME_STMT);
  gsi_remove (&si, true);

  si = gsi_last_bb (store_bb);
  gcc_assert (gimple_code (gsi_stmt (si)) == GIMPLE_OMP_ATOMIC_STORE);

  stmt = gimple_build_assign (build_fold_indirect_ref (unshare_expr (addr)),
				stored_val);
  gsi_insert_before (&si, stmt, GSI_SAME_STMT);

  t = built_in_decls[BUILT_IN_GOMP_ATOMIC_END];
  t = build_function_call_expr (t, 0);
  force_gimple_operand_gsi (&si, t, true, NULL_TREE, true, GSI_SAME_STMT);
  gsi_remove (&si, true);

  if (gimple_in_ssa_p (cfun))
    update_ssa (TODO_update_ssa_no_phi);
  return true;
}

/* Expand an GIMPLE_OMP_ATOMIC statement.  We try to expand 
   using expand_omp_atomic_fetch_op. If it failed, we try to 
   call expand_omp_atomic_pipeline, and if it fails too, the
   ultimate fallback is wrapping the operation in a mutex
   (expand_omp_atomic_mutex).  REGION is the atomic region built 
   by build_omp_regions_1().  */ 

static void
expand_omp_atomic (struct omp_region *region)
{
  basic_block load_bb = region->entry, store_bb = region->exit;
  gimple load = last_stmt (load_bb), store = last_stmt (store_bb);
  tree loaded_val = gimple_omp_atomic_load_lhs (load);
  tree addr = gimple_omp_atomic_load_rhs (load);
  tree stored_val = gimple_omp_atomic_store_val (store);
  tree type = TYPE_MAIN_VARIANT (TREE_TYPE (TREE_TYPE (addr)));
  HOST_WIDE_INT index;

  /* Make sure the type is one of the supported sizes.  */
  index = tree_low_cst (TYPE_SIZE_UNIT (type), 1);
  index = exact_log2 (index);
  if (index >= 0 && index <= 4)
    {
      unsigned int align = TYPE_ALIGN_UNIT (type);

      /* __sync builtins require strict data alignment.  */
      if (exact_log2 (align) >= index)
	{
	  /* When possible, use specialized atomic update functions.  */
	  if ((INTEGRAL_TYPE_P (type) || POINTER_TYPE_P (type))
	      && store_bb == single_succ (load_bb))
	    {
	      if (expand_omp_atomic_fetch_op (load_bb, addr,
					      loaded_val, stored_val, index))
		return;
	    }

	  /* If we don't have specialized __sync builtins, try and implement
	     as a compare and swap loop.  */
	  if (expand_omp_atomic_pipeline (load_bb, store_bb, addr,
					  loaded_val, stored_val, index))
	    return;
	}
    }

  /* The ultimate fallback is wrapping the operation in a mutex.  */
  expand_omp_atomic_mutex (load_bb, store_bb, addr, loaded_val, stored_val);
}


/* Expand the parallel region tree rooted at REGION.  Expansion
   proceeds in depth-first order.  Innermost regions are expanded
   first.  This way, parallel regions that require a new function to
   be created (e.g., GIMPLE_OMP_PARALLEL) can be expanded without having any
   internal dependencies in their body.  */

static void
expand_omp (struct omp_region *region)
{
  while (region)
    {
      location_t saved_location;

      /* First, determine whether this is a combined parallel+workshare
       	 region.  */
      if (region->type == GIMPLE_OMP_PARALLEL)
	determine_parallel_type (region);

      if (region->inner)
	expand_omp (region->inner);

      saved_location = input_location;
      if (gimple_has_location (last_stmt (region->entry)))
	input_location = gimple_location (last_stmt (region->entry));

      switch (region->type)
	{
	case GIMPLE_OMP_PARALLEL:
	case GIMPLE_OMP_TASK:
	  expand_omp_taskreg (region);
	  break;

	case GIMPLE_OMP_FOR:
	  expand_omp_for (region);
	  break;

	case GIMPLE_OMP_SECTIONS:
	  expand_omp_sections (region);
	  break;

	case GIMPLE_OMP_SECTION:
	  /* Individual omp sections are handled together with their
	     parent GIMPLE_OMP_SECTIONS region.  */
	  break;

	case GIMPLE_OMP_SINGLE:
	  expand_omp_single (region);
	  break;

	case GIMPLE_OMP_MASTER:
	case GIMPLE_OMP_ORDERED:
	case GIMPLE_OMP_CRITICAL:
	  expand_omp_synch (region);
	  break;

	case GIMPLE_OMP_ATOMIC_LOAD:
	  expand_omp_atomic (region);
	  break;

	default:
	  gcc_unreachable ();
	}

      input_location = saved_location;
      region = region->next;
    }
}


/* Helper for build_omp_regions.  Scan the dominator tree starting at
   block BB.  PARENT is the region that contains BB.  If SINGLE_TREE is
   true, the function ends once a single tree is built (otherwise, whole
   forest of OMP constructs may be built).  */

static void
build_omp_regions_1 (basic_block bb, struct omp_region *parent,
		     bool single_tree)
{
  gimple_stmt_iterator gsi;
  gimple stmt;
  basic_block son;

  gsi = gsi_last_bb (bb);
  if (!gsi_end_p (gsi) && is_gimple_omp (gsi_stmt (gsi)))
    {
      struct omp_region *region;
      enum gimple_code code;

      stmt = gsi_stmt (gsi);
      code = gimple_code (stmt);
      if (code == GIMPLE_OMP_RETURN)
	{
	  /* STMT is the return point out of region PARENT.  Mark it
	     as the exit point and make PARENT the immediately
	     enclosing region.  */
	  gcc_assert (parent);
	  region = parent;
	  region->exit = bb;
	  parent = parent->outer;
	}
      else if (code == GIMPLE_OMP_ATOMIC_STORE)
	{
	  /* GIMPLE_OMP_ATOMIC_STORE is analoguous to
	     GIMPLE_OMP_RETURN, but matches with
	     GIMPLE_OMP_ATOMIC_LOAD.  */
	  gcc_assert (parent);
	  gcc_assert (parent->type == GIMPLE_OMP_ATOMIC_LOAD);
	  region = parent;
	  region->exit = bb;
	  parent = parent->outer;
	}

      else if (code == GIMPLE_OMP_CONTINUE)
	{
	  gcc_assert (parent);
	  parent->cont = bb;
	}
      else if (code == GIMPLE_OMP_SECTIONS_SWITCH)
	{
	  /* GIMPLE_OMP_SECTIONS_SWITCH is part of
	     GIMPLE_OMP_SECTIONS, and we do nothing for it.  */
	  ;
	}
      else
	{
	  /* Otherwise, this directive becomes the parent for a new
	     region.  */
	  region = new_omp_region (bb, code, parent);
	  parent = region;
	}
    }

  if (single_tree && !parent)
    return;

  for (son = first_dom_son (CDI_DOMINATORS, bb);
       son;
       son = next_dom_son (CDI_DOMINATORS, son))
    build_omp_regions_1 (son, parent, single_tree);
}

/* Builds the tree of OMP regions rooted at ROOT, storing it to
   root_omp_region.  */

static void
build_omp_regions_root (basic_block root)
{
  gcc_assert (root_omp_region == NULL);
  build_omp_regions_1 (root, NULL, true);
  gcc_assert (root_omp_region != NULL);
}

/* Expands omp construct (and its subconstructs) starting in HEAD.  */

void
omp_expand_local (basic_block head)
{
  build_omp_regions_root (head);
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "\nOMP region tree\n\n");
      dump_omp_region (dump_file, root_omp_region, 0);
      fprintf (dump_file, "\n");
    }

  remove_exit_barriers (root_omp_region);
  expand_omp (root_omp_region);

  free_omp_regions ();
}

/* Scan the CFG and build a tree of OMP regions.  Return the root of
   the OMP region tree.  */

static void
build_omp_regions (void)
{
  gcc_assert (root_omp_region == NULL);
  calculate_dominance_info (CDI_DOMINATORS);
  build_omp_regions_1 (ENTRY_BLOCK_PTR, NULL, false);
}

/* Main entry point for expanding OMP-GIMPLE into runtime calls.  */

static unsigned int
execute_expand_omp (void)
{
  build_omp_regions ();

  if (!root_omp_region)
    return 0;

  if (dump_file)
    {
      fprintf (dump_file, "\nOMP region tree\n\n");
      dump_omp_region (dump_file, root_omp_region, 0);
      fprintf (dump_file, "\n");
    }

  remove_exit_barriers (root_omp_region);

  expand_omp (root_omp_region);

  cleanup_tree_cfg ();

  free_omp_regions ();

  return 0;
}

/* OMP expansion -- the default pass, run before creation of SSA form.  */

static bool
gate_expand_omp (void)
{
  return (flag_openmp != 0 && errorcount == 0);
}

struct gimple_opt_pass pass_expand_omp = 
{
 {
  GIMPLE_PASS,
  "ompexp",				/* name */
  gate_expand_omp,			/* gate */
  execute_expand_omp,			/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  0,					/* tv_id */
  PROP_gimple_any,			/* properties_required */
  PROP_gimple_lomp,			/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_dump_func			/* todo_flags_finish */
 }
};

/* Routines to lower OpenMP directives into OMP-GIMPLE.  */

/* Lower the OpenMP sections directive in the current statement in GSI_P.
   CTX is the enclosing OMP context for the current statement.  */

static void
lower_omp_sections (gimple_stmt_iterator *gsi_p, omp_context *ctx)
{
  tree block, control;
  gimple_stmt_iterator tgsi;
  unsigned i, len;
  gimple stmt, new_stmt, bind, t;
  gimple_seq ilist, dlist, olist, new_body, body;
  struct gimplify_ctx gctx;

  stmt = gsi_stmt (*gsi_p);

  push_gimplify_context (&gctx);

  dlist = NULL;
  ilist = NULL;
  lower_rec_input_clauses (gimple_omp_sections_clauses (stmt),
      			   &ilist, &dlist, ctx);

  tgsi = gsi_start (gimple_omp_body (stmt));
  for (len = 0; !gsi_end_p (tgsi); len++, gsi_next (&tgsi))
    continue;

  tgsi = gsi_start (gimple_omp_body (stmt));
  body = NULL;
  for (i = 0; i < len; i++, gsi_next (&tgsi))
    {
      omp_context *sctx;
      gimple sec_start;

      sec_start = gsi_stmt (tgsi);
      sctx = maybe_lookup_ctx (sec_start);
      gcc_assert (sctx);

      gimple_seq_add_stmt (&body, sec_start);

      lower_omp (gimple_omp_body (sec_start), sctx);
      gimple_seq_add_seq (&body, gimple_omp_body (sec_start));
      gimple_omp_set_body (sec_start, NULL);

      if (i == len - 1)
	{
	  gimple_seq l = NULL;
	  lower_lastprivate_clauses (gimple_omp_sections_clauses (stmt), NULL,
				     &l, ctx);
	  gimple_seq_add_seq (&body, l);
	  gimple_omp_section_set_last (sec_start);
	}
      
      gimple_seq_add_stmt (&body, gimple_build_omp_return (false));
    }

  block = make_node (BLOCK);
  bind = gimple_build_bind (NULL, body, block);

  olist = NULL;
  lower_reduction_clauses (gimple_omp_sections_clauses (stmt), &olist, ctx);

  block = make_node (BLOCK);
  new_stmt = gimple_build_bind (NULL, NULL, block);

  pop_gimplify_context (new_stmt);
  gimple_bind_append_vars (new_stmt, ctx->block_vars);
  BLOCK_VARS (block) = gimple_bind_vars (bind);
  if (BLOCK_VARS (block))
    TREE_USED (block) = 1;

  new_body = NULL;
  gimple_seq_add_seq (&new_body, ilist);
  gimple_seq_add_stmt (&new_body, stmt);
  gimple_seq_add_stmt (&new_body, gimple_build_omp_sections_switch ());
  gimple_seq_add_stmt (&new_body, bind);

  control = create_tmp_var (unsigned_type_node, ".section");
  t = gimple_build_omp_continue (control, control);
  gimple_omp_sections_set_control (stmt, control);
  gimple_seq_add_stmt (&new_body, t);

  gimple_seq_add_seq (&new_body, olist);
  gimple_seq_add_seq (&new_body, dlist);

  new_body = maybe_catch_exception (new_body);

  t = gimple_build_omp_return
        (!!find_omp_clause (gimple_omp_sections_clauses (stmt),
			    OMP_CLAUSE_NOWAIT));
  gimple_seq_add_stmt (&new_body, t);

  gimple_bind_set_body (new_stmt, new_body);
  gimple_omp_set_body (stmt, NULL);

  gsi_replace (gsi_p, new_stmt, true);
}


/* A subroutine of lower_omp_single.  Expand the simple form of
   a GIMPLE_OMP_SINGLE, without a copyprivate clause:

     	if (GOMP_single_start ())
	  BODY;
	[ GOMP_barrier (); ]	-> unless 'nowait' is present.

  FIXME.  It may be better to delay expanding the logic of this until
  pass_expand_omp.  The expanded logic may make the job more difficult
  to a synchronization analysis pass.  */

static void
lower_omp_single_simple (gimple single_stmt, gimple_seq *pre_p)
{
  tree tlabel = create_artificial_label ();
  tree flabel = create_artificial_label ();
  gimple call, cond;
  tree lhs, decl;

  decl = built_in_decls[BUILT_IN_GOMP_SINGLE_START];
  lhs = create_tmp_var (TREE_TYPE (TREE_TYPE (decl)), NULL);
  call = gimple_build_call (decl, 0);
  gimple_call_set_lhs (call, lhs);
  gimple_seq_add_stmt (pre_p, call);

  cond = gimple_build_cond (EQ_EXPR, lhs,
			    fold_convert (TREE_TYPE (lhs), boolean_true_node),
			    tlabel, flabel);
  gimple_seq_add_stmt (pre_p, cond);
  gimple_seq_add_stmt (pre_p, gimple_build_label (tlabel));
  gimple_seq_add_seq (pre_p, gimple_omp_body (single_stmt));
  gimple_seq_add_stmt (pre_p, gimple_build_label (flabel));
}


/* A subroutine of lower_omp_single.  Expand the simple form of
   a GIMPLE_OMP_SINGLE, with a copyprivate clause:

	#pragma omp single copyprivate (a, b, c)

   Create a new structure to hold copies of 'a', 'b' and 'c' and emit:

      {
	if ((copyout_p = GOMP_single_copy_start ()) == NULL)
	  {
	    BODY;
	    copyout.a = a;
	    copyout.b = b;
	    copyout.c = c;
	    GOMP_single_copy_end (&copyout);
	  }
	else
	  {
	    a = copyout_p->a;
	    b = copyout_p->b;
	    c = copyout_p->c;
	  }
	GOMP_barrier ();
      }

  FIXME.  It may be better to delay expanding the logic of this until
  pass_expand_omp.  The expanded logic may make the job more difficult
  to a synchronization analysis pass.  */

static void
lower_omp_single_copy (gimple single_stmt, gimple_seq *pre_p, omp_context *ctx)
{
  tree ptr_type, t, l0, l1, l2;
  gimple_seq copyin_seq;

  ctx->sender_decl = create_tmp_var (ctx->record_type, ".omp_copy_o");

  ptr_type = build_pointer_type (ctx->record_type);
  ctx->receiver_decl = create_tmp_var (ptr_type, ".omp_copy_i");

  l0 = create_artificial_label ();
  l1 = create_artificial_label ();
  l2 = create_artificial_label ();

  t = build_call_expr (built_in_decls[BUILT_IN_GOMP_SINGLE_COPY_START], 0);
  t = fold_convert (ptr_type, t);
  gimplify_assign (ctx->receiver_decl, t, pre_p);

  t = build2 (EQ_EXPR, boolean_type_node, ctx->receiver_decl,
	      build_int_cst (ptr_type, 0));
  t = build3 (COND_EXPR, void_type_node, t,
	      build_and_jump (&l0), build_and_jump (&l1));
  gimplify_and_add (t, pre_p);

  gimple_seq_add_stmt (pre_p, gimple_build_label (l0));

  gimple_seq_add_seq (pre_p, gimple_omp_body (single_stmt));

  copyin_seq = NULL;
  lower_copyprivate_clauses (gimple_omp_single_clauses (single_stmt), pre_p,
			      &copyin_seq, ctx);

  t = build_fold_addr_expr (ctx->sender_decl);
  t = build_call_expr (built_in_decls[BUILT_IN_GOMP_SINGLE_COPY_END], 1, t);
  gimplify_and_add (t, pre_p);

  t = build_and_jump (&l2);
  gimplify_and_add (t, pre_p);

  gimple_seq_add_stmt (pre_p, gimple_build_label (l1));

  gimple_seq_add_seq (pre_p, copyin_seq);

  gimple_seq_add_stmt (pre_p, gimple_build_label (l2));
}


/* Expand code for an OpenMP single directive.  */

static void
lower_omp_single (gimple_stmt_iterator *gsi_p, omp_context *ctx)
{
  tree block;
  gimple t, bind, single_stmt = gsi_stmt (*gsi_p);
  gimple_seq bind_body, dlist;
  struct gimplify_ctx gctx;

  push_gimplify_context (&gctx);

  bind_body = NULL;
  lower_rec_input_clauses (gimple_omp_single_clauses (single_stmt),
			   &bind_body, &dlist, ctx);
  lower_omp (gimple_omp_body (single_stmt), ctx);

  gimple_seq_add_stmt (&bind_body, single_stmt);

  if (ctx->record_type)
    lower_omp_single_copy (single_stmt, &bind_body, ctx);
  else
    lower_omp_single_simple (single_stmt, &bind_body);

  gimple_omp_set_body (single_stmt, NULL);

  gimple_seq_add_seq (&bind_body, dlist);

  bind_body = maybe_catch_exception (bind_body);

  t = gimple_build_omp_return 
        (!!find_omp_clause (gimple_omp_single_clauses (single_stmt),
			    OMP_CLAUSE_NOWAIT));
  gimple_seq_add_stmt (&bind_body, t);

  block = make_node (BLOCK);
  bind = gimple_build_bind (NULL, bind_body, block);

  pop_gimplify_context (bind);

  gimple_bind_append_vars (bind, ctx->block_vars);
  BLOCK_VARS (block) = ctx->block_vars;
  gsi_replace (gsi_p, bind, true);
  if (BLOCK_VARS (block))
    TREE_USED (block) = 1;
}


/* Expand code for an OpenMP master directive.  */

static void
lower_omp_master (gimple_stmt_iterator *gsi_p, omp_context *ctx)
{
  tree block, lab = NULL, x;
  gimple stmt = gsi_stmt (*gsi_p), bind;
  gimple_seq tseq;
  struct gimplify_ctx gctx;

  push_gimplify_context (&gctx);

  block = make_node (BLOCK);
  bind = gimple_build_bind (NULL, gimple_seq_alloc_with_stmt (stmt),
      				 block);

  x = build_call_expr (built_in_decls[BUILT_IN_OMP_GET_THREAD_NUM], 0);
  x = build2 (EQ_EXPR, boolean_type_node, x, integer_zero_node);
  x = build3 (COND_EXPR, void_type_node, x, NULL, build_and_jump (&lab));
  tseq = NULL;
  gimplify_and_add (x, &tseq);
  gimple_bind_add_seq (bind, tseq);

  lower_omp (gimple_omp_body (stmt), ctx);
  gimple_omp_set_body (stmt, maybe_catch_exception (gimple_omp_body (stmt)));
  gimple_bind_add_seq (bind, gimple_omp_body (stmt));
  gimple_omp_set_body (stmt, NULL);

  gimple_bind_add_stmt (bind, gimple_build_label (lab));

  gimple_bind_add_stmt (bind, gimple_build_omp_return (true));

  pop_gimplify_context (bind);

  gimple_bind_append_vars (bind, ctx->block_vars);
  BLOCK_VARS (block) = ctx->block_vars;
  gsi_replace (gsi_p, bind, true);
}


/* Expand code for an OpenMP ordered directive.  */

static void
lower_omp_ordered (gimple_stmt_iterator *gsi_p, omp_context *ctx)
{
  tree block;
  gimple stmt = gsi_stmt (*gsi_p), bind, x;
  struct gimplify_ctx gctx;

  push_gimplify_context (&gctx);

  block = make_node (BLOCK);
  bind = gimple_build_bind (NULL, gimple_seq_alloc_with_stmt (stmt),
      				   block);

  x = gimple_build_call (built_in_decls[BUILT_IN_GOMP_ORDERED_START], 0);
  gimple_bind_add_stmt (bind, x);

  lower_omp (gimple_omp_body (stmt), ctx);
  gimple_omp_set_body (stmt, maybe_catch_exception (gimple_omp_body (stmt)));
  gimple_bind_add_seq (bind, gimple_omp_body (stmt));
  gimple_omp_set_body (stmt, NULL);

  x = gimple_build_call (built_in_decls[BUILT_IN_GOMP_ORDERED_END], 0);
  gimple_bind_add_stmt (bind, x);

  gimple_bind_add_stmt (bind, gimple_build_omp_return (true));

  pop_gimplify_context (bind);

  gimple_bind_append_vars (bind, ctx->block_vars);
  BLOCK_VARS (block) = gimple_bind_vars (bind);
  gsi_replace (gsi_p, bind, true);
}


/* Gimplify a GIMPLE_OMP_CRITICAL statement.  This is a relatively simple
   substitution of a couple of function calls.  But in the NAMED case,
   requires that languages coordinate a symbol name.  It is therefore
   best put here in common code.  */

static GTY((param1_is (tree), param2_is (tree)))
  splay_tree critical_name_mutexes;

static void
lower_omp_critical (gimple_stmt_iterator *gsi_p, omp_context *ctx)
{
  tree block;
  tree name, lock, unlock;
  gimple stmt = gsi_stmt (*gsi_p), bind;
  gimple_seq tbody;
  struct gimplify_ctx gctx;

  name = gimple_omp_critical_name (stmt);
  if (name)
    {
      tree decl;
      splay_tree_node n;

      if (!critical_name_mutexes)
	critical_name_mutexes
	  = splay_tree_new_ggc (splay_tree_compare_pointers);

      n = splay_tree_lookup (critical_name_mutexes, (splay_tree_key) name);
      if (n == NULL)
	{
	  char *new_str;

	  decl = create_tmp_var_raw (ptr_type_node, NULL);

	  new_str = ACONCAT ((".gomp_critical_user_",
			      IDENTIFIER_POINTER (name), NULL));
	  DECL_NAME (decl) = get_identifier (new_str);
	  TREE_PUBLIC (decl) = 1;
	  TREE_STATIC (decl) = 1;
	  DECL_COMMON (decl) = 1;
	  DECL_ARTIFICIAL (decl) = 1;
	  DECL_IGNORED_P (decl) = 1;
	  varpool_finalize_decl (decl);

	  splay_tree_insert (critical_name_mutexes, (splay_tree_key) name,
			     (splay_tree_value) decl);
	}
      else
	decl = (tree) n->value;

      lock = built_in_decls[BUILT_IN_GOMP_CRITICAL_NAME_START];
      lock = build_call_expr (lock, 1, build_fold_addr_expr (decl));

      unlock = built_in_decls[BUILT_IN_GOMP_CRITICAL_NAME_END];
      unlock = build_call_expr (unlock, 1, build_fold_addr_expr (decl));
    }
  else
    {
      lock = built_in_decls[BUILT_IN_GOMP_CRITICAL_START];
      lock = build_call_expr (lock, 0);

      unlock = built_in_decls[BUILT_IN_GOMP_CRITICAL_END];
      unlock = build_call_expr (unlock, 0);
    }

  push_gimplify_context (&gctx);

  block = make_node (BLOCK);
  bind = gimple_build_bind (NULL, gimple_seq_alloc_with_stmt (stmt), block);

  tbody = gimple_bind_body (bind);
  gimplify_and_add (lock, &tbody);
  gimple_bind_set_body (bind, tbody);

  lower_omp (gimple_omp_body (stmt), ctx);
  gimple_omp_set_body (stmt, maybe_catch_exception (gimple_omp_body (stmt)));
  gimple_bind_add_seq (bind, gimple_omp_body (stmt));
  gimple_omp_set_body (stmt, NULL);

  tbody = gimple_bind_body (bind);
  gimplify_and_add (unlock, &tbody);
  gimple_bind_set_body (bind, tbody);

  gimple_bind_add_stmt (bind, gimple_build_omp_return (true));

  pop_gimplify_context (bind);
  gimple_bind_append_vars (bind, ctx->block_vars);
  BLOCK_VARS (block) = gimple_bind_vars (bind);
  gsi_replace (gsi_p, bind, true);
}


/* A subroutine of lower_omp_for.  Generate code to emit the predicate
   for a lastprivate clause.  Given a loop control predicate of (V
   cond N2), we gate the clause on (!(V cond N2)).  The lowered form
   is appended to *DLIST, iterator initialization is appended to
   *BODY_P.  */

static void
lower_omp_for_lastprivate (struct omp_for_data *fd, gimple_seq *body_p,
			   gimple_seq *dlist, struct omp_context *ctx)
{
  tree clauses, cond, vinit;
  enum tree_code cond_code;
  gimple_seq stmts;
  
  cond_code = fd->loop.cond_code;
  cond_code = cond_code == LT_EXPR ? GE_EXPR : LE_EXPR;

  /* When possible, use a strict equality expression.  This can let VRP
     type optimizations deduce the value and remove a copy.  */
  if (host_integerp (fd->loop.step, 0))
    {
      HOST_WIDE_INT step = TREE_INT_CST_LOW (fd->loop.step);
      if (step == 1 || step == -1)
	cond_code = EQ_EXPR;
    }

  cond = build2 (cond_code, boolean_type_node, fd->loop.v, fd->loop.n2);

  clauses = gimple_omp_for_clauses (fd->for_stmt);
  stmts = NULL;
  lower_lastprivate_clauses (clauses, cond, &stmts, ctx);
  if (!gimple_seq_empty_p (stmts))
    {
      gimple_seq_add_seq (&stmts, *dlist);
      *dlist = stmts;

      /* Optimize: v = 0; is usually cheaper than v = some_other_constant.  */
      vinit = fd->loop.n1;
      if (cond_code == EQ_EXPR
	  && host_integerp (fd->loop.n2, 0)
	  && ! integer_zerop (fd->loop.n2))
	vinit = build_int_cst (TREE_TYPE (fd->loop.v), 0);

      /* Initialize the iterator variable, so that threads that don't execute
	 any iterations don't execute the lastprivate clauses by accident.  */
      gimplify_assign (fd->loop.v, vinit, body_p);
    }
}


/* Lower code for an OpenMP loop directive.  */

static void
lower_omp_for (gimple_stmt_iterator *gsi_p, omp_context *ctx)
{
  tree *rhs_p, block;
  struct omp_for_data fd;
  gimple stmt = gsi_stmt (*gsi_p), new_stmt;
  gimple_seq omp_for_body, body, dlist, ilist;
  size_t i;
  struct gimplify_ctx gctx;

  push_gimplify_context (&gctx);

  lower_omp (gimple_omp_for_pre_body (stmt), ctx);
  lower_omp (gimple_omp_body (stmt), ctx);

  block = make_node (BLOCK);
  new_stmt = gimple_build_bind (NULL, NULL, block);

  /* Move declaration of temporaries in the loop body before we make
     it go away.  */
  omp_for_body = gimple_omp_body (stmt);
  if (!gimple_seq_empty_p (omp_for_body)
      && gimple_code (gimple_seq_first_stmt (omp_for_body)) == GIMPLE_BIND)
    {
      tree vars = gimple_bind_vars (gimple_seq_first_stmt (omp_for_body));
      gimple_bind_append_vars (new_stmt, vars);
    }

  /* The pre-body and input clauses go before the lowered GIMPLE_OMP_FOR.  */
  ilist = NULL;
  dlist = NULL;
  body = NULL;
  lower_rec_input_clauses (gimple_omp_for_clauses (stmt), &body, &dlist, ctx);
  gimple_seq_add_seq (&body, gimple_omp_for_pre_body (stmt));

  /* Lower the header expressions.  At this point, we can assume that
     the header is of the form:

     	#pragma omp for (V = VAL1; V {<|>|<=|>=} VAL2; V = V [+-] VAL3)

     We just need to make sure that VAL1, VAL2 and VAL3 are lowered
     using the .omp_data_s mapping, if needed.  */
  for (i = 0; i < gimple_omp_for_collapse (stmt); i++)
    {
      rhs_p = gimple_omp_for_initial_ptr (stmt, i);
      if (!is_gimple_min_invariant (*rhs_p))
	*rhs_p = get_formal_tmp_var (*rhs_p, &body);

      rhs_p = gimple_omp_for_final_ptr (stmt, i);
      if (!is_gimple_min_invariant (*rhs_p))
	*rhs_p = get_formal_tmp_var (*rhs_p, &body);

      rhs_p = &TREE_OPERAND (gimple_omp_for_incr (stmt, i), 1);
      if (!is_gimple_min_invariant (*rhs_p))
	*rhs_p = get_formal_tmp_var (*rhs_p, &body);
    }

  /* Once lowered, extract the bounds and clauses.  */
  extract_omp_for_data (stmt, &fd, NULL);

  lower_omp_for_lastprivate (&fd, &body, &dlist, ctx);

  gimple_seq_add_stmt (&body, stmt);
  gimple_seq_add_seq (&body, gimple_omp_body (stmt));

  gimple_seq_add_stmt (&body, gimple_build_omp_continue (fd.loop.v,
							 fd.loop.v));

  /* After the loop, add exit clauses.  */
  lower_reduction_clauses (gimple_omp_for_clauses (stmt), &body, ctx);
  gimple_seq_add_seq (&body, dlist);

  body = maybe_catch_exception (body);

  /* Region exit marker goes at the end of the loop body.  */
  gimple_seq_add_stmt (&body, gimple_build_omp_return (fd.have_nowait));

  pop_gimplify_context (new_stmt);

  gimple_bind_append_vars (new_stmt, ctx->block_vars);
  BLOCK_VARS (block) = gimple_bind_vars (new_stmt);
  if (BLOCK_VARS (block))
    TREE_USED (block) = 1;

  gimple_bind_set_body (new_stmt, body);
  gimple_omp_set_body (stmt, NULL);
  gimple_omp_for_set_pre_body (stmt, NULL);
  gsi_replace (gsi_p, new_stmt, true);
}

/* Callback for walk_stmts.  Check if the current statement only contains 
   GIMPLE_OMP_FOR or GIMPLE_OMP_PARALLEL.  */

static tree
check_combined_parallel (gimple_stmt_iterator *gsi_p,
    			 bool *handled_ops_p,
    			 struct walk_stmt_info *wi)
{
  int *info = (int *) wi->info;
  gimple stmt = gsi_stmt (*gsi_p);

  *handled_ops_p = true;
  switch (gimple_code (stmt))
    {
    WALK_SUBSTMTS;

    case GIMPLE_OMP_FOR:
    case GIMPLE_OMP_SECTIONS:
      *info = *info == 0 ? 1 : -1;
      break;
    default:
      *info = -1;
      break;
    }
  return NULL;
}

struct omp_taskcopy_context
{
  /* This field must be at the beginning, as we do "inheritance": Some
     callback functions for tree-inline.c (e.g., omp_copy_decl)
     receive a copy_body_data pointer that is up-casted to an
     omp_context pointer.  */
  copy_body_data cb;
  omp_context *ctx;
};

static tree
task_copyfn_copy_decl (tree var, copy_body_data *cb)
{
  struct omp_taskcopy_context *tcctx = (struct omp_taskcopy_context *) cb;

  if (splay_tree_lookup (tcctx->ctx->sfield_map, (splay_tree_key) var))
    return create_tmp_var (TREE_TYPE (var), NULL);

  return var;
}

static tree
task_copyfn_remap_type (struct omp_taskcopy_context *tcctx, tree orig_type)
{
  tree name, new_fields = NULL, type, f;

  type = lang_hooks.types.make_type (RECORD_TYPE);
  name = DECL_NAME (TYPE_NAME (orig_type));
  name = build_decl (TYPE_DECL, name, type);
  TYPE_NAME (type) = name;

  for (f = TYPE_FIELDS (orig_type); f ; f = TREE_CHAIN (f))
    {
      tree new_f = copy_node (f);
      DECL_CONTEXT (new_f) = type;
      TREE_TYPE (new_f) = remap_type (TREE_TYPE (f), &tcctx->cb);
      TREE_CHAIN (new_f) = new_fields;
      walk_tree (&DECL_SIZE (new_f), copy_tree_body_r, &tcctx->cb, NULL);
      walk_tree (&DECL_SIZE_UNIT (new_f), copy_tree_body_r, &tcctx->cb, NULL);
      walk_tree (&DECL_FIELD_OFFSET (new_f), copy_tree_body_r,
		 &tcctx->cb, NULL);
      new_fields = new_f;
      *pointer_map_insert (tcctx->cb.decl_map, f) = new_f;
    }
  TYPE_FIELDS (type) = nreverse (new_fields);
  layout_type (type);
  return type;
}

/* Create task copyfn.  */

static void
create_task_copyfn (gimple task_stmt, omp_context *ctx)
{
  struct function *child_cfun;
  tree child_fn, t, c, src, dst, f, sf, arg, sarg, decl;
  tree record_type, srecord_type, bind, list;
  bool record_needs_remap = false, srecord_needs_remap = false;
  splay_tree_node n;
  struct omp_taskcopy_context tcctx;
  struct gimplify_ctx gctx;

  child_fn = gimple_omp_task_copy_fn (task_stmt);
  child_cfun = DECL_STRUCT_FUNCTION (child_fn);
  gcc_assert (child_cfun->cfg == NULL);
  child_cfun->dont_save_pending_sizes_p = 1;
  DECL_SAVED_TREE (child_fn) = alloc_stmt_list ();

  /* Reset DECL_CONTEXT on function arguments.  */
  for (t = DECL_ARGUMENTS (child_fn); t; t = TREE_CHAIN (t))
    DECL_CONTEXT (t) = child_fn;

  /* Populate the function.  */
  push_gimplify_context (&gctx);
  current_function_decl = child_fn;

  bind = build3 (BIND_EXPR, void_type_node, NULL, NULL, NULL);
  TREE_SIDE_EFFECTS (bind) = 1;
  list = NULL;
  DECL_SAVED_TREE (child_fn) = bind;
  DECL_SOURCE_LOCATION (child_fn) = gimple_location (task_stmt);

  /* Remap src and dst argument types if needed.  */
  record_type = ctx->record_type;
  srecord_type = ctx->srecord_type;
  for (f = TYPE_FIELDS (record_type); f ; f = TREE_CHAIN (f))
    if (variably_modified_type_p (TREE_TYPE (f), ctx->cb.src_fn))
      {
	record_needs_remap = true;
	break;
      }
  for (f = TYPE_FIELDS (srecord_type); f ; f = TREE_CHAIN (f))
    if (variably_modified_type_p (TREE_TYPE (f), ctx->cb.src_fn))
      {
	srecord_needs_remap = true;
	break;
      }

  if (record_needs_remap || srecord_needs_remap)
    {
      memset (&tcctx, '\0', sizeof (tcctx));
      tcctx.cb.src_fn = ctx->cb.src_fn;
      tcctx.cb.dst_fn = child_fn;
      tcctx.cb.src_node = cgraph_node (tcctx.cb.src_fn);
      tcctx.cb.dst_node = tcctx.cb.src_node;
      tcctx.cb.src_cfun = ctx->cb.src_cfun;
      tcctx.cb.copy_decl = task_copyfn_copy_decl;
      tcctx.cb.eh_region = -1;
      tcctx.cb.transform_call_graph_edges = CB_CGE_MOVE;
      tcctx.cb.decl_map = pointer_map_create ();
      tcctx.ctx = ctx;

      if (record_needs_remap)
	record_type = task_copyfn_remap_type (&tcctx, record_type);
      if (srecord_needs_remap)
	srecord_type = task_copyfn_remap_type (&tcctx, srecord_type);
    }
  else
    tcctx.cb.decl_map = NULL;

  push_cfun (child_cfun);

  arg = DECL_ARGUMENTS (child_fn);
  TREE_TYPE (arg) = build_pointer_type (record_type);
  sarg = TREE_CHAIN (arg);
  TREE_TYPE (sarg) = build_pointer_type (srecord_type);

  /* First pass: initialize temporaries used in record_type and srecord_type
     sizes and field offsets.  */
  if (tcctx.cb.decl_map)
    for (c = gimple_omp_task_clauses (task_stmt); c; c = OMP_CLAUSE_CHAIN (c))
      if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_FIRSTPRIVATE)
	{
	  tree *p;

	  decl = OMP_CLAUSE_DECL (c);
	  p = (tree *) pointer_map_contains (tcctx.cb.decl_map, decl);
	  if (p == NULL)
	    continue;
	  n = splay_tree_lookup (ctx->sfield_map, (splay_tree_key) decl);
	  sf = (tree) n->value;
	  sf = *(tree *) pointer_map_contains (tcctx.cb.decl_map, sf);
	  src = build_fold_indirect_ref (sarg);
	  src = build3 (COMPONENT_REF, TREE_TYPE (sf), src, sf, NULL);
	  t = build2 (MODIFY_EXPR, TREE_TYPE (*p), *p, src);
	  append_to_statement_list (t, &list);
	}

  /* Second pass: copy shared var pointers and copy construct non-VLA
     firstprivate vars.  */
  for (c = gimple_omp_task_clauses (task_stmt); c; c = OMP_CLAUSE_CHAIN (c))
    switch (OMP_CLAUSE_CODE (c))
      {
      case OMP_CLAUSE_SHARED:
	decl = OMP_CLAUSE_DECL (c);
	n = splay_tree_lookup (ctx->field_map, (splay_tree_key) decl);
	if (n == NULL)
	  break;
	f = (tree) n->value;
	if (tcctx.cb.decl_map)
	  f = *(tree *) pointer_map_contains (tcctx.cb.decl_map, f);
	n = splay_tree_lookup (ctx->sfield_map, (splay_tree_key) decl);
	sf = (tree) n->value;
	if (tcctx.cb.decl_map)
	  sf = *(tree *) pointer_map_contains (tcctx.cb.decl_map, sf);
	src = build_fold_indirect_ref (sarg);
	src = build3 (COMPONENT_REF, TREE_TYPE (sf), src, sf, NULL);
	dst = build_fold_indirect_ref (arg);
	dst = build3 (COMPONENT_REF, TREE_TYPE (f), dst, f, NULL);
	t = build2 (MODIFY_EXPR, TREE_TYPE (dst), dst, src);
	append_to_statement_list (t, &list);
	break;
      case OMP_CLAUSE_FIRSTPRIVATE:
	decl = OMP_CLAUSE_DECL (c);
	if (is_variable_sized (decl))
	  break;
	n = splay_tree_lookup (ctx->field_map, (splay_tree_key) decl);
	if (n == NULL)
	  break;
	f = (tree) n->value;
	if (tcctx.cb.decl_map)
	  f = *(tree *) pointer_map_contains (tcctx.cb.decl_map, f);
	n = splay_tree_lookup (ctx->sfield_map, (splay_tree_key) decl);
	if (n != NULL)
	  {
	    sf = (tree) n->value;
	    if (tcctx.cb.decl_map)
	      sf = *(tree *) pointer_map_contains (tcctx.cb.decl_map, sf);
	    src = build_fold_indirect_ref (sarg);
	    src = build3 (COMPONENT_REF, TREE_TYPE (sf), src, sf, NULL);
	    if (use_pointer_for_field (decl, NULL) || is_reference (decl))
	      src = build_fold_indirect_ref (src);
	  }
	else
	  src = decl;
	dst = build_fold_indirect_ref (arg);
	dst = build3 (COMPONENT_REF, TREE_TYPE (f), dst, f, NULL);
	t = lang_hooks.decls.omp_clause_copy_ctor (c, dst, src);
	append_to_statement_list (t, &list);
	break;
      case OMP_CLAUSE_PRIVATE:
	if (! OMP_CLAUSE_PRIVATE_OUTER_REF (c))
	  break;
	decl = OMP_CLAUSE_DECL (c);
	n = splay_tree_lookup (ctx->field_map, (splay_tree_key) decl);
	f = (tree) n->value;
	if (tcctx.cb.decl_map)
	  f = *(tree *) pointer_map_contains (tcctx.cb.decl_map, f);
	n = splay_tree_lookup (ctx->sfield_map, (splay_tree_key) decl);
	if (n != NULL)
	  {
	    sf = (tree) n->value;
	    if (tcctx.cb.decl_map)
	      sf = *(tree *) pointer_map_contains (tcctx.cb.decl_map, sf);
	    src = build_fold_indirect_ref (sarg);
	    src = build3 (COMPONENT_REF, TREE_TYPE (sf), src, sf, NULL);
	    if (use_pointer_for_field (decl, NULL))
	      src = build_fold_indirect_ref (src);
	  }
	else
	  src = decl;
	dst = build_fold_indirect_ref (arg);
	dst = build3 (COMPONENT_REF, TREE_TYPE (f), dst, f, NULL);
	t = build2 (MODIFY_EXPR, TREE_TYPE (dst), dst, src);
	append_to_statement_list (t, &list);
	break;
      default:
	break;
      }

  /* Last pass: handle VLA firstprivates.  */
  if (tcctx.cb.decl_map)
    for (c = gimple_omp_task_clauses (task_stmt); c; c = OMP_CLAUSE_CHAIN (c))
      if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_FIRSTPRIVATE)
	{
	  tree ind, ptr, df;

	  decl = OMP_CLAUSE_DECL (c);
	  if (!is_variable_sized (decl))
	    continue;
	  n = splay_tree_lookup (ctx->field_map, (splay_tree_key) decl);
	  if (n == NULL)
	    continue;
	  f = (tree) n->value;
	  f = *(tree *) pointer_map_contains (tcctx.cb.decl_map, f);
	  gcc_assert (DECL_HAS_VALUE_EXPR_P (decl));
	  ind = DECL_VALUE_EXPR (decl);
	  gcc_assert (TREE_CODE (ind) == INDIRECT_REF);
	  gcc_assert (DECL_P (TREE_OPERAND (ind, 0)));
	  n = splay_tree_lookup (ctx->sfield_map,
				 (splay_tree_key) TREE_OPERAND (ind, 0));
	  sf = (tree) n->value;
	  sf = *(tree *) pointer_map_contains (tcctx.cb.decl_map, sf);
	  src = build_fold_indirect_ref (sarg);
	  src = build3 (COMPONENT_REF, TREE_TYPE (sf), src, sf, NULL);
	  src = build_fold_indirect_ref (src);
	  dst = build_fold_indirect_ref (arg);
	  dst = build3 (COMPONENT_REF, TREE_TYPE (f), dst, f, NULL);
	  t = lang_hooks.decls.omp_clause_copy_ctor (c, dst, src);
	  append_to_statement_list (t, &list);
	  n = splay_tree_lookup (ctx->field_map,
				 (splay_tree_key) TREE_OPERAND (ind, 0));
	  df = (tree) n->value;
	  df = *(tree *) pointer_map_contains (tcctx.cb.decl_map, df);
	  ptr = build_fold_indirect_ref (arg);
	  ptr = build3 (COMPONENT_REF, TREE_TYPE (df), ptr, df, NULL);
	  t = build2 (MODIFY_EXPR, TREE_TYPE (ptr), ptr,
		      build_fold_addr_expr (dst));
	  append_to_statement_list (t, &list);
	}

  t = build1 (RETURN_EXPR, void_type_node, NULL);
  append_to_statement_list (t, &list);

  if (tcctx.cb.decl_map)
    pointer_map_destroy (tcctx.cb.decl_map);
  pop_gimplify_context (NULL);
  BIND_EXPR_BODY (bind) = list;
  pop_cfun ();
  current_function_decl = ctx->cb.src_fn;
}

/* Lower the OpenMP parallel or task directive in the current statement
   in GSI_P.  CTX holds context information for the directive.  */

static void
lower_omp_taskreg (gimple_stmt_iterator *gsi_p, omp_context *ctx)
{
  tree clauses;
  tree child_fn, t;
  gimple stmt = gsi_stmt (*gsi_p);
  gimple par_bind, bind;
  gimple_seq par_body, olist, ilist, par_olist, par_ilist, new_body;
  struct gimplify_ctx gctx;

  clauses = gimple_omp_taskreg_clauses (stmt);
  par_bind = gimple_seq_first_stmt (gimple_omp_body (stmt));
  par_body = gimple_bind_body (par_bind);
  child_fn = ctx->cb.dst_fn;
  if (gimple_code (stmt) == GIMPLE_OMP_PARALLEL
      && !gimple_omp_parallel_combined_p (stmt))
    {
      struct walk_stmt_info wi;
      int ws_num = 0;

      memset (&wi, 0, sizeof (wi));
      wi.info = &ws_num;
      wi.val_only = true;
      walk_gimple_seq (par_body, check_combined_parallel, NULL, &wi);
      if (ws_num == 1)
	gimple_omp_parallel_set_combined_p (stmt, true);
    }
  if (ctx->srecord_type)
    create_task_copyfn (stmt, ctx);

  push_gimplify_context (&gctx);

  par_olist = NULL;
  par_ilist = NULL;
  lower_rec_input_clauses (clauses, &par_ilist, &par_olist, ctx);
  lower_omp (par_body, ctx);
  if (gimple_code (stmt) == GIMPLE_OMP_PARALLEL)
    lower_reduction_clauses (clauses, &par_olist, ctx);

  /* Declare all the variables created by mapping and the variables
     declared in the scope of the parallel body.  */
  record_vars_into (ctx->block_vars, child_fn);
  record_vars_into (gimple_bind_vars (par_bind), child_fn);

  if (ctx->record_type)
    {
      ctx->sender_decl
	= create_tmp_var (ctx->srecord_type ? ctx->srecord_type
			  : ctx->record_type, ".omp_data_o");
      gimple_omp_taskreg_set_data_arg (stmt, ctx->sender_decl);
    }

  olist = NULL;
  ilist = NULL;
  lower_send_clauses (clauses, &ilist, &olist, ctx);
  lower_send_shared_vars (&ilist, &olist, ctx);

  /* Once all the expansions are done, sequence all the different
     fragments inside gimple_omp_body.  */

  new_body = NULL;

  if (ctx->record_type)
    {
      t = build_fold_addr_expr (ctx->sender_decl);
      /* fixup_child_record_type might have changed receiver_decl's type.  */
      t = fold_convert (TREE_TYPE (ctx->receiver_decl), t);
      gimple_seq_add_stmt (&new_body,
	  		   gimple_build_assign (ctx->receiver_decl, t));
    }

  gimple_seq_add_seq (&new_body, par_ilist);
  gimple_seq_add_seq (&new_body, par_body);
  gimple_seq_add_seq (&new_body, par_olist);
  new_body = maybe_catch_exception (new_body);
  gimple_seq_add_stmt (&new_body, gimple_build_omp_return (false));
  gimple_omp_set_body (stmt, new_body);

  bind = gimple_build_bind (NULL, NULL, gimple_bind_block (par_bind));
  gimple_bind_add_stmt (bind, stmt);
  if (ilist || olist)
    {
      gimple_seq_add_stmt (&ilist, bind);
      gimple_seq_add_seq (&ilist, olist);
      bind = gimple_build_bind (NULL, ilist, NULL);
    }

  gsi_replace (gsi_p, bind, true);

  pop_gimplify_context (NULL);
}

/* Callback for lower_omp_1.  Return non-NULL if *tp needs to be
   regimplified.  If DATA is non-NULL, lower_omp_1 is outside
   of OpenMP context, but with task_shared_vars set.  */

static tree
lower_omp_regimplify_p (tree *tp, int *walk_subtrees,
    			void *data)
{
  tree t = *tp;

  /* Any variable with DECL_VALUE_EXPR needs to be regimplified.  */
  if (TREE_CODE (t) == VAR_DECL && data == NULL && DECL_HAS_VALUE_EXPR_P (t))
    return t;

  if (task_shared_vars
      && DECL_P (t)
      && bitmap_bit_p (task_shared_vars, DECL_UID (t)))
    return t;

  /* If a global variable has been privatized, TREE_CONSTANT on
     ADDR_EXPR might be wrong.  */
  if (data == NULL && TREE_CODE (t) == ADDR_EXPR)
    recompute_tree_invariant_for_addr_expr (t);

  *walk_subtrees = !TYPE_P (t) && !DECL_P (t);
  return NULL_TREE;
}

static void
lower_omp_1 (gimple_stmt_iterator *gsi_p, omp_context *ctx)
{
  gimple stmt = gsi_stmt (*gsi_p);
  struct walk_stmt_info wi;

  if (gimple_has_location (stmt))
    input_location = gimple_location (stmt);

  if (task_shared_vars)
    memset (&wi, '\0', sizeof (wi));

  /* If we have issued syntax errors, avoid doing any heavy lifting.
     Just replace the OpenMP directives with a NOP to avoid
     confusing RTL expansion.  */
  if (errorcount && is_gimple_omp (stmt))
    {
      gsi_replace (gsi_p, gimple_build_nop (), true);
      return;
    }

  switch (gimple_code (stmt))
    {
    case GIMPLE_COND:
      if ((ctx || task_shared_vars)
	  && (walk_tree (gimple_cond_lhs_ptr (stmt), lower_omp_regimplify_p,
	      		 ctx ? NULL : &wi, NULL)
	      || walk_tree (gimple_cond_rhs_ptr (stmt), lower_omp_regimplify_p,
			    ctx ? NULL : &wi, NULL)))
	gimple_regimplify_operands (stmt, gsi_p);
      break;
    case GIMPLE_CATCH:
      lower_omp (gimple_catch_handler (stmt), ctx);
      break;
    case GIMPLE_EH_FILTER:
      lower_omp (gimple_eh_filter_failure (stmt), ctx);
      break;
    case GIMPLE_TRY:
      lower_omp (gimple_try_eval (stmt), ctx);
      lower_omp (gimple_try_cleanup (stmt), ctx);
      break;
    case GIMPLE_BIND:
      lower_omp (gimple_bind_body (stmt), ctx);
      break;
    case GIMPLE_OMP_PARALLEL:
    case GIMPLE_OMP_TASK:
      ctx = maybe_lookup_ctx (stmt);
      lower_omp_taskreg (gsi_p, ctx);
      break;
    case GIMPLE_OMP_FOR:
      ctx = maybe_lookup_ctx (stmt);
      gcc_assert (ctx);
      lower_omp_for (gsi_p, ctx);
      break;
    case GIMPLE_OMP_SECTIONS:
      ctx = maybe_lookup_ctx (stmt);
      gcc_assert (ctx);
      lower_omp_sections (gsi_p, ctx);
      break;
    case GIMPLE_OMP_SINGLE:
      ctx = maybe_lookup_ctx (stmt);
      gcc_assert (ctx);
      lower_omp_single (gsi_p, ctx);
      break;
    case GIMPLE_OMP_MASTER:
      ctx = maybe_lookup_ctx (stmt);
      gcc_assert (ctx);
      lower_omp_master (gsi_p, ctx);
      break;
    case GIMPLE_OMP_ORDERED:
      ctx = maybe_lookup_ctx (stmt);
      gcc_assert (ctx);
      lower_omp_ordered (gsi_p, ctx);
      break;
    case GIMPLE_OMP_CRITICAL:
      ctx = maybe_lookup_ctx (stmt);
      gcc_assert (ctx);
      lower_omp_critical (gsi_p, ctx);
      break;
    case GIMPLE_OMP_ATOMIC_LOAD:
      if ((ctx || task_shared_vars)
	  && walk_tree (gimple_omp_atomic_load_rhs_ptr (stmt),
			lower_omp_regimplify_p, ctx ? NULL : &wi, NULL))
	gimple_regimplify_operands (stmt, gsi_p);
      break;
    default:
      if ((ctx || task_shared_vars)
	  && walk_gimple_op (stmt, lower_omp_regimplify_p,
			     ctx ? NULL : &wi))
	gimple_regimplify_operands (stmt, gsi_p);
      break;
    }
}

static void
lower_omp (gimple_seq body, omp_context *ctx)
{
  location_t saved_location = input_location;
  gimple_stmt_iterator gsi = gsi_start (body);
  for (gsi = gsi_start (body); !gsi_end_p (gsi); gsi_next (&gsi))
    lower_omp_1 (&gsi, ctx);
  input_location = saved_location;
}

/* Main entry point.  */

static unsigned int
execute_lower_omp (void)
{
  gimple_seq body;

  all_contexts = splay_tree_new (splay_tree_compare_pointers, 0,
				 delete_omp_context);

  body = gimple_body (current_function_decl);
  scan_omp (body, NULL);
  gcc_assert (taskreg_nesting_level == 0);

  if (all_contexts->root)
    {
      struct gimplify_ctx gctx;

      if (task_shared_vars)
	push_gimplify_context (&gctx);
      lower_omp (body, NULL);
      if (task_shared_vars)
	pop_gimplify_context (NULL);
    }

  if (all_contexts)
    {
      splay_tree_delete (all_contexts);
      all_contexts = NULL;
    }
  BITMAP_FREE (task_shared_vars);
  return 0;
}

static bool
gate_lower_omp (void)
{
  return flag_openmp != 0;
}

struct gimple_opt_pass pass_lower_omp = 
{
 {
  GIMPLE_PASS,
  "omplower",				/* name */
  gate_lower_omp,			/* gate */
  execute_lower_omp,			/* execute */
  NULL,					/* sub */
  NULL,					/* next */
  0,					/* static_pass_number */
  0,					/* tv_id */
  PROP_gimple_any,			/* properties_required */
  PROP_gimple_lomp,			/* properties_provided */
  0,					/* properties_destroyed */
  0,					/* todo_flags_start */
  TODO_dump_func			/* todo_flags_finish */
 }
};

/* The following is a utility to diagnose OpenMP structured block violations.
   It is not part of the "omplower" pass, as that's invoked too late.  It
   should be invoked by the respective front ends after gimplification.  */

static splay_tree all_labels;

/* Check for mismatched contexts and generate an error if needed.  Return
   true if an error is detected.  */

static bool
diagnose_sb_0 (gimple_stmt_iterator *gsi_p,
    	       gimple branch_ctx, gimple label_ctx)
{
  if (label_ctx == branch_ctx)
    return false;

     
  /*
     Previously we kept track of the label's entire context in diagnose_sb_[12]
     so we could traverse it and issue a correct "exit" or "enter" error
     message upon a structured block violation.

     We built the context by building a list with tree_cons'ing, but there is
     no easy counterpart in gimple tuples.  It seems like far too much work
     for issuing exit/enter error messages.  If someone really misses the
     distinct error message... patches welcome.
   */
     
#if 0
  /* Try to avoid confusing the user by producing and error message
     with correct "exit" or "enter" verbiage.  We prefer "exit"
     unless we can show that LABEL_CTX is nested within BRANCH_CTX.  */
  if (branch_ctx == NULL)
    exit_p = false;
  else
    {
      while (label_ctx)
	{
	  if (TREE_VALUE (label_ctx) == branch_ctx)
	    {
	      exit_p = false;
	      break;
	    }
	  label_ctx = TREE_CHAIN (label_ctx);
	}
    }

  if (exit_p)
    error ("invalid exit from OpenMP structured block");
  else
    error ("invalid entry to OpenMP structured block");
#endif

  /* If it's obvious we have an invalid entry, be specific about the error.  */
  if (branch_ctx == NULL)
    error ("invalid entry to OpenMP structured block");
  else
    /* Otherwise, be vague and lazy, but efficient.  */
    error ("invalid branch to/from an OpenMP structured block");

  gsi_replace (gsi_p, gimple_build_nop (), false);
  return true;
}

/* Pass 1: Create a minimal tree of OpenMP structured blocks, and record
   where each label is found.  */

static tree
diagnose_sb_1 (gimple_stmt_iterator *gsi_p, bool *handled_ops_p,
    	       struct walk_stmt_info *wi)
{
  gimple context = (gimple) wi->info;
  gimple inner_context;
  gimple stmt = gsi_stmt (*gsi_p);

  *handled_ops_p = true;

 switch (gimple_code (stmt))
    {
    WALK_SUBSTMTS;
      
    case GIMPLE_OMP_PARALLEL:
    case GIMPLE_OMP_TASK:
    case GIMPLE_OMP_SECTIONS:
    case GIMPLE_OMP_SINGLE:
    case GIMPLE_OMP_SECTION:
    case GIMPLE_OMP_MASTER:
    case GIMPLE_OMP_ORDERED:
    case GIMPLE_OMP_CRITICAL:
      /* The minimal context here is just the current OMP construct.  */
      inner_context = stmt;
      wi->info = inner_context;
      walk_gimple_seq (gimple_omp_body (stmt), diagnose_sb_1, NULL, wi);
      wi->info = context;
      break;

    case GIMPLE_OMP_FOR:
      inner_context = stmt;
      wi->info = inner_context;
      /* gimple_omp_for_{index,initial,final} are all DECLs; no need to
	 walk them.  */
      walk_gimple_seq (gimple_omp_for_pre_body (stmt),
	  	       diagnose_sb_1, NULL, wi);
      walk_gimple_seq (gimple_omp_body (stmt), diagnose_sb_1, NULL, wi);
      wi->info = context;
      break;

    case GIMPLE_LABEL:
      splay_tree_insert (all_labels, (splay_tree_key) gimple_label_label (stmt),
			 (splay_tree_value) context);
      break;

    default:
      break;
    }

  return NULL_TREE;
}

/* Pass 2: Check each branch and see if its context differs from that of
   the destination label's context.  */

static tree
diagnose_sb_2 (gimple_stmt_iterator *gsi_p, bool *handled_ops_p,
    	       struct walk_stmt_info *wi)
{
  gimple context = (gimple) wi->info;
  splay_tree_node n;
  gimple stmt = gsi_stmt (*gsi_p);

  *handled_ops_p = true;

  switch (gimple_code (stmt))
    {
    WALK_SUBSTMTS;

    case GIMPLE_OMP_PARALLEL:
    case GIMPLE_OMP_TASK:
    case GIMPLE_OMP_SECTIONS:
    case GIMPLE_OMP_SINGLE:
    case GIMPLE_OMP_SECTION:
    case GIMPLE_OMP_MASTER:
    case GIMPLE_OMP_ORDERED:
    case GIMPLE_OMP_CRITICAL:
      wi->info = stmt;
      walk_gimple_seq (gimple_omp_body (stmt), diagnose_sb_2, NULL, wi);
      wi->info = context;
      break;

    case GIMPLE_OMP_FOR:
      wi->info = stmt;
      /* gimple_omp_for_{index,initial,final} are all DECLs; no need to
	 walk them.  */
      walk_gimple_seq (gimple_omp_for_pre_body (stmt),
	  	       diagnose_sb_2, NULL, wi);
      walk_gimple_seq (gimple_omp_body (stmt), diagnose_sb_2, NULL, wi);
      wi->info = context;
      break;

    case GIMPLE_COND:
	{
	  tree lab = gimple_cond_true_label (stmt);
	  if (lab)
	    {
	      n = splay_tree_lookup (all_labels,
				     (splay_tree_key) lab);
	      diagnose_sb_0 (gsi_p, context,
			     n ? (gimple) n->value : NULL);
	    }
	  lab = gimple_cond_false_label (stmt);
	  if (lab)
	    {
	      n = splay_tree_lookup (all_labels,
				     (splay_tree_key) lab);
	      diagnose_sb_0 (gsi_p, context,
			     n ? (gimple) n->value : NULL);
	    }
	}
      break;

    case GIMPLE_GOTO:
      {
	tree lab = gimple_goto_dest (stmt);
	if (TREE_CODE (lab) != LABEL_DECL)
	  break;

	n = splay_tree_lookup (all_labels, (splay_tree_key) lab);
	diagnose_sb_0 (gsi_p, context, n ? (gimple) n->value : NULL);
      }
      break;

    case GIMPLE_SWITCH:
      {
	unsigned int i;
	for (i = 0; i < gimple_switch_num_labels (stmt); ++i)
	  {
	    tree lab = CASE_LABEL (gimple_switch_label (stmt, i));
	    n = splay_tree_lookup (all_labels, (splay_tree_key) lab);
	    if (n && diagnose_sb_0 (gsi_p, context, (gimple) n->value))
	      break;
	  }
      }
      break;

    case GIMPLE_RETURN:
      diagnose_sb_0 (gsi_p, context, NULL);
      break;

    default:
      break;
    }

  return NULL_TREE;
}

void
diagnose_omp_structured_block_errors (tree fndecl)
{
  tree save_current = current_function_decl;
  struct walk_stmt_info wi;
  struct function *old_cfun = cfun;
  gimple_seq body = gimple_body (fndecl);

  current_function_decl = fndecl;
  set_cfun (DECL_STRUCT_FUNCTION (fndecl));

  all_labels = splay_tree_new (splay_tree_compare_pointers, 0, 0);

  memset (&wi, 0, sizeof (wi));
  walk_gimple_seq (body, diagnose_sb_1, NULL, &wi);

  memset (&wi, 0, sizeof (wi));
  wi.want_locations = true;
  walk_gimple_seq (body, diagnose_sb_2, NULL, &wi);

  splay_tree_delete (all_labels);
  all_labels = NULL;

  set_cfun (old_cfun);
  current_function_decl = save_current;
}

#include "gt-omp-low.h"
