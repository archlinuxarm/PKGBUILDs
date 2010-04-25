/* Dependency analysis
   Copyright (C) 2000, 2001, 2002, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.
   Contributed by Paul Brook <paul@nowt.org>

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

/* dependency.c -- Expression dependency analysis code.  */
/* There's probably quite a bit of duplication in this file.  We currently
   have different dependency checking functions for different types
   if dependencies.  Ideally these would probably be merged.  */
   
#include "config.h"
#include "gfortran.h"
#include "dependency.h"

/* static declarations */
/* Enums  */
enum range {LHS, RHS, MID};

/* Dependency types.  These must be in reverse order of priority.  */
typedef enum
{
  GFC_DEP_ERROR,
  GFC_DEP_EQUAL,	/* Identical Ranges.  */
  GFC_DEP_FORWARD,	/* e.g., a(1:3), a(2:4).  */
  GFC_DEP_OVERLAP,	/* May overlap in some other way.  */
  GFC_DEP_NODEP		/* Distinct ranges.  */
}
gfc_dependency;

/* Macros */
#define IS_ARRAY_EXPLICIT(as) ((as->type == AS_EXPLICIT ? 1 : 0))


/* Returns 1 if the expr is an integer constant value 1, 0 if it is not or
   def if the value could not be determined.  */

int
gfc_expr_is_one (gfc_expr *expr, int def)
{
  gcc_assert (expr != NULL);

  if (expr->expr_type != EXPR_CONSTANT)
    return def;

  if (expr->ts.type != BT_INTEGER)
    return def;

  return mpz_cmp_si (expr->value.integer, 1) == 0;
}


/* Compare two values.  Returns 0 if e1 == e2, -1 if e1 < e2, +1 if e1 > e2,
   and -2 if the relationship could not be determined.  */

int
gfc_dep_compare_expr (gfc_expr *e1, gfc_expr *e2)
{
  gfc_actual_arglist *args1;
  gfc_actual_arglist *args2;
  int i;

  if (e1->expr_type == EXPR_OP
      && (e1->value.op.op == INTRINSIC_UPLUS
	  || e1->value.op.op == INTRINSIC_PARENTHESES))
    return gfc_dep_compare_expr (e1->value.op.op1, e2);
  if (e2->expr_type == EXPR_OP
      && (e2->value.op.op == INTRINSIC_UPLUS
	  || e2->value.op.op == INTRINSIC_PARENTHESES))
    return gfc_dep_compare_expr (e1, e2->value.op.op1);

  if (e1->expr_type == EXPR_OP && e1->value.op.op == INTRINSIC_PLUS)
    {
      /* Compare X+C vs. X.  */
      if (e1->value.op.op2->expr_type == EXPR_CONSTANT
	  && e1->value.op.op2->ts.type == BT_INTEGER
	  && gfc_dep_compare_expr (e1->value.op.op1, e2) == 0)
	return mpz_sgn (e1->value.op.op2->value.integer);

      /* Compare P+Q vs. R+S.  */
      if (e2->expr_type == EXPR_OP && e2->value.op.op == INTRINSIC_PLUS)
	{
	  int l, r;

	  l = gfc_dep_compare_expr (e1->value.op.op1, e2->value.op.op1);
	  r = gfc_dep_compare_expr (e1->value.op.op2, e2->value.op.op2);
	  if (l == 0 && r == 0)
	    return 0;
	  if (l == 0 && r != -2)
	    return r;
	  if (l != -2 && r == 0)
	    return l;
	  if (l == 1 && r == 1)
	    return 1;
	  if (l == -1 && r == -1)
	    return -1;

	  l = gfc_dep_compare_expr (e1->value.op.op1, e2->value.op.op2);
	  r = gfc_dep_compare_expr (e1->value.op.op2, e2->value.op.op1);
	  if (l == 0 && r == 0)
	    return 0;
	  if (l == 0 && r != -2)
	    return r;
	  if (l != -2 && r == 0)
	    return l;
	  if (l == 1 && r == 1)
	    return 1;
	  if (l == -1 && r == -1)
	    return -1;
	}
    }

  /* Compare X vs. X+C.  */
  if (e2->expr_type == EXPR_OP && e2->value.op.op == INTRINSIC_PLUS)
    {
      if (e2->value.op.op2->expr_type == EXPR_CONSTANT
	  && e2->value.op.op2->ts.type == BT_INTEGER
	  && gfc_dep_compare_expr (e1, e2->value.op.op1) == 0)
	return -mpz_sgn (e2->value.op.op2->value.integer);
    }

  /* Compare X-C vs. X.  */
  if (e1->expr_type == EXPR_OP && e1->value.op.op == INTRINSIC_MINUS)
    {
      if (e1->value.op.op2->expr_type == EXPR_CONSTANT
	  && e1->value.op.op2->ts.type == BT_INTEGER
	  && gfc_dep_compare_expr (e1->value.op.op1, e2) == 0)
	return -mpz_sgn (e1->value.op.op2->value.integer);

      /* Compare P-Q vs. R-S.  */
      if (e2->expr_type == EXPR_OP && e2->value.op.op == INTRINSIC_MINUS)
	{
	  int l, r;

	  l = gfc_dep_compare_expr (e1->value.op.op1, e2->value.op.op1);
	  r = gfc_dep_compare_expr (e1->value.op.op2, e2->value.op.op2);
	  if (l == 0 && r == 0)
	    return 0;
	  if (l != -2 && r == 0)
	    return l;
	  if (l == 0 && r != -2)
	    return -r;
	  if (l == 1 && r == -1)
	    return 1;
	  if (l == -1 && r == 1)
	    return -1;
	}
    }

  /* Compare X vs. X-C.  */
  if (e2->expr_type == EXPR_OP && e2->value.op.op == INTRINSIC_MINUS)
    {
      if (e2->value.op.op2->expr_type == EXPR_CONSTANT
	  && e2->value.op.op2->ts.type == BT_INTEGER
	  && gfc_dep_compare_expr (e1, e2->value.op.op1) == 0)
	return mpz_sgn (e2->value.op.op2->value.integer);
    }

  if (e1->expr_type != e2->expr_type)
    return -2;

  switch (e1->expr_type)
    {
    case EXPR_CONSTANT:
      if (e1->ts.type != BT_INTEGER || e2->ts.type != BT_INTEGER)
	return -2;

      i = mpz_cmp (e1->value.integer, e2->value.integer);
      if (i == 0)
	return 0;
      else if (i < 0)
	return -1;
      return 1;

    case EXPR_VARIABLE:
      if (e1->ref || e2->ref)
	return -2;
      if (e1->symtree->n.sym == e2->symtree->n.sym)
	return 0;
      return -2;

    case EXPR_OP:
      /* Intrinsic operators are the same if their operands are the same.  */
      if (e1->value.op.op != e2->value.op.op)
	return -2;
      if (e1->value.op.op2 == 0)
	{
	  i = gfc_dep_compare_expr (e1->value.op.op1, e2->value.op.op1);
	  return i == 0 ? 0 : -2;
	}
      if (gfc_dep_compare_expr (e1->value.op.op1, e2->value.op.op1) == 0
	  && gfc_dep_compare_expr (e1->value.op.op2, e2->value.op.op2) == 0)
	return 0;
      /* TODO Handle commutative binary operators here?  */
      return -2;

    case EXPR_FUNCTION:
      /* We can only compare calls to the same intrinsic function.  */
      if (e1->value.function.isym == 0 || e2->value.function.isym == 0
	  || e1->value.function.isym != e2->value.function.isym)
	return -2;

      args1 = e1->value.function.actual;
      args2 = e2->value.function.actual;

      /* We should list the "constant" intrinsic functions.  Those
	 without side-effects that provide equal results given equal
	 argument lists.  */
      switch (e1->value.function.isym->id)
	{
	case GFC_ISYM_CONVERSION:
	  /* Handle integer extensions specially, as __convert_i4_i8
	     is not only "constant" but also "unary" and "increasing".  */
	  if (args1 && !args1->next
	      && args2 && !args2->next
	      && e1->ts.type == BT_INTEGER
	      && args1->expr->ts.type == BT_INTEGER
	      && e1->ts.kind > args1->expr->ts.kind
	      && e2->ts.type == e1->ts.type
	      && e2->ts.kind == e1->ts.kind
	      && args2->expr->ts.type == args1->expr->ts.type
	      && args2->expr->ts.kind == args2->expr->ts.kind)
	    return gfc_dep_compare_expr (args1->expr, args2->expr);
	  break;

	case GFC_ISYM_REAL:
	case GFC_ISYM_LOGICAL:
	case GFC_ISYM_DBLE:
	  break;

	default:
	  return -2;
	}

      /* Compare the argument lists for equality.  */
      while (args1 && args2)
	{
	  if (gfc_dep_compare_expr (args1->expr, args2->expr) != 0)
	    return -2;
	  args1 = args1->next;
	  args2 = args2->next;
	}
      return (args1 || args2) ? -2 : 0;
      
    default:
      return -2;
    }
}


/* Returns 1 if the two ranges are the same, 0 if they are not, and def
   if the results are indeterminate.  N is the dimension to compare.  */

int
gfc_is_same_range (gfc_array_ref *ar1, gfc_array_ref *ar2, int n, int def)
{
  gfc_expr *e1;
  gfc_expr *e2;
  int i;

  /* TODO: More sophisticated range comparison.  */
  gcc_assert (ar1 && ar2);

  gcc_assert (ar1->dimen_type[n] == ar2->dimen_type[n]);

  e1 = ar1->stride[n];
  e2 = ar2->stride[n];
  /* Check for mismatching strides.  A NULL stride means a stride of 1.  */
  if (e1 && !e2)
    {
      i = gfc_expr_is_one (e1, -1);
      if (i == -1)
	return def;
      else if (i == 0)
	return 0;
    }
  else if (e2 && !e1)
    {
      i = gfc_expr_is_one (e2, -1);
      if (i == -1)
	return def;
      else if (i == 0)
	return 0;
    }
  else if (e1 && e2)
    {
      i = gfc_dep_compare_expr (e1, e2);
      if (i == -2)
	return def;
      else if (i != 0)
	return 0;
    }
  /* The strides match.  */

  /* Check the range start.  */
  e1 = ar1->start[n];
  e2 = ar2->start[n];
  if (e1 || e2)
    {
      /* Use the bound of the array if no bound is specified.  */
      if (ar1->as && !e1)
	e1 = ar1->as->lower[n];

      if (ar2->as && !e2)
	e2 = ar2->as->lower[n];

      /* Check we have values for both.  */
      if (!(e1 && e2))
	return def;

      i = gfc_dep_compare_expr (e1, e2);
      if (i == -2)
	return def;
      else if (i != 0)
	return 0;
    }

  /* Check the range end.  */
  e1 = ar1->end[n];
  e2 = ar2->end[n];
  if (e1 || e2)
    {
      /* Use the bound of the array if no bound is specified.  */
      if (ar1->as && !e1)
	e1 = ar1->as->upper[n];

      if (ar2->as && !e2)
	e2 = ar2->as->upper[n];

      /* Check we have values for both.  */
      if (!(e1 && e2))
	return def;

      i = gfc_dep_compare_expr (e1, e2);
      if (i == -2)
	return def;
      else if (i != 0)
	return 0;
    }

  return 1;
}


/* Some array-returning intrinsics can be implemented by reusing the
   data from one of the array arguments.  For example, TRANSPOSE does
   not necessarily need to allocate new data: it can be implemented
   by copying the original array's descriptor and simply swapping the
   two dimension specifications.

   If EXPR is a call to such an intrinsic, return the argument
   whose data can be reused, otherwise return NULL.  */

gfc_expr *
gfc_get_noncopying_intrinsic_argument (gfc_expr *expr)
{
  if (expr->expr_type != EXPR_FUNCTION || !expr->value.function.isym)
    return NULL;

  switch (expr->value.function.isym->id)
    {
    case GFC_ISYM_TRANSPOSE:
      return expr->value.function.actual->expr;

    default:
      return NULL;
    }
}


/* Return true if the result of reference REF can only be constructed
   using a temporary array.  */

bool
gfc_ref_needs_temporary_p (gfc_ref *ref)
{
  int n;
  bool subarray_p;

  subarray_p = false;
  for (; ref; ref = ref->next)
    switch (ref->type)
      {
      case REF_ARRAY:
	/* Vector dimensions are generally not monotonic and must be
	   handled using a temporary.  */
	if (ref->u.ar.type == AR_SECTION)
	  for (n = 0; n < ref->u.ar.dimen; n++)
	    if (ref->u.ar.dimen_type[n] == DIMEN_VECTOR)
	      return true;

	subarray_p = true;
	break;

      case REF_SUBSTRING:
	/* Within an array reference, character substrings generally
	   need a temporary.  Character array strides are expressed as
	   multiples of the element size (consistent with other array
	   types), not in characters.  */
	return subarray_p;

      case REF_COMPONENT:
	break;
      }

  return false;
}


int
gfc_is_data_pointer (gfc_expr *e)
{
  gfc_ref *ref;

  if (e->expr_type != EXPR_VARIABLE && e->expr_type != EXPR_FUNCTION)
    return 0;

  /* No subreference if it is a function  */
  gcc_assert (e->expr_type == EXPR_VARIABLE || !e->ref);

  if (e->symtree->n.sym->attr.pointer)
    return 1;

  for (ref = e->ref; ref; ref = ref->next)
    if (ref->type == REF_COMPONENT && ref->u.c.component->attr.pointer)
      return 1;

  return 0;
}


/* Return true if array variable VAR could be passed to the same function
   as argument EXPR without interfering with EXPR.  INTENT is the intent
   of VAR.

   This is considerably less conservative than other dependencies
   because many function arguments will already be copied into a
   temporary.  */

static int
gfc_check_argument_var_dependency (gfc_expr *var, sym_intent intent,
				   gfc_expr *expr, gfc_dep_check elemental)
{
  gfc_expr *arg;

  gcc_assert (var->expr_type == EXPR_VARIABLE);
  gcc_assert (var->rank > 0);

  switch (expr->expr_type)
    {
    case EXPR_VARIABLE:
      /* In case of elemental subroutines, there is no dependency 
         between two same-range array references.  */
      if (gfc_ref_needs_temporary_p (expr->ref)
	  || gfc_check_dependency (var, expr, !elemental))
	{
	  if (elemental == ELEM_DONT_CHECK_VARIABLE)
	    {
	      /* Too many false positive with pointers.  */
	      if (!gfc_is_data_pointer (var) && !gfc_is_data_pointer (expr))
		{
		  /* Elemental procedures forbid unspecified intents, 
		     and we don't check dependencies for INTENT_IN args.  */
		  gcc_assert (intent == INTENT_OUT || intent == INTENT_INOUT);

		  /* We are told not to check dependencies. 
		     We do it, however, and issue a warning in case we find one.
		     If a dependency is found in the case 
		     elemental == ELEM_CHECK_VARIABLE, we will generate
		     a temporary, so we don't need to bother the user.  */
		  gfc_warning ("INTENT(%s) actual argument at %L might "
			       "interfere with actual argument at %L.", 
		   	       intent == INTENT_OUT ? "OUT" : "INOUT", 
		   	       &var->where, &expr->where);
		}
	      return 0;
	    }
	  else
	    return 1; 
	}
      return 0;

    case EXPR_ARRAY:
      return gfc_check_dependency (var, expr, 1);

    case EXPR_FUNCTION:
      if (intent != INTENT_IN && expr->inline_noncopying_intrinsic
	  && (arg = gfc_get_noncopying_intrinsic_argument (expr))
	  && gfc_check_argument_var_dependency (var, intent, arg, elemental))
	return 1;
      if (elemental)
	{
	  if ((expr->value.function.esym
	       && expr->value.function.esym->attr.elemental)
	      || (expr->value.function.isym
		  && expr->value.function.isym->elemental))
	    return gfc_check_fncall_dependency (var, intent, NULL,
						expr->value.function.actual,
						ELEM_CHECK_VARIABLE);
	}
      return 0;

    case EXPR_OP:
      /* In case of non-elemental procedures, there is no need to catch
	 dependencies, as we will make a temporary anyway.  */
      if (elemental)
	{
	  /* If the actual arg EXPR is an expression, we need to catch 
	     a dependency between variables in EXPR and VAR, 
	     an intent((IN)OUT) variable.  */
	  if (expr->value.op.op1
	      && gfc_check_argument_var_dependency (var, intent, 
						    expr->value.op.op1, 
						    ELEM_CHECK_VARIABLE))
	    return 1;
	  else if (expr->value.op.op2
		   && gfc_check_argument_var_dependency (var, intent, 
							 expr->value.op.op2, 
							 ELEM_CHECK_VARIABLE))
	    return 1;
	}
      return 0;

    default:
      return 0;
    }
}
  
  
/* Like gfc_check_argument_var_dependency, but extended to any
   array expression OTHER, not just variables.  */

static int
gfc_check_argument_dependency (gfc_expr *other, sym_intent intent,
			       gfc_expr *expr, gfc_dep_check elemental)
{
  switch (other->expr_type)
    {
    case EXPR_VARIABLE:
      return gfc_check_argument_var_dependency (other, intent, expr, elemental);

    case EXPR_FUNCTION:
      if (other->inline_noncopying_intrinsic)
	{
	  other = gfc_get_noncopying_intrinsic_argument (other);
	  return gfc_check_argument_dependency (other, INTENT_IN, expr, 
						elemental);
	}
      return 0;

    default:
      return 0;
    }
}


/* Like gfc_check_argument_dependency, but check all the arguments in ACTUAL.
   FNSYM is the function being called, or NULL if not known.  */

int
gfc_check_fncall_dependency (gfc_expr *other, sym_intent intent,
			     gfc_symbol *fnsym, gfc_actual_arglist *actual,
			     gfc_dep_check elemental)
{
  gfc_formal_arglist *formal;
  gfc_expr *expr;

  formal = fnsym ? fnsym->formal : NULL;
  for (; actual; actual = actual->next, formal = formal ? formal->next : NULL)
    {
      expr = actual->expr;

      /* Skip args which are not present.  */
      if (!expr)
	continue;

      /* Skip other itself.  */
      if (expr == other)
	continue;

      /* Skip intent(in) arguments if OTHER itself is intent(in).  */
      if (formal && intent == INTENT_IN
	  && formal->sym->attr.intent == INTENT_IN)
	continue;

      if (gfc_check_argument_dependency (other, intent, expr, elemental))
	return 1;
    }

  return 0;
}


/* Return 1 if e1 and e2 are equivalenced arrays, either
   directly or indirectly; i.e., equivalence (a,b) for a and b
   or equivalence (a,c),(b,c).  This function uses the equiv_
   lists, generated in trans-common(add_equivalences), that are
   guaranteed to pick up indirect equivalences.  We explicitly
   check for overlap using the offset and length of the equivalence.
   This function is symmetric.
   TODO: This function only checks whether the full top-level
   symbols overlap.  An improved implementation could inspect
   e1->ref and e2->ref to determine whether the actually accessed
   portions of these variables/arrays potentially overlap.  */

int
gfc_are_equivalenced_arrays (gfc_expr *e1, gfc_expr *e2)
{
  gfc_equiv_list *l;
  gfc_equiv_info *s, *fl1, *fl2;

  gcc_assert (e1->expr_type == EXPR_VARIABLE
	      && e2->expr_type == EXPR_VARIABLE);

  if (!e1->symtree->n.sym->attr.in_equivalence
      || !e2->symtree->n.sym->attr.in_equivalence|| !e1->rank || !e2->rank)
    return 0;

  if (e1->symtree->n.sym->ns
	&& e1->symtree->n.sym->ns != gfc_current_ns)
    l = e1->symtree->n.sym->ns->equiv_lists;
  else
    l = gfc_current_ns->equiv_lists;

  /* Go through the equiv_lists and return 1 if the variables
     e1 and e2 are members of the same group and satisfy the
     requirement on their relative offsets.  */
  for (; l; l = l->next)
    {
      fl1 = NULL;
      fl2 = NULL;
      for (s = l->equiv; s; s = s->next)
	{
	  if (s->sym == e1->symtree->n.sym)
	    {
	      fl1 = s;
	      if (fl2)
		break;
	    }
	  if (s->sym == e2->symtree->n.sym)
	    {
	      fl2 = s;
	      if (fl1)
		break;
	    }
	}

      if (s)
	{
	  /* Can these lengths be zero?  */
	  if (fl1->length <= 0 || fl2->length <= 0)
	    return 1;
	  /* These can't overlap if [f11,fl1+length] is before 
	     [fl2,fl2+length], or [fl2,fl2+length] is before
	     [fl1,fl1+length], otherwise they do overlap.  */
	  if (fl1->offset + fl1->length > fl2->offset
	      && fl2->offset + fl2->length > fl1->offset)
	    return 1;
	}
    }
  return 0;
}


/* Return true if the statement body redefines the condition.  Returns
   true if expr2 depends on expr1.  expr1 should be a single term
   suitable for the lhs of an assignment.  The IDENTICAL flag indicates
   whether array references to the same symbol with identical range
   references count as a dependency or not.  Used for forall and where
   statements.  Also used with functions returning arrays without a
   temporary.  */

int
gfc_check_dependency (gfc_expr *expr1, gfc_expr *expr2, bool identical)
{
  gfc_actual_arglist *actual;
  gfc_constructor *c;
  int n;

  gcc_assert (expr1->expr_type == EXPR_VARIABLE);

  switch (expr2->expr_type)
    {
    case EXPR_OP:
      n = gfc_check_dependency (expr1, expr2->value.op.op1, identical);
      if (n)
	return n;
      if (expr2->value.op.op2)
	return gfc_check_dependency (expr1, expr2->value.op.op2, identical);
      return 0;

    case EXPR_VARIABLE:
      /* The interesting cases are when the symbols don't match.  */
      if (expr1->symtree->n.sym != expr2->symtree->n.sym)
	{
	  gfc_typespec *ts1 = &expr1->symtree->n.sym->ts;
	  gfc_typespec *ts2 = &expr2->symtree->n.sym->ts;

	  /* Return 1 if expr1 and expr2 are equivalenced arrays.  */
	  if (gfc_are_equivalenced_arrays (expr1, expr2))
	    return 1;

	  /* Symbols can only alias if they have the same type.  */
	  if (ts1->type != BT_UNKNOWN && ts2->type != BT_UNKNOWN
	      && ts1->type != BT_DERIVED && ts2->type != BT_DERIVED)
	    {
	      if (ts1->type != ts2->type || ts1->kind != ts2->kind)
		return 0;
	    }

	  /* If either variable is a pointer, assume the worst.  */
	  /* TODO: -fassume-no-pointer-aliasing */
	  if (gfc_is_data_pointer (expr1) || gfc_is_data_pointer (expr2))
	    return 1;

	  /* Otherwise distinct symbols have no dependencies.  */
	  return 0;
	}

      if (identical)
	return 1;

      /* Identical and disjoint ranges return 0,
	 overlapping ranges return 1.  */
      if (expr1->ref && expr2->ref)
	return gfc_dep_resolver (expr1->ref, expr2->ref);

      return 1;

    case EXPR_FUNCTION:
      if (expr2->inline_noncopying_intrinsic)
	identical = 1;
      /* Remember possible differences between elemental and
	 transformational functions.  All functions inside a FORALL
	 will be pure.  */
      for (actual = expr2->value.function.actual;
	   actual; actual = actual->next)
	{
	  if (!actual->expr)
	    continue;
	  n = gfc_check_dependency (expr1, actual->expr, identical);
	  if (n)
	    return n;
	}
      return 0;

    case EXPR_CONSTANT:
    case EXPR_NULL:
      return 0;

    case EXPR_ARRAY:
      /* Loop through the array constructor's elements.  */
      for (c = expr2->value.constructor; c; c = c->next)
	{
	  /* If this is an iterator, assume the worst.  */
	  if (c->iterator)
	    return 1;
	  /* Avoid recursion in the common case.  */
	  if (c->expr->expr_type == EXPR_CONSTANT)
	    continue;
	  if (gfc_check_dependency (expr1, c->expr, 1))
	    return 1;
	}
      return 0;

    default:
      return 1;
    }
}


/* Determines overlapping for two array sections.  */

static gfc_dependency
gfc_check_section_vs_section (gfc_ref *lref, gfc_ref *rref, int n)
{
  gfc_array_ref l_ar;
  gfc_expr *l_start;
  gfc_expr *l_end;
  gfc_expr *l_stride;
  gfc_expr *l_lower;
  gfc_expr *l_upper;
  int l_dir;

  gfc_array_ref r_ar;
  gfc_expr *r_start;
  gfc_expr *r_end;
  gfc_expr *r_stride;
  gfc_expr *r_lower;
  gfc_expr *r_upper;
  int r_dir;

  l_ar = lref->u.ar;
  r_ar = rref->u.ar;
  
  /* If they are the same range, return without more ado.  */
  if (gfc_is_same_range (&l_ar, &r_ar, n, 0))
    return GFC_DEP_EQUAL;

  l_start = l_ar.start[n];
  l_end = l_ar.end[n];
  l_stride = l_ar.stride[n];

  r_start = r_ar.start[n];
  r_end = r_ar.end[n];
  r_stride = r_ar.stride[n];

  /* If l_start is NULL take it from array specifier.  */
  if (NULL == l_start && IS_ARRAY_EXPLICIT (l_ar.as))
    l_start = l_ar.as->lower[n];
  /* If l_end is NULL take it from array specifier.  */
  if (NULL == l_end && IS_ARRAY_EXPLICIT (l_ar.as))
    l_end = l_ar.as->upper[n];

  /* If r_start is NULL take it from array specifier.  */
  if (NULL == r_start && IS_ARRAY_EXPLICIT (r_ar.as))
    r_start = r_ar.as->lower[n];
  /* If r_end is NULL take it from array specifier.  */
  if (NULL == r_end && IS_ARRAY_EXPLICIT (r_ar.as))
    r_end = r_ar.as->upper[n];

  /* Determine whether the l_stride is positive or negative.  */
  if (!l_stride)
    l_dir = 1;
  else if (l_stride->expr_type == EXPR_CONSTANT
	   && l_stride->ts.type == BT_INTEGER)
    l_dir = mpz_sgn (l_stride->value.integer);
  else if (l_start && l_end)
    l_dir = gfc_dep_compare_expr (l_end, l_start);
  else
    l_dir = -2;

  /* Determine whether the r_stride is positive or negative.  */
  if (!r_stride)
    r_dir = 1;
  else if (r_stride->expr_type == EXPR_CONSTANT
	   && r_stride->ts.type == BT_INTEGER)
    r_dir = mpz_sgn (r_stride->value.integer);
  else if (r_start && r_end)
    r_dir = gfc_dep_compare_expr (r_end, r_start);
  else
    r_dir = -2;

  /* The strides should never be zero.  */
  if (l_dir == 0 || r_dir == 0)
    return GFC_DEP_OVERLAP;

  /* Determine LHS upper and lower bounds.  */
  if (l_dir == 1)
    {
      l_lower = l_start;
      l_upper = l_end;
    }
  else if (l_dir == -1)
    {
      l_lower = l_end;
      l_upper = l_start;
    }
  else
    {
      l_lower = NULL;
      l_upper = NULL;
    }

  /* Determine RHS upper and lower bounds.  */
  if (r_dir == 1)
    {
      r_lower = r_start;
      r_upper = r_end;
    }
  else if (r_dir == -1)
    {
      r_lower = r_end;
      r_upper = r_start;
    }
  else
    {
      r_lower = NULL;
      r_upper = NULL;
    }

  /* Check whether the ranges are disjoint.  */
  if (l_upper && r_lower && gfc_dep_compare_expr (l_upper, r_lower) == -1)
    return GFC_DEP_NODEP;
  if (r_upper && l_lower && gfc_dep_compare_expr (r_upper, l_lower) == -1)
    return GFC_DEP_NODEP;

  /* Handle cases like x:y:1 vs. x:z:-1 as GFC_DEP_EQUAL.  */
  if (l_start && r_start && gfc_dep_compare_expr (l_start, r_start) == 0)
    {
      if (l_dir == 1 && r_dir == -1)
	return GFC_DEP_EQUAL;
      if (l_dir == -1 && r_dir == 1)
	return GFC_DEP_EQUAL;
    }

  /* Handle cases like x:y:1 vs. z:y:-1 as GFC_DEP_EQUAL.  */
  if (l_end && r_end && gfc_dep_compare_expr (l_end, r_end) == 0)
    {
      if (l_dir == 1 && r_dir == -1)
	return GFC_DEP_EQUAL;
      if (l_dir == -1 && r_dir == 1)
	return GFC_DEP_EQUAL;
    }

  /* Check for forward dependencies x:y vs. x+1:z.  */
  if (l_dir == 1 && r_dir == 1
      && l_start && r_start && gfc_dep_compare_expr (l_start, r_start) == -1
      && l_end && r_end && gfc_dep_compare_expr (l_end, r_end) == -1)
    {
      /* Check that the strides are the same.  */
      if (!l_stride && !r_stride)
	return GFC_DEP_FORWARD;
      if (l_stride && r_stride
	  && gfc_dep_compare_expr (l_stride, r_stride) == 0)
	return GFC_DEP_FORWARD;
    }

  /* Check for forward dependencies x:y:-1 vs. x-1:z:-1.  */
  if (l_dir == -1 && r_dir == -1
      && l_start && r_start && gfc_dep_compare_expr (l_start, r_start) == 1
      && l_end && r_end && gfc_dep_compare_expr (l_end, r_end) == 1)
    {
      /* Check that the strides are the same.  */
      if (!l_stride && !r_stride)
	return GFC_DEP_FORWARD;
      if (l_stride && r_stride
	  && gfc_dep_compare_expr (l_stride, r_stride) == 0)
	return GFC_DEP_FORWARD;
    }

  return GFC_DEP_OVERLAP;
}


/* Determines overlapping for a single element and a section.  */

static gfc_dependency
gfc_check_element_vs_section( gfc_ref *lref, gfc_ref *rref, int n)
{
  gfc_array_ref *ref;
  gfc_expr *elem;
  gfc_expr *start;
  gfc_expr *end;
  gfc_expr *stride;
  int s;

  elem = lref->u.ar.start[n];
  if (!elem)
    return GFC_DEP_OVERLAP;

  ref = &rref->u.ar;
  start = ref->start[n] ;
  end = ref->end[n] ;
  stride = ref->stride[n];

  if (!start && IS_ARRAY_EXPLICIT (ref->as))
    start = ref->as->lower[n];
  if (!end && IS_ARRAY_EXPLICIT (ref->as))
    end = ref->as->upper[n];

  /* Determine whether the stride is positive or negative.  */
  if (!stride)
    s = 1;
  else if (stride->expr_type == EXPR_CONSTANT
	   && stride->ts.type == BT_INTEGER)
    s = mpz_sgn (stride->value.integer);
  else
    s = -2;

  /* Stride should never be zero.  */
  if (s == 0)
    return GFC_DEP_OVERLAP;

  /* Positive strides.  */
  if (s == 1)
    {
      /* Check for elem < lower.  */
      if (start && gfc_dep_compare_expr (elem, start) == -1)
	return GFC_DEP_NODEP;
      /* Check for elem > upper.  */
      if (end && gfc_dep_compare_expr (elem, end) == 1)
	return GFC_DEP_NODEP;

      if (start && end)
	{
	  s = gfc_dep_compare_expr (start, end);
	  /* Check for an empty range.  */
	  if (s == 1)
	    return GFC_DEP_NODEP;
	  if (s == 0 && gfc_dep_compare_expr (elem, start) == 0)
	    return GFC_DEP_EQUAL;
	}
    }
  /* Negative strides.  */
  else if (s == -1)
    {
      /* Check for elem > upper.  */
      if (end && gfc_dep_compare_expr (elem, start) == 1)
	return GFC_DEP_NODEP;
      /* Check for elem < lower.  */
      if (start && gfc_dep_compare_expr (elem, end) == -1)
	return GFC_DEP_NODEP;

      if (start && end)
	{
	  s = gfc_dep_compare_expr (start, end);
	  /* Check for an empty range.  */
	  if (s == -1)
	    return GFC_DEP_NODEP;
	  if (s == 0 && gfc_dep_compare_expr (elem, start) == 0)
	    return GFC_DEP_EQUAL;
	}
    }
  /* Unknown strides.  */
  else
    {
      if (!start || !end)
	return GFC_DEP_OVERLAP;
      s = gfc_dep_compare_expr (start, end);
      if (s == -2)
	return GFC_DEP_OVERLAP;
      /* Assume positive stride.  */
      if (s == -1)
	{
	  /* Check for elem < lower.  */
	  if (gfc_dep_compare_expr (elem, start) == -1)
	    return GFC_DEP_NODEP;
	  /* Check for elem > upper.  */
	  if (gfc_dep_compare_expr (elem, end) == 1)
	    return GFC_DEP_NODEP;
	}
      /* Assume negative stride.  */
      else if (s == 1)
	{
	  /* Check for elem > upper.  */
	  if (gfc_dep_compare_expr (elem, start) == 1)
	    return GFC_DEP_NODEP;
	  /* Check for elem < lower.  */
	  if (gfc_dep_compare_expr (elem, end) == -1)
	    return GFC_DEP_NODEP;
	}
      /* Equal bounds.  */
      else if (s == 0)
	{
	  s = gfc_dep_compare_expr (elem, start);
	  if (s == 0)
	    return GFC_DEP_EQUAL;
	  if (s == 1 || s == -1)
	    return GFC_DEP_NODEP;
	}
    }

  return GFC_DEP_OVERLAP;
}


/* Traverse expr, checking all EXPR_VARIABLE symbols for their
   forall_index attribute.  Return true if any variable may be
   being used as a FORALL index.  Its safe to pessimistically
   return true, and assume a dependency.  */

static bool
contains_forall_index_p (gfc_expr *expr)
{
  gfc_actual_arglist *arg;
  gfc_constructor *c;
  gfc_ref *ref;
  int i;

  if (!expr)
    return false;

  switch (expr->expr_type)
    {
    case EXPR_VARIABLE:
      if (expr->symtree->n.sym->forall_index)
	return true;
      break;

    case EXPR_OP:
      if (contains_forall_index_p (expr->value.op.op1)
	  || contains_forall_index_p (expr->value.op.op2))
	return true;
      break;

    case EXPR_FUNCTION:
      for (arg = expr->value.function.actual; arg; arg = arg->next)
	if (contains_forall_index_p (arg->expr))
	  return true;
      break;

    case EXPR_CONSTANT:
    case EXPR_NULL:
    case EXPR_SUBSTRING:
      break;

    case EXPR_STRUCTURE:
    case EXPR_ARRAY:
      for (c = expr->value.constructor; c; c = c->next)
	if (contains_forall_index_p (c->expr))
	  return true;
      break;

    default:
      gcc_unreachable ();
    }

  for (ref = expr->ref; ref; ref = ref->next)
    switch (ref->type)
      {
      case REF_ARRAY:
	for (i = 0; i < ref->u.ar.dimen; i++)
	  if (contains_forall_index_p (ref->u.ar.start[i])
	      || contains_forall_index_p (ref->u.ar.end[i])
	      || contains_forall_index_p (ref->u.ar.stride[i]))
	    return true;
	break;

      case REF_COMPONENT:
	break;

      case REF_SUBSTRING:
	if (contains_forall_index_p (ref->u.ss.start)
	    || contains_forall_index_p (ref->u.ss.end))
	  return true;
	break;

      default:
	gcc_unreachable ();
      }

  return false;
}

/* Determines overlapping for two single element array references.  */

static gfc_dependency
gfc_check_element_vs_element (gfc_ref *lref, gfc_ref *rref, int n)
{
  gfc_array_ref l_ar;
  gfc_array_ref r_ar;
  gfc_expr *l_start;
  gfc_expr *r_start;
  int i;

  l_ar = lref->u.ar;
  r_ar = rref->u.ar;
  l_start = l_ar.start[n] ;
  r_start = r_ar.start[n] ;
  i = gfc_dep_compare_expr (r_start, l_start);
  if (i == 0)
    return GFC_DEP_EQUAL;

  /* Treat two scalar variables as potentially equal.  This allows
     us to prove that a(i,:) and a(j,:) have no dependency.  See
     Gerald Roth, "Evaluation of Array Syntax Dependence Analysis",
     Proceedings of the International Conference on Parallel and
     Distributed Processing Techniques and Applications (PDPTA2001),
     Las Vegas, Nevada, June 2001.  */
  /* However, we need to be careful when either scalar expression
     contains a FORALL index, as these can potentially change value
     during the scalarization/traversal of this array reference.  */
  if (contains_forall_index_p (r_start) || contains_forall_index_p (l_start))
    return GFC_DEP_OVERLAP;

  if (i != -2)
    return GFC_DEP_NODEP;
  return GFC_DEP_EQUAL;
}


/* Determine if an array ref, usually an array section specifies the
   entire array.  In addition, if the second, pointer argument is
   provided, the function will return true if the reference is
   contiguous; eg. (:, 1) gives true but (1,:) gives false.  */

bool
gfc_full_array_ref_p (gfc_ref *ref, bool *contiguous)
{
  int i;
  bool lbound_OK = true;
  bool ubound_OK = true;

  if (contiguous)
    *contiguous = false;

  if (ref->type != REF_ARRAY)
    return false;
  if (ref->u.ar.type == AR_FULL)
    {
      if (contiguous)
	*contiguous = true;
      return true;
    }
  if (ref->u.ar.type != AR_SECTION)
    return false;
  if (ref->next)
    return false;

  for (i = 0; i < ref->u.ar.dimen; i++)
    {
      /* If we have a single element in the reference, we need to check
	 that the array has a single element and that we actually reference
	 the correct element.  */
      if (ref->u.ar.dimen_type[i] == DIMEN_ELEMENT)
	{
	  /* This is a contiguous reference.  */
	  if (contiguous)
	    *contiguous = (i + 1 == ref->u.ar.dimen);

	  if (!ref->u.ar.as
	      || !ref->u.ar.as->lower[i]
	      || !ref->u.ar.as->upper[i]
	      || gfc_dep_compare_expr (ref->u.ar.as->lower[i],
				       ref->u.ar.as->upper[i])
	      || !ref->u.ar.start[i]
	      || gfc_dep_compare_expr (ref->u.ar.start[i],
				       ref->u.ar.as->lower[i]))
	    return false;
	  else
	    continue;
	}

      /* Check the lower bound.  */
      if (ref->u.ar.start[i]
	  && (!ref->u.ar.as
	      || !ref->u.ar.as->lower[i]
	      || gfc_dep_compare_expr (ref->u.ar.start[i],
				       ref->u.ar.as->lower[i])))
	lbound_OK = false;
      /* Check the upper bound.  */
      if (ref->u.ar.end[i]
	  && (!ref->u.ar.as
	      || !ref->u.ar.as->upper[i]
	      || gfc_dep_compare_expr (ref->u.ar.end[i],
				       ref->u.ar.as->upper[i])))
	ubound_OK = false;
      /* Check the stride.  */
      if (ref->u.ar.stride[i] && !gfc_expr_is_one (ref->u.ar.stride[i], 0))
	return false;

      /* This is a contiguous reference.  */
      if (contiguous)
	*contiguous = (i + 1 == ref->u.ar.dimen);

      if (!lbound_OK || !ubound_OK)
	return false;
    }
  return true;
}


/* Finds if two array references are overlapping or not.
   Return value
   	1 : array references are overlapping.
   	0 : array references are identical or not overlapping.  */

int
gfc_dep_resolver (gfc_ref *lref, gfc_ref *rref)
{
  int n;
  gfc_dependency fin_dep;
  gfc_dependency this_dep;

  fin_dep = GFC_DEP_ERROR;
  /* Dependencies due to pointers should already have been identified.
     We only need to check for overlapping array references.  */

  while (lref && rref)
    {
      /* We're resolving from the same base symbol, so both refs should be
	 the same type.  We traverse the reference chain until we find ranges
	 that are not equal.  */
      gcc_assert (lref->type == rref->type);
      switch (lref->type)
	{
	case REF_COMPONENT:
	  /* The two ranges can't overlap if they are from different
	     components.  */
	  if (lref->u.c.component != rref->u.c.component)
	    return 0;
	  break;
	  
	case REF_SUBSTRING:
	  /* Substring overlaps are handled by the string assignment code
	     if there is not an underlying dependency.  */
	  return (fin_dep == GFC_DEP_OVERLAP) ? 1 : 0;
	
	case REF_ARRAY:
	  if (lref->u.ar.dimen != rref->u.ar.dimen)
	    {
	      if (lref->u.ar.type == AR_FULL)
		fin_dep = gfc_full_array_ref_p (rref, NULL) ? GFC_DEP_EQUAL
							    : GFC_DEP_OVERLAP;
	      else if (rref->u.ar.type == AR_FULL)
		fin_dep = gfc_full_array_ref_p (lref, NULL) ? GFC_DEP_EQUAL
							    : GFC_DEP_OVERLAP;
	      else
		return 1;
	      break;
	    }

	  for (n=0; n < lref->u.ar.dimen; n++)
	    {
	      /* Assume dependency when either of array reference is vector
		 subscript.  */
	      if (lref->u.ar.dimen_type[n] == DIMEN_VECTOR
		  || rref->u.ar.dimen_type[n] == DIMEN_VECTOR)
		return 1;
	      if (lref->u.ar.dimen_type[n] == DIMEN_RANGE
		  && rref->u.ar.dimen_type[n] == DIMEN_RANGE)
		this_dep = gfc_check_section_vs_section (lref, rref, n);
	      else if (lref->u.ar.dimen_type[n] == DIMEN_ELEMENT
		       && rref->u.ar.dimen_type[n] == DIMEN_RANGE)
		this_dep = gfc_check_element_vs_section (lref, rref, n);
	      else if (rref->u.ar.dimen_type[n] == DIMEN_ELEMENT
		       && lref->u.ar.dimen_type[n] == DIMEN_RANGE)
		this_dep = gfc_check_element_vs_section (rref, lref, n);
	      else 
		{
		  gcc_assert (rref->u.ar.dimen_type[n] == DIMEN_ELEMENT
			      && lref->u.ar.dimen_type[n] == DIMEN_ELEMENT);
		  this_dep = gfc_check_element_vs_element (rref, lref, n);
		}

	      /* If any dimension doesn't overlap, we have no dependency.  */
	      if (this_dep == GFC_DEP_NODEP)
		return 0;

	      /* Overlap codes are in order of priority.  We only need to
		 know the worst one.*/
	      if (this_dep > fin_dep)
		fin_dep = this_dep;
	    }

	  /* If this is an equal element, we have to keep going until we find
	     the "real" array reference.  */
	  if (lref->u.ar.type == AR_ELEMENT
		&& rref->u.ar.type == AR_ELEMENT
		&& fin_dep == GFC_DEP_EQUAL)
	    break;

	  /* Exactly matching and forward overlapping ranges don't cause a
	     dependency.  */
	  if (fin_dep < GFC_DEP_OVERLAP)
	    return 0;

	  /* Keep checking.  We only have a dependency if
	     subsequent references also overlap.  */
	  break;

	default:
	  gcc_unreachable ();
	}
      lref = lref->next;
      rref = rref->next;
    }

  /* If we haven't seen any array refs then something went wrong.  */
  gcc_assert (fin_dep != GFC_DEP_ERROR);

  /* Assume the worst if we nest to different depths.  */
  if (lref || rref)
    return 1;

  return fin_dep == GFC_DEP_OVERLAP;
}

