/* C++-specific tree lowering bits; see also c-gimplify.c and tree-gimple.c.

   Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.
   Contributed by Jason Merrill <jason@redhat.com>

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
#include "cp-tree.h"
#include "c-common.h"
#include "toplev.h"
#include "tree-iterator.h"
#include "gimple.h"
#include "hashtab.h"
#include "pointer-set.h"
#include "flags.h"

/* Local declarations.  */

enum bc_t { bc_break = 0, bc_continue = 1 };

/* Stack of labels which are targets for "break" or "continue",
   linked through TREE_CHAIN.  */
static tree bc_label[2];

/* Begin a scope which can be exited by a break or continue statement.  BC
   indicates which.

   Just creates a label and pushes it into the current context.  */

static tree
begin_bc_block (enum bc_t bc)
{
  tree label = create_artificial_label ();
  TREE_CHAIN (label) = bc_label[bc];
  bc_label[bc] = label;
  return label;
}

/* Finish a scope which can be exited by a break or continue statement.
   LABEL was returned from the most recent call to begin_bc_block.  BODY is
   an expression for the contents of the scope.

   If we saw a break (or continue) in the scope, append a LABEL_EXPR to
   body.  Otherwise, just forget the label.  */

static gimple_seq
finish_bc_block (enum bc_t bc, tree label, gimple_seq body)
{
  gcc_assert (label == bc_label[bc]);

  if (TREE_USED (label))
    {
      gimple_seq_add_stmt (&body, gimple_build_label (label));
    }

  bc_label[bc] = TREE_CHAIN (label);
  TREE_CHAIN (label) = NULL_TREE;
  return body;
}

/* Get the LABEL_EXPR to represent a break or continue statement
   in the current block scope.  BC indicates which.  */

static tree
get_bc_label (enum bc_t bc)
{
  tree label = bc_label[bc];

  if (label == NULL_TREE)
    {
      if (bc == bc_break)
	error ("break statement not within loop or switch");
      else
	error ("continue statement not within loop or switch");

      return NULL_TREE;
    }

  /* Mark the label used for finish_bc_block.  */
  TREE_USED (label) = 1;
  return label;
}

/* Genericize a TRY_BLOCK.  */

static void
genericize_try_block (tree *stmt_p)
{
  tree body = TRY_STMTS (*stmt_p);
  tree cleanup = TRY_HANDLERS (*stmt_p);

  *stmt_p = build2 (TRY_CATCH_EXPR, void_type_node, body, cleanup);
}

/* Genericize a HANDLER by converting to a CATCH_EXPR.  */

static void
genericize_catch_block (tree *stmt_p)
{
  tree type = HANDLER_TYPE (*stmt_p);
  tree body = HANDLER_BODY (*stmt_p);

  /* FIXME should the caught type go in TREE_TYPE?  */
  *stmt_p = build2 (CATCH_EXPR, void_type_node, type, body);
}

/* A terser interface for building a representation of an exception
   specification.  */

static tree
build_gimple_eh_filter_tree (tree body, tree allowed, tree failure)
{
  tree t;

  /* FIXME should the allowed types go in TREE_TYPE?  */
  t = build2 (EH_FILTER_EXPR, void_type_node, allowed, NULL_TREE);
  append_to_statement_list (failure, &EH_FILTER_FAILURE (t));

  t = build2 (TRY_CATCH_EXPR, void_type_node, NULL_TREE, t);
  append_to_statement_list (body, &TREE_OPERAND (t, 0));

  return t;
}

/* Genericize an EH_SPEC_BLOCK by converting it to a
   TRY_CATCH_EXPR/EH_FILTER_EXPR pair.  */

static void
genericize_eh_spec_block (tree *stmt_p)
{
  tree body = EH_SPEC_STMTS (*stmt_p);
  tree allowed = EH_SPEC_RAISES (*stmt_p);
  tree failure = build_call_n (call_unexpected_node, 1, build_exc_ptr ());

  *stmt_p = build_gimple_eh_filter_tree (body, allowed, failure);
}

/* Genericize an IF_STMT by turning it into a COND_EXPR.  */

static void
genericize_if_stmt (tree *stmt_p)
{
  tree stmt, cond, then_, else_;
  location_t locus = EXPR_LOCATION (*stmt_p);

  stmt = *stmt_p;
  cond = IF_COND (stmt);
  then_ = THEN_CLAUSE (stmt);
  else_ = ELSE_CLAUSE (stmt);

  if (!then_)
    then_ = build_empty_stmt ();
  if (!else_)
    else_ = build_empty_stmt ();

  if (integer_nonzerop (cond) && !TREE_SIDE_EFFECTS (else_))
    stmt = then_;
  else if (integer_zerop (cond) && !TREE_SIDE_EFFECTS (then_))
    stmt = else_;
  else
    stmt = build3 (COND_EXPR, void_type_node, cond, then_, else_);
  if (CAN_HAVE_LOCATION_P (stmt) && !EXPR_HAS_LOCATION (stmt))
    SET_EXPR_LOCATION (stmt, locus);
  *stmt_p = stmt;
}

/* Build a generic representation of one of the C loop forms.  COND is the
   loop condition or NULL_TREE.  BODY is the (possibly compound) statement
   controlled by the loop.  INCR is the increment expression of a for-loop,
   or NULL_TREE.  COND_IS_FIRST indicates whether the condition is
   evaluated before the loop body as in while and for loops, or after the
   loop body as in do-while loops.  */

static gimple_seq
gimplify_cp_loop (tree cond, tree body, tree incr, bool cond_is_first)
{
  gimple top, entry, stmt;
  gimple_seq stmt_list, body_seq, incr_seq, exit_seq;
  tree cont_block, break_block;
  location_t stmt_locus;

  stmt_locus = input_location;
  stmt_list = NULL;
  body_seq = NULL;
  incr_seq = NULL;
  exit_seq = NULL;
  entry = NULL;

  break_block = begin_bc_block (bc_break);
  cont_block = begin_bc_block (bc_continue);

  /* If condition is zero don't generate a loop construct.  */
  if (cond && integer_zerop (cond))
    {
      top = NULL;
      if (cond_is_first)
	{
	  stmt = gimple_build_goto (get_bc_label (bc_break));
	  gimple_set_location (stmt, stmt_locus);
	  gimple_seq_add_stmt (&stmt_list, stmt);
	}
    }
  else
    {
      /* If we use a LOOP_EXPR here, we have to feed the whole thing
	 back through the main gimplifier to lower it.  Given that we
	 have to gimplify the loop body NOW so that we can resolve
	 break/continue stmts, seems easier to just expand to gotos.  */
      top = gimple_build_label (create_artificial_label ());

      /* If we have an exit condition, then we build an IF with gotos either
	 out of the loop, or to the top of it.  If there's no exit condition,
	 then we just build a jump back to the top.  */
      if (cond && !integer_nonzerop (cond))
	{
	  if (cond != error_mark_node)
	    { 
	      gimplify_expr (&cond, &exit_seq, NULL, is_gimple_val, fb_rvalue);
	      stmt = gimple_build_cond (NE_EXPR, cond,
					build_int_cst (TREE_TYPE (cond), 0),
					gimple_label_label (top),
					get_bc_label (bc_break));
	      gimple_seq_add_stmt (&exit_seq, stmt);
	    }

	  if (cond_is_first)
	    {
	      if (incr)
		{
		  entry = gimple_build_label (create_artificial_label ());
		  stmt = gimple_build_goto (gimple_label_label (entry));
		}
	      else
		stmt = gimple_build_goto (get_bc_label (bc_continue));
	      gimple_set_location (stmt, stmt_locus);
	      gimple_seq_add_stmt (&stmt_list, stmt);
	    }
	}
      else
	{
	  stmt = gimple_build_goto (gimple_label_label (top));
	  gimple_seq_add_stmt (&exit_seq, stmt);
	}
    }

  gimplify_stmt (&body, &body_seq);
  gimplify_stmt (&incr, &incr_seq);

  body_seq = finish_bc_block (bc_continue, cont_block, body_seq);

  gimple_seq_add_stmt (&stmt_list, top);
  gimple_seq_add_seq (&stmt_list, body_seq);
  gimple_seq_add_seq (&stmt_list, incr_seq);
  gimple_seq_add_stmt (&stmt_list, entry);
  gimple_seq_add_seq (&stmt_list, exit_seq);

  annotate_all_with_location (stmt_list, stmt_locus);

  return finish_bc_block (bc_break, break_block, stmt_list);
}

/* Gimplify a FOR_STMT node.  Move the stuff in the for-init-stmt into the
   prequeue and hand off to gimplify_cp_loop.  */

static void
gimplify_for_stmt (tree *stmt_p, gimple_seq *pre_p)
{
  tree stmt = *stmt_p;

  if (FOR_INIT_STMT (stmt))
    gimplify_and_add (FOR_INIT_STMT (stmt), pre_p);

  gimple_seq_add_seq (pre_p,
		      gimplify_cp_loop (FOR_COND (stmt), FOR_BODY (stmt),
					FOR_EXPR (stmt), 1));
  *stmt_p = NULL_TREE;
}

/* Gimplify a WHILE_STMT node.  */

static void
gimplify_while_stmt (tree *stmt_p, gimple_seq *pre_p)
{
  tree stmt = *stmt_p;
  gimple_seq_add_seq (pre_p,
		      gimplify_cp_loop (WHILE_COND (stmt), WHILE_BODY (stmt),
					NULL_TREE, 1));
  *stmt_p = NULL_TREE;
}

/* Gimplify a DO_STMT node.  */

static void
gimplify_do_stmt (tree *stmt_p, gimple_seq *pre_p)
{
  tree stmt = *stmt_p;
  gimple_seq_add_seq (pre_p,
		      gimplify_cp_loop (DO_COND (stmt), DO_BODY (stmt),
					NULL_TREE, 0));
  *stmt_p = NULL_TREE;
}

/* Genericize a SWITCH_STMT by turning it into a SWITCH_EXPR.  */

static void
gimplify_switch_stmt (tree *stmt_p, gimple_seq *pre_p)
{
  tree stmt = *stmt_p;
  tree break_block, body, t;
  location_t stmt_locus = input_location;
  gimple_seq seq = NULL;

  break_block = begin_bc_block (bc_break);

  body = SWITCH_STMT_BODY (stmt);
  if (!body)
    body = build_empty_stmt ();

  t = build3 (SWITCH_EXPR, SWITCH_STMT_TYPE (stmt),
	      SWITCH_STMT_COND (stmt), body, NULL_TREE);
  SET_EXPR_LOCATION (t, stmt_locus);
  gimplify_and_add (t, &seq);

  seq = finish_bc_block (bc_break, break_block, seq);
  gimple_seq_add_seq (pre_p, seq);
  *stmt_p = NULL_TREE;
}

/* Hook into the middle of gimplifying an OMP_FOR node.  This is required
   in order to properly gimplify CONTINUE statements.  Here we merely
   manage the continue stack; the rest of the job is performed by the
   regular gimplifier.  */

static enum gimplify_status
cp_gimplify_omp_for (tree *expr_p, gimple_seq *pre_p)
{
  tree for_stmt = *expr_p;
  tree cont_block;
  gimple stmt;
  gimple_seq seq = NULL;

  /* Protect ourselves from recursion.  */
  if (OMP_FOR_GIMPLIFYING_P (for_stmt))
    return GS_UNHANDLED;
  OMP_FOR_GIMPLIFYING_P (for_stmt) = 1;

  /* Note that while technically the continue label is enabled too soon
     here, we should have already diagnosed invalid continues nested within
     statement expressions within the INIT, COND, or INCR expressions.  */
  cont_block = begin_bc_block (bc_continue);

  gimplify_and_add (for_stmt, &seq);
  stmt = gimple_seq_last_stmt (seq);
  if (gimple_code (stmt) == GIMPLE_OMP_FOR)
    gimple_omp_set_body (stmt, finish_bc_block (bc_continue, cont_block,
						gimple_omp_body (stmt)));
  else
    seq = finish_bc_block (bc_continue, cont_block, seq);
  gimple_seq_add_seq (pre_p, seq);

  OMP_FOR_GIMPLIFYING_P (for_stmt) = 0;

  return GS_ALL_DONE;
}

/*  Gimplify an EXPR_STMT node.  */

static void
gimplify_expr_stmt (tree *stmt_p)
{
  tree stmt = EXPR_STMT_EXPR (*stmt_p);

  if (stmt == error_mark_node)
    stmt = NULL;

  /* Gimplification of a statement expression will nullify the
     statement if all its side effects are moved to *PRE_P and *POST_P.

     In this case we will not want to emit the gimplified statement.
     However, we may still want to emit a warning, so we do that before
     gimplification.  */
  if (stmt && warn_unused_value)
    {
      if (!TREE_SIDE_EFFECTS (stmt))
	{
	  if (!IS_EMPTY_STMT (stmt)
	      && !VOID_TYPE_P (TREE_TYPE (stmt))
	      && !TREE_NO_WARNING (stmt))
	    warning (OPT_Wunused_value, "statement with no effect");
	}
      else
	warn_if_unused_value (stmt, input_location);
    }

  if (stmt == NULL_TREE)
    stmt = alloc_stmt_list ();

  *stmt_p = stmt;
}

/* Gimplify initialization from an AGGR_INIT_EXPR.  */

static void
cp_gimplify_init_expr (tree *expr_p, gimple_seq *pre_p, gimple_seq *post_p)
{
  tree from = TREE_OPERAND (*expr_p, 1);
  tree to = TREE_OPERAND (*expr_p, 0);
  tree t;
  tree slot = NULL_TREE;

  /* What about code that pulls out the temp and uses it elsewhere?  I
     think that such code never uses the TARGET_EXPR as an initializer.  If
     I'm wrong, we'll abort because the temp won't have any RTL.  In that
     case, I guess we'll need to replace references somehow.  */
  if (TREE_CODE (from) == TARGET_EXPR)
    {
      slot = TARGET_EXPR_SLOT (from);
      from = TARGET_EXPR_INITIAL (from);
    }

  /* Look through any COMPOUND_EXPRs, since build_compound_expr pushes them
     inside the TARGET_EXPR.  */
  for (t = from; t; )
    {
      tree sub = TREE_CODE (t) == COMPOUND_EXPR ? TREE_OPERAND (t, 0) : t;

      /* If we are initializing from an AGGR_INIT_EXPR, drop the INIT_EXPR and
	 replace the slot operand with our target.

	 Should we add a target parm to gimplify_expr instead?  No, as in this
	 case we want to replace the INIT_EXPR.  */
      if (TREE_CODE (sub) == AGGR_INIT_EXPR)
	{
	  gimplify_expr (&to, pre_p, post_p, is_gimple_lvalue, fb_lvalue);
	  AGGR_INIT_EXPR_SLOT (sub) = to;
	  *expr_p = from;

	  /* The initialization is now a side-effect, so the container can
	     become void.  */
	  if (from != sub)
	    TREE_TYPE (from) = void_type_node;
	}

      if (t == sub)
	break;
      else
	t = TREE_OPERAND (t, 1);
    }

}

/* Gimplify a MUST_NOT_THROW_EXPR.  */

static enum gimplify_status
gimplify_must_not_throw_expr (tree *expr_p, gimple_seq *pre_p)
{
  tree stmt = *expr_p;
  tree temp = voidify_wrapper_expr (stmt, NULL);
  tree body = TREE_OPERAND (stmt, 0);

  stmt = build_gimple_eh_filter_tree (body, NULL_TREE,
				      build_call_n (terminate_node, 0));

  gimplify_and_add (stmt, pre_p);
  if (temp)
    {
      *expr_p = temp;
      return GS_OK;
    }

  *expr_p = NULL;
  return GS_ALL_DONE;
}

/* Do C++-specific gimplification.  Args are as for gimplify_expr.  */

int
cp_gimplify_expr (tree *expr_p, gimple_seq *pre_p, gimple_seq *post_p)
{
  int saved_stmts_are_full_exprs_p = 0;
  enum tree_code code = TREE_CODE (*expr_p);
  enum gimplify_status ret;
  tree block = NULL;
  VEC(gimple, heap) *bind_expr_stack = NULL;

  if (STATEMENT_CODE_P (code))
    {
      saved_stmts_are_full_exprs_p = stmts_are_full_exprs_p ();
      current_stmt_tree ()->stmts_are_full_exprs_p
	= STMT_IS_FULL_EXPR_P (*expr_p);
    }

  switch (code)
    {
    case PTRMEM_CST:
      *expr_p = cplus_expand_constant (*expr_p);
      ret = GS_OK;
      break;

    case AGGR_INIT_EXPR:
      simplify_aggr_init_expr (expr_p);
      ret = GS_OK;
      break;

    case THROW_EXPR:
      /* FIXME communicate throw type to back end, probably by moving
	 THROW_EXPR into ../tree.def.  */
      *expr_p = TREE_OPERAND (*expr_p, 0);
      ret = GS_OK;
      break;

    case MUST_NOT_THROW_EXPR:
      ret = gimplify_must_not_throw_expr (expr_p, pre_p);
      break;

      /* We used to do this for MODIFY_EXPR as well, but that's unsafe; the
	 LHS of an assignment might also be involved in the RHS, as in bug
	 25979.  */
    case INIT_EXPR:
      cp_gimplify_init_expr (expr_p, pre_p, post_p);
      ret = GS_OK;
      break;

    case EMPTY_CLASS_EXPR:
      /* We create an empty CONSTRUCTOR with RECORD_TYPE.  */
      *expr_p = build_constructor (TREE_TYPE (*expr_p), NULL);
      ret = GS_OK;
      break;

    case BASELINK:
      *expr_p = BASELINK_FUNCTIONS (*expr_p);
      ret = GS_OK;
      break;

    case TRY_BLOCK:
      genericize_try_block (expr_p);
      ret = GS_OK;
      break;

    case HANDLER:
      genericize_catch_block (expr_p);
      ret = GS_OK;
      break;

    case EH_SPEC_BLOCK:
      genericize_eh_spec_block (expr_p);
      ret = GS_OK;
      break;

    case USING_STMT:
      /* Get the innermost inclosing GIMPLE_BIND that has a non NULL
         BLOCK, and append an IMPORTED_DECL to its
	 BLOCK_VARS chained list.  */

      bind_expr_stack = gimple_bind_expr_stack ();
      if (bind_expr_stack)
	{
	  int i;
	  for (i = VEC_length (gimple, bind_expr_stack) - 1; i >= 0; i--)
	    if ((block = gimple_bind_block (VEC_index (gimple,
						       bind_expr_stack,
						       i))))
	      break;
	}
      if (block)
	{
	  tree using_directive;
	  gcc_assert (TREE_OPERAND (*expr_p, 0));

	  using_directive = make_node (IMPORTED_DECL);
	  TREE_TYPE (using_directive) = void_type_node;

	  IMPORTED_DECL_ASSOCIATED_DECL (using_directive)
	    = TREE_OPERAND (*expr_p, 0);
	  TREE_CHAIN (using_directive) = BLOCK_VARS (block);
	  BLOCK_VARS (block) = using_directive;
	}
      /* The USING_STMT won't appear in GIMPLE.  */
      *expr_p = NULL;
      ret = GS_ALL_DONE;
      break;

    case FOR_STMT:
      gimplify_for_stmt (expr_p, pre_p);
      ret = GS_OK;
      break;

    case WHILE_STMT:
      gimplify_while_stmt (expr_p, pre_p);
      ret = GS_OK;
      break;

    case DO_STMT:
      gimplify_do_stmt (expr_p, pre_p);
      ret = GS_OK;
      break;

    case SWITCH_STMT:
      gimplify_switch_stmt (expr_p, pre_p);
      ret = GS_OK;
      break;

    case OMP_FOR:
      ret = cp_gimplify_omp_for (expr_p, pre_p);
      break;

    case CONTINUE_STMT:
      gimple_seq_add_stmt (pre_p, gimple_build_predict (PRED_CONTINUE, NOT_TAKEN));
      gimple_seq_add_stmt (pre_p, gimple_build_goto (get_bc_label (bc_continue)));
      *expr_p = NULL_TREE;
      ret = GS_ALL_DONE;
      break;

    case BREAK_STMT:
      gimple_seq_add_stmt (pre_p, gimple_build_goto (get_bc_label (bc_break)));
      *expr_p = NULL_TREE;
      ret = GS_ALL_DONE;
      break;

    case EXPR_STMT:
      gimplify_expr_stmt (expr_p);
      ret = GS_OK;
      break;

    case UNARY_PLUS_EXPR:
      {
	tree arg = TREE_OPERAND (*expr_p, 0);
	tree type = TREE_TYPE (*expr_p);
	*expr_p = (TREE_TYPE (arg) != type) ? fold_convert (type, arg)
					    : arg;
	ret = GS_OK;
      }
      break;

    default:
      ret = c_gimplify_expr (expr_p, pre_p, post_p);
      break;
    }

  /* Restore saved state.  */
  if (STATEMENT_CODE_P (code))
    current_stmt_tree ()->stmts_are_full_exprs_p
      = saved_stmts_are_full_exprs_p;

  return ret;
}

static inline bool
is_invisiref_parm (const_tree t)
{
  return ((TREE_CODE (t) == PARM_DECL || TREE_CODE (t) == RESULT_DECL)
	  && DECL_BY_REFERENCE (t));
}

/* Return true if the uid in both int tree maps are equal.  */

int
cxx_int_tree_map_eq (const void *va, const void *vb)
{
  const struct cxx_int_tree_map *a = (const struct cxx_int_tree_map *) va;
  const struct cxx_int_tree_map *b = (const struct cxx_int_tree_map *) vb;
  return (a->uid == b->uid);
}

/* Hash a UID in a cxx_int_tree_map.  */

unsigned int
cxx_int_tree_map_hash (const void *item)
{
  return ((const struct cxx_int_tree_map *)item)->uid;
}

/* Perform any pre-gimplification lowering of C++ front end trees to
   GENERIC.  */

static tree
cp_genericize_r (tree *stmt_p, int *walk_subtrees, void *data)
{
  tree stmt = *stmt_p;
  struct pointer_set_t *p_set = (struct pointer_set_t*) data;

  if (is_invisiref_parm (stmt)
      /* Don't dereference parms in a thunk, pass the references through. */
      && !(DECL_THUNK_P (current_function_decl)
	   && TREE_CODE (stmt) == PARM_DECL))
    {
      *stmt_p = convert_from_reference (stmt);
      *walk_subtrees = 0;
      return NULL;
    }

  /* Map block scope extern declarations to visible declarations with the
     same name and type in outer scopes if any.  */
  if (cp_function_chain->extern_decl_map
      && (TREE_CODE (stmt) == FUNCTION_DECL || TREE_CODE (stmt) == VAR_DECL)
      && DECL_EXTERNAL (stmt))
    {
      struct cxx_int_tree_map *h, in;
      in.uid = DECL_UID (stmt);
      h = (struct cxx_int_tree_map *)
	  htab_find_with_hash (cp_function_chain->extern_decl_map,
			       &in, in.uid);
      if (h)
	{
	  *stmt_p = h->to;
	  *walk_subtrees = 0;
	  return NULL;
	}
    }

  /* Other than invisiref parms, don't walk the same tree twice.  */
  if (pointer_set_contains (p_set, stmt))
    {
      *walk_subtrees = 0;
      return NULL_TREE;
    }

  if (TREE_CODE (stmt) == ADDR_EXPR
      && is_invisiref_parm (TREE_OPERAND (stmt, 0)))
    {
      *stmt_p = convert (TREE_TYPE (stmt), TREE_OPERAND (stmt, 0));
      *walk_subtrees = 0;
    }
  else if (TREE_CODE (stmt) == RETURN_EXPR
	   && TREE_OPERAND (stmt, 0)
	   && is_invisiref_parm (TREE_OPERAND (stmt, 0)))
    /* Don't dereference an invisiref RESULT_DECL inside a RETURN_EXPR.  */
    *walk_subtrees = 0;
  else if (TREE_CODE (stmt) == OMP_CLAUSE)
    switch (OMP_CLAUSE_CODE (stmt))
      {
      case OMP_CLAUSE_LASTPRIVATE:
	/* Don't dereference an invisiref in OpenMP clauses.  */
	if (is_invisiref_parm (OMP_CLAUSE_DECL (stmt)))
	  {
	    *walk_subtrees = 0;
	    if (OMP_CLAUSE_LASTPRIVATE_STMT (stmt))
	      cp_walk_tree (&OMP_CLAUSE_LASTPRIVATE_STMT (stmt),
			    cp_genericize_r, p_set, NULL);
	  }
	break;
      case OMP_CLAUSE_PRIVATE:
      case OMP_CLAUSE_SHARED:
      case OMP_CLAUSE_FIRSTPRIVATE:
      case OMP_CLAUSE_COPYIN:
      case OMP_CLAUSE_COPYPRIVATE:
	/* Don't dereference an invisiref in OpenMP clauses.  */
	if (is_invisiref_parm (OMP_CLAUSE_DECL (stmt)))
	  *walk_subtrees = 0;
	break;
      case OMP_CLAUSE_REDUCTION:
	gcc_assert (!is_invisiref_parm (OMP_CLAUSE_DECL (stmt)));
	break;
      default:
	break;
      }
  else if (IS_TYPE_OR_DECL_P (stmt))
    *walk_subtrees = 0;

  /* Due to the way voidify_wrapper_expr is written, we don't get a chance
     to lower this construct before scanning it, so we need to lower these
     before doing anything else.  */
  else if (TREE_CODE (stmt) == CLEANUP_STMT)
    *stmt_p = build2 (CLEANUP_EH_ONLY (stmt) ? TRY_CATCH_EXPR
					     : TRY_FINALLY_EXPR,
		      void_type_node,
		      CLEANUP_BODY (stmt),
		      CLEANUP_EXPR (stmt));

  else if (TREE_CODE (stmt) == IF_STMT)
    {
      genericize_if_stmt (stmt_p);
      /* *stmt_p has changed, tail recurse to handle it again.  */
      return cp_genericize_r (stmt_p, walk_subtrees, data);
    }

  /* COND_EXPR might have incompatible types in branches if one or both
     arms are bitfields.  Fix it up now.  */
  else if (TREE_CODE (stmt) == COND_EXPR)
    {
      tree type_left
	= (TREE_OPERAND (stmt, 1)
	   ? is_bitfield_expr_with_lowered_type (TREE_OPERAND (stmt, 1))
	   : NULL_TREE);
      tree type_right
	= (TREE_OPERAND (stmt, 2)
	   ? is_bitfield_expr_with_lowered_type (TREE_OPERAND (stmt, 2))
	   : NULL_TREE);
      if (type_left
	  && !useless_type_conversion_p (TREE_TYPE (stmt),
					 TREE_TYPE (TREE_OPERAND (stmt, 1))))
	{
	  TREE_OPERAND (stmt, 1)
	    = fold_convert (type_left, TREE_OPERAND (stmt, 1));
	  gcc_assert (useless_type_conversion_p (TREE_TYPE (stmt),
						 type_left));
	}
      if (type_right
	  && !useless_type_conversion_p (TREE_TYPE (stmt),
					 TREE_TYPE (TREE_OPERAND (stmt, 2))))
	{
	  TREE_OPERAND (stmt, 2)
	    = fold_convert (type_right, TREE_OPERAND (stmt, 2));
	  gcc_assert (useless_type_conversion_p (TREE_TYPE (stmt),
						 type_right));
	}
    }

  pointer_set_insert (p_set, *stmt_p);

  return NULL;
}

void
cp_genericize (tree fndecl)
{
  tree t;
  struct pointer_set_t *p_set;

  /* Fix up the types of parms passed by invisible reference.  */
  for (t = DECL_ARGUMENTS (fndecl); t; t = TREE_CHAIN (t))
    if (TREE_ADDRESSABLE (TREE_TYPE (t)))
      {
	/* If a function's arguments are copied to create a thunk,
	   then DECL_BY_REFERENCE will be set -- but the type of the
	   argument will be a pointer type, so we will never get
	   here.  */
	gcc_assert (!DECL_BY_REFERENCE (t));
	gcc_assert (DECL_ARG_TYPE (t) != TREE_TYPE (t));
	TREE_TYPE (t) = DECL_ARG_TYPE (t);
	DECL_BY_REFERENCE (t) = 1;
	TREE_ADDRESSABLE (t) = 0;
	relayout_decl (t);
      }

  /* Do the same for the return value.  */
  if (TREE_ADDRESSABLE (TREE_TYPE (DECL_RESULT (fndecl))))
    {
      t = DECL_RESULT (fndecl);
      TREE_TYPE (t) = build_reference_type (TREE_TYPE (t));
      DECL_BY_REFERENCE (t) = 1;
      TREE_ADDRESSABLE (t) = 0;
      relayout_decl (t);
    }

  /* If we're a clone, the body is already GIMPLE.  */
  if (DECL_CLONED_FUNCTION_P (fndecl))
    return;

  /* We do want to see every occurrence of the parms, so we can't just use
     walk_tree's hash functionality.  */
  p_set = pointer_set_create ();
  cp_walk_tree (&DECL_SAVED_TREE (fndecl), cp_genericize_r, p_set, NULL);
  pointer_set_destroy (p_set);

  /* Do everything else.  */
  c_genericize (fndecl);

  gcc_assert (bc_label[bc_break] == NULL);
  gcc_assert (bc_label[bc_continue] == NULL);
}

/* Build code to apply FN to each member of ARG1 and ARG2.  FN may be
   NULL if there is in fact nothing to do.  ARG2 may be null if FN
   actually only takes one argument.  */

static tree
cxx_omp_clause_apply_fn (tree fn, tree arg1, tree arg2)
{
  tree defparm, parm, t;
  int i = 0;
  int nargs;
  tree *argarray;

  if (fn == NULL)
    return NULL;

  nargs = list_length (DECL_ARGUMENTS (fn));
  argarray = (tree *) alloca (nargs * sizeof (tree));

  defparm = TREE_CHAIN (TYPE_ARG_TYPES (TREE_TYPE (fn)));
  if (arg2)
    defparm = TREE_CHAIN (defparm);

  if (TREE_CODE (TREE_TYPE (arg1)) == ARRAY_TYPE)
    {
      tree inner_type = TREE_TYPE (arg1);
      tree start1, end1, p1;
      tree start2 = NULL, p2 = NULL;
      tree ret = NULL, lab;

      start1 = arg1;
      start2 = arg2;
      do
	{
	  inner_type = TREE_TYPE (inner_type);
	  start1 = build4 (ARRAY_REF, inner_type, start1,
			   size_zero_node, NULL, NULL);
	  if (arg2)
	    start2 = build4 (ARRAY_REF, inner_type, start2,
			     size_zero_node, NULL, NULL);
	}
      while (TREE_CODE (inner_type) == ARRAY_TYPE);
      start1 = build_fold_addr_expr (start1);
      if (arg2)
	start2 = build_fold_addr_expr (start2);

      end1 = TYPE_SIZE_UNIT (TREE_TYPE (arg1));
      end1 = build2 (POINTER_PLUS_EXPR, TREE_TYPE (start1), start1, end1);

      p1 = create_tmp_var (TREE_TYPE (start1), NULL);
      t = build2 (MODIFY_EXPR, TREE_TYPE (p1), p1, start1);
      append_to_statement_list (t, &ret);

      if (arg2)
	{
	  p2 = create_tmp_var (TREE_TYPE (start2), NULL);
	  t = build2 (MODIFY_EXPR, TREE_TYPE (p2), p2, start2);
	  append_to_statement_list (t, &ret);
	}

      lab = create_artificial_label ();
      t = build1 (LABEL_EXPR, void_type_node, lab);
      append_to_statement_list (t, &ret);

      argarray[i++] = p1;
      if (arg2)
	argarray[i++] = p2;
      /* Handle default arguments.  */
      for (parm = defparm; parm && parm != void_list_node;
	   parm = TREE_CHAIN (parm), i++)
	argarray[i] = convert_default_arg (TREE_VALUE (parm),
					   TREE_PURPOSE (parm), fn, i);
      t = build_call_a (fn, i, argarray);
      t = fold_convert (void_type_node, t);
      t = fold_build_cleanup_point_expr (TREE_TYPE (t), t);
      append_to_statement_list (t, &ret);

      t = TYPE_SIZE_UNIT (inner_type);
      t = build2 (POINTER_PLUS_EXPR, TREE_TYPE (p1), p1, t);
      t = build2 (MODIFY_EXPR, TREE_TYPE (p1), p1, t);
      append_to_statement_list (t, &ret);

      if (arg2)
	{
	  t = TYPE_SIZE_UNIT (inner_type);
	  t = build2 (POINTER_PLUS_EXPR, TREE_TYPE (p2), p2, t);
	  t = build2 (MODIFY_EXPR, TREE_TYPE (p2), p2, t);
	  append_to_statement_list (t, &ret);
	}

      t = build2 (NE_EXPR, boolean_type_node, p1, end1);
      t = build3 (COND_EXPR, void_type_node, t, build_and_jump (&lab), NULL);
      append_to_statement_list (t, &ret);

      return ret;
    }
  else
    {
      argarray[i++] = build_fold_addr_expr (arg1);
      if (arg2)
	argarray[i++] = build_fold_addr_expr (arg2);
      /* Handle default arguments.  */
      for (parm = defparm; parm && parm != void_list_node;
	   parm = TREE_CHAIN (parm), i++)
	argarray[i] = convert_default_arg (TREE_VALUE (parm),
					   TREE_PURPOSE (parm),
					   fn, i);
      t = build_call_a (fn, i, argarray);
      t = fold_convert (void_type_node, t);
      return fold_build_cleanup_point_expr (TREE_TYPE (t), t);
    }
}

/* Return code to initialize DECL with its default constructor, or
   NULL if there's nothing to do.  */

tree
cxx_omp_clause_default_ctor (tree clause, tree decl,
			     tree outer ATTRIBUTE_UNUSED)
{
  tree info = CP_OMP_CLAUSE_INFO (clause);
  tree ret = NULL;

  if (info)
    ret = cxx_omp_clause_apply_fn (TREE_VEC_ELT (info, 0), decl, NULL);

  return ret;
}

/* Return code to initialize DST with a copy constructor from SRC.  */

tree
cxx_omp_clause_copy_ctor (tree clause, tree dst, tree src)
{
  tree info = CP_OMP_CLAUSE_INFO (clause);
  tree ret = NULL;

  if (info)
    ret = cxx_omp_clause_apply_fn (TREE_VEC_ELT (info, 0), dst, src);
  if (ret == NULL)
    ret = build2 (MODIFY_EXPR, TREE_TYPE (dst), dst, src);

  return ret;
}

/* Similarly, except use an assignment operator instead.  */

tree
cxx_omp_clause_assign_op (tree clause, tree dst, tree src)
{
  tree info = CP_OMP_CLAUSE_INFO (clause);
  tree ret = NULL;

  if (info)
    ret = cxx_omp_clause_apply_fn (TREE_VEC_ELT (info, 2), dst, src);
  if (ret == NULL)
    ret = build2 (MODIFY_EXPR, TREE_TYPE (dst), dst, src);

  return ret;
}

/* Return code to destroy DECL.  */

tree
cxx_omp_clause_dtor (tree clause, tree decl)
{
  tree info = CP_OMP_CLAUSE_INFO (clause);
  tree ret = NULL;

  if (info)
    ret = cxx_omp_clause_apply_fn (TREE_VEC_ELT (info, 1), decl, NULL);

  return ret;
}

/* True if OpenMP should privatize what this DECL points to rather
   than the DECL itself.  */

bool
cxx_omp_privatize_by_reference (const_tree decl)
{
  return is_invisiref_parm (decl);
}

/* True if OpenMP sharing attribute of DECL is predetermined.  */

enum omp_clause_default_kind
cxx_omp_predetermined_sharing (tree decl)
{
  tree type;

  /* Static data members are predetermined as shared.  */
  if (TREE_STATIC (decl))
    {
      tree ctx = CP_DECL_CONTEXT (decl);
      if (TYPE_P (ctx) && MAYBE_CLASS_TYPE_P (ctx))
	return OMP_CLAUSE_DEFAULT_SHARED;
    }

  type = TREE_TYPE (decl);
  if (TREE_CODE (type) == REFERENCE_TYPE)
    {
      if (!is_invisiref_parm (decl))
	return OMP_CLAUSE_DEFAULT_UNSPECIFIED;
      type = TREE_TYPE (type);

      if (TREE_CODE (decl) == RESULT_DECL && DECL_NAME (decl))
	{
	  /* NVR doesn't preserve const qualification of the
	     variable's type.  */
	  tree outer = outer_curly_brace_block (current_function_decl);
	  tree var;

	  if (outer)
	    for (var = BLOCK_VARS (outer); var; var = TREE_CHAIN (var))
	      if (DECL_NAME (decl) == DECL_NAME (var)
		  && (TYPE_MAIN_VARIANT (type)
		      == TYPE_MAIN_VARIANT (TREE_TYPE (var))))
		{
		  if (TYPE_READONLY (TREE_TYPE (var)))
		    type = TREE_TYPE (var);
		  break;
		}
	}
    }

  if (type == error_mark_node)
    return OMP_CLAUSE_DEFAULT_UNSPECIFIED;

  /* Variables with const-qualified type having no mutable member
     are predetermined shared.  */
  if (TYPE_READONLY (type) && !cp_has_mutable_p (type))
    return OMP_CLAUSE_DEFAULT_SHARED;

  return OMP_CLAUSE_DEFAULT_UNSPECIFIED;
}

/* Finalize an implicitly determined clause.  */

void
cxx_omp_finish_clause (tree c)
{
  tree decl, inner_type;
  bool make_shared = false;

  if (OMP_CLAUSE_CODE (c) != OMP_CLAUSE_FIRSTPRIVATE)
    return;

  decl = OMP_CLAUSE_DECL (c);
  decl = require_complete_type (decl);
  inner_type = TREE_TYPE (decl);
  if (decl == error_mark_node)
    make_shared = true;
  else if (TREE_CODE (TREE_TYPE (decl)) == REFERENCE_TYPE)
    {
      if (is_invisiref_parm (decl))
	inner_type = TREE_TYPE (inner_type);
      else
	{
	  error ("%qE implicitly determined as %<firstprivate%> has reference type",
		 decl);
	  make_shared = true;
	}
    }

  /* We're interested in the base element, not arrays.  */
  while (TREE_CODE (inner_type) == ARRAY_TYPE)
    inner_type = TREE_TYPE (inner_type);

  /* Check for special function availability by building a call to one.
     Save the results, because later we won't be in the right context
     for making these queries.  */
  if (!make_shared
      && CLASS_TYPE_P (inner_type)
      && cxx_omp_create_clause_info (c, inner_type, false, true, false))
    make_shared = true;

  if (make_shared)
    OMP_CLAUSE_CODE (c) = OMP_CLAUSE_SHARED;
}
