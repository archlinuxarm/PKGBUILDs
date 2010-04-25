/* Perform the semantic phase of parsing, i.e., the process of
   building tree structure, checking semantic consistency, and
   building RTL.  These routines are used both during actual parsing
   and during the instantiation of template functions.

   Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
		 2008, 2009 Free Software Foundation, Inc.
   Written by Mark Mitchell (mmitchell@usa.net) based on code found
   formerly in parse.y and pt.c.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

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
#include "tree-inline.h"
#include "tree-mudflap.h"
#include "except.h"
#include "toplev.h"
#include "flags.h"
#include "rtl.h"
#include "expr.h"
#include "output.h"
#include "timevar.h"
#include "debug.h"
#include "diagnostic.h"
#include "cgraph.h"
#include "tree-iterator.h"
#include "vec.h"
#include "target.h"
#include "gimple.h"

/* There routines provide a modular interface to perform many parsing
   operations.  They may therefore be used during actual parsing, or
   during template instantiation, which may be regarded as a
   degenerate form of parsing.  */

static tree maybe_convert_cond (tree);
static tree finalize_nrv_r (tree *, int *, void *);


/* Deferred Access Checking Overview
   ---------------------------------

   Most C++ expressions and declarations require access checking
   to be performed during parsing.  However, in several cases,
   this has to be treated differently.

   For member declarations, access checking has to be deferred
   until more information about the declaration is known.  For
   example:

     class A {
	 typedef int X;
       public:
	 X f();
     };

     A::X A::f();
     A::X g();

   When we are parsing the function return type `A::X', we don't
   really know if this is allowed until we parse the function name.

   Furthermore, some contexts require that access checking is
   never performed at all.  These include class heads, and template
   instantiations.

   Typical use of access checking functions is described here:

   1. When we enter a context that requires certain access checking
      mode, the function `push_deferring_access_checks' is called with
      DEFERRING argument specifying the desired mode.  Access checking
      may be performed immediately (dk_no_deferred), deferred
      (dk_deferred), or not performed (dk_no_check).

   2. When a declaration such as a type, or a variable, is encountered,
      the function `perform_or_defer_access_check' is called.  It
      maintains a VEC of all deferred checks.

   3. The global `current_class_type' or `current_function_decl' is then
      setup by the parser.  `enforce_access' relies on these information
      to check access.

   4. Upon exiting the context mentioned in step 1,
      `perform_deferred_access_checks' is called to check all declaration
      stored in the VEC. `pop_deferring_access_checks' is then
      called to restore the previous access checking mode.

      In case of parsing error, we simply call `pop_deferring_access_checks'
      without `perform_deferred_access_checks'.  */

typedef struct deferred_access GTY(())
{
  /* A VEC representing name-lookups for which we have deferred
     checking access controls.  We cannot check the accessibility of
     names used in a decl-specifier-seq until we know what is being
     declared because code like:

       class A {
	 class B {};
	 B* f();
       }

       A::B* A::f() { return 0; }

     is valid, even though `A::B' is not generally accessible.  */
  VEC (deferred_access_check,gc)* GTY(()) deferred_access_checks;

  /* The current mode of access checks.  */
  enum deferring_kind deferring_access_checks_kind;

} deferred_access;
DEF_VEC_O (deferred_access);
DEF_VEC_ALLOC_O (deferred_access,gc);

/* Data for deferred access checking.  */
static GTY(()) VEC(deferred_access,gc) *deferred_access_stack;
static GTY(()) unsigned deferred_access_no_check;

/* Save the current deferred access states and start deferred
   access checking iff DEFER_P is true.  */

void
push_deferring_access_checks (deferring_kind deferring)
{
  /* For context like template instantiation, access checking
     disabling applies to all nested context.  */
  if (deferred_access_no_check || deferring == dk_no_check)
    deferred_access_no_check++;
  else
    {
      deferred_access *ptr;

      ptr = VEC_safe_push (deferred_access, gc, deferred_access_stack, NULL);
      ptr->deferred_access_checks = NULL;
      ptr->deferring_access_checks_kind = deferring;
    }
}

/* Resume deferring access checks again after we stopped doing
   this previously.  */

void
resume_deferring_access_checks (void)
{
  if (!deferred_access_no_check)
    VEC_last (deferred_access, deferred_access_stack)
      ->deferring_access_checks_kind = dk_deferred;
}

/* Stop deferring access checks.  */

void
stop_deferring_access_checks (void)
{
  if (!deferred_access_no_check)
    VEC_last (deferred_access, deferred_access_stack)
      ->deferring_access_checks_kind = dk_no_deferred;
}

/* Discard the current deferred access checks and restore the
   previous states.  */

void
pop_deferring_access_checks (void)
{
  if (deferred_access_no_check)
    deferred_access_no_check--;
  else
    VEC_pop (deferred_access, deferred_access_stack);
}

/* Returns a TREE_LIST representing the deferred checks.
   The TREE_PURPOSE of each node is the type through which the
   access occurred; the TREE_VALUE is the declaration named.
   */

VEC (deferred_access_check,gc)*
get_deferred_access_checks (void)
{
  if (deferred_access_no_check)
    return NULL;
  else
    return (VEC_last (deferred_access, deferred_access_stack)
	    ->deferred_access_checks);
}

/* Take current deferred checks and combine with the
   previous states if we also defer checks previously.
   Otherwise perform checks now.  */

void
pop_to_parent_deferring_access_checks (void)
{
  if (deferred_access_no_check)
    deferred_access_no_check--;
  else
    {
      VEC (deferred_access_check,gc) *checks;
      deferred_access *ptr;

      checks = (VEC_last (deferred_access, deferred_access_stack)
		->deferred_access_checks);

      VEC_pop (deferred_access, deferred_access_stack);
      ptr = VEC_last (deferred_access, deferred_access_stack);
      if (ptr->deferring_access_checks_kind == dk_no_deferred)
	{
	  /* Check access.  */
	  perform_access_checks (checks);
	}
      else
	{
	  /* Merge with parent.  */
	  int i, j;
	  deferred_access_check *chk, *probe;

	  for (i = 0 ;
	       VEC_iterate (deferred_access_check, checks, i, chk) ;
	       ++i)
	    {
	      for (j = 0 ;
		   VEC_iterate (deferred_access_check,
				ptr->deferred_access_checks, j, probe) ;
		   ++j)
		{
		  if (probe->binfo == chk->binfo &&
		      probe->decl == chk->decl &&
		      probe->diag_decl == chk->diag_decl)
		    goto found;
		}
	      /* Insert into parent's checks.  */
	      VEC_safe_push (deferred_access_check, gc,
			     ptr->deferred_access_checks, chk);
	    found:;
	    }
	}
    }
}

/* Perform the access checks in CHECKS.  The TREE_PURPOSE of each node
   is the BINFO indicating the qualifying scope used to access the
   DECL node stored in the TREE_VALUE of the node.  */

void
perform_access_checks (VEC (deferred_access_check,gc)* checks)
{
  int i;
  deferred_access_check *chk;

  if (!checks)
    return;

  for (i = 0 ; VEC_iterate (deferred_access_check, checks, i, chk) ; ++i)
    enforce_access (chk->binfo, chk->decl, chk->diag_decl);
}

/* Perform the deferred access checks.

   After performing the checks, we still have to keep the list
   `deferred_access_stack->deferred_access_checks' since we may want
   to check access for them again later in a different context.
   For example:

     class A {
       typedef int X;
       static X a;
     };
     A::X A::a, x;	// No error for `A::a', error for `x'

   We have to perform deferred access of `A::X', first with `A::a',
   next with `x'.  */

void
perform_deferred_access_checks (void)
{
  perform_access_checks (get_deferred_access_checks ());
}

/* Defer checking the accessibility of DECL, when looked up in
   BINFO. DIAG_DECL is the declaration to use to print diagnostics.  */

void
perform_or_defer_access_check (tree binfo, tree decl, tree diag_decl)
{
  int i;
  deferred_access *ptr;
  deferred_access_check *chk;
  deferred_access_check *new_access;


  /* Exit if we are in a context that no access checking is performed.
     */
  if (deferred_access_no_check)
    return;

  gcc_assert (TREE_CODE (binfo) == TREE_BINFO);

  ptr = VEC_last (deferred_access, deferred_access_stack);

  /* If we are not supposed to defer access checks, just check now.  */
  if (ptr->deferring_access_checks_kind == dk_no_deferred)
    {
      enforce_access (binfo, decl, diag_decl);
      return;
    }

  /* See if we are already going to perform this check.  */
  for (i = 0 ;
       VEC_iterate (deferred_access_check,
		    ptr->deferred_access_checks, i, chk) ;
       ++i)
    {
      if (chk->decl == decl && chk->binfo == binfo &&
	  chk->diag_decl == diag_decl)
	{
	  return;
	}
    }
  /* If not, record the check.  */
  new_access =
    VEC_safe_push (deferred_access_check, gc,
		   ptr->deferred_access_checks, 0);
  new_access->binfo = binfo;
  new_access->decl = decl;
  new_access->diag_decl = diag_decl;
}

/* Returns nonzero if the current statement is a full expression,
   i.e. temporaries created during that statement should be destroyed
   at the end of the statement.  */

int
stmts_are_full_exprs_p (void)
{
  return current_stmt_tree ()->stmts_are_full_exprs_p;
}

/* T is a statement.  Add it to the statement-tree.  This is the C++
   version.  The C/ObjC frontends have a slightly different version of
   this function.  */

tree
add_stmt (tree t)
{
  enum tree_code code = TREE_CODE (t);

  if (EXPR_P (t) && code != LABEL_EXPR)
    {
      if (!EXPR_HAS_LOCATION (t))
	SET_EXPR_LOCATION (t, input_location);

      /* When we expand a statement-tree, we must know whether or not the
	 statements are full-expressions.  We record that fact here.  */
      STMT_IS_FULL_EXPR_P (t) = stmts_are_full_exprs_p ();
    }

  /* Add T to the statement-tree.  Non-side-effect statements need to be
     recorded during statement expressions.  */
  append_to_statement_list_force (t, &cur_stmt_list);

  return t;
}

/* Returns the stmt_tree to which statements are currently being added.  */

stmt_tree
current_stmt_tree (void)
{
  return (cfun
	  ? &cfun->language->base.x_stmt_tree
	  : &scope_chain->x_stmt_tree);
}

/* If statements are full expressions, wrap STMT in a CLEANUP_POINT_EXPR.  */

static tree
maybe_cleanup_point_expr (tree expr)
{
  if (!processing_template_decl && stmts_are_full_exprs_p ())
    expr = fold_build_cleanup_point_expr (TREE_TYPE (expr), expr);
  return expr;
}

/* Like maybe_cleanup_point_expr except have the type of the new expression be
   void so we don't need to create a temporary variable to hold the inner
   expression.  The reason why we do this is because the original type might be
   an aggregate and we cannot create a temporary variable for that type.  */

static tree
maybe_cleanup_point_expr_void (tree expr)
{
  if (!processing_template_decl && stmts_are_full_exprs_p ())
    expr = fold_build_cleanup_point_expr (void_type_node, expr);
  return expr;
}



/* Create a declaration statement for the declaration given by the DECL.  */

void
add_decl_expr (tree decl)
{
  tree r = build_stmt (DECL_EXPR, decl);
  if (DECL_INITIAL (decl)
      || (DECL_SIZE (decl) && TREE_SIDE_EFFECTS (DECL_SIZE (decl))))
    r = maybe_cleanup_point_expr_void (r);
  add_stmt (r);
}

/* Finish a scope.  */

tree
do_poplevel (tree stmt_list)
{
  tree block = NULL;

  if (stmts_are_full_exprs_p ())
    block = poplevel (kept_level_p (), 1, 0);

  stmt_list = pop_stmt_list (stmt_list);

  if (!processing_template_decl)
    {
      stmt_list = c_build_bind_expr (block, stmt_list);
      /* ??? See c_end_compound_stmt re statement expressions.  */
    }

  return stmt_list;
}

/* Begin a new scope.  */

static tree
do_pushlevel (scope_kind sk)
{
  tree ret = push_stmt_list ();
  if (stmts_are_full_exprs_p ())
    begin_scope (sk, NULL);
  return ret;
}

/* Queue a cleanup.  CLEANUP is an expression/statement to be executed
   when the current scope is exited.  EH_ONLY is true when this is not
   meant to apply to normal control flow transfer.  */

void
push_cleanup (tree decl, tree cleanup, bool eh_only)
{
  tree stmt = build_stmt (CLEANUP_STMT, NULL, cleanup, decl);
  CLEANUP_EH_ONLY (stmt) = eh_only;
  add_stmt (stmt);
  CLEANUP_BODY (stmt) = push_stmt_list ();
}

/* Begin a conditional that might contain a declaration.  When generating
   normal code, we want the declaration to appear before the statement
   containing the conditional.  When generating template code, we want the
   conditional to be rendered as the raw DECL_EXPR.  */

static void
begin_cond (tree *cond_p)
{
  if (processing_template_decl)
    *cond_p = push_stmt_list ();
}

/* Finish such a conditional.  */

static void
finish_cond (tree *cond_p, tree expr)
{
  if (processing_template_decl)
    {
      tree cond = pop_stmt_list (*cond_p);
      if (TREE_CODE (cond) == DECL_EXPR)
	expr = cond;

      if (check_for_bare_parameter_packs (expr))
        *cond_p = error_mark_node;
    }
  *cond_p = expr;
}

/* If *COND_P specifies a conditional with a declaration, transform the
   loop such that
	    while (A x = 42) { }
	    for (; A x = 42;) { }
   becomes
	    while (true) { A x = 42; if (!x) break; }
	    for (;;) { A x = 42; if (!x) break; }
   The statement list for BODY will be empty if the conditional did
   not declare anything.  */

static void
simplify_loop_decl_cond (tree *cond_p, tree body)
{
  tree cond, if_stmt;

  if (!TREE_SIDE_EFFECTS (body))
    return;

  cond = *cond_p;
  *cond_p = boolean_true_node;

  if_stmt = begin_if_stmt ();
  cond = cp_build_unary_op (TRUTH_NOT_EXPR, cond, 0, tf_warning_or_error);
  finish_if_stmt_cond (cond, if_stmt);
  finish_break_stmt ();
  finish_then_clause (if_stmt);
  finish_if_stmt (if_stmt);
}

/* Finish a goto-statement.  */

tree
finish_goto_stmt (tree destination)
{
  if (TREE_CODE (destination) == IDENTIFIER_NODE)
    destination = lookup_label (destination);

  /* We warn about unused labels with -Wunused.  That means we have to
     mark the used labels as used.  */
  if (TREE_CODE (destination) == LABEL_DECL)
    TREE_USED (destination) = 1;
  else
    {
      /* The DESTINATION is being used as an rvalue.  */
      if (!processing_template_decl)
	{
	  destination = decay_conversion (destination);
	  destination = cp_convert (ptr_type_node, destination);
	  if (error_operand_p (destination))
	    return NULL_TREE;
	}
      /* We don't inline calls to functions with computed gotos.
	 Those functions are typically up to some funny business,
	 and may be depending on the labels being at particular
	 addresses, or some such.  */
      DECL_UNINLINABLE (current_function_decl) = 1;
    }

  check_goto (destination);

  return add_stmt (build_stmt (GOTO_EXPR, destination));
}

/* COND is the condition-expression for an if, while, etc.,
   statement.  Convert it to a boolean value, if appropriate.
   In addition, verify sequence points if -Wsequence-point is enabled.  */

static tree
maybe_convert_cond (tree cond)
{
  /* Empty conditions remain empty.  */
  if (!cond)
    return NULL_TREE;

  /* Wait until we instantiate templates before doing conversion.  */
  if (processing_template_decl)
    return cond;

  if (warn_sequence_point)
    verify_sequence_points (cond);

  /* Do the conversion.  */
  cond = convert_from_reference (cond);

  if (TREE_CODE (cond) == MODIFY_EXPR
      && !TREE_NO_WARNING (cond)
      && warn_parentheses)
    {
      warning (OPT_Wparentheses,
	       "suggest parentheses around assignment used as truth value");
      TREE_NO_WARNING (cond) = 1;
    }

  return condition_conversion (cond);
}

/* Finish an expression-statement, whose EXPRESSION is as indicated.  */

tree
finish_expr_stmt (tree expr)
{
  tree r = NULL_TREE;

  if (expr != NULL_TREE)
    {
      if (!processing_template_decl)
	{
	  if (warn_sequence_point)
	    verify_sequence_points (expr);
	  expr = convert_to_void (expr, "statement", tf_warning_or_error);
	}
      else if (!type_dependent_expression_p (expr))
	convert_to_void (build_non_dependent_expr (expr), "statement", 
                         tf_warning_or_error);

      if (check_for_bare_parameter_packs (expr))
        expr = error_mark_node;

      /* Simplification of inner statement expressions, compound exprs,
	 etc can result in us already having an EXPR_STMT.  */
      if (TREE_CODE (expr) != CLEANUP_POINT_EXPR)
	{
	  if (TREE_CODE (expr) != EXPR_STMT)
	    expr = build_stmt (EXPR_STMT, expr);
	  expr = maybe_cleanup_point_expr_void (expr);
	}

      r = add_stmt (expr);
    }

  finish_stmt ();

  return r;
}


/* Begin an if-statement.  Returns a newly created IF_STMT if
   appropriate.  */

tree
begin_if_stmt (void)
{
  tree r, scope;
  scope = do_pushlevel (sk_block);
  r = build_stmt (IF_STMT, NULL_TREE, NULL_TREE, NULL_TREE);
  TREE_CHAIN (r) = scope;
  begin_cond (&IF_COND (r));
  return r;
}

/* Process the COND of an if-statement, which may be given by
   IF_STMT.  */

void
finish_if_stmt_cond (tree cond, tree if_stmt)
{
  finish_cond (&IF_COND (if_stmt), maybe_convert_cond (cond));
  add_stmt (if_stmt);
  THEN_CLAUSE (if_stmt) = push_stmt_list ();
}

/* Finish the then-clause of an if-statement, which may be given by
   IF_STMT.  */

tree
finish_then_clause (tree if_stmt)
{
  THEN_CLAUSE (if_stmt) = pop_stmt_list (THEN_CLAUSE (if_stmt));
  return if_stmt;
}

/* Begin the else-clause of an if-statement.  */

void
begin_else_clause (tree if_stmt)
{
  ELSE_CLAUSE (if_stmt) = push_stmt_list ();
}

/* Finish the else-clause of an if-statement, which may be given by
   IF_STMT.  */

void
finish_else_clause (tree if_stmt)
{
  ELSE_CLAUSE (if_stmt) = pop_stmt_list (ELSE_CLAUSE (if_stmt));
}

/* Finish an if-statement.  */

void
finish_if_stmt (tree if_stmt)
{
  tree scope = TREE_CHAIN (if_stmt);
  TREE_CHAIN (if_stmt) = NULL;
  add_stmt (do_poplevel (scope));
  finish_stmt ();
}

/* Begin a while-statement.  Returns a newly created WHILE_STMT if
   appropriate.  */

tree
begin_while_stmt (void)
{
  tree r;
  r = build_stmt (WHILE_STMT, NULL_TREE, NULL_TREE);
  add_stmt (r);
  WHILE_BODY (r) = do_pushlevel (sk_block);
  begin_cond (&WHILE_COND (r));
  return r;
}

/* Process the COND of a while-statement, which may be given by
   WHILE_STMT.  */

void
finish_while_stmt_cond (tree cond, tree while_stmt)
{
  finish_cond (&WHILE_COND (while_stmt), maybe_convert_cond (cond));
  simplify_loop_decl_cond (&WHILE_COND (while_stmt), WHILE_BODY (while_stmt));
}

/* Finish a while-statement, which may be given by WHILE_STMT.  */

void
finish_while_stmt (tree while_stmt)
{
  WHILE_BODY (while_stmt) = do_poplevel (WHILE_BODY (while_stmt));
  finish_stmt ();
}

/* Begin a do-statement.  Returns a newly created DO_STMT if
   appropriate.  */

tree
begin_do_stmt (void)
{
  tree r = build_stmt (DO_STMT, NULL_TREE, NULL_TREE);
  add_stmt (r);
  DO_BODY (r) = push_stmt_list ();
  return r;
}

/* Finish the body of a do-statement, which may be given by DO_STMT.  */

void
finish_do_body (tree do_stmt)
{
  tree body = DO_BODY (do_stmt) = pop_stmt_list (DO_BODY (do_stmt));

  if (TREE_CODE (body) == STATEMENT_LIST && STATEMENT_LIST_TAIL (body))
    body = STATEMENT_LIST_TAIL (body)->stmt;

  if (IS_EMPTY_STMT (body))
    warning (OPT_Wempty_body,
            "suggest explicit braces around empty body in %<do%> statement");
}

/* Finish a do-statement, which may be given by DO_STMT, and whose
   COND is as indicated.  */

void
finish_do_stmt (tree cond, tree do_stmt)
{
  cond = maybe_convert_cond (cond);
  DO_COND (do_stmt) = cond;
  finish_stmt ();
}

/* Finish a return-statement.  The EXPRESSION returned, if any, is as
   indicated.  */

tree
finish_return_stmt (tree expr)
{
  tree r;
  bool no_warning;

  expr = check_return_expr (expr, &no_warning);

  if (flag_openmp && !check_omp_return ())
    return error_mark_node;
  if (!processing_template_decl)
    {
      if (warn_sequence_point)
	verify_sequence_points (expr);
      
      if (DECL_DESTRUCTOR_P (current_function_decl)
	  || (DECL_CONSTRUCTOR_P (current_function_decl)
	      && targetm.cxx.cdtor_returns_this ()))
	{
	  /* Similarly, all destructors must run destructors for
	     base-classes before returning.  So, all returns in a
	     destructor get sent to the DTOR_LABEL; finish_function emits
	     code to return a value there.  */
	  return finish_goto_stmt (cdtor_label);
	}
    }

  r = build_stmt (RETURN_EXPR, expr);
  TREE_NO_WARNING (r) |= no_warning;
  r = maybe_cleanup_point_expr_void (r);
  r = add_stmt (r);
  finish_stmt ();

  return r;
}

/* Begin a for-statement.  Returns a new FOR_STMT if appropriate.  */

tree
begin_for_stmt (void)
{
  tree r;

  r = build_stmt (FOR_STMT, NULL_TREE, NULL_TREE,
		  NULL_TREE, NULL_TREE);

  if (flag_new_for_scope > 0)
    TREE_CHAIN (r) = do_pushlevel (sk_for);

  if (processing_template_decl)
    FOR_INIT_STMT (r) = push_stmt_list ();

  return r;
}

/* Finish the for-init-statement of a for-statement, which may be
   given by FOR_STMT.  */

void
finish_for_init_stmt (tree for_stmt)
{
  if (processing_template_decl)
    FOR_INIT_STMT (for_stmt) = pop_stmt_list (FOR_INIT_STMT (for_stmt));
  add_stmt (for_stmt);
  FOR_BODY (for_stmt) = do_pushlevel (sk_block);
  begin_cond (&FOR_COND (for_stmt));
}

/* Finish the COND of a for-statement, which may be given by
   FOR_STMT.  */

void
finish_for_cond (tree cond, tree for_stmt)
{
  finish_cond (&FOR_COND (for_stmt), maybe_convert_cond (cond));
  simplify_loop_decl_cond (&FOR_COND (for_stmt), FOR_BODY (for_stmt));
}

/* Finish the increment-EXPRESSION in a for-statement, which may be
   given by FOR_STMT.  */

void
finish_for_expr (tree expr, tree for_stmt)
{
  if (!expr)
    return;
  /* If EXPR is an overloaded function, issue an error; there is no
     context available to use to perform overload resolution.  */
  if (type_unknown_p (expr))
    {
      cxx_incomplete_type_error (expr, TREE_TYPE (expr));
      expr = error_mark_node;
    }
  if (!processing_template_decl)
    {
      if (warn_sequence_point)
	verify_sequence_points (expr);
      expr = convert_to_void (expr, "3rd expression in for",
                              tf_warning_or_error);
    }
  else if (!type_dependent_expression_p (expr))
    convert_to_void (build_non_dependent_expr (expr), "3rd expression in for",
                     tf_warning_or_error);
  expr = maybe_cleanup_point_expr_void (expr);
  if (check_for_bare_parameter_packs (expr))
    expr = error_mark_node;
  FOR_EXPR (for_stmt) = expr;
}

/* Finish the body of a for-statement, which may be given by
   FOR_STMT.  The increment-EXPR for the loop must be
   provided.  */

void
finish_for_stmt (tree for_stmt)
{
  FOR_BODY (for_stmt) = do_poplevel (FOR_BODY (for_stmt));

  /* Pop the scope for the body of the loop.  */
  if (flag_new_for_scope > 0)
    {
      tree scope = TREE_CHAIN (for_stmt);
      TREE_CHAIN (for_stmt) = NULL;
      add_stmt (do_poplevel (scope));
    }

  finish_stmt ();
}

/* Finish a break-statement.  */

tree
finish_break_stmt (void)
{
  return add_stmt (build_stmt (BREAK_STMT));
}

/* Finish a continue-statement.  */

tree
finish_continue_stmt (void)
{
  return add_stmt (build_stmt (CONTINUE_STMT));
}

/* Begin a switch-statement.  Returns a new SWITCH_STMT if
   appropriate.  */

tree
begin_switch_stmt (void)
{
  tree r, scope;

  r = build_stmt (SWITCH_STMT, NULL_TREE, NULL_TREE, NULL_TREE);

  scope = do_pushlevel (sk_block);
  TREE_CHAIN (r) = scope;
  begin_cond (&SWITCH_STMT_COND (r));

  return r;
}

/* Finish the cond of a switch-statement.  */

void
finish_switch_cond (tree cond, tree switch_stmt)
{
  tree orig_type = NULL;
  if (!processing_template_decl)
    {
      /* Convert the condition to an integer or enumeration type.  */
      cond = build_expr_type_conversion (WANT_INT | WANT_ENUM, cond, true);
      if (cond == NULL_TREE)
	{
	  error ("switch quantity not an integer");
	  cond = error_mark_node;
	}
      orig_type = TREE_TYPE (cond);
      if (cond != error_mark_node)
	{
	  /* [stmt.switch]

	     Integral promotions are performed.  */
	  cond = perform_integral_promotions (cond);
	  cond = maybe_cleanup_point_expr (cond);
	}
    }
  if (check_for_bare_parameter_packs (cond))
    cond = error_mark_node;
  else if (!processing_template_decl && warn_sequence_point)
    verify_sequence_points (cond);

  finish_cond (&SWITCH_STMT_COND (switch_stmt), cond);
  SWITCH_STMT_TYPE (switch_stmt) = orig_type;
  add_stmt (switch_stmt);
  push_switch (switch_stmt);
  SWITCH_STMT_BODY (switch_stmt) = push_stmt_list ();
}

/* Finish the body of a switch-statement, which may be given by
   SWITCH_STMT.  The COND to switch on is indicated.  */

void
finish_switch_stmt (tree switch_stmt)
{
  tree scope;

  SWITCH_STMT_BODY (switch_stmt) =
    pop_stmt_list (SWITCH_STMT_BODY (switch_stmt));
  pop_switch ();
  finish_stmt ();

  scope = TREE_CHAIN (switch_stmt);
  TREE_CHAIN (switch_stmt) = NULL;
  add_stmt (do_poplevel (scope));
}

/* Begin a try-block.  Returns a newly-created TRY_BLOCK if
   appropriate.  */

tree
begin_try_block (void)
{
  tree r = build_stmt (TRY_BLOCK, NULL_TREE, NULL_TREE);
  add_stmt (r);
  TRY_STMTS (r) = push_stmt_list ();
  return r;
}

/* Likewise, for a function-try-block.  The block returned in
   *COMPOUND_STMT is an artificial outer scope, containing the
   function-try-block.  */

tree
begin_function_try_block (tree *compound_stmt)
{
  tree r;
  /* This outer scope does not exist in the C++ standard, but we need
     a place to put __FUNCTION__ and similar variables.  */
  *compound_stmt = begin_compound_stmt (0);
  r = begin_try_block ();
  FN_TRY_BLOCK_P (r) = 1;
  return r;
}

/* Finish a try-block, which may be given by TRY_BLOCK.  */

void
finish_try_block (tree try_block)
{
  TRY_STMTS (try_block) = pop_stmt_list (TRY_STMTS (try_block));
  TRY_HANDLERS (try_block) = push_stmt_list ();
}

/* Finish the body of a cleanup try-block, which may be given by
   TRY_BLOCK.  */

void
finish_cleanup_try_block (tree try_block)
{
  TRY_STMTS (try_block) = pop_stmt_list (TRY_STMTS (try_block));
}

/* Finish an implicitly generated try-block, with a cleanup is given
   by CLEANUP.  */

void
finish_cleanup (tree cleanup, tree try_block)
{
  TRY_HANDLERS (try_block) = cleanup;
  CLEANUP_P (try_block) = 1;
}

/* Likewise, for a function-try-block.  */

void
finish_function_try_block (tree try_block)
{
  finish_try_block (try_block);
  /* FIXME : something queer about CTOR_INITIALIZER somehow following
     the try block, but moving it inside.  */
  in_function_try_handler = 1;
}

/* Finish a handler-sequence for a try-block, which may be given by
   TRY_BLOCK.  */

void
finish_handler_sequence (tree try_block)
{
  TRY_HANDLERS (try_block) = pop_stmt_list (TRY_HANDLERS (try_block));
  check_handlers (TRY_HANDLERS (try_block));
}

/* Finish the handler-seq for a function-try-block, given by
   TRY_BLOCK.  COMPOUND_STMT is the outer block created by
   begin_function_try_block.  */

void
finish_function_handler_sequence (tree try_block, tree compound_stmt)
{
  in_function_try_handler = 0;
  finish_handler_sequence (try_block);
  finish_compound_stmt (compound_stmt);
}

/* Begin a handler.  Returns a HANDLER if appropriate.  */

tree
begin_handler (void)
{
  tree r;

  r = build_stmt (HANDLER, NULL_TREE, NULL_TREE);
  add_stmt (r);

  /* Create a binding level for the eh_info and the exception object
     cleanup.  */
  HANDLER_BODY (r) = do_pushlevel (sk_catch);

  return r;
}

/* Finish the handler-parameters for a handler, which may be given by
   HANDLER.  DECL is the declaration for the catch parameter, or NULL
   if this is a `catch (...)' clause.  */

void
finish_handler_parms (tree decl, tree handler)
{
  tree type = NULL_TREE;
  if (processing_template_decl)
    {
      if (decl)
	{
	  decl = pushdecl (decl);
	  decl = push_template_decl (decl);
	  HANDLER_PARMS (handler) = decl;
	  type = TREE_TYPE (decl);
	}
    }
  else
    type = expand_start_catch_block (decl);
  HANDLER_TYPE (handler) = type;
  if (!processing_template_decl && type)
    mark_used (eh_type_info (type));
}

/* Finish a handler, which may be given by HANDLER.  The BLOCKs are
   the return value from the matching call to finish_handler_parms.  */

void
finish_handler (tree handler)
{
  if (!processing_template_decl)
    expand_end_catch_block ();
  HANDLER_BODY (handler) = do_poplevel (HANDLER_BODY (handler));
}

/* Begin a compound statement.  FLAGS contains some bits that control the
   behavior and context.  If BCS_NO_SCOPE is set, the compound statement
   does not define a scope.  If BCS_FN_BODY is set, this is the outermost
   block of a function.  If BCS_TRY_BLOCK is set, this is the block
   created on behalf of a TRY statement.  Returns a token to be passed to
   finish_compound_stmt.  */

tree
begin_compound_stmt (unsigned int flags)
{
  tree r;

  if (flags & BCS_NO_SCOPE)
    {
      r = push_stmt_list ();
      STATEMENT_LIST_NO_SCOPE (r) = 1;

      /* Normally, we try hard to keep the BLOCK for a statement-expression.
	 But, if it's a statement-expression with a scopeless block, there's
	 nothing to keep, and we don't want to accidentally keep a block
	 *inside* the scopeless block.  */
      keep_next_level (false);
    }
  else
    r = do_pushlevel (flags & BCS_TRY_BLOCK ? sk_try : sk_block);

  /* When processing a template, we need to remember where the braces were,
     so that we can set up identical scopes when instantiating the template
     later.  BIND_EXPR is a handy candidate for this.
     Note that do_poplevel won't create a BIND_EXPR itself here (and thus
     result in nested BIND_EXPRs), since we don't build BLOCK nodes when
     processing templates.  */
  if (processing_template_decl)
    {
      r = build3 (BIND_EXPR, NULL, NULL, r, NULL);
      BIND_EXPR_TRY_BLOCK (r) = (flags & BCS_TRY_BLOCK) != 0;
      BIND_EXPR_BODY_BLOCK (r) = (flags & BCS_FN_BODY) != 0;
      TREE_SIDE_EFFECTS (r) = 1;
    }

  return r;
}

/* Finish a compound-statement, which is given by STMT.  */

void
finish_compound_stmt (tree stmt)
{
  if (TREE_CODE (stmt) == BIND_EXPR)
    BIND_EXPR_BODY (stmt) = do_poplevel (BIND_EXPR_BODY (stmt));
  else if (STATEMENT_LIST_NO_SCOPE (stmt))
    stmt = pop_stmt_list (stmt);
  else
    {
      /* Destroy any ObjC "super" receivers that may have been
	 created.  */
      objc_clear_super_receiver ();

      stmt = do_poplevel (stmt);
    }

  /* ??? See c_end_compound_stmt wrt statement expressions.  */
  add_stmt (stmt);
  finish_stmt ();
}

/* Finish an asm-statement, whose components are a STRING, some
   OUTPUT_OPERANDS, some INPUT_OPERANDS, and some CLOBBERS.  Also note
   whether the asm-statement should be considered volatile.  */

tree
finish_asm_stmt (int volatile_p, tree string, tree output_operands,
		 tree input_operands, tree clobbers)
{
  tree r;
  tree t;
  int ninputs = list_length (input_operands);
  int noutputs = list_length (output_operands);

  if (!processing_template_decl)
    {
      const char *constraint;
      const char **oconstraints;
      bool allows_mem, allows_reg, is_inout;
      tree operand;
      int i;

      oconstraints = (const char **) alloca (noutputs * sizeof (char *));

      string = resolve_asm_operand_names (string, output_operands,
					  input_operands);

      for (i = 0, t = output_operands; t; t = TREE_CHAIN (t), ++i)
	{
	  operand = TREE_VALUE (t);

	  /* ??? Really, this should not be here.  Users should be using a
	     proper lvalue, dammit.  But there's a long history of using
	     casts in the output operands.  In cases like longlong.h, this
	     becomes a primitive form of typechecking -- if the cast can be
	     removed, then the output operand had a type of the proper width;
	     otherwise we'll get an error.  Gross, but ...  */
	  STRIP_NOPS (operand);

	  if (!lvalue_or_else (operand, lv_asm, tf_warning_or_error))
	    operand = error_mark_node;

	  if (operand != error_mark_node
	      && (TREE_READONLY (operand)
		  || CP_TYPE_CONST_P (TREE_TYPE (operand))
		  /* Functions are not modifiable, even though they are
		     lvalues.  */
		  || TREE_CODE (TREE_TYPE (operand)) == FUNCTION_TYPE
		  || TREE_CODE (TREE_TYPE (operand)) == METHOD_TYPE
		  /* If it's an aggregate and any field is const, then it is
		     effectively const.  */
		  || (CLASS_TYPE_P (TREE_TYPE (operand))
		      && C_TYPE_FIELDS_READONLY (TREE_TYPE (operand)))))
	    readonly_error (operand, "assignment (via 'asm' output)");

	  constraint = TREE_STRING_POINTER (TREE_VALUE (TREE_PURPOSE (t)));
	  oconstraints[i] = constraint;

	  if (parse_output_constraint (&constraint, i, ninputs, noutputs,
				       &allows_mem, &allows_reg, &is_inout))
	    {
	      /* If the operand is going to end up in memory,
		 mark it addressable.  */
	      if (!allows_reg && !cxx_mark_addressable (operand))
		operand = error_mark_node;
	    }
	  else
	    operand = error_mark_node;

	  TREE_VALUE (t) = operand;
	}

      for (i = 0, t = input_operands; t; ++i, t = TREE_CHAIN (t))
	{
	  constraint = TREE_STRING_POINTER (TREE_VALUE (TREE_PURPOSE (t)));
	  operand = decay_conversion (TREE_VALUE (t));

	  /* If the type of the operand hasn't been determined (e.g.,
	     because it involves an overloaded function), then issue
	     an error message.  There's no context available to
	     resolve the overloading.  */
	  if (TREE_TYPE (operand) == unknown_type_node)
	    {
	      error ("type of asm operand %qE could not be determined",
		     TREE_VALUE (t));
	      operand = error_mark_node;
	    }

	  if (parse_input_constraint (&constraint, i, ninputs, noutputs, 0,
				      oconstraints, &allows_mem, &allows_reg))
	    {
	      /* If the operand is going to end up in memory,
		 mark it addressable.  */
	      if (!allows_reg && allows_mem)
		{
		  /* Strip the nops as we allow this case.  FIXME, this really
		     should be rejected or made deprecated.  */
		  STRIP_NOPS (operand);
		  if (!cxx_mark_addressable (operand))
		    operand = error_mark_node;
		}
	    }
	  else
	    operand = error_mark_node;

	  TREE_VALUE (t) = operand;
	}
    }

  r = build_stmt (ASM_EXPR, string,
		  output_operands, input_operands,
		  clobbers);
  ASM_VOLATILE_P (r) = volatile_p || noutputs == 0;
  r = maybe_cleanup_point_expr_void (r);
  return add_stmt (r);
}

/* Finish a label with the indicated NAME.  */

tree
finish_label_stmt (tree name)
{
  tree decl = define_label (input_location, name);

  if (decl  == error_mark_node)
    return error_mark_node;

  return add_stmt (build_stmt (LABEL_EXPR, decl));
}

/* Finish a series of declarations for local labels.  G++ allows users
   to declare "local" labels, i.e., labels with scope.  This extension
   is useful when writing code involving statement-expressions.  */

void
finish_label_decl (tree name)
{
  if (!at_function_scope_p ())
    {
      error ("__label__ declarations are only allowed in function scopes");
      return;
    }

  add_decl_expr (declare_local_label (name));
}

/* When DECL goes out of scope, make sure that CLEANUP is executed.  */

void
finish_decl_cleanup (tree decl, tree cleanup)
{
  push_cleanup (decl, cleanup, false);
}

/* If the current scope exits with an exception, run CLEANUP.  */

void
finish_eh_cleanup (tree cleanup)
{
  push_cleanup (NULL, cleanup, true);
}

/* The MEM_INITS is a list of mem-initializers, in reverse of the
   order they were written by the user.  Each node is as for
   emit_mem_initializers.  */

void
finish_mem_initializers (tree mem_inits)
{
  /* Reorder the MEM_INITS so that they are in the order they appeared
     in the source program.  */
  mem_inits = nreverse (mem_inits);

  if (processing_template_decl)
    {
      tree mem;

      for (mem = mem_inits; mem; mem = TREE_CHAIN (mem))
        {
          /* If the TREE_PURPOSE is a TYPE_PACK_EXPANSION, skip the
             check for bare parameter packs in the TREE_VALUE, because
             any parameter packs in the TREE_VALUE have already been
             bound as part of the TREE_PURPOSE.  See
             make_pack_expansion for more information.  */
          if (TREE_CODE (TREE_PURPOSE (mem)) != TYPE_PACK_EXPANSION
              && check_for_bare_parameter_packs (TREE_VALUE (mem)))
            TREE_VALUE (mem) = error_mark_node;
        }

      add_stmt (build_min_nt (CTOR_INITIALIZER, mem_inits));
    }
  else
    emit_mem_initializers (mem_inits);
}

/* Finish a parenthesized expression EXPR.  */

tree
finish_parenthesized_expr (tree expr)
{
  if (EXPR_P (expr))
    /* This inhibits warnings in c_common_truthvalue_conversion.  */
    TREE_NO_WARNING (expr) = 1;

  if (TREE_CODE (expr) == OFFSET_REF)
    /* [expr.unary.op]/3 The qualified id of a pointer-to-member must not be
       enclosed in parentheses.  */
    PTRMEM_OK_P (expr) = 0;

  if (TREE_CODE (expr) == STRING_CST)
    PAREN_STRING_LITERAL_P (expr) = 1;

  return expr;
}

/* Finish a reference to a non-static data member (DECL) that is not
   preceded by `.' or `->'.  */

tree
finish_non_static_data_member (tree decl, tree object, tree qualifying_scope)
{
  gcc_assert (TREE_CODE (decl) == FIELD_DECL);

  if (!object && skip_evaluation)
    {
      /* DR 613: Can use non-static data members without an associated
         object in sizeof/decltype/alignof.  */
      tree scope = qualifying_scope;
      if (scope == NULL_TREE)
	scope = context_for_name_lookup (decl);
      object = maybe_dummy_object (scope, NULL);
    }

  if (!object)
    {
      if (current_function_decl
	  && DECL_STATIC_FUNCTION_P (current_function_decl))
	error ("invalid use of member %q+D in static member function", decl);
      else
	error ("invalid use of non-static data member %q+D", decl);
      error ("from this location");

      return error_mark_node;
    }
  if (current_class_ptr)
    TREE_USED (current_class_ptr) = 1;
  if (processing_template_decl && !qualifying_scope)
    {
      tree type = TREE_TYPE (decl);

      if (TREE_CODE (type) == REFERENCE_TYPE)
	type = TREE_TYPE (type);
      else
	{
	  /* Set the cv qualifiers.  */
	  int quals = (current_class_ref
		       ? cp_type_quals (TREE_TYPE (current_class_ref))
		       : TYPE_UNQUALIFIED);

	  if (DECL_MUTABLE_P (decl))
	    quals &= ~TYPE_QUAL_CONST;

	  quals |= cp_type_quals (TREE_TYPE (decl));
	  type = cp_build_qualified_type (type, quals);
	}

      return build_min (COMPONENT_REF, type, object, decl, NULL_TREE);
    }
  /* If PROCESSING_TEMPLATE_DECL is nonzero here, then
     QUALIFYING_SCOPE is also non-null.  Wrap this in a SCOPE_REF
     for now.  */
  else if (processing_template_decl)
    return build_qualified_name (TREE_TYPE (decl),
				 qualifying_scope,
				 DECL_NAME (decl),
				 /*template_p=*/false);
  else
    {
      tree access_type = TREE_TYPE (object);
      tree lookup_context = context_for_name_lookup (decl);

      while (!DERIVED_FROM_P (lookup_context, access_type))
	{
	  access_type = TYPE_CONTEXT (access_type);
	  while (access_type && DECL_P (access_type))
	    access_type = DECL_CONTEXT (access_type);

	  if (!access_type)
	    {
	      error ("object missing in reference to %q+D", decl);
	      error ("from this location");
	      return error_mark_node;
	    }
	}

      perform_or_defer_access_check (TYPE_BINFO (access_type), decl,
				     decl);

      /* If the data member was named `C::M', convert `*this' to `C'
	 first.  */
      if (qualifying_scope)
	{
	  tree binfo = NULL_TREE;
	  object = build_scoped_ref (object, qualifying_scope,
				     &binfo);
	}

      return build_class_member_access_expr (object, decl,
					     /*access_path=*/NULL_TREE,
					     /*preserve_reference=*/false,
					     tf_warning_or_error);
    }
}

/* DECL was the declaration to which a qualified-id resolved.  Issue
   an error message if it is not accessible.  If OBJECT_TYPE is
   non-NULL, we have just seen `x->' or `x.' and OBJECT_TYPE is the
   type of `*x', or `x', respectively.  If the DECL was named as
   `A::B' then NESTED_NAME_SPECIFIER is `A'.  */

void
check_accessibility_of_qualified_id (tree decl,
				     tree object_type,
				     tree nested_name_specifier)
{
  tree scope;
  tree qualifying_type = NULL_TREE;

  /* If we're not checking, return immediately.  */
  if (deferred_access_no_check)
    return;

  /* Determine the SCOPE of DECL.  */
  scope = context_for_name_lookup (decl);
  /* If the SCOPE is not a type, then DECL is not a member.  */
  if (!TYPE_P (scope))
    return;
  /* Compute the scope through which DECL is being accessed.  */
  if (object_type
      /* OBJECT_TYPE might not be a class type; consider:

	   class A { typedef int I; };
	   I *p;
	   p->A::I::~I();

	 In this case, we will have "A::I" as the DECL, but "I" as the
	 OBJECT_TYPE.  */
      && CLASS_TYPE_P (object_type)
      && DERIVED_FROM_P (scope, object_type))
    /* If we are processing a `->' or `.' expression, use the type of the
       left-hand side.  */
    qualifying_type = object_type;
  else if (nested_name_specifier)
    {
      /* If the reference is to a non-static member of the
	 current class, treat it as if it were referenced through
	 `this'.  */
      if (DECL_NONSTATIC_MEMBER_P (decl)
	  && current_class_ptr
	  && DERIVED_FROM_P (scope, current_class_type))
	qualifying_type = current_class_type;
      /* Otherwise, use the type indicated by the
	 nested-name-specifier.  */
      else
	qualifying_type = nested_name_specifier;
    }
  else
    /* Otherwise, the name must be from the current class or one of
       its bases.  */
    qualifying_type = currently_open_derived_class (scope);

  if (qualifying_type 
      /* It is possible for qualifying type to be a TEMPLATE_TYPE_PARM
	 or similar in a default argument value.  */
      && CLASS_TYPE_P (qualifying_type)
      && !dependent_type_p (qualifying_type))
    perform_or_defer_access_check (TYPE_BINFO (qualifying_type), decl,
				   decl);
}

/* EXPR is the result of a qualified-id.  The QUALIFYING_CLASS was the
   class named to the left of the "::" operator.  DONE is true if this
   expression is a complete postfix-expression; it is false if this
   expression is followed by '->', '[', '(', etc.  ADDRESS_P is true
   iff this expression is the operand of '&'.  TEMPLATE_P is true iff
   the qualified-id was of the form "A::template B".  TEMPLATE_ARG_P
   is true iff this qualified name appears as a template argument.  */

tree
finish_qualified_id_expr (tree qualifying_class,
			  tree expr,
			  bool done,
			  bool address_p,
			  bool template_p,
			  bool template_arg_p)
{
  gcc_assert (TYPE_P (qualifying_class));

  if (error_operand_p (expr))
    return error_mark_node;

  if (DECL_P (expr) || BASELINK_P (expr))
    mark_used (expr);

  if (template_p)
    check_template_keyword (expr);

  /* If EXPR occurs as the operand of '&', use special handling that
     permits a pointer-to-member.  */
  if (address_p && done)
    {
      if (TREE_CODE (expr) == SCOPE_REF)
	expr = TREE_OPERAND (expr, 1);
      expr = build_offset_ref (qualifying_class, expr,
			       /*address_p=*/true);
      return expr;
    }

  /* Within the scope of a class, turn references to non-static
     members into expression of the form "this->...".  */
  if (template_arg_p)
    /* But, within a template argument, we do not want make the
       transformation, as there is no "this" pointer.  */
    ;
  else if (TREE_CODE (expr) == FIELD_DECL)
    {
      push_deferring_access_checks (dk_no_check);
      expr = finish_non_static_data_member (expr, current_class_ref,
					    qualifying_class);
      pop_deferring_access_checks ();
    }
  else if (BASELINK_P (expr) && !processing_template_decl)
    {
      tree fns;

      /* See if any of the functions are non-static members.  */
      fns = BASELINK_FUNCTIONS (expr);
      if (TREE_CODE (fns) == TEMPLATE_ID_EXPR)
	fns = TREE_OPERAND (fns, 0);
      /* If so, the expression may be relative to the current
	 class.  */
      if (!shared_member_p (fns)
	  && current_class_type
	  && DERIVED_FROM_P (qualifying_class, current_class_type))
	expr = (build_class_member_access_expr
		(maybe_dummy_object (qualifying_class, NULL),
		 expr,
		 BASELINK_ACCESS_BINFO (expr),
		 /*preserve_reference=*/false,
		 tf_warning_or_error));
      else if (done)
	/* The expression is a qualified name whose address is not
	   being taken.  */
	expr = build_offset_ref (qualifying_class, expr, /*address_p=*/false);
    }

  return expr;
}

/* Begin a statement-expression.  The value returned must be passed to
   finish_stmt_expr.  */

tree
begin_stmt_expr (void)
{
  return push_stmt_list ();
}

/* Process the final expression of a statement expression. EXPR can be
   NULL, if the final expression is empty.  Return a STATEMENT_LIST
   containing all the statements in the statement-expression, or
   ERROR_MARK_NODE if there was an error.  */

tree
finish_stmt_expr_expr (tree expr, tree stmt_expr)
{
  if (error_operand_p (expr))
    {
      /* The type of the statement-expression is the type of the last
         expression.  */
      TREE_TYPE (stmt_expr) = error_mark_node;
      return error_mark_node;
    }

  /* If the last statement does not have "void" type, then the value
     of the last statement is the value of the entire expression.  */
  if (expr)
    {
      tree type = TREE_TYPE (expr);

      if (processing_template_decl)
	{
	  expr = build_stmt (EXPR_STMT, expr);
	  expr = add_stmt (expr);
	  /* Mark the last statement so that we can recognize it as such at
	     template-instantiation time.  */
	  EXPR_STMT_STMT_EXPR_RESULT (expr) = 1;
	}
      else if (VOID_TYPE_P (type))
	{
	  /* Just treat this like an ordinary statement.  */
	  expr = finish_expr_stmt (expr);
	}
      else
	{
	  /* It actually has a value we need to deal with.  First, force it
	     to be an rvalue so that we won't need to build up a copy
	     constructor call later when we try to assign it to something.  */
	  expr = force_rvalue (expr);
	  if (error_operand_p (expr))
	    return error_mark_node;

	  /* Update for array-to-pointer decay.  */
	  type = TREE_TYPE (expr);

	  /* Wrap it in a CLEANUP_POINT_EXPR and add it to the list like a
	     normal statement, but don't convert to void or actually add
	     the EXPR_STMT.  */
	  if (TREE_CODE (expr) != CLEANUP_POINT_EXPR)
	    expr = maybe_cleanup_point_expr (expr);
	  add_stmt (expr);
	}

      /* The type of the statement-expression is the type of the last
	 expression.  */
      TREE_TYPE (stmt_expr) = type;
    }

  return stmt_expr;
}

/* Finish a statement-expression.  EXPR should be the value returned
   by the previous begin_stmt_expr.  Returns an expression
   representing the statement-expression.  */

tree
finish_stmt_expr (tree stmt_expr, bool has_no_scope)
{
  tree type;
  tree result;

  if (error_operand_p (stmt_expr))
    {
      pop_stmt_list (stmt_expr);
      return error_mark_node;
    }

  gcc_assert (TREE_CODE (stmt_expr) == STATEMENT_LIST);

  type = TREE_TYPE (stmt_expr);
  result = pop_stmt_list (stmt_expr);
  TREE_TYPE (result) = type;

  if (processing_template_decl)
    {
      result = build_min (STMT_EXPR, type, result);
      TREE_SIDE_EFFECTS (result) = 1;
      STMT_EXPR_NO_SCOPE (result) = has_no_scope;
    }
  else if (CLASS_TYPE_P (type))
    {
      /* Wrap the statement-expression in a TARGET_EXPR so that the
	 temporary object created by the final expression is destroyed at
	 the end of the full-expression containing the
	 statement-expression.  */
      result = force_target_expr (type, result);
    }

  return result;
}

/* Returns the expression which provides the value of STMT_EXPR.  */

tree
stmt_expr_value_expr (tree stmt_expr)
{
  tree t = STMT_EXPR_STMT (stmt_expr);

  if (TREE_CODE (t) == BIND_EXPR)
    t = BIND_EXPR_BODY (t);

  if (TREE_CODE (t) == STATEMENT_LIST && STATEMENT_LIST_TAIL (t))
    t = STATEMENT_LIST_TAIL (t)->stmt;

  if (TREE_CODE (t) == EXPR_STMT)
    t = EXPR_STMT_EXPR (t);

  return t;
}

/* Perform Koenig lookup.  FN is the postfix-expression representing
   the function (or functions) to call; ARGS are the arguments to the
   call.  Returns the functions to be considered by overload
   resolution.  */

tree
perform_koenig_lookup (tree fn, tree args)
{
  tree identifier = NULL_TREE;
  tree functions = NULL_TREE;
  tree tmpl_args = NULL_TREE;
  bool template_id = false;

  if (TREE_CODE (fn) == TEMPLATE_ID_EXPR)
    {
      /* Use a separate flag to handle null args.  */
      template_id = true;
      tmpl_args = TREE_OPERAND (fn, 1);
      fn = TREE_OPERAND (fn, 0);
    }

  /* Find the name of the overloaded function.  */
  if (TREE_CODE (fn) == IDENTIFIER_NODE)
    identifier = fn;
  else if (is_overloaded_fn (fn))
    {
      functions = fn;
      identifier = DECL_NAME (get_first_fn (functions));
    }
  else if (DECL_P (fn))
    {
      functions = fn;
      identifier = DECL_NAME (fn);
    }

  /* A call to a namespace-scope function using an unqualified name.

     Do Koenig lookup -- unless any of the arguments are
     type-dependent.  */
  if (!any_type_dependent_arguments_p (args)
      && !any_dependent_template_arguments_p (tmpl_args))
    {
      fn = lookup_arg_dependent (identifier, functions, args);
      if (!fn)
	/* The unqualified name could not be resolved.  */
	fn = unqualified_fn_lookup_error (identifier);
    }

  if (fn && template_id)
    fn = build2 (TEMPLATE_ID_EXPR, unknown_type_node, fn, tmpl_args);
  
  return fn;
}

/* Generate an expression for `FN (ARGS)'.

   If DISALLOW_VIRTUAL is true, the call to FN will be not generated
   as a virtual call, even if FN is virtual.  (This flag is set when
   encountering an expression where the function name is explicitly
   qualified.  For example a call to `X::f' never generates a virtual
   call.)

   Returns code for the call.  */

tree
finish_call_expr (tree fn, tree args, bool disallow_virtual, bool koenig_p,
		  tsubst_flags_t complain)
{
  tree result;
  tree orig_fn;
  tree orig_args;

  if (fn == error_mark_node || args == error_mark_node)
    return error_mark_node;

  /* ARGS should be a list of arguments.  */
  gcc_assert (!args || TREE_CODE (args) == TREE_LIST);
  gcc_assert (!TYPE_P (fn));

  orig_fn = fn;
  orig_args = args;

  if (processing_template_decl)
    {
      if (type_dependent_expression_p (fn)
	  || any_type_dependent_arguments_p (args))
	{
	  result = build_nt_call_list (fn, args);
	  KOENIG_LOOKUP_P (result) = koenig_p;
	  if (cfun)
	    {
	      do
		{
		  tree fndecl = OVL_CURRENT (fn);
		  if (TREE_CODE (fndecl) != FUNCTION_DECL
		      || !TREE_THIS_VOLATILE (fndecl))
		    break;
		  fn = OVL_NEXT (fn);
		}
	      while (fn);
	      if (!fn)
		current_function_returns_abnormally = 1;
	    }
	  return result;
	}
      if (!BASELINK_P (fn)
	  && TREE_CODE (fn) != PSEUDO_DTOR_EXPR
	  && TREE_TYPE (fn) != unknown_type_node)
	fn = build_non_dependent_expr (fn);
      args = build_non_dependent_args (orig_args);
    }

  if (is_overloaded_fn (fn))
    fn = baselink_for_fns (fn);

  result = NULL_TREE;
  if (BASELINK_P (fn))
    {
      tree object;

      /* A call to a member function.  From [over.call.func]:

	   If the keyword this is in scope and refers to the class of
	   that member function, or a derived class thereof, then the
	   function call is transformed into a qualified function call
	   using (*this) as the postfix-expression to the left of the
	   . operator.... [Otherwise] a contrived object of type T
	   becomes the implied object argument.

	This paragraph is unclear about this situation:

	  struct A { void f(); };
	  struct B : public A {};
	  struct C : public A { void g() { B::f(); }};

	In particular, for `B::f', this paragraph does not make clear
	whether "the class of that member function" refers to `A' or
	to `B'.  We believe it refers to `B'.  */
      if (current_class_type
	  && DERIVED_FROM_P (BINFO_TYPE (BASELINK_ACCESS_BINFO (fn)),
			     current_class_type)
	  && current_class_ref)
	object = maybe_dummy_object (BINFO_TYPE (BASELINK_ACCESS_BINFO (fn)),
				     NULL);
      else
	{
	  tree representative_fn;

	  representative_fn = BASELINK_FUNCTIONS (fn);
	  if (TREE_CODE (representative_fn) == TEMPLATE_ID_EXPR)
	    representative_fn = TREE_OPERAND (representative_fn, 0);
	  representative_fn = get_first_fn (representative_fn);
	  object = build_dummy_object (DECL_CONTEXT (representative_fn));
	}

      if (processing_template_decl)
	{
	  if (type_dependent_expression_p (object))
	    return build_nt_call_list (orig_fn, orig_args);
	  object = build_non_dependent_expr (object);
	}

      result = build_new_method_call (object, fn, args, NULL_TREE,
				      (disallow_virtual
				       ? LOOKUP_NONVIRTUAL : 0),
				      /*fn_p=*/NULL,
				      complain);
    }
  else if (is_overloaded_fn (fn))
    {
      /* If the function is an overloaded builtin, resolve it.  */
      if (TREE_CODE (fn) == FUNCTION_DECL
	  && (DECL_BUILT_IN_CLASS (fn) == BUILT_IN_NORMAL
	      || DECL_BUILT_IN_CLASS (fn) == BUILT_IN_MD))
	result = resolve_overloaded_builtin (fn, args);

      if (!result)
	/* A call to a namespace-scope function.  */
	result = build_new_function_call (fn, args, koenig_p, complain);
    }
  else if (TREE_CODE (fn) == PSEUDO_DTOR_EXPR)
    {
      if (args)
	error ("arguments to destructor are not allowed");
      /* Mark the pseudo-destructor call as having side-effects so
	 that we do not issue warnings about its use.  */
      result = build1 (NOP_EXPR,
		       void_type_node,
		       TREE_OPERAND (fn, 0));
      TREE_SIDE_EFFECTS (result) = 1;
    }
  else if (CLASS_TYPE_P (TREE_TYPE (fn)))
    /* If the "function" is really an object of class type, it might
       have an overloaded `operator ()'.  */
    result = build_new_op (CALL_EXPR, LOOKUP_NORMAL, fn, args, NULL_TREE,
			   /*overloaded_p=*/NULL, complain);

  if (!result)
    /* A call where the function is unknown.  */
    result = cp_build_function_call (fn, args, complain);

  if (processing_template_decl)
    {
      result = build_call_list (TREE_TYPE (result), orig_fn, orig_args);
      KOENIG_LOOKUP_P (result) = koenig_p;
    }
  return result;
}

/* Finish a call to a postfix increment or decrement or EXPR.  (Which
   is indicated by CODE, which should be POSTINCREMENT_EXPR or
   POSTDECREMENT_EXPR.)  */

tree
finish_increment_expr (tree expr, enum tree_code code)
{
  return build_x_unary_op (code, expr, tf_warning_or_error);
}

/* Finish a use of `this'.  Returns an expression for `this'.  */

tree
finish_this_expr (void)
{
  tree result;

  if (current_class_ptr)
    {
      result = current_class_ptr;
    }
  else if (current_function_decl
	   && DECL_STATIC_FUNCTION_P (current_function_decl))
    {
      error ("%<this%> is unavailable for static member functions");
      result = error_mark_node;
    }
  else
    {
      if (current_function_decl)
	error ("invalid use of %<this%> in non-member function");
      else
	error ("invalid use of %<this%> at top level");
      result = error_mark_node;
    }

  return result;
}

/* Finish a pseudo-destructor expression.  If SCOPE is NULL, the
   expression was of the form `OBJECT.~DESTRUCTOR' where DESTRUCTOR is
   the TYPE for the type given.  If SCOPE is non-NULL, the expression
   was of the form `OBJECT.SCOPE::~DESTRUCTOR'.  */

tree
finish_pseudo_destructor_expr (tree object, tree scope, tree destructor)
{
  if (object == error_mark_node || destructor == error_mark_node)
    return error_mark_node;

  gcc_assert (TYPE_P (destructor));

  if (!processing_template_decl)
    {
      if (scope == error_mark_node)
	{
	  error ("invalid qualifying scope in pseudo-destructor name");
	  return error_mark_node;
	}
      if (scope && TYPE_P (scope) && !check_dtor_name (scope, destructor))
	{
	  error ("qualified type %qT does not match destructor name ~%qT",
		 scope, destructor);
	  return error_mark_node;
	}


      /* [expr.pseudo] says both:

	   The type designated by the pseudo-destructor-name shall be
	   the same as the object type.

	 and:

	   The cv-unqualified versions of the object type and of the
	   type designated by the pseudo-destructor-name shall be the
	   same type.

	 We implement the more generous second sentence, since that is
	 what most other compilers do.  */
      if (!same_type_ignoring_top_level_qualifiers_p (TREE_TYPE (object),
						      destructor))
	{
	  error ("%qE is not of type %qT", object, destructor);
	  return error_mark_node;
	}
    }

  return build3 (PSEUDO_DTOR_EXPR, void_type_node, object, scope, destructor);
}

/* Finish an expression of the form CODE EXPR.  */

tree
finish_unary_op_expr (enum tree_code code, tree expr)
{
  tree result = build_x_unary_op (code, expr, tf_warning_or_error);
  /* Inside a template, build_x_unary_op does not fold the
     expression. So check whether the result is folded before
     setting TREE_NEGATED_INT.  */
  if (code == NEGATE_EXPR && TREE_CODE (expr) == INTEGER_CST
      && TREE_CODE (result) == INTEGER_CST
      && !TYPE_UNSIGNED (TREE_TYPE (result))
      && INT_CST_LT (result, integer_zero_node))
    {
      /* RESULT may be a cached INTEGER_CST, so we must copy it before
	 setting TREE_NEGATED_INT.  */
      result = copy_node (result);
      TREE_NEGATED_INT (result) = 1;
    }
  if (TREE_OVERFLOW_P (result) && !TREE_OVERFLOW_P (expr))
    overflow_warning (result);

  return result;
}

/* Finish a compound-literal expression.  TYPE is the type to which
   the CONSTRUCTOR in COMPOUND_LITERAL is being cast.  */

tree
finish_compound_literal (tree type, tree compound_literal)
{
  if (type == error_mark_node)
    return error_mark_node;

  if (!TYPE_OBJ_P (type))
    {
      error ("compound literal of non-object type %qT", type);
      return error_mark_node;
    }

  if (processing_template_decl)
    {
      TREE_TYPE (compound_literal) = type;
      /* Mark the expression as a compound literal.  */
      TREE_HAS_CONSTRUCTOR (compound_literal) = 1;
      return compound_literal;
    }

  type = complete_type (type);

  if (TYPE_NON_AGGREGATE_CLASS (type))
    {
      /* Trying to deal with a CONSTRUCTOR instead of a TREE_LIST
	 everywhere that deals with function arguments would be a pain, so
	 just wrap it in a TREE_LIST.  The parser set a flag so we know
	 that it came from T{} rather than T({}).  */
      CONSTRUCTOR_IS_DIRECT_INIT (compound_literal) = 1;
      compound_literal = build_tree_list (NULL_TREE, compound_literal);
      return build_functional_cast (type, compound_literal, tf_error);
    }

  if (TREE_CODE (type) == ARRAY_TYPE
      && check_array_initializer (NULL_TREE, type, compound_literal))
    return error_mark_node;
  compound_literal = reshape_init (type, compound_literal);
  if (TREE_CODE (type) == ARRAY_TYPE)
    cp_complete_array_type (&type, compound_literal, false);
  compound_literal = digest_init (type, compound_literal);
  if ((!at_function_scope_p () || cp_type_readonly (type))
      && initializer_constant_valid_p (compound_literal, type))
    {
      tree decl = create_temporary_var (type);
      DECL_INITIAL (decl) = compound_literal;
      TREE_STATIC (decl) = 1;
      decl = pushdecl_top_level (decl);
      DECL_NAME (decl) = make_anon_name ();
      SET_DECL_ASSEMBLER_NAME (decl, DECL_NAME (decl));
      return decl;
    }
  else
    return get_target_expr (compound_literal);
}

/* Return the declaration for the function-name variable indicated by
   ID.  */

tree
finish_fname (tree id)
{
  tree decl;

  decl = fname_decl (input_location, C_RID_CODE (id), id);
  if (processing_template_decl)
    decl = DECL_NAME (decl);
  return decl;
}

/* Finish a translation unit.  */

void
finish_translation_unit (void)
{
  /* In case there were missing closebraces,
     get us back to the global binding level.  */
  pop_everything ();
  while (current_namespace != global_namespace)
    pop_namespace ();

  /* Do file scope __FUNCTION__ et al.  */
  finish_fname_decls ();
}

/* Finish a template type parameter, specified as AGGR IDENTIFIER.
   Returns the parameter.  */

tree
finish_template_type_parm (tree aggr, tree identifier)
{
  if (aggr != class_type_node)
    {
      permerror (input_location, "template type parameters must use the keyword %<class%> or %<typename%>");
      aggr = class_type_node;
    }

  return build_tree_list (aggr, identifier);
}

/* Finish a template template parameter, specified as AGGR IDENTIFIER.
   Returns the parameter.  */

tree
finish_template_template_parm (tree aggr, tree identifier)
{
  tree decl = build_decl (TYPE_DECL, identifier, NULL_TREE);
  tree tmpl = build_lang_decl (TEMPLATE_DECL, identifier, NULL_TREE);
  DECL_TEMPLATE_PARMS (tmpl) = current_template_parms;
  DECL_TEMPLATE_RESULT (tmpl) = decl;
  DECL_ARTIFICIAL (decl) = 1;
  end_template_decl ();

  gcc_assert (DECL_TEMPLATE_PARMS (tmpl));

  check_default_tmpl_args (decl, DECL_TEMPLATE_PARMS (tmpl), 
			   /*is_primary=*/true, /*is_partial=*/false,
			   /*is_friend=*/0);

  return finish_template_type_parm (aggr, tmpl);
}

/* ARGUMENT is the default-argument value for a template template
   parameter.  If ARGUMENT is invalid, issue error messages and return
   the ERROR_MARK_NODE.  Otherwise, ARGUMENT itself is returned.  */

tree
check_template_template_default_arg (tree argument)
{
  if (TREE_CODE (argument) != TEMPLATE_DECL
      && TREE_CODE (argument) != TEMPLATE_TEMPLATE_PARM
      && TREE_CODE (argument) != UNBOUND_CLASS_TEMPLATE)
    {
      if (TREE_CODE (argument) == TYPE_DECL)
	error ("invalid use of type %qT as a default value for a template "
	       "template-parameter", TREE_TYPE (argument));
      else
	error ("invalid default argument for a template template parameter");
      return error_mark_node;
    }

  return argument;
}

/* Begin a class definition, as indicated by T.  */

tree
begin_class_definition (tree t, tree attributes)
{
  if (error_operand_p (t) || error_operand_p (TYPE_MAIN_DECL (t)))
    return error_mark_node;

  if (processing_template_parmlist)
    {
      error ("definition of %q#T inside template parameter list", t);
      return error_mark_node;
    }
  /* A non-implicit typename comes from code like:

       template <typename T> struct A {
	 template <typename U> struct A<T>::B ...

     This is erroneous.  */
  else if (TREE_CODE (t) == TYPENAME_TYPE)
    {
      error ("invalid definition of qualified type %qT", t);
      t = error_mark_node;
    }

  if (t == error_mark_node || ! MAYBE_CLASS_TYPE_P (t))
    {
      t = make_class_type (RECORD_TYPE);
      pushtag (make_anon_name (), t, /*tag_scope=*/ts_current);
    }

  /* Update the location of the decl.  */
  DECL_SOURCE_LOCATION (TYPE_NAME (t)) = input_location;

  if (TYPE_BEING_DEFINED (t))
    {
      t = make_class_type (TREE_CODE (t));
      pushtag (TYPE_IDENTIFIER (t), t, /*tag_scope=*/ts_current);
    }
  maybe_process_partial_specialization (t);
  pushclass (t);
  TYPE_BEING_DEFINED (t) = 1;

  cplus_decl_attributes (&t, attributes, (int) ATTR_FLAG_TYPE_IN_PLACE);

  if (flag_pack_struct)
    {
      tree v;
      TYPE_PACKED (t) = 1;
      /* Even though the type is being defined for the first time
	 here, there might have been a forward declaration, so there
	 might be cv-qualified variants of T.  */
      for (v = TYPE_NEXT_VARIANT (t); v; v = TYPE_NEXT_VARIANT (v))
	TYPE_PACKED (v) = 1;
    }
  /* Reset the interface data, at the earliest possible
     moment, as it might have been set via a class foo;
     before.  */
  if (! TYPE_ANONYMOUS_P (t))
    {
      struct c_fileinfo *finfo = get_fileinfo (input_filename);
      CLASSTYPE_INTERFACE_ONLY (t) = finfo->interface_only;
      SET_CLASSTYPE_INTERFACE_UNKNOWN_X
	(t, finfo->interface_unknown);
    }
  reset_specialization();

  /* Make a declaration for this class in its own scope.  */
  build_self_reference ();

  return t;
}

/* Finish the member declaration given by DECL.  */

void
finish_member_declaration (tree decl)
{
  if (decl == error_mark_node || decl == NULL_TREE)
    return;

  if (decl == void_type_node)
    /* The COMPONENT was a friend, not a member, and so there's
       nothing for us to do.  */
    return;

  /* We should see only one DECL at a time.  */
  gcc_assert (TREE_CHAIN (decl) == NULL_TREE);

  /* Set up access control for DECL.  */
  TREE_PRIVATE (decl)
    = (current_access_specifier == access_private_node);
  TREE_PROTECTED (decl)
    = (current_access_specifier == access_protected_node);
  if (TREE_CODE (decl) == TEMPLATE_DECL)
    {
      TREE_PRIVATE (DECL_TEMPLATE_RESULT (decl)) = TREE_PRIVATE (decl);
      TREE_PROTECTED (DECL_TEMPLATE_RESULT (decl)) = TREE_PROTECTED (decl);
    }

  /* Mark the DECL as a member of the current class.  */
  DECL_CONTEXT (decl) = current_class_type;

  /* Check for bare parameter packs in the member variable declaration.  */
  if (TREE_CODE (decl) == FIELD_DECL)
    {
      if (check_for_bare_parameter_packs (TREE_TYPE (decl)))
        TREE_TYPE (decl) = error_mark_node;
      if (check_for_bare_parameter_packs (DECL_ATTRIBUTES (decl)))
        DECL_ATTRIBUTES (decl) = NULL_TREE;
    }

  /* [dcl.link]

     A C language linkage is ignored for the names of class members
     and the member function type of class member functions.  */
  if (DECL_LANG_SPECIFIC (decl) && DECL_LANGUAGE (decl) == lang_c)
    SET_DECL_LANGUAGE (decl, lang_cplusplus);

  /* Put functions on the TYPE_METHODS list and everything else on the
     TYPE_FIELDS list.  Note that these are built up in reverse order.
     We reverse them (to obtain declaration order) in finish_struct.  */
  if (TREE_CODE (decl) == FUNCTION_DECL
      || DECL_FUNCTION_TEMPLATE_P (decl))
    {
      /* We also need to add this function to the
	 CLASSTYPE_METHOD_VEC.  */
      if (add_method (current_class_type, decl, NULL_TREE))
	{
	  TREE_CHAIN (decl) = TYPE_METHODS (current_class_type);
	  TYPE_METHODS (current_class_type) = decl;

	  maybe_add_class_template_decl_list (current_class_type, decl,
					      /*friend_p=*/0);
	}
    }
  /* Enter the DECL into the scope of the class.  */
  else if ((TREE_CODE (decl) == USING_DECL && !DECL_DEPENDENT_P (decl))
	   || pushdecl_class_level (decl))
    {
      /* All TYPE_DECLs go at the end of TYPE_FIELDS.  Ordinary fields
	 go at the beginning.  The reason is that lookup_field_1
	 searches the list in order, and we want a field name to
	 override a type name so that the "struct stat hack" will
	 work.  In particular:

	   struct S { enum E { }; int E } s;
	   s.E = 3;

	 is valid.  In addition, the FIELD_DECLs must be maintained in
	 declaration order so that class layout works as expected.
	 However, we don't need that order until class layout, so we
	 save a little time by putting FIELD_DECLs on in reverse order
	 here, and then reversing them in finish_struct_1.  (We could
	 also keep a pointer to the correct insertion points in the
	 list.)  */

      if (TREE_CODE (decl) == TYPE_DECL)
	TYPE_FIELDS (current_class_type)
	  = chainon (TYPE_FIELDS (current_class_type), decl);
      else
	{
	  TREE_CHAIN (decl) = TYPE_FIELDS (current_class_type);
	  TYPE_FIELDS (current_class_type) = decl;
	}

      maybe_add_class_template_decl_list (current_class_type, decl,
					  /*friend_p=*/0);
    }

  if (pch_file)
    note_decl_for_pch (decl);
}

/* DECL has been declared while we are building a PCH file.  Perform
   actions that we might normally undertake lazily, but which can be
   performed now so that they do not have to be performed in
   translation units which include the PCH file.  */

void
note_decl_for_pch (tree decl)
{
  gcc_assert (pch_file);

  /* There's a good chance that we'll have to mangle names at some
     point, even if only for emission in debugging information.  */
  if ((TREE_CODE (decl) == VAR_DECL
       || TREE_CODE (decl) == FUNCTION_DECL)
      && !processing_template_decl)
    mangle_decl (decl);
}

/* Finish processing a complete template declaration.  The PARMS are
   the template parameters.  */

void
finish_template_decl (tree parms)
{
  if (parms)
    end_template_decl ();
  else
    end_specialization ();
}

/* Finish processing a template-id (which names a type) of the form
   NAME < ARGS >.  Return the TYPE_DECL for the type named by the
   template-id.  If ENTERING_SCOPE is nonzero we are about to enter
   the scope of template-id indicated.  */

tree
finish_template_type (tree name, tree args, int entering_scope)
{
  tree decl;

  decl = lookup_template_class (name, args,
				NULL_TREE, NULL_TREE, entering_scope,
				tf_warning_or_error | tf_user);
  if (decl != error_mark_node)
    decl = TYPE_STUB_DECL (decl);

  return decl;
}

/* Finish processing a BASE_CLASS with the indicated ACCESS_SPECIFIER.
   Return a TREE_LIST containing the ACCESS_SPECIFIER and the
   BASE_CLASS, or NULL_TREE if an error occurred.  The
   ACCESS_SPECIFIER is one of
   access_{default,public,protected_private}_node.  For a virtual base
   we set TREE_TYPE.  */

tree
finish_base_specifier (tree base, tree access, bool virtual_p)
{
  tree result;

  if (base == error_mark_node)
    {
      error ("invalid base-class specification");
      result = NULL_TREE;
    }
  else if (! MAYBE_CLASS_TYPE_P (base))
    {
      error ("%qT is not a class type", base);
      result = NULL_TREE;
    }
  else
    {
      if (cp_type_quals (base) != 0)
	{
	  error ("base class %qT has cv qualifiers", base);
	  base = TYPE_MAIN_VARIANT (base);
	}
      result = build_tree_list (access, base);
      if (virtual_p)
	TREE_TYPE (result) = integer_type_node;
    }

  return result;
}

/* Issue a diagnostic that NAME cannot be found in SCOPE.  DECL is
   what we found when we tried to do the lookup.
   LOCATION is the location of the NAME identifier;
   The location is used in the error message*/

void
qualified_name_lookup_error (tree scope, tree name,
			     tree decl, location_t location)
{
  if (scope == error_mark_node)
    ; /* We already complained.  */
  else if (TYPE_P (scope))
    {
      if (!COMPLETE_TYPE_P (scope))
	error ("%Hincomplete type %qT used in nested name specifier",
	       &location, scope);
      else if (TREE_CODE (decl) == TREE_LIST)
	{
	  error ("%Hreference to %<%T::%D%> is ambiguous", &location, scope, name);
	  print_candidates (decl);
	}
      else
	error ("%H%qD is not a member of %qT", &location, name, scope);
    }
  else if (scope != global_namespace)
    error ("%H%qD is not a member of %qD", &location, name, scope);
  else
    error ("%H%<::%D%> has not been declared", &location, name);
}

/* If FNS is a member function, a set of member functions, or a
   template-id referring to one or more member functions, return a
   BASELINK for FNS, incorporating the current access context.
   Otherwise, return FNS unchanged.  */

tree
baselink_for_fns (tree fns)
{
  tree fn;
  tree cl;

  if (BASELINK_P (fns) 
      || error_operand_p (fns))
    return fns;
  
  fn = fns;
  if (TREE_CODE (fn) == TEMPLATE_ID_EXPR)
    fn = TREE_OPERAND (fn, 0);
  fn = get_first_fn (fn);
  if (!DECL_FUNCTION_MEMBER_P (fn))
    return fns;

  cl = currently_open_derived_class (DECL_CONTEXT (fn));
  if (!cl)
    cl = DECL_CONTEXT (fn);
  cl = TYPE_BINFO (cl);
  return build_baselink (cl, cl, fns, /*optype=*/NULL_TREE);
}

/* ID_EXPRESSION is a representation of parsed, but unprocessed,
   id-expression.  (See cp_parser_id_expression for details.)  SCOPE,
   if non-NULL, is the type or namespace used to explicitly qualify
   ID_EXPRESSION.  DECL is the entity to which that name has been
   resolved.

   *CONSTANT_EXPRESSION_P is true if we are presently parsing a
   constant-expression.  In that case, *NON_CONSTANT_EXPRESSION_P will
   be set to true if this expression isn't permitted in a
   constant-expression, but it is otherwise not set by this function.
   *ALLOW_NON_CONSTANT_EXPRESSION_P is true if we are parsing a
   constant-expression, but a non-constant expression is also
   permissible.

   DONE is true if this expression is a complete postfix-expression;
   it is false if this expression is followed by '->', '[', '(', etc.
   ADDRESS_P is true iff this expression is the operand of '&'.
   TEMPLATE_P is true iff the qualified-id was of the form
   "A::template B".  TEMPLATE_ARG_P is true iff this qualified name
   appears as a template argument.

   If an error occurs, and it is the kind of error that might cause
   the parser to abort a tentative parse, *ERROR_MSG is filled in.  It
   is the caller's responsibility to issue the message.  *ERROR_MSG
   will be a string with static storage duration, so the caller need
   not "free" it.

   Return an expression for the entity, after issuing appropriate
   diagnostics.  This function is also responsible for transforming a
   reference to a non-static member into a COMPONENT_REF that makes
   the use of "this" explicit.

   Upon return, *IDK will be filled in appropriately.  */
tree
finish_id_expression (tree id_expression,
		      tree decl,
		      tree scope,
		      cp_id_kind *idk,
		      bool integral_constant_expression_p,
		      bool allow_non_integral_constant_expression_p,
		      bool *non_integral_constant_expression_p,
		      bool template_p,
		      bool done,
		      bool address_p,
		      bool template_arg_p,
		      const char **error_msg,
		      location_t location)
{
  /* Initialize the output parameters.  */
  *idk = CP_ID_KIND_NONE;
  *error_msg = NULL;

  if (id_expression == error_mark_node)
    return error_mark_node;
  /* If we have a template-id, then no further lookup is
     required.  If the template-id was for a template-class, we
     will sometimes have a TYPE_DECL at this point.  */
  else if (TREE_CODE (decl) == TEMPLATE_ID_EXPR
	   || TREE_CODE (decl) == TYPE_DECL)
    ;
  /* Look up the name.  */
  else
    {
      if (decl == error_mark_node)
	{
	  /* Name lookup failed.  */
	  if (scope
	      && (!TYPE_P (scope)
		  || (!dependent_type_p (scope)
		      && !(TREE_CODE (id_expression) == IDENTIFIER_NODE
			   && IDENTIFIER_TYPENAME_P (id_expression)
			   && dependent_type_p (TREE_TYPE (id_expression))))))
	    {
	      /* If the qualifying type is non-dependent (and the name
		 does not name a conversion operator to a dependent
		 type), issue an error.  */
	      qualified_name_lookup_error (scope, id_expression, decl, location);
	      return error_mark_node;
	    }
	  else if (!scope)
	    {
	      /* It may be resolved via Koenig lookup.  */
	      *idk = CP_ID_KIND_UNQUALIFIED;
	      return id_expression;
	    }
	  else
	    decl = id_expression;
	}
      /* If DECL is a variable that would be out of scope under
	 ANSI/ISO rules, but in scope in the ARM, name lookup
	 will succeed.  Issue a diagnostic here.  */
      else
	decl = check_for_out_of_scope_variable (decl);

      /* Remember that the name was used in the definition of
	 the current class so that we can check later to see if
	 the meaning would have been different after the class
	 was entirely defined.  */
      if (!scope && decl != error_mark_node)
	maybe_note_name_used_in_class (id_expression, decl);

      /* Disallow uses of local variables from containing functions.  */
      if (TREE_CODE (decl) == VAR_DECL || TREE_CODE (decl) == PARM_DECL)
	{
	  tree context = decl_function_context (decl);
	  if (context != NULL_TREE && context != current_function_decl
	      && ! TREE_STATIC (decl))
	    {
	      error (TREE_CODE (decl) == VAR_DECL
		     ? "use of %<auto%> variable from containing function"
		     : "use of parameter from containing function");
	      error ("  %q+#D declared here", decl);
	      return error_mark_node;
	    }
	}
    }

  /* If we didn't find anything, or what we found was a type,
     then this wasn't really an id-expression.  */
  if (TREE_CODE (decl) == TEMPLATE_DECL
      && !DECL_FUNCTION_TEMPLATE_P (decl))
    {
      *error_msg = "missing template arguments";
      return error_mark_node;
    }
  else if (TREE_CODE (decl) == TYPE_DECL
	   || TREE_CODE (decl) == NAMESPACE_DECL)
    {
      *error_msg = "expected primary-expression";
      return error_mark_node;
    }

  /* If the name resolved to a template parameter, there is no
     need to look it up again later.  */
  if ((TREE_CODE (decl) == CONST_DECL && DECL_TEMPLATE_PARM_P (decl))
      || TREE_CODE (decl) == TEMPLATE_PARM_INDEX)
    {
      tree r;

      *idk = CP_ID_KIND_NONE;
      if (TREE_CODE (decl) == TEMPLATE_PARM_INDEX)
	decl = TEMPLATE_PARM_DECL (decl);
      r = convert_from_reference (DECL_INITIAL (decl));

      if (integral_constant_expression_p
	  && !dependent_type_p (TREE_TYPE (decl))
	  && !(INTEGRAL_OR_ENUMERATION_TYPE_P (TREE_TYPE (r))))
	{
	  if (!allow_non_integral_constant_expression_p)
	    error ("template parameter %qD of type %qT is not allowed in "
		   "an integral constant expression because it is not of "
		   "integral or enumeration type", decl, TREE_TYPE (decl));
	  *non_integral_constant_expression_p = true;
	}
      return r;
    }
  /* Similarly, we resolve enumeration constants to their
     underlying values.  */
  else if (TREE_CODE (decl) == CONST_DECL)
    {
      *idk = CP_ID_KIND_NONE;
      if (!processing_template_decl)
	{
	  used_types_insert (TREE_TYPE (decl));
	  return DECL_INITIAL (decl);
	}
      return decl;
    }
  else
    {
      bool dependent_p;

      /* If the declaration was explicitly qualified indicate
	 that.  The semantics of `A::f(3)' are different than
	 `f(3)' if `f' is virtual.  */
      *idk = (scope
	      ? CP_ID_KIND_QUALIFIED
	      : (TREE_CODE (decl) == TEMPLATE_ID_EXPR
		 ? CP_ID_KIND_TEMPLATE_ID
		 : CP_ID_KIND_UNQUALIFIED));


      /* [temp.dep.expr]

	 An id-expression is type-dependent if it contains an
	 identifier that was declared with a dependent type.

	 The standard is not very specific about an id-expression that
	 names a set of overloaded functions.  What if some of them
	 have dependent types and some of them do not?  Presumably,
	 such a name should be treated as a dependent name.  */
      /* Assume the name is not dependent.  */
      dependent_p = false;
      if (!processing_template_decl)
	/* No names are dependent outside a template.  */
	;
      /* A template-id where the name of the template was not resolved
	 is definitely dependent.  */
      else if (TREE_CODE (decl) == TEMPLATE_ID_EXPR
	       && (TREE_CODE (TREE_OPERAND (decl, 0))
		   == IDENTIFIER_NODE))
	dependent_p = true;
      /* For anything except an overloaded function, just check its
	 type.  */
      else if (!is_overloaded_fn (decl))
	dependent_p
	  = dependent_type_p (TREE_TYPE (decl));
      /* For a set of overloaded functions, check each of the
	 functions.  */
      else
	{
	  tree fns = decl;

	  if (BASELINK_P (fns))
	    fns = BASELINK_FUNCTIONS (fns);

	  /* For a template-id, check to see if the template
	     arguments are dependent.  */
	  if (TREE_CODE (fns) == TEMPLATE_ID_EXPR)
	    {
	      tree args = TREE_OPERAND (fns, 1);
	      dependent_p = any_dependent_template_arguments_p (args);
	      /* The functions are those referred to by the
		 template-id.  */
	      fns = TREE_OPERAND (fns, 0);
	    }

	  /* If there are no dependent template arguments, go through
	     the overloaded functions.  */
	  while (fns && !dependent_p)
	    {
	      tree fn = OVL_CURRENT (fns);

	      /* Member functions of dependent classes are
		 dependent.  */
	      if (TREE_CODE (fn) == FUNCTION_DECL
		  && type_dependent_expression_p (fn))
		dependent_p = true;
	      else if (TREE_CODE (fn) == TEMPLATE_DECL
		       && dependent_template_p (fn))
		dependent_p = true;

	      fns = OVL_NEXT (fns);
	    }
	}

      /* If the name was dependent on a template parameter, we will
	 resolve the name at instantiation time.  */
      if (dependent_p)
	{
	  /* Create a SCOPE_REF for qualified names, if the scope is
	     dependent.  */
	  if (scope)
	    {
	      if (TYPE_P (scope))
		{
		  if (address_p && done)
		    decl = finish_qualified_id_expr (scope, decl,
						     done, address_p,
						     template_p,
						     template_arg_p);
		  else
		    {
		      tree type = NULL_TREE;
		      if (DECL_P (decl) && !dependent_scope_p (scope))
			type = TREE_TYPE (decl);
		      decl = build_qualified_name (type,
						   scope,
						   id_expression,
						   template_p);
		    }
		}
	      if (TREE_TYPE (decl))
		decl = convert_from_reference (decl);
	      return decl;
	    }
	  /* A TEMPLATE_ID already contains all the information we
	     need.  */
	  if (TREE_CODE (id_expression) == TEMPLATE_ID_EXPR)
	    return id_expression;
	  *idk = CP_ID_KIND_UNQUALIFIED_DEPENDENT;
	  /* If we found a variable, then name lookup during the
	     instantiation will always resolve to the same VAR_DECL
	     (or an instantiation thereof).  */
	  if (TREE_CODE (decl) == VAR_DECL
	      || TREE_CODE (decl) == PARM_DECL)
	    return convert_from_reference (decl);
	  /* The same is true for FIELD_DECL, but we also need to
	     make sure that the syntax is correct.  */
	  else if (TREE_CODE (decl) == FIELD_DECL)
	    {
	      /* Since SCOPE is NULL here, this is an unqualified name.
		 Access checking has been performed during name lookup
		 already.  Turn off checking to avoid duplicate errors.  */
	      push_deferring_access_checks (dk_no_check);
	      decl = finish_non_static_data_member
		       (decl, current_class_ref,
			/*qualifying_scope=*/NULL_TREE);
	      pop_deferring_access_checks ();
	      return decl;
	    }
	  return id_expression;
	}

      /* Only certain kinds of names are allowed in constant
	 expression.  Enumerators and template parameters have already
	 been handled above.  */
      if (integral_constant_expression_p
	  && ! DECL_INTEGRAL_CONSTANT_VAR_P (decl)
	  && ! builtin_valid_in_constant_expr_p (decl))
	{
	  if (!allow_non_integral_constant_expression_p)
	    {
	      error ("%qD cannot appear in a constant-expression", decl);
	      return error_mark_node;
	    }
	  *non_integral_constant_expression_p = true;
	}

      if (TREE_CODE (decl) == NAMESPACE_DECL)
	{
	  error ("use of namespace %qD as expression", decl);
	  return error_mark_node;
	}
      else if (DECL_CLASS_TEMPLATE_P (decl))
	{
	  error ("use of class template %qT as expression", decl);
	  return error_mark_node;
	}
      else if (TREE_CODE (decl) == TREE_LIST)
	{
	  /* Ambiguous reference to base members.  */
	  error ("request for member %qD is ambiguous in "
		 "multiple inheritance lattice", id_expression);
	  print_candidates (decl);
	  return error_mark_node;
	}

      /* Mark variable-like entities as used.  Functions are similarly
	 marked either below or after overload resolution.  */
      if (TREE_CODE (decl) == VAR_DECL
	  || TREE_CODE (decl) == PARM_DECL
	  || TREE_CODE (decl) == RESULT_DECL)
	mark_used (decl);

      if (scope)
	{
	  decl = (adjust_result_of_qualified_name_lookup
		  (decl, scope, current_class_type));

	  if (TREE_CODE (decl) == FUNCTION_DECL)
	    mark_used (decl);

	  if (TREE_CODE (decl) == FIELD_DECL || BASELINK_P (decl))
	    decl = finish_qualified_id_expr (scope,
					     decl,
					     done,
					     address_p,
					     template_p,
					     template_arg_p);
	  else
	    {
	      tree r = convert_from_reference (decl);

	      if (processing_template_decl && TYPE_P (scope))
		r = build_qualified_name (TREE_TYPE (r),
					  scope, decl,
					  template_p);
	      decl = r;
	    }
	}
      else if (TREE_CODE (decl) == FIELD_DECL)
	{
	  /* Since SCOPE is NULL here, this is an unqualified name.
	     Access checking has been performed during name lookup
	     already.  Turn off checking to avoid duplicate errors.  */
	  push_deferring_access_checks (dk_no_check);
	  decl = finish_non_static_data_member (decl, current_class_ref,
						/*qualifying_scope=*/NULL_TREE);
	  pop_deferring_access_checks ();
	}
      else if (is_overloaded_fn (decl))
	{
	  tree first_fn;

	  first_fn = decl;
	  if (TREE_CODE (first_fn) == TEMPLATE_ID_EXPR)
	    first_fn = TREE_OPERAND (first_fn, 0);
	  first_fn = get_first_fn (first_fn);
	  if (TREE_CODE (first_fn) == TEMPLATE_DECL)
	    first_fn = DECL_TEMPLATE_RESULT (first_fn);

	  if (!really_overloaded_fn (decl))
	    mark_used (first_fn);

	  if (!template_arg_p
	      && TREE_CODE (first_fn) == FUNCTION_DECL
	      && DECL_FUNCTION_MEMBER_P (first_fn)
	      && !shared_member_p (decl))
	    {
	      /* A set of member functions.  */
	      decl = maybe_dummy_object (DECL_CONTEXT (first_fn), 0);
	      return finish_class_member_access_expr (decl, id_expression,
						      /*template_p=*/false,
						      tf_warning_or_error);
	    }

	  decl = baselink_for_fns (decl);
	}
      else
	{
	  if (DECL_P (decl) && DECL_NONLOCAL (decl)
	      && DECL_CLASS_SCOPE_P (decl))
	    {
	      tree context = context_for_name_lookup (decl); 
	      if (context != current_class_type)
		{
		  tree path = currently_open_derived_class (context);
		  perform_or_defer_access_check (TYPE_BINFO (path),
						 decl, decl);
		}
	    }

	  decl = convert_from_reference (decl);
	}
    }

  if (TREE_DEPRECATED (decl))
    warn_deprecated_use (decl);

  return decl;
}

/* Implement the __typeof keyword: Return the type of EXPR, suitable for
   use as a type-specifier.  */

tree
finish_typeof (tree expr)
{
  tree type;

  if (type_dependent_expression_p (expr))
    {
      type = cxx_make_type (TYPEOF_TYPE);
      TYPEOF_TYPE_EXPR (type) = expr;
      SET_TYPE_STRUCTURAL_EQUALITY (type);

      return type;
    }

  type = unlowered_expr_type (expr);

  if (!type || type == unknown_type_node)
    {
      error ("type of %qE is unknown", expr);
      return error_mark_node;
    }

  return type;
}

/* Perform C++-specific checks for __builtin_offsetof before calling
   fold_offsetof.  */

tree
finish_offsetof (tree expr)
{
  if (TREE_CODE (expr) == PSEUDO_DTOR_EXPR)
    {
      error ("cannot apply %<offsetof%> to destructor %<~%T%>",
	      TREE_OPERAND (expr, 2));
      return error_mark_node;
    }
  if (TREE_CODE (TREE_TYPE (expr)) == FUNCTION_TYPE
      || TREE_CODE (TREE_TYPE (expr)) == METHOD_TYPE
      || TREE_CODE (TREE_TYPE (expr)) == UNKNOWN_TYPE)
    {
      if (TREE_CODE (expr) == COMPONENT_REF
	  || TREE_CODE (expr) == COMPOUND_EXPR)
	expr = TREE_OPERAND (expr, 1);
      error ("cannot apply %<offsetof%> to member function %qD", expr);
      return error_mark_node;
    }
  if (TREE_CODE (expr) == INDIRECT_REF && REFERENCE_REF_P (expr))
    expr = TREE_OPERAND (expr, 0);
  return fold_offsetof (expr, NULL_TREE);
}

/* Replace the AGGR_INIT_EXPR at *TP with an equivalent CALL_EXPR.  This
   function is broken out from the above for the benefit of the tree-ssa
   project.  */

void
simplify_aggr_init_expr (tree *tp)
{
  tree aggr_init_expr = *tp;

  /* Form an appropriate CALL_EXPR.  */
  tree fn = AGGR_INIT_EXPR_FN (aggr_init_expr);
  tree slot = AGGR_INIT_EXPR_SLOT (aggr_init_expr);
  tree type = TREE_TYPE (slot);

  tree call_expr;
  enum style_t { ctor, arg, pcc } style;

  if (AGGR_INIT_VIA_CTOR_P (aggr_init_expr))
    style = ctor;
#ifdef PCC_STATIC_STRUCT_RETURN
  else if (1)
    style = pcc;
#endif
  else
    {
      gcc_assert (TREE_ADDRESSABLE (type));
      style = arg;
    }

  call_expr = build_call_array (TREE_TYPE (TREE_TYPE (TREE_TYPE (fn))),
				fn,
				aggr_init_expr_nargs (aggr_init_expr),
				AGGR_INIT_EXPR_ARGP (aggr_init_expr));

  if (style == ctor)
    {
      /* Replace the first argument to the ctor with the address of the
	 slot.  */
      cxx_mark_addressable (slot);
      CALL_EXPR_ARG (call_expr, 0) =
	build1 (ADDR_EXPR, build_pointer_type (type), slot);
    }
  else if (style == arg)
    {
      /* Just mark it addressable here, and leave the rest to
	 expand_call{,_inline}.  */
      cxx_mark_addressable (slot);
      CALL_EXPR_RETURN_SLOT_OPT (call_expr) = true;
      call_expr = build2 (MODIFY_EXPR, TREE_TYPE (call_expr), slot, call_expr);
    }
  else if (style == pcc)
    {
      /* If we're using the non-reentrant PCC calling convention, then we
	 need to copy the returned value out of the static buffer into the
	 SLOT.  */
      push_deferring_access_checks (dk_no_check);
      call_expr = build_aggr_init (slot, call_expr,
				   DIRECT_BIND | LOOKUP_ONLYCONVERTING,
                                   tf_warning_or_error);
      pop_deferring_access_checks ();
      call_expr = build2 (COMPOUND_EXPR, TREE_TYPE (slot), call_expr, slot);
    }

  if (AGGR_INIT_ZERO_FIRST (aggr_init_expr))
    {
      tree init = build_zero_init (type, NULL_TREE,
				   /*static_storage_p=*/false);
      init = build2 (INIT_EXPR, void_type_node, slot, init);
      call_expr = build2 (COMPOUND_EXPR, TREE_TYPE (call_expr),
			  init, call_expr);
    }

  *tp = call_expr;
}

/* Emit all thunks to FN that should be emitted when FN is emitted.  */

void
emit_associated_thunks (tree fn)
{
  /* When we use vcall offsets, we emit thunks with the virtual
     functions to which they thunk. The whole point of vcall offsets
     is so that you can know statically the entire set of thunks that
     will ever be needed for a given virtual function, thereby
     enabling you to output all the thunks with the function itself.  */
  if (DECL_VIRTUAL_P (fn))
    {
      tree thunk;

      for (thunk = DECL_THUNKS (fn); thunk; thunk = TREE_CHAIN (thunk))
	{
	  if (!THUNK_ALIAS (thunk))
	    {
	      use_thunk (thunk, /*emit_p=*/1);
	      if (DECL_RESULT_THUNK_P (thunk))
		{
		  tree probe;

		  for (probe = DECL_THUNKS (thunk);
		       probe; probe = TREE_CHAIN (probe))
		    use_thunk (probe, /*emit_p=*/1);
		}
	    }
	  else
	    gcc_assert (!DECL_THUNKS (thunk));
	}
    }
}

/* Generate RTL for FN.  */

void
expand_or_defer_fn (tree fn)
{
  /* When the parser calls us after finishing the body of a template
     function, we don't really want to expand the body.  */
  if (processing_template_decl)
    {
      /* Normally, collection only occurs in rest_of_compilation.  So,
	 if we don't collect here, we never collect junk generated
	 during the processing of templates until we hit a
	 non-template function.  It's not safe to do this inside a
	 nested class, though, as the parser may have local state that
	 is not a GC root.  */
      if (!function_depth)
	ggc_collect ();
      return;
    }

  gcc_assert (gimple_body (fn));

  /* If this is a constructor or destructor body, we have to clone
     it.  */
  if (maybe_clone_body (fn))
    {
      /* We don't want to process FN again, so pretend we've written
	 it out, even though we haven't.  */
      TREE_ASM_WRITTEN (fn) = 1;
      return;
    }

  /* We make a decision about linkage for these functions at the end
     of the compilation.  Until that point, we do not want the back
     end to output them -- but we do want it to see the bodies of
     these functions so that it can inline them as appropriate.  */
  if (DECL_DECLARED_INLINE_P (fn) || DECL_IMPLICIT_INSTANTIATION (fn))
    {
      if (DECL_INTERFACE_KNOWN (fn))
	/* We've already made a decision as to how this function will
	   be handled.  */;
      else if (!at_eof)
	{
	  DECL_EXTERNAL (fn) = 1;
	  DECL_NOT_REALLY_EXTERN (fn) = 1;
	  note_vague_linkage_fn (fn);
	  /* A non-template inline function with external linkage will
	     always be COMDAT.  As we must eventually determine the
	     linkage of all functions, and as that causes writes to
	     the data mapped in from the PCH file, it's advantageous
	     to mark the functions at this point.  */
	  if (!DECL_IMPLICIT_INSTANTIATION (fn))
	    {
	      /* This function must have external linkage, as
		 otherwise DECL_INTERFACE_KNOWN would have been
		 set.  */
	      gcc_assert (TREE_PUBLIC (fn));
	      comdat_linkage (fn);
	      DECL_INTERFACE_KNOWN (fn) = 1;
	    }
	}
      else
	import_export_decl (fn);

      /* If the user wants us to keep all inline functions, then mark
	 this function as needed so that finish_file will make sure to
	 output it later.  */
      if (flag_keep_inline_functions && DECL_DECLARED_INLINE_P (fn))
	mark_needed (fn);
    }

  /* There's no reason to do any of the work here if we're only doing
     semantic analysis; this code just generates RTL.  */
  if (flag_syntax_only)
    return;

  function_depth++;

  /* Expand or defer, at the whim of the compilation unit manager.  */
  cgraph_finalize_function (fn, function_depth > 1);

  function_depth--;
}

struct nrv_data
{
  tree var;
  tree result;
  htab_t visited;
};

/* Helper function for walk_tree, used by finalize_nrv below.  */

static tree
finalize_nrv_r (tree* tp, int* walk_subtrees, void* data)
{
  struct nrv_data *dp = (struct nrv_data *)data;
  void **slot;

  /* No need to walk into types.  There wouldn't be any need to walk into
     non-statements, except that we have to consider STMT_EXPRs.  */
  if (TYPE_P (*tp))
    *walk_subtrees = 0;
  /* Change all returns to just refer to the RESULT_DECL; this is a nop,
     but differs from using NULL_TREE in that it indicates that we care
     about the value of the RESULT_DECL.  */
  else if (TREE_CODE (*tp) == RETURN_EXPR)
    TREE_OPERAND (*tp, 0) = dp->result;
  /* Change all cleanups for the NRV to only run when an exception is
     thrown.  */
  else if (TREE_CODE (*tp) == CLEANUP_STMT
	   && CLEANUP_DECL (*tp) == dp->var)
    CLEANUP_EH_ONLY (*tp) = 1;
  /* Replace the DECL_EXPR for the NRV with an initialization of the
     RESULT_DECL, if needed.  */
  else if (TREE_CODE (*tp) == DECL_EXPR
	   && DECL_EXPR_DECL (*tp) == dp->var)
    {
      tree init;
      if (DECL_INITIAL (dp->var)
	  && DECL_INITIAL (dp->var) != error_mark_node)
	init = build2 (INIT_EXPR, void_type_node, dp->result,
		       DECL_INITIAL (dp->var));
      else
	init = build_empty_stmt ();
      DECL_INITIAL (dp->var) = NULL_TREE;
      SET_EXPR_LOCUS (init, EXPR_LOCUS (*tp));
      *tp = init;
    }
  /* And replace all uses of the NRV with the RESULT_DECL.  */
  else if (*tp == dp->var)
    *tp = dp->result;

  /* Avoid walking into the same tree more than once.  Unfortunately, we
     can't just use walk_tree_without duplicates because it would only call
     us for the first occurrence of dp->var in the function body.  */
  slot = htab_find_slot (dp->visited, *tp, INSERT);
  if (*slot)
    *walk_subtrees = 0;
  else
    *slot = *tp;

  /* Keep iterating.  */
  return NULL_TREE;
}

/* Called from finish_function to implement the named return value
   optimization by overriding all the RETURN_EXPRs and pertinent
   CLEANUP_STMTs and replacing all occurrences of VAR with RESULT, the
   RESULT_DECL for the function.  */

void
finalize_nrv (tree *tp, tree var, tree result)
{
  struct nrv_data data;

  /* Copy debugging information from VAR to RESULT.  */
  DECL_NAME (result) = DECL_NAME (var);
  DECL_ARTIFICIAL (result) = DECL_ARTIFICIAL (var);
  DECL_IGNORED_P (result) = DECL_IGNORED_P (var);
  DECL_SOURCE_LOCATION (result) = DECL_SOURCE_LOCATION (var);
  DECL_ABSTRACT_ORIGIN (result) = DECL_ABSTRACT_ORIGIN (var);
  /* Don't forget that we take its address.  */
  TREE_ADDRESSABLE (result) = TREE_ADDRESSABLE (var);

  data.var = var;
  data.result = result;
  data.visited = htab_create (37, htab_hash_pointer, htab_eq_pointer, NULL);
  cp_walk_tree (tp, finalize_nrv_r, &data, 0);
  htab_delete (data.visited);
}

/* Return the declaration for the function called by CALL_EXPR T,
   TYPE is the class type of the clause decl.  */

static tree
omp_clause_info_fndecl (tree t, tree type)
{
  tree ret = get_callee_fndecl (t);

  if (ret)
    return ret;

  gcc_assert (TREE_CODE (t) == CALL_EXPR);
  t = CALL_EXPR_FN (t);
  STRIP_NOPS (t);
  if (TREE_CODE (t) == OBJ_TYPE_REF)
    {
      t = cp_fold_obj_type_ref (t, type);
      if (TREE_CODE (t) == ADDR_EXPR
	  && TREE_CODE (TREE_OPERAND (t, 0)) == FUNCTION_DECL)
	return TREE_OPERAND (t, 0);
    }

  return NULL_TREE;
}

/* Create CP_OMP_CLAUSE_INFO for clause C.  Returns true if it is invalid.  */

bool
cxx_omp_create_clause_info (tree c, tree type, bool need_default_ctor,
			    bool need_copy_ctor, bool need_copy_assignment)
{
  int save_errorcount = errorcount;
  tree info, t;

  /* Always allocate 3 elements for simplicity.  These are the
     function decls for the ctor, dtor, and assignment op.
     This layout is known to the three lang hooks,
     cxx_omp_clause_default_init, cxx_omp_clause_copy_init,
     and cxx_omp_clause_assign_op.  */
  info = make_tree_vec (3);
  CP_OMP_CLAUSE_INFO (c) = info;

  if (need_default_ctor
      || (need_copy_ctor && !TYPE_HAS_TRIVIAL_INIT_REF (type)))
    {
      if (need_default_ctor)
	t = NULL;
      else
	{
	  t = build_int_cst (build_pointer_type (type), 0);
	  t = build1 (INDIRECT_REF, type, t);
	  t = build_tree_list (NULL, t);
	}
      t = build_special_member_call (NULL_TREE, complete_ctor_identifier,
				     t, type, LOOKUP_NORMAL,
				     tf_warning_or_error);

      if (targetm.cxx.cdtor_returns_this () || errorcount)
	/* Because constructors and destructors return this,
	   the call will have been cast to "void".  Remove the
	   cast here.  We would like to use STRIP_NOPS, but it
	   wouldn't work here because TYPE_MODE (t) and
	   TYPE_MODE (TREE_OPERAND (t, 0)) are different.
	   They are VOIDmode and Pmode, respectively.  */
	if (TREE_CODE (t) == NOP_EXPR)
	  t = TREE_OPERAND (t, 0);

      TREE_VEC_ELT (info, 0) = get_callee_fndecl (t);
    }

  if ((need_default_ctor || need_copy_ctor)
      && TYPE_HAS_NONTRIVIAL_DESTRUCTOR (type))
    {
      t = build_int_cst (build_pointer_type (type), 0);
      t = build1 (INDIRECT_REF, type, t);
      t = build_special_member_call (t, complete_dtor_identifier,
				     NULL, type, LOOKUP_NORMAL,
				     tf_warning_or_error);

      if (targetm.cxx.cdtor_returns_this () || errorcount)
	/* Because constructors and destructors return this,
	   the call will have been cast to "void".  Remove the
	   cast here.  We would like to use STRIP_NOPS, but it
	   wouldn't work here because TYPE_MODE (t) and
	   TYPE_MODE (TREE_OPERAND (t, 0)) are different.
	   They are VOIDmode and Pmode, respectively.  */
	if (TREE_CODE (t) == NOP_EXPR)
	  t = TREE_OPERAND (t, 0);

      TREE_VEC_ELT (info, 1) = omp_clause_info_fndecl (t, type);
    }

  if (need_copy_assignment && !TYPE_HAS_TRIVIAL_ASSIGN_REF (type))
    {
      t = build_int_cst (build_pointer_type (type), 0);
      t = build1 (INDIRECT_REF, type, t);
      t = build_special_member_call (t, ansi_assopname (NOP_EXPR),
				     build_tree_list (NULL, t),
				     type, LOOKUP_NORMAL,
				     tf_warning_or_error);

      /* We'll have called convert_from_reference on the call, which
	 may well have added an indirect_ref.  It's unneeded here,
	 and in the way, so kill it.  */
      if (TREE_CODE (t) == INDIRECT_REF)
	t = TREE_OPERAND (t, 0);

      TREE_VEC_ELT (info, 2) = omp_clause_info_fndecl (t, type);
    }

  return errorcount != save_errorcount;
}

/* For all elements of CLAUSES, validate them vs OpenMP constraints.
   Remove any elements from the list that are invalid.  */

tree
finish_omp_clauses (tree clauses)
{
  bitmap_head generic_head, firstprivate_head, lastprivate_head;
  tree c, t, *pc = &clauses;
  const char *name;

  bitmap_obstack_initialize (NULL);
  bitmap_initialize (&generic_head, &bitmap_default_obstack);
  bitmap_initialize (&firstprivate_head, &bitmap_default_obstack);
  bitmap_initialize (&lastprivate_head, &bitmap_default_obstack);

  for (pc = &clauses, c = clauses; c ; c = *pc)
    {
      bool remove = false;

      switch (OMP_CLAUSE_CODE (c))
	{
	case OMP_CLAUSE_SHARED:
	  name = "shared";
	  goto check_dup_generic;
	case OMP_CLAUSE_PRIVATE:
	  name = "private";
	  goto check_dup_generic;
	case OMP_CLAUSE_REDUCTION:
	  name = "reduction";
	  goto check_dup_generic;
	case OMP_CLAUSE_COPYPRIVATE:
	  name = "copyprivate";
	  goto check_dup_generic;
	case OMP_CLAUSE_COPYIN:
	  name = "copyin";
	  goto check_dup_generic;
	check_dup_generic:
	  t = OMP_CLAUSE_DECL (c);
	  if (TREE_CODE (t) != VAR_DECL && TREE_CODE (t) != PARM_DECL)
	    {
	      if (processing_template_decl)
		break;
	      if (DECL_P (t))
		error ("%qD is not a variable in clause %qs", t, name);
	      else
		error ("%qE is not a variable in clause %qs", t, name);
	      remove = true;
	    }
	  else if (bitmap_bit_p (&generic_head, DECL_UID (t))
		   || bitmap_bit_p (&firstprivate_head, DECL_UID (t))
		   || bitmap_bit_p (&lastprivate_head, DECL_UID (t)))
	    {
	      error ("%qD appears more than once in data clauses", t);
	      remove = true;
	    }
	  else
	    bitmap_set_bit (&generic_head, DECL_UID (t));
	  break;

	case OMP_CLAUSE_FIRSTPRIVATE:
	  t = OMP_CLAUSE_DECL (c);
	  if (TREE_CODE (t) != VAR_DECL && TREE_CODE (t) != PARM_DECL)
	    {
	      if (processing_template_decl)
		break;
	      if (DECL_P (t))
		error ("%qD is not a variable in clause %<firstprivate%>", t);
	      else
		error ("%qE is not a variable in clause %<firstprivate%>", t);
	      remove = true;
	    }
	  else if (bitmap_bit_p (&generic_head, DECL_UID (t))
		   || bitmap_bit_p (&firstprivate_head, DECL_UID (t)))
	    {
	      error ("%qD appears more than once in data clauses", t);
	      remove = true;
	    }
	  else
	    bitmap_set_bit (&firstprivate_head, DECL_UID (t));
	  break;

	case OMP_CLAUSE_LASTPRIVATE:
	  t = OMP_CLAUSE_DECL (c);
	  if (TREE_CODE (t) != VAR_DECL && TREE_CODE (t) != PARM_DECL)
	    {
	      if (processing_template_decl)
		break;
	      if (DECL_P (t))
		error ("%qD is not a variable in clause %<lastprivate%>", t);
	      else
		error ("%qE is not a variable in clause %<lastprivate%>", t);
	      remove = true;
	    }
	  else if (bitmap_bit_p (&generic_head, DECL_UID (t))
		   || bitmap_bit_p (&lastprivate_head, DECL_UID (t)))
	    {
	      error ("%qD appears more than once in data clauses", t);
	      remove = true;
	    }
	  else
	    bitmap_set_bit (&lastprivate_head, DECL_UID (t));
	  break;

	case OMP_CLAUSE_IF:
	  t = OMP_CLAUSE_IF_EXPR (c);
	  t = maybe_convert_cond (t);
	  if (t == error_mark_node)
	    remove = true;
	  OMP_CLAUSE_IF_EXPR (c) = t;
	  break;

	case OMP_CLAUSE_NUM_THREADS:
	  t = OMP_CLAUSE_NUM_THREADS_EXPR (c);
	  if (t == error_mark_node)
	    remove = true;
	  else if (!type_dependent_expression_p (t)
		   && !INTEGRAL_TYPE_P (TREE_TYPE (t)))
	    {
	      error ("num_threads expression must be integral");
	      remove = true;
	    }
	  break;

	case OMP_CLAUSE_SCHEDULE:
	  t = OMP_CLAUSE_SCHEDULE_CHUNK_EXPR (c);
	  if (t == NULL)
	    ;
	  else if (t == error_mark_node)
	    remove = true;
	  else if (!type_dependent_expression_p (t)
		   && !INTEGRAL_TYPE_P (TREE_TYPE (t)))
	    {
	      error ("schedule chunk size expression must be integral");
	      remove = true;
	    }
	  break;

	case OMP_CLAUSE_NOWAIT:
	case OMP_CLAUSE_ORDERED:
	case OMP_CLAUSE_DEFAULT:
	case OMP_CLAUSE_UNTIED:
	case OMP_CLAUSE_COLLAPSE:
	  break;

	default:
	  gcc_unreachable ();
	}

      if (remove)
	*pc = OMP_CLAUSE_CHAIN (c);
      else
	pc = &OMP_CLAUSE_CHAIN (c);
    }

  for (pc = &clauses, c = clauses; c ; c = *pc)
    {
      enum tree_code c_kind = OMP_CLAUSE_CODE (c);
      bool remove = false;
      bool need_complete_non_reference = false;
      bool need_default_ctor = false;
      bool need_copy_ctor = false;
      bool need_copy_assignment = false;
      bool need_implicitly_determined = false;
      tree type, inner_type;

      switch (c_kind)
	{
	case OMP_CLAUSE_SHARED:
	  name = "shared";
	  need_implicitly_determined = true;
	  break;
	case OMP_CLAUSE_PRIVATE:
	  name = "private";
	  need_complete_non_reference = true;
	  need_default_ctor = true;
	  need_implicitly_determined = true;
	  break;
	case OMP_CLAUSE_FIRSTPRIVATE:
	  name = "firstprivate";
	  need_complete_non_reference = true;
	  need_copy_ctor = true;
	  need_implicitly_determined = true;
	  break;
	case OMP_CLAUSE_LASTPRIVATE:
	  name = "lastprivate";
	  need_complete_non_reference = true;
	  need_copy_assignment = true;
	  need_implicitly_determined = true;
	  break;
	case OMP_CLAUSE_REDUCTION:
	  name = "reduction";
	  need_implicitly_determined = true;
	  break;
	case OMP_CLAUSE_COPYPRIVATE:
	  name = "copyprivate";
	  need_copy_assignment = true;
	  break;
	case OMP_CLAUSE_COPYIN:
	  name = "copyin";
	  need_copy_assignment = true;
	  break;
	default:
	  pc = &OMP_CLAUSE_CHAIN (c);
	  continue;
	}

      t = OMP_CLAUSE_DECL (c);
      if (processing_template_decl
	  && TREE_CODE (t) != VAR_DECL && TREE_CODE (t) != PARM_DECL)
	{
	  pc = &OMP_CLAUSE_CHAIN (c);
	  continue;
	}

      switch (c_kind)
	{
	case OMP_CLAUSE_LASTPRIVATE:
	  if (!bitmap_bit_p (&firstprivate_head, DECL_UID (t)))
	    need_default_ctor = true;
	  break;

	case OMP_CLAUSE_REDUCTION:
	  if (AGGREGATE_TYPE_P (TREE_TYPE (t))
	      || POINTER_TYPE_P (TREE_TYPE (t)))
	    {
	      error ("%qE has invalid type for %<reduction%>", t);
	      remove = true;
	    }
	  else if (FLOAT_TYPE_P (TREE_TYPE (t)))
	    {
	      enum tree_code r_code = OMP_CLAUSE_REDUCTION_CODE (c);
	      switch (r_code)
		{
		case PLUS_EXPR:
		case MULT_EXPR:
		case MINUS_EXPR:
		  break;
		default:
		  error ("%qE has invalid type for %<reduction(%s)%>",
			 t, operator_name_info[r_code].name);
		  remove = true;
		}
	    }
	  break;

	case OMP_CLAUSE_COPYIN:
	  if (TREE_CODE (t) != VAR_DECL || !DECL_THREAD_LOCAL_P (t))
	    {
	      error ("%qE must be %<threadprivate%> for %<copyin%>", t);
	      remove = true;
	    }
	  break;

	default:
	  break;
	}

      if (need_complete_non_reference)
	{
	  t = require_complete_type (t);
	  if (t == error_mark_node)
	    remove = true;
	  else if (TREE_CODE (TREE_TYPE (t)) == REFERENCE_TYPE)
	    {
	      error ("%qE has reference type for %qs", t, name);
	      remove = true;
	    }
	}
      if (need_implicitly_determined)
	{
	  const char *share_name = NULL;

	  if (TREE_CODE (t) == VAR_DECL && DECL_THREAD_LOCAL_P (t))
	    share_name = "threadprivate";
	  else switch (cxx_omp_predetermined_sharing (t))
	    {
	    case OMP_CLAUSE_DEFAULT_UNSPECIFIED:
	      break;
	    case OMP_CLAUSE_DEFAULT_SHARED:
	      share_name = "shared";
	      break;
	    case OMP_CLAUSE_DEFAULT_PRIVATE:
	      share_name = "private";
	      break;
	    default:
	      gcc_unreachable ();
	    }
	  if (share_name)
	    {
	      error ("%qE is predetermined %qs for %qs",
		     t, share_name, name);
	      remove = true;
	    }
	}

      /* We're interested in the base element, not arrays.  */
      inner_type = type = TREE_TYPE (t);
      while (TREE_CODE (inner_type) == ARRAY_TYPE)
	inner_type = TREE_TYPE (inner_type);

      /* Check for special function availability by building a call to one.
	 Save the results, because later we won't be in the right context
	 for making these queries.  */
      if (CLASS_TYPE_P (inner_type)
	  && (need_default_ctor || need_copy_ctor || need_copy_assignment)
	  && !type_dependent_expression_p (t)
	  && cxx_omp_create_clause_info (c, inner_type, need_default_ctor,
					 need_copy_ctor, need_copy_assignment))
	remove = true;

      if (remove)
	*pc = OMP_CLAUSE_CHAIN (c);
      else
	pc = &OMP_CLAUSE_CHAIN (c);
    }

  bitmap_obstack_release (NULL);
  return clauses;
}

/* For all variables in the tree_list VARS, mark them as thread local.  */

void
finish_omp_threadprivate (tree vars)
{
  tree t;

  /* Mark every variable in VARS to be assigned thread local storage.  */
  for (t = vars; t; t = TREE_CHAIN (t))
    {
      tree v = TREE_PURPOSE (t);

      if (error_operand_p (v))
	;
      else if (TREE_CODE (v) != VAR_DECL)
	error ("%<threadprivate%> %qD is not file, namespace "
	       "or block scope variable", v);
      /* If V had already been marked threadprivate, it doesn't matter
	 whether it had been used prior to this point.  */
      else if (TREE_USED (v)
	  && (DECL_LANG_SPECIFIC (v) == NULL
	      || !CP_DECL_THREADPRIVATE_P (v)))
	error ("%qE declared %<threadprivate%> after first use", v);
      else if (! TREE_STATIC (v) && ! DECL_EXTERNAL (v))
	error ("automatic variable %qE cannot be %<threadprivate%>", v);
      else if (! COMPLETE_TYPE_P (TREE_TYPE (v)))
	error ("%<threadprivate%> %qE has incomplete type", v);
      else if (TREE_STATIC (v) && TYPE_P (CP_DECL_CONTEXT (v))
	       && CP_DECL_CONTEXT (v) != current_class_type)
	error ("%<threadprivate%> %qE directive not "
	       "in %qT definition", v, CP_DECL_CONTEXT (v));
      else
	{
	  /* Allocate a LANG_SPECIFIC structure for V, if needed.  */
	  if (DECL_LANG_SPECIFIC (v) == NULL)
	    {
	      retrofit_lang_decl (v);

	      /* Make sure that DECL_DISCRIMINATOR_P continues to be true
		 after the allocation of the lang_decl structure.  */
	      if (DECL_DISCRIMINATOR_P (v))
		DECL_LANG_SPECIFIC (v)->decl_flags.u2sel = 1;
	    }

	  if (! DECL_THREAD_LOCAL_P (v))
	    {
	      DECL_TLS_MODEL (v) = decl_default_tls_model (v);
	      /* If rtl has been already set for this var, call
		 make_decl_rtl once again, so that encode_section_info
		 has a chance to look at the new decl flags.  */
	      if (DECL_RTL_SET_P (v))
		make_decl_rtl (v);
	    }
	  CP_DECL_THREADPRIVATE_P (v) = 1;
	}
    }
}

/* Build an OpenMP structured block.  */

tree
begin_omp_structured_block (void)
{
  return do_pushlevel (sk_omp);
}

tree
finish_omp_structured_block (tree block)
{
  return do_poplevel (block);
}

/* Similarly, except force the retention of the BLOCK.  */

tree
begin_omp_parallel (void)
{
  keep_next_level (true);
  return begin_omp_structured_block ();
}

tree
finish_omp_parallel (tree clauses, tree body)
{
  tree stmt;

  body = finish_omp_structured_block (body);

  stmt = make_node (OMP_PARALLEL);
  TREE_TYPE (stmt) = void_type_node;
  OMP_PARALLEL_CLAUSES (stmt) = clauses;
  OMP_PARALLEL_BODY (stmt) = body;

  return add_stmt (stmt);
}

tree
begin_omp_task (void)
{
  keep_next_level (true);
  return begin_omp_structured_block ();
}

tree
finish_omp_task (tree clauses, tree body)
{
  tree stmt;

  body = finish_omp_structured_block (body);

  stmt = make_node (OMP_TASK);
  TREE_TYPE (stmt) = void_type_node;
  OMP_TASK_CLAUSES (stmt) = clauses;
  OMP_TASK_BODY (stmt) = body;

  return add_stmt (stmt);
}

/* Helper function for finish_omp_for.  Convert Ith random access iterator
   into integral iterator.  Return FALSE if successful.  */

static bool
handle_omp_for_class_iterator (int i, location_t locus, tree declv, tree initv,
			       tree condv, tree incrv, tree *body,
			       tree *pre_body, tree clauses)
{
  tree diff, iter_init, iter_incr = NULL, last;
  tree incr_var = NULL, orig_pre_body, orig_body, c;
  tree decl = TREE_VEC_ELT (declv, i);
  tree init = TREE_VEC_ELT (initv, i);
  tree cond = TREE_VEC_ELT (condv, i);
  tree incr = TREE_VEC_ELT (incrv, i);
  tree iter = decl;
  location_t elocus = locus;

  if (init && EXPR_HAS_LOCATION (init))
    elocus = EXPR_LOCATION (init);

  switch (TREE_CODE (cond))
    {
    case GT_EXPR:
    case GE_EXPR:
    case LT_EXPR:
    case LE_EXPR:
      if (TREE_OPERAND (cond, 1) == iter)
	cond = build2 (swap_tree_comparison (TREE_CODE (cond)),
		       TREE_TYPE (cond), iter, TREE_OPERAND (cond, 0));
      if (TREE_OPERAND (cond, 0) != iter)
	cond = error_mark_node;
      else
	{
	  tree tem = build_x_binary_op (TREE_CODE (cond), iter, ERROR_MARK,
					TREE_OPERAND (cond, 1), ERROR_MARK,
					NULL, tf_warning_or_error);
	  if (error_operand_p (tem))
	    return true;
	}
      break;
    default:
      cond = error_mark_node;
      break;
    }
  if (cond == error_mark_node)
    {
      error ("%Hinvalid controlling predicate", &elocus);
      return true;
    }
  diff = build_x_binary_op (MINUS_EXPR, TREE_OPERAND (cond, 1),
			    ERROR_MARK, iter, ERROR_MARK, NULL,
			    tf_warning_or_error);
  if (error_operand_p (diff))
    return true;
  if (TREE_CODE (TREE_TYPE (diff)) != INTEGER_TYPE)
    {
      error ("%Hdifference between %qE and %qD does not have integer type",
	     &elocus, TREE_OPERAND (cond, 1), iter);
      return true;
    }

  switch (TREE_CODE (incr))
    {
    case PREINCREMENT_EXPR:
    case PREDECREMENT_EXPR:
    case POSTINCREMENT_EXPR:
    case POSTDECREMENT_EXPR:
      if (TREE_OPERAND (incr, 0) != iter)
	{
	  incr = error_mark_node;
	  break;
	}
      iter_incr = build_x_unary_op (TREE_CODE (incr), iter,
				    tf_warning_or_error);
      if (error_operand_p (iter_incr))
	return true;
      else if (TREE_CODE (incr) == PREINCREMENT_EXPR
	       || TREE_CODE (incr) == POSTINCREMENT_EXPR)
	incr = integer_one_node;
      else
	incr = integer_minus_one_node;
      break;
    case MODIFY_EXPR:
      if (TREE_OPERAND (incr, 0) != iter)
	incr = error_mark_node;
      else if (TREE_CODE (TREE_OPERAND (incr, 1)) == PLUS_EXPR
	       || TREE_CODE (TREE_OPERAND (incr, 1)) == MINUS_EXPR)
	{
	  tree rhs = TREE_OPERAND (incr, 1);
	  if (TREE_OPERAND (rhs, 0) == iter)
	    {
	      if (TREE_CODE (TREE_TYPE (TREE_OPERAND (rhs, 1)))
		  != INTEGER_TYPE)
		incr = error_mark_node;
	      else
		{
		  iter_incr = build_x_modify_expr (iter, TREE_CODE (rhs),
						   TREE_OPERAND (rhs, 1),
						   tf_warning_or_error);
		  if (error_operand_p (iter_incr))
		    return true;
		  incr = TREE_OPERAND (rhs, 1);
		  incr = cp_convert (TREE_TYPE (diff), incr);
		  if (TREE_CODE (rhs) == MINUS_EXPR)
		    {
		      incr = build1 (NEGATE_EXPR, TREE_TYPE (diff), incr);
		      incr = fold_if_not_in_template (incr);
		    }
		  if (TREE_CODE (incr) != INTEGER_CST
		      && (TREE_CODE (incr) != NOP_EXPR
			  || (TREE_CODE (TREE_OPERAND (incr, 0))
			      != INTEGER_CST)))
		    iter_incr = NULL;
		}
	    }
	  else if (TREE_OPERAND (rhs, 1) == iter)
	    {
	      if (TREE_CODE (TREE_TYPE (TREE_OPERAND (rhs, 0))) != INTEGER_TYPE
		  || TREE_CODE (rhs) != PLUS_EXPR)
		incr = error_mark_node;
	      else
		{
		  iter_incr = build_x_binary_op (PLUS_EXPR,
						 TREE_OPERAND (rhs, 0),
						 ERROR_MARK, iter,
						 ERROR_MARK, NULL,
						 tf_warning_or_error);
		  if (error_operand_p (iter_incr))
		    return true;
		  iter_incr = build_x_modify_expr (iter, NOP_EXPR,
						   iter_incr,
						   tf_warning_or_error);
		  if (error_operand_p (iter_incr))
		    return true;
		  incr = TREE_OPERAND (rhs, 0);
		  iter_incr = NULL;
		}
	    }
	  else
	    incr = error_mark_node;
	}
      else
	incr = error_mark_node;
      break;
    default:
      incr = error_mark_node;
      break;
    }

  if (incr == error_mark_node)
    {
      error ("%Hinvalid increment expression", &elocus);
      return true;
    }

  incr = cp_convert (TREE_TYPE (diff), incr);
  for (c = clauses; c ; c = OMP_CLAUSE_CHAIN (c))
    if (OMP_CLAUSE_CODE (c) == OMP_CLAUSE_LASTPRIVATE
	&& OMP_CLAUSE_DECL (c) == iter)
      break;

  decl = create_temporary_var (TREE_TYPE (diff));
  pushdecl (decl);
  add_decl_expr (decl);
  last = create_temporary_var (TREE_TYPE (diff));
  pushdecl (last);
  add_decl_expr (last);
  if (c && iter_incr == NULL)
    {
      incr_var = create_temporary_var (TREE_TYPE (diff));
      pushdecl (incr_var);
      add_decl_expr (incr_var);
    }
  gcc_assert (stmts_are_full_exprs_p ());

  orig_pre_body = *pre_body;
  *pre_body = push_stmt_list ();
  if (orig_pre_body)
    add_stmt (orig_pre_body);
  if (init != NULL)
    finish_expr_stmt (build_x_modify_expr (iter, NOP_EXPR, init,
					   tf_warning_or_error));
  init = build_int_cst (TREE_TYPE (diff), 0);
  if (c && iter_incr == NULL)
    {
      finish_expr_stmt (build_x_modify_expr (incr_var, NOP_EXPR,
					     incr, tf_warning_or_error));
      incr = incr_var;
      iter_incr = build_x_modify_expr (iter, PLUS_EXPR, incr,
				       tf_warning_or_error);
    }
  finish_expr_stmt (build_x_modify_expr (last, NOP_EXPR, init,
					 tf_warning_or_error));
  *pre_body = pop_stmt_list (*pre_body);

  cond = cp_build_binary_op (elocus,
			     TREE_CODE (cond), decl, diff,
			     tf_warning_or_error);
  incr = build_modify_expr (elocus, decl, PLUS_EXPR, incr);

  orig_body = *body;
  *body = push_stmt_list ();
  iter_init = build2 (MINUS_EXPR, TREE_TYPE (diff), decl, last);
  iter_init = build_x_modify_expr (iter, PLUS_EXPR, iter_init,
				   tf_warning_or_error);
  iter_init = build1 (NOP_EXPR, void_type_node, iter_init);
  finish_expr_stmt (iter_init);
  finish_expr_stmt (build_x_modify_expr (last, NOP_EXPR, decl,
					 tf_warning_or_error));
  add_stmt (orig_body);
  *body = pop_stmt_list (*body);

  if (c)
    {
      OMP_CLAUSE_LASTPRIVATE_STMT (c) = push_stmt_list ();
      finish_expr_stmt (iter_incr);
      OMP_CLAUSE_LASTPRIVATE_STMT (c)
	= pop_stmt_list (OMP_CLAUSE_LASTPRIVATE_STMT (c));
    }

  TREE_VEC_ELT (declv, i) = decl;
  TREE_VEC_ELT (initv, i) = init;
  TREE_VEC_ELT (condv, i) = cond;
  TREE_VEC_ELT (incrv, i) = incr;

  return false;
}

/* Build and validate an OMP_FOR statement.  CLAUSES, BODY, COND, INCR
   are directly for their associated operands in the statement.  DECL
   and INIT are a combo; if DECL is NULL then INIT ought to be a
   MODIFY_EXPR, and the DECL should be extracted.  PRE_BODY are
   optional statements that need to go before the loop into its
   sk_omp scope.  */

tree
finish_omp_for (location_t locus, tree declv, tree initv, tree condv,
		tree incrv, tree body, tree pre_body, tree clauses)
{
  tree omp_for = NULL, orig_incr = NULL;
  tree decl, init, cond, incr;
  location_t elocus;
  int i;

  gcc_assert (TREE_VEC_LENGTH (declv) == TREE_VEC_LENGTH (initv));
  gcc_assert (TREE_VEC_LENGTH (declv) == TREE_VEC_LENGTH (condv));
  gcc_assert (TREE_VEC_LENGTH (declv) == TREE_VEC_LENGTH (incrv));
  for (i = 0; i < TREE_VEC_LENGTH (declv); i++)
    {
      decl = TREE_VEC_ELT (declv, i);
      init = TREE_VEC_ELT (initv, i);
      cond = TREE_VEC_ELT (condv, i);
      incr = TREE_VEC_ELT (incrv, i);
      elocus = locus;

      if (decl == NULL)
	{
	  if (init != NULL)
	    switch (TREE_CODE (init))
	      {
	      case MODIFY_EXPR:
		decl = TREE_OPERAND (init, 0);
		init = TREE_OPERAND (init, 1);
		break;
	      case MODOP_EXPR:
		if (TREE_CODE (TREE_OPERAND (init, 1)) == NOP_EXPR)
		  {
		    decl = TREE_OPERAND (init, 0);
		    init = TREE_OPERAND (init, 2);
		  }
		break;
	      default:
		break;
	      }

	  if (decl == NULL)
	    {
	      error ("%Hexpected iteration declaration or initialization",
		     &locus);
	      return NULL;
	    }
	}

      if (init && EXPR_HAS_LOCATION (init))
	elocus = EXPR_LOCATION (init);

      if (cond == NULL)
	{
	  error ("%Hmissing controlling predicate", &elocus);
	  return NULL;
	}

      if (incr == NULL)
	{
	  error ("%Hmissing increment expression", &elocus);
	  return NULL;
	}

      TREE_VEC_ELT (declv, i) = decl;
      TREE_VEC_ELT (initv, i) = init;
    }

  if (dependent_omp_for_p (declv, initv, condv, incrv))
    {
      tree stmt;

      stmt = make_node (OMP_FOR);

      for (i = 0; i < TREE_VEC_LENGTH (declv); i++)
	{
	  /* This is really just a place-holder.  We'll be decomposing this
	     again and going through the cp_build_modify_expr path below when
	     we instantiate the thing.  */
	  TREE_VEC_ELT (initv, i)
	    = build2 (MODIFY_EXPR, void_type_node, TREE_VEC_ELT (declv, i),
		      TREE_VEC_ELT (initv, i));
	}

      TREE_TYPE (stmt) = void_type_node;
      OMP_FOR_INIT (stmt) = initv;
      OMP_FOR_COND (stmt) = condv;
      OMP_FOR_INCR (stmt) = incrv;
      OMP_FOR_BODY (stmt) = body;
      OMP_FOR_PRE_BODY (stmt) = pre_body;
      OMP_FOR_CLAUSES (stmt) = clauses;

      SET_EXPR_LOCATION (stmt, locus);
      return add_stmt (stmt);
    }

  if (processing_template_decl)
    orig_incr = make_tree_vec (TREE_VEC_LENGTH (incrv));

  for (i = 0; i < TREE_VEC_LENGTH (declv); )
    {
      decl = TREE_VEC_ELT (declv, i);
      init = TREE_VEC_ELT (initv, i);
      cond = TREE_VEC_ELT (condv, i);
      incr = TREE_VEC_ELT (incrv, i);
      if (orig_incr)
	TREE_VEC_ELT (orig_incr, i) = incr;
      elocus = locus;

      if (init && EXPR_HAS_LOCATION (init))
	elocus = EXPR_LOCATION (init);

      if (!DECL_P (decl))
	{
	  error ("%Hexpected iteration declaration or initialization",
		 &elocus);
	  return NULL;
	}

      if (incr && TREE_CODE (incr) == MODOP_EXPR)
	{
	  if (orig_incr)
	    TREE_VEC_ELT (orig_incr, i) = incr;
	  incr = cp_build_modify_expr (TREE_OPERAND (incr, 0),
				       TREE_CODE (TREE_OPERAND (incr, 1)),
				       TREE_OPERAND (incr, 2),
				       tf_warning_or_error);
	}

      if (CLASS_TYPE_P (TREE_TYPE (decl)))
	{
	  if (handle_omp_for_class_iterator (i, locus, declv, initv, condv,
					     incrv, &body, &pre_body, clauses))
	    return NULL;
	  continue;
	}

      if (!INTEGRAL_TYPE_P (TREE_TYPE (decl))
	  && TREE_CODE (TREE_TYPE (decl)) != POINTER_TYPE)
	{
	  error ("%Hinvalid type for iteration variable %qE", &elocus, decl);
	  return NULL;
	}

      if (!processing_template_decl)
	{
	  init = fold_build_cleanup_point_expr (TREE_TYPE (init), init);
	  init = cp_build_modify_expr (decl, NOP_EXPR, init, tf_warning_or_error);
	}
      else
	init = build2 (MODIFY_EXPR, void_type_node, decl, init);
      if (cond
	  && TREE_SIDE_EFFECTS (cond)
	  && COMPARISON_CLASS_P (cond)
	  && !processing_template_decl)
	{
	  tree t = TREE_OPERAND (cond, 0);
	  if (TREE_SIDE_EFFECTS (t)
	      && t != decl
	      && (TREE_CODE (t) != NOP_EXPR
		  || TREE_OPERAND (t, 0) != decl))
	    TREE_OPERAND (cond, 0)
	      = fold_build_cleanup_point_expr (TREE_TYPE (t), t);

	  t = TREE_OPERAND (cond, 1);
	  if (TREE_SIDE_EFFECTS (t)
	      && t != decl
	      && (TREE_CODE (t) != NOP_EXPR
		  || TREE_OPERAND (t, 0) != decl))
	    TREE_OPERAND (cond, 1)
	      = fold_build_cleanup_point_expr (TREE_TYPE (t), t);
	}
      if (decl == error_mark_node || init == error_mark_node)
	return NULL;

      TREE_VEC_ELT (declv, i) = decl;
      TREE_VEC_ELT (initv, i) = init;
      TREE_VEC_ELT (condv, i) = cond;
      TREE_VEC_ELT (incrv, i) = incr;
      i++;
    }

  if (IS_EMPTY_STMT (pre_body))
    pre_body = NULL;

  omp_for = c_finish_omp_for (locus, declv, initv, condv, incrv,
			      body, pre_body);

  if (omp_for == NULL)
    return NULL;

  for (i = 0; i < TREE_VEC_LENGTH (OMP_FOR_INCR (omp_for)); i++)
    {
      decl = TREE_OPERAND (TREE_VEC_ELT (OMP_FOR_INIT (omp_for), i), 0);
      incr = TREE_VEC_ELT (OMP_FOR_INCR (omp_for), i);

      if (TREE_CODE (incr) != MODIFY_EXPR)
	continue;

      if (TREE_SIDE_EFFECTS (TREE_OPERAND (incr, 1))
	  && BINARY_CLASS_P (TREE_OPERAND (incr, 1))
	  && !processing_template_decl)
	{
	  tree t = TREE_OPERAND (TREE_OPERAND (incr, 1), 0);
	  if (TREE_SIDE_EFFECTS (t)
	      && t != decl
	      && (TREE_CODE (t) != NOP_EXPR
		  || TREE_OPERAND (t, 0) != decl))
	    TREE_OPERAND (TREE_OPERAND (incr, 1), 0)
	      = fold_build_cleanup_point_expr (TREE_TYPE (t), t);

	  t = TREE_OPERAND (TREE_OPERAND (incr, 1), 1);
	  if (TREE_SIDE_EFFECTS (t)
	      && t != decl
	      && (TREE_CODE (t) != NOP_EXPR
		  || TREE_OPERAND (t, 0) != decl))
	    TREE_OPERAND (TREE_OPERAND (incr, 1), 1)
	      = fold_build_cleanup_point_expr (TREE_TYPE (t), t);
	}

      if (orig_incr)
	TREE_VEC_ELT (OMP_FOR_INCR (omp_for), i) = TREE_VEC_ELT (orig_incr, i);
    }
  if (omp_for != NULL)
    OMP_FOR_CLAUSES (omp_for) = clauses;
  return omp_for;
}

void
finish_omp_atomic (enum tree_code code, tree lhs, tree rhs)
{
  tree orig_lhs;
  tree orig_rhs;
  bool dependent_p;
  tree stmt;

  orig_lhs = lhs;
  orig_rhs = rhs;
  dependent_p = false;
  stmt = NULL_TREE;

  /* Even in a template, we can detect invalid uses of the atomic
     pragma if neither LHS nor RHS is type-dependent.  */
  if (processing_template_decl)
    {
      dependent_p = (type_dependent_expression_p (lhs)
		     || type_dependent_expression_p (rhs));
      if (!dependent_p)
	{
	  lhs = build_non_dependent_expr (lhs);
	  rhs = build_non_dependent_expr (rhs);
	}
    }
  if (!dependent_p)
    {
      stmt = c_finish_omp_atomic (code, lhs, rhs);
      if (stmt == error_mark_node)
	return;
    }
  if (processing_template_decl)
    stmt = build2 (OMP_ATOMIC, void_type_node, integer_zero_node,
		   build2 (code, void_type_node, orig_lhs, orig_rhs));
  add_stmt (stmt);
}

void
finish_omp_barrier (void)
{
  tree fn = built_in_decls[BUILT_IN_GOMP_BARRIER];
  tree stmt = finish_call_expr (fn, NULL, false, false, tf_warning_or_error);
  finish_expr_stmt (stmt);
}

void
finish_omp_flush (void)
{
  tree fn = built_in_decls[BUILT_IN_SYNCHRONIZE];
  tree stmt = finish_call_expr (fn, NULL, false, false, tf_warning_or_error);
  finish_expr_stmt (stmt);
}

void
finish_omp_taskwait (void)
{
  tree fn = built_in_decls[BUILT_IN_GOMP_TASKWAIT];
  tree stmt = finish_call_expr (fn, NULL, false, false, tf_warning_or_error);
  finish_expr_stmt (stmt);
}

void
init_cp_semantics (void)
{
}

/* Build a STATIC_ASSERT for a static assertion with the condition
   CONDITION and the message text MESSAGE.  LOCATION is the location
   of the static assertion in the source code.  When MEMBER_P, this
   static assertion is a member of a class.  */
void 
finish_static_assert (tree condition, tree message, location_t location, 
                      bool member_p)
{
  if (check_for_bare_parameter_packs (condition))
    condition = error_mark_node;

  if (type_dependent_expression_p (condition) 
      || value_dependent_expression_p (condition))
    {
      /* We're in a template; build a STATIC_ASSERT and put it in
         the right place. */
      tree assertion;

      assertion = make_node (STATIC_ASSERT);
      STATIC_ASSERT_CONDITION (assertion) = condition;
      STATIC_ASSERT_MESSAGE (assertion) = message;
      STATIC_ASSERT_SOURCE_LOCATION (assertion) = location;

      if (member_p)
        maybe_add_class_template_decl_list (current_class_type, 
                                            assertion,
                                            /*friend_p=*/0);
      else
        add_stmt (assertion);

      return;
    }

  /* Fold the expression and convert it to a boolean value. */
  condition = fold_non_dependent_expr (condition);
  condition = cp_convert (boolean_type_node, condition);

  if (TREE_CODE (condition) == INTEGER_CST && !integer_zerop (condition))
    /* Do nothing; the condition is satisfied. */
    ;
  else 
    {
      location_t saved_loc = input_location;

      input_location = location;
      if (TREE_CODE (condition) == INTEGER_CST 
          && integer_zerop (condition))
        /* Report the error. */
        error ("static assertion failed: %E", message);
      else if (condition && condition != error_mark_node)
        error ("non-constant condition for static assertion");
      input_location = saved_loc;
    }
}

/* Returns decltype((EXPR)) for cases where we can drop the decltype and
   just return the type even though EXPR is a type-dependent expression.
   The ABI specifies which cases this applies to, which is a subset of the
   possible cases.  */

tree
describable_type (tree expr)
{
  tree type = NULL_TREE;

  /* processing_template_decl isn't set when we're called from the mangling
     code, so bump it now.  */
  ++processing_template_decl;
  if (! type_dependent_expression_p (expr)
      && ! type_unknown_p (expr))
    {
      type = TREE_TYPE (expr);
      if (real_lvalue_p (expr))
	type = build_reference_type (type);
    }
  --processing_template_decl;

  if (type)
    return type;

  switch (TREE_CODE (expr))
    {
    case VAR_DECL:
    case PARM_DECL:
    case RESULT_DECL:
    case FUNCTION_DECL:
      /* Named rvalue reference becomes lvalue.  */
      type = build_reference_type (non_reference (TREE_TYPE (expr)));
      break;

    case NEW_EXPR:
    case CONST_DECL:
    case TEMPLATE_PARM_INDEX:
    case CAST_EXPR:
    case STATIC_CAST_EXPR:
    case REINTERPRET_CAST_EXPR:
    case CONST_CAST_EXPR:
    case DYNAMIC_CAST_EXPR:
      type = TREE_TYPE (expr);
      break;

    case INDIRECT_REF:
      {
	tree ptrtype = describable_type (TREE_OPERAND (expr, 0));
	if (ptrtype && POINTER_TYPE_P (ptrtype))
	  type = build_reference_type (TREE_TYPE (ptrtype));
      }
      break;

    default:
      if (TREE_CODE_CLASS (TREE_CODE (expr)) == tcc_constant)
	type = TREE_TYPE (expr);
      break;
    }

  if (type && type_uses_auto (type))
    return NULL_TREE;
  else
    return type;
}

/* Implements the C++0x decltype keyword. Returns the type of EXPR,
   suitable for use as a type-specifier.

   ID_EXPRESSION_OR_MEMBER_ACCESS_P is true when EXPR was parsed as an
   id-expression or a class member access, FALSE when it was parsed as
   a full expression.  */

tree
finish_decltype_type (tree expr, bool id_expression_or_member_access_p)
{
  tree orig_expr = expr;
  tree type = NULL_TREE;

  if (!expr || error_operand_p (expr))
    return error_mark_node;

  if (TYPE_P (expr)
      || TREE_CODE (expr) == TYPE_DECL
      || (TREE_CODE (expr) == BIT_NOT_EXPR
	  && TYPE_P (TREE_OPERAND (expr, 0))))
    {
      error ("argument to decltype must be an expression");
      return error_mark_node;
    }

  if (type_dependent_expression_p (expr))
    {
      if (id_expression_or_member_access_p)
	{
	  switch (TREE_CODE (expr))
	    {
	    case VAR_DECL:
	    case PARM_DECL:
	    case RESULT_DECL:
	    case FUNCTION_DECL:
	    case CONST_DECL:
	    case TEMPLATE_PARM_INDEX:
	      type = TREE_TYPE (expr);
	      break;

	    default:
	      break;
	    }
	}

      if (type && !type_uses_auto (type))
	return type;

      type = cxx_make_type (DECLTYPE_TYPE);
      DECLTYPE_TYPE_EXPR (type) = expr;
      DECLTYPE_TYPE_ID_EXPR_OR_MEMBER_ACCESS_P (type)
        = id_expression_or_member_access_p;
      SET_TYPE_STRUCTURAL_EQUALITY (type);

      return type;
    }

  /* The type denoted by decltype(e) is defined as follows:  */

  if (id_expression_or_member_access_p)
    {
      /* If e is an id-expression or a class member access (5.2.5
         [expr.ref]), decltype(e) is defined as the type of the entity
         named by e. If there is no such entity, or e names a set of
         overloaded functions, the program is ill-formed.  */
      if (TREE_CODE (expr) == IDENTIFIER_NODE)
        expr = lookup_name (expr);

      if (TREE_CODE (expr) == INDIRECT_REF)
        /* This can happen when the expression is, e.g., "a.b". Just
           look at the underlying operand.  */
        expr = TREE_OPERAND (expr, 0);

      if (TREE_CODE (expr) == OFFSET_REF
          || TREE_CODE (expr) == MEMBER_REF)
        /* We're only interested in the field itself. If it is a
           BASELINK, we will need to see through it in the next
           step.  */
        expr = TREE_OPERAND (expr, 1);

      if (TREE_CODE (expr) == BASELINK)
        /* See through BASELINK nodes to the underlying functions.  */
        expr = BASELINK_FUNCTIONS (expr);

      if (TREE_CODE (expr) == OVERLOAD)
        {
          if (OVL_CHAIN (expr))
            {
              error ("%qE refers to a set of overloaded functions", orig_expr);
              return error_mark_node;
            }
          else
            /* An overload set containing only one function: just look
               at that function.  */
            expr = OVL_FUNCTION (expr);
        }

      switch (TREE_CODE (expr))
        {
        case FIELD_DECL:
          if (DECL_BIT_FIELD_TYPE (expr))
            {
              type = DECL_BIT_FIELD_TYPE (expr);
              break;
            }
          /* Fall through for fields that aren't bitfields.  */

        case FUNCTION_DECL:
        case VAR_DECL:
        case CONST_DECL:
        case PARM_DECL:
        case RESULT_DECL:
        case TEMPLATE_PARM_INDEX:
          type = TREE_TYPE (expr);
          break;

        case ERROR_MARK:
          type = error_mark_node;
          break;

        case COMPONENT_REF:
          type = is_bitfield_expr_with_lowered_type (expr);
          if (!type)
            type = TREE_TYPE (TREE_OPERAND (expr, 1));
          break;

        case BIT_FIELD_REF:
          gcc_unreachable ();

        case INTEGER_CST:
          /* We can get here when the id-expression refers to an
             enumerator.  */
          type = TREE_TYPE (expr);
          break;

        default:
	  gcc_assert (TYPE_P (expr) || DECL_P (expr)
		      || TREE_CODE (expr) == SCOPE_REF);
          error ("argument to decltype must be an expression");
          return error_mark_node;
        }
    }
  else
    {
      /* Expressions of reference type are sometimes wrapped in
         INDIRECT_REFs.  INDIRECT_REFs are just internal compiler
         representation, not part of the language, so we have to look
         through them.  */
      if (TREE_CODE (expr) == INDIRECT_REF
          && TREE_CODE (TREE_TYPE (TREE_OPERAND (expr, 0)))
  	  == REFERENCE_TYPE)
        expr = TREE_OPERAND (expr, 0);

      if (TREE_CODE (expr) == CALL_EXPR)
        {
          /* If e is a function call (5.2.2 [expr.call]) or an
           invocation of an overloaded operator (parentheses around e
           are ignored), decltype(e) is defined as the return type of
           that function.  */
          tree fndecl = get_callee_fndecl (expr);
          if (fndecl && fndecl != error_mark_node)
            type = TREE_TYPE (TREE_TYPE (fndecl));
          else 
            {
              tree target_type = TREE_TYPE (CALL_EXPR_FN (expr));
              if ((TREE_CODE (target_type) == REFERENCE_TYPE
                   || TREE_CODE (target_type) == POINTER_TYPE)
                  && (TREE_CODE (TREE_TYPE (target_type)) == FUNCTION_TYPE
                      || TREE_CODE (TREE_TYPE (target_type)) == METHOD_TYPE))
                type = TREE_TYPE (TREE_TYPE (target_type));
              else
                sorry ("unable to determine the declared type of expression %<%E%>",
                       expr);
            }
        }
      else 
        {
          type = is_bitfield_expr_with_lowered_type (expr);
          if (type)
            {
              /* Bitfields are special, because their type encodes the
                 number of bits they store.  If the expression referenced a
                 bitfield, TYPE now has the declared type of that
                 bitfield.  */
              type = cp_build_qualified_type (type, 
                                              cp_type_quals (TREE_TYPE (expr)));
              
              if (real_lvalue_p (expr))
                type = build_reference_type (type);
            }
          else
            {
              /* Otherwise, where T is the type of e, if e is an lvalue,
                 decltype(e) is defined as T&, otherwise decltype(e) is
                 defined as T.  */
              type = TREE_TYPE (expr);
              if (type == error_mark_node)
                return error_mark_node;
              else if (expr == current_class_ptr)
                /* If the expression is just "this", we want the
                   cv-unqualified pointer for the "this" type.  */
                type = TYPE_MAIN_VARIANT (type);
              else if (real_lvalue_p (expr))
                {
                  if (TREE_CODE (type) != REFERENCE_TYPE)
                    type = build_reference_type (type);
                }
              else
                type = non_reference (type);
            }
        }
    }

  if (!type || type == unknown_type_node)
    {
      error ("type of %qE is unknown", expr);
      return error_mark_node;
    }

  return type;
}

/* Called from trait_expr_value to evaluate either __has_nothrow_assign or 
   __has_nothrow_copy, depending on assign_p.  */

static bool
classtype_has_nothrow_assign_or_copy_p (tree type, bool assign_p)
{
  tree fns;

  if (assign_p)
    {
      int ix;
      ix = lookup_fnfields_1 (type, ansi_assopname (NOP_EXPR));
      if (ix < 0)
	return false;
      fns = VEC_index (tree, CLASSTYPE_METHOD_VEC (type), ix);
    } 
  else if (TYPE_HAS_INIT_REF (type))
    {
      /* If construction of the copy constructor was postponed, create
	 it now.  */
      if (CLASSTYPE_LAZY_COPY_CTOR (type))
	lazily_declare_fn (sfk_copy_constructor, type);
      fns = CLASSTYPE_CONSTRUCTORS (type);
    }
  else
    return false;

  for (; fns; fns = OVL_NEXT (fns))
    {
      tree fn = OVL_CURRENT (fns);
 
      if (assign_p)
	{
	  if (copy_fn_p (fn) == 0)
	    continue;
	}
      else if (copy_fn_p (fn) <= 0)
	continue;

      if (!TYPE_NOTHROW_P (TREE_TYPE (fn)))
	return false;
    }

  return true;
}

/* Actually evaluates the trait.  */

static bool
trait_expr_value (cp_trait_kind kind, tree type1, tree type2)
{
  enum tree_code type_code1;
  tree t;

  type_code1 = TREE_CODE (type1);

  switch (kind)
    {
    case CPTK_HAS_NOTHROW_ASSIGN:
      return (!CP_TYPE_CONST_P (type1) && type_code1 != REFERENCE_TYPE
	      && (trait_expr_value (CPTK_HAS_TRIVIAL_ASSIGN, type1, type2)
		  || (CLASS_TYPE_P (type1)
		      && classtype_has_nothrow_assign_or_copy_p (type1,
								 true))));

    case CPTK_HAS_TRIVIAL_ASSIGN:
      return (!CP_TYPE_CONST_P (type1) && type_code1 != REFERENCE_TYPE
	      && (pod_type_p (type1)
		    || (CLASS_TYPE_P (type1)
			&& TYPE_HAS_TRIVIAL_ASSIGN_REF (type1))));

    case CPTK_HAS_NOTHROW_CONSTRUCTOR:
      type1 = strip_array_types (type1);
      return (trait_expr_value (CPTK_HAS_TRIVIAL_CONSTRUCTOR, type1, type2) 
	      || (CLASS_TYPE_P (type1)
		  && (t = locate_ctor (type1, NULL))
		  && TYPE_NOTHROW_P (TREE_TYPE (t))));

    case CPTK_HAS_TRIVIAL_CONSTRUCTOR:
      type1 = strip_array_types (type1);
      return (pod_type_p (type1)
	      || (CLASS_TYPE_P (type1) && TYPE_HAS_TRIVIAL_DFLT (type1)));

    case CPTK_HAS_NOTHROW_COPY:
      return (trait_expr_value (CPTK_HAS_TRIVIAL_COPY, type1, type2)
	      || (CLASS_TYPE_P (type1)
		  && classtype_has_nothrow_assign_or_copy_p (type1, false)));

    case CPTK_HAS_TRIVIAL_COPY:
      return (pod_type_p (type1) || type_code1 == REFERENCE_TYPE
	      || (CLASS_TYPE_P (type1) && TYPE_HAS_TRIVIAL_INIT_REF (type1)));

    case CPTK_HAS_TRIVIAL_DESTRUCTOR:
      type1 = strip_array_types (type1);
      return (pod_type_p (type1) || type_code1 == REFERENCE_TYPE
	      || (CLASS_TYPE_P (type1)
		  && TYPE_HAS_TRIVIAL_DESTRUCTOR (type1)));

    case CPTK_HAS_VIRTUAL_DESTRUCTOR:
      return (CLASS_TYPE_P (type1)
	      && (t = locate_dtor (type1, NULL)) && DECL_VIRTUAL_P (t));

    case CPTK_IS_ABSTRACT:
      return (CLASS_TYPE_P (type1) && CLASSTYPE_PURE_VIRTUALS (type1));

    case CPTK_IS_BASE_OF:
      return (NON_UNION_CLASS_TYPE_P (type1) && NON_UNION_CLASS_TYPE_P (type2)
	      && DERIVED_FROM_P (type1, type2));

    case CPTK_IS_CLASS:
      return (NON_UNION_CLASS_TYPE_P (type1));

    case CPTK_IS_CONVERTIBLE_TO:
      /* TODO  */
      return false;

    case CPTK_IS_EMPTY:
      return (NON_UNION_CLASS_TYPE_P (type1) && CLASSTYPE_EMPTY_P (type1));

    case CPTK_IS_ENUM:
      return (type_code1 == ENUMERAL_TYPE);

    case CPTK_IS_POD:
      return (pod_type_p (type1));

    case CPTK_IS_POLYMORPHIC:
      return (CLASS_TYPE_P (type1) && TYPE_POLYMORPHIC_P (type1));

    case CPTK_IS_UNION:
      return (type_code1 == UNION_TYPE);

    default:
      gcc_unreachable ();
      return false;
    }
}

/* Returns true if TYPE is a complete type, an array of unknown bound,
   or (possibly cv-qualified) void, returns false otherwise.  */

static bool
check_trait_type (tree type)
{
  if (COMPLETE_TYPE_P (type))
    return true;

  if (TREE_CODE (type) == ARRAY_TYPE && !TYPE_DOMAIN (type))
    return true;

  if (VOID_TYPE_P (type))
    return true;

  return false;
}

/* Process a trait expression.  */

tree
finish_trait_expr (cp_trait_kind kind, tree type1, tree type2)
{
  gcc_assert (kind == CPTK_HAS_NOTHROW_ASSIGN
	      || kind == CPTK_HAS_NOTHROW_CONSTRUCTOR
	      || kind == CPTK_HAS_NOTHROW_COPY
	      || kind == CPTK_HAS_TRIVIAL_ASSIGN
	      || kind == CPTK_HAS_TRIVIAL_CONSTRUCTOR
	      || kind == CPTK_HAS_TRIVIAL_COPY
	      || kind == CPTK_HAS_TRIVIAL_DESTRUCTOR
	      || kind == CPTK_HAS_VIRTUAL_DESTRUCTOR	      
	      || kind == CPTK_IS_ABSTRACT
	      || kind == CPTK_IS_BASE_OF
	      || kind == CPTK_IS_CLASS
	      || kind == CPTK_IS_CONVERTIBLE_TO
	      || kind == CPTK_IS_EMPTY
	      || kind == CPTK_IS_ENUM
	      || kind == CPTK_IS_POD
	      || kind == CPTK_IS_POLYMORPHIC
	      || kind == CPTK_IS_UNION);

  if (kind == CPTK_IS_CONVERTIBLE_TO)
    {
      sorry ("__is_convertible_to");
      return error_mark_node;
    }

  if (type1 == error_mark_node
      || ((kind == CPTK_IS_BASE_OF || kind == CPTK_IS_CONVERTIBLE_TO)
	  && type2 == error_mark_node))
    return error_mark_node;

  if (processing_template_decl)
    {
      tree trait_expr = make_node (TRAIT_EXPR);
      TREE_TYPE (trait_expr) = boolean_type_node;
      TRAIT_EXPR_TYPE1 (trait_expr) = type1;
      TRAIT_EXPR_TYPE2 (trait_expr) = type2;
      TRAIT_EXPR_KIND (trait_expr) = kind;
      return trait_expr;
    }

  complete_type (type1);
  if (type2)
    complete_type (type2);

  switch (kind)
    {
    case CPTK_HAS_NOTHROW_ASSIGN:
    case CPTK_HAS_TRIVIAL_ASSIGN:
    case CPTK_HAS_NOTHROW_CONSTRUCTOR:
    case CPTK_HAS_TRIVIAL_CONSTRUCTOR:
    case CPTK_HAS_NOTHROW_COPY:
    case CPTK_HAS_TRIVIAL_COPY:
    case CPTK_HAS_TRIVIAL_DESTRUCTOR:
    case CPTK_HAS_VIRTUAL_DESTRUCTOR:
    case CPTK_IS_ABSTRACT:
    case CPTK_IS_EMPTY:
    case CPTK_IS_POD:
    case CPTK_IS_POLYMORPHIC:
      if (!check_trait_type (type1))
	{
	  error ("incomplete type %qT not allowed", type1);
	  return error_mark_node;
	}
      break;

    case CPTK_IS_BASE_OF:
      if (NON_UNION_CLASS_TYPE_P (type1) && NON_UNION_CLASS_TYPE_P (type2)
	  && !same_type_ignoring_top_level_qualifiers_p (type1, type2)
	  && !COMPLETE_TYPE_P (type2))
	{
	  error ("incomplete type %qT not allowed", type2);
	  return error_mark_node;
	}
      break;

    case CPTK_IS_CLASS:
    case CPTK_IS_ENUM:
    case CPTK_IS_UNION:
      break;
    
    case CPTK_IS_CONVERTIBLE_TO:
    default:
      gcc_unreachable ();
    }

  return (trait_expr_value (kind, type1, type2)
	  ? boolean_true_node : boolean_false_node);
}

#include "gt-cp-semantics.h"
