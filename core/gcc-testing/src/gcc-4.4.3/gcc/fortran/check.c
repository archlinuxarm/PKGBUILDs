/* Check functions
   Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.
   Contributed by Andy Vaught & Katherine Holcomb

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


/* These functions check to see if an argument list is compatible with
   a particular intrinsic function or subroutine.  Presence of
   required arguments has already been established, the argument list
   has been sorted into the right order and has NULL arguments in the
   correct places for missing optional arguments.  */

#include "config.h"
#include "system.h"
#include "flags.h"
#include "gfortran.h"
#include "intrinsic.h"


/* Make sure an expression is a scalar.  */

static gfc_try
scalar_check (gfc_expr *e, int n)
{
  if (e->rank == 0)
    return SUCCESS;

  gfc_error ("'%s' argument of '%s' intrinsic at %L must be a scalar",
	     gfc_current_intrinsic_arg[n], gfc_current_intrinsic, &e->where);

  return FAILURE;
}


/* Check the type of an expression.  */

static gfc_try
type_check (gfc_expr *e, int n, bt type)
{
  if (e->ts.type == type)
    return SUCCESS;

  gfc_error ("'%s' argument of '%s' intrinsic at %L must be %s",
	     gfc_current_intrinsic_arg[n], gfc_current_intrinsic, &e->where,
	     gfc_basic_typename (type));

  return FAILURE;
}


/* Check that the expression is a numeric type.  */

static gfc_try
numeric_check (gfc_expr *e, int n)
{
  if (gfc_numeric_ts (&e->ts))
    return SUCCESS;

  /* If the expression has not got a type, check if its namespace can
     offer a default type.  */
  if ((e->expr_type == EXPR_VARIABLE || e->expr_type == EXPR_VARIABLE)
	&& e->symtree->n.sym->ts.type == BT_UNKNOWN
	&& gfc_set_default_type (e->symtree->n.sym, 0,
				 e->symtree->n.sym->ns) == SUCCESS
	&& gfc_numeric_ts (&e->symtree->n.sym->ts))
    {
      e->ts = e->symtree->n.sym->ts;
      return SUCCESS;
    }

  gfc_error ("'%s' argument of '%s' intrinsic at %L must be a numeric type",
	     gfc_current_intrinsic_arg[n], gfc_current_intrinsic, &e->where);

  return FAILURE;
}


/* Check that an expression is integer or real.  */

static gfc_try
int_or_real_check (gfc_expr *e, int n)
{
  if (e->ts.type != BT_INTEGER && e->ts.type != BT_REAL)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be INTEGER "
		 "or REAL", gfc_current_intrinsic_arg[n],
		 gfc_current_intrinsic, &e->where);
      return FAILURE;
    }

  return SUCCESS;
}


/* Check that an expression is real or complex.  */

static gfc_try
real_or_complex_check (gfc_expr *e, int n)
{
  if (e->ts.type != BT_REAL && e->ts.type != BT_COMPLEX)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be REAL "
		 "or COMPLEX", gfc_current_intrinsic_arg[n],
		 gfc_current_intrinsic, &e->where);
      return FAILURE;
    }

  return SUCCESS;
}


/* Check that the expression is an optional constant integer
   and that it specifies a valid kind for that type.  */

static gfc_try
kind_check (gfc_expr *k, int n, bt type)
{
  int kind;

  if (k == NULL)
    return SUCCESS;

  if (type_check (k, n, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (k, n) == FAILURE)
    return FAILURE;

  if (k->expr_type != EXPR_CONSTANT)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be a constant",
		 gfc_current_intrinsic_arg[n], gfc_current_intrinsic,
		 &k->where);
      return FAILURE;
    }

  if (gfc_extract_int (k, &kind) != NULL
      || gfc_validate_kind (type, kind, true) < 0)
    {
      gfc_error ("Invalid kind for %s at %L", gfc_basic_typename (type),
		 &k->where);
      return FAILURE;
    }

  return SUCCESS;
}


/* Make sure the expression is a double precision real.  */

static gfc_try
double_check (gfc_expr *d, int n)
{
  if (type_check (d, n, BT_REAL) == FAILURE)
    return FAILURE;

  if (d->ts.kind != gfc_default_double_kind)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be double "
		 "precision", gfc_current_intrinsic_arg[n],
		 gfc_current_intrinsic, &d->where);
      return FAILURE;
    }

  return SUCCESS;
}


/* Make sure the expression is a logical array.  */

static gfc_try
logical_array_check (gfc_expr *array, int n)
{
  if (array->ts.type != BT_LOGICAL || array->rank == 0)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be a logical "
		 "array", gfc_current_intrinsic_arg[n], gfc_current_intrinsic,
		 &array->where);
      return FAILURE;
    }

  return SUCCESS;
}


/* Make sure an expression is an array.  */

static gfc_try
array_check (gfc_expr *e, int n)
{
  if (e->rank != 0)
    return SUCCESS;

  gfc_error ("'%s' argument of '%s' intrinsic at %L must be an array",
	     gfc_current_intrinsic_arg[n], gfc_current_intrinsic, &e->where);

  return FAILURE;
}


/* Make sure two expressions have the same type.  */

static gfc_try
same_type_check (gfc_expr *e, int n, gfc_expr *f, int m)
{
  if (gfc_compare_types (&e->ts, &f->ts))
    return SUCCESS;

  gfc_error ("'%s' argument of '%s' intrinsic at %L must be the same type "
	     "and kind as '%s'", gfc_current_intrinsic_arg[m],
	     gfc_current_intrinsic, &f->where, gfc_current_intrinsic_arg[n]);

  return FAILURE;
}


/* Make sure that an expression has a certain (nonzero) rank.  */

static gfc_try
rank_check (gfc_expr *e, int n, int rank)
{
  if (e->rank == rank)
    return SUCCESS;

  gfc_error ("'%s' argument of '%s' intrinsic at %L must be of rank %d",
	     gfc_current_intrinsic_arg[n], gfc_current_intrinsic,
	     &e->where, rank);

  return FAILURE;
}


/* Make sure a variable expression is not an optional dummy argument.  */

static gfc_try
nonoptional_check (gfc_expr *e, int n)
{
  if (e->expr_type == EXPR_VARIABLE && e->symtree->n.sym->attr.optional)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must not be OPTIONAL",
		 gfc_current_intrinsic_arg[n], gfc_current_intrinsic,
		 &e->where);
    }

  /* TODO: Recursive check on nonoptional variables?  */

  return SUCCESS;
}


/* Check that an expression has a particular kind.  */

static gfc_try
kind_value_check (gfc_expr *e, int n, int k)
{
  if (e->ts.kind == k)
    return SUCCESS;

  gfc_error ("'%s' argument of '%s' intrinsic at %L must be of kind %d",
	     gfc_current_intrinsic_arg[n], gfc_current_intrinsic,
	     &e->where, k);

  return FAILURE;
}


/* Make sure an expression is a variable.  */

static gfc_try
variable_check (gfc_expr *e, int n)
{
  if ((e->expr_type == EXPR_VARIABLE
       && e->symtree->n.sym->attr.flavor != FL_PARAMETER)
      || (e->expr_type == EXPR_FUNCTION
	  && e->symtree->n.sym->result == e->symtree->n.sym))
    return SUCCESS;

  if (e->expr_type == EXPR_VARIABLE
      && e->symtree->n.sym->attr.intent == INTENT_IN)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L cannot be INTENT(IN)",
		 gfc_current_intrinsic_arg[n], gfc_current_intrinsic,
		 &e->where);
      return FAILURE;
    }

  gfc_error ("'%s' argument of '%s' intrinsic at %L must be a variable",
	     gfc_current_intrinsic_arg[n], gfc_current_intrinsic, &e->where);

  return FAILURE;
}


/* Check the common DIM parameter for correctness.  */

static gfc_try
dim_check (gfc_expr *dim, int n, bool optional)
{
  if (dim == NULL)
    return SUCCESS;

  if (type_check (dim, n, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (dim, n) == FAILURE)
    return FAILURE;

  if (!optional && nonoptional_check (dim, n) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* If a DIM parameter is a constant, make sure that it is greater than
   zero and less than or equal to the rank of the given array.  If
   allow_assumed is zero then dim must be less than the rank of the array
   for assumed size arrays.  */

static gfc_try
dim_rank_check (gfc_expr *dim, gfc_expr *array, int allow_assumed)
{
  gfc_array_ref *ar;
  int rank;

  if (dim->expr_type != EXPR_CONSTANT
      || (array->expr_type != EXPR_VARIABLE
	  && array->expr_type != EXPR_ARRAY))
    return SUCCESS;

  rank = array->rank;
  if (array->expr_type == EXPR_VARIABLE)
    {
      ar = gfc_find_array_ref (array);
      if (ar->as->type == AS_ASSUMED_SIZE
	  && !allow_assumed
	  && ar->type != AR_ELEMENT
	  && ar->type != AR_SECTION)
	rank--;
    }

  if (mpz_cmp_ui (dim->value.integer, 1) < 0
      || mpz_cmp_ui (dim->value.integer, rank) > 0)
    {
      gfc_error ("'dim' argument of '%s' intrinsic at %L is not a valid "
		 "dimension index", gfc_current_intrinsic, &dim->where);

      return FAILURE;
    }

  return SUCCESS;
}


/* Compare the size of a along dimension ai with the size of b along
   dimension bi, returning 0 if they are known not to be identical,
   and 1 if they are identical, or if this cannot be determined.  */

static int
identical_dimen_shape (gfc_expr *a, int ai, gfc_expr *b, int bi)
{
  mpz_t a_size, b_size;
  int ret;

  gcc_assert (a->rank > ai);
  gcc_assert (b->rank > bi);

  ret = 1;

  if (gfc_array_dimen_size (a, ai, &a_size) == SUCCESS)
    {
      if (gfc_array_dimen_size (b, bi, &b_size) == SUCCESS)
	{
	  if (mpz_cmp (a_size, b_size) != 0)
	    ret = 0;
  
	  mpz_clear (b_size);
	}
      mpz_clear (a_size);
    }
  return ret;
}


/* Check whether two character expressions have the same length;
   returns SUCCESS if they have or if the length cannot be determined.  */

gfc_try
gfc_check_same_strlen (const gfc_expr *a, const gfc_expr *b, const char *name)
{
   long len_a, len_b;
   len_a = len_b = -1;

   if (a->ts.cl && a->ts.cl->length
       && a->ts.cl->length->expr_type == EXPR_CONSTANT)
     len_a = mpz_get_si (a->ts.cl->length->value.integer);
   else if (a->expr_type == EXPR_CONSTANT
	    && (a->ts.cl == NULL || a->ts.cl->length == NULL))
     len_a = a->value.character.length;
   else
     return SUCCESS;

   if (b->ts.cl && b->ts.cl->length
       && b->ts.cl->length->expr_type == EXPR_CONSTANT)
     len_b = mpz_get_si (b->ts.cl->length->value.integer);
   else if (b->expr_type == EXPR_CONSTANT
	    && (b->ts.cl == NULL || b->ts.cl->length == NULL))
     len_b = b->value.character.length;
   else
     return SUCCESS;

   if (len_a == len_b)
     return SUCCESS;

   gfc_error ("Unequal character lengths (%ld/%ld) in %s at %L",
	      len_a, len_b, name, &a->where);
   return FAILURE;
}


/***** Check functions *****/

/* Check subroutine suitable for intrinsics taking a real argument and
   a kind argument for the result.  */

static gfc_try
check_a_kind (gfc_expr *a, gfc_expr *kind, bt type)
{
  if (type_check (a, 0, BT_REAL) == FAILURE)
    return FAILURE;
  if (kind_check (kind, 1, type) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* Check subroutine suitable for ceiling, floor and nint.  */

gfc_try
gfc_check_a_ikind (gfc_expr *a, gfc_expr *kind)
{
  return check_a_kind (a, kind, BT_INTEGER);
}


/* Check subroutine suitable for aint, anint.  */

gfc_try
gfc_check_a_xkind (gfc_expr *a, gfc_expr *kind)
{
  return check_a_kind (a, kind, BT_REAL);
}


gfc_try
gfc_check_abs (gfc_expr *a)
{
  if (numeric_check (a, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_achar (gfc_expr *a, gfc_expr *kind)
{
  if (type_check (a, 0, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind_check (kind, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_access_func (gfc_expr *name, gfc_expr *mode)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE
      || scalar_check (name, 0) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (mode, 1, BT_CHARACTER) == FAILURE
      || scalar_check (mode, 1) == FAILURE)
    return FAILURE;
  if (kind_value_check (mode, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_all_any (gfc_expr *mask, gfc_expr *dim)
{
  if (logical_array_check (mask, 0) == FAILURE)
    return FAILURE;

  if (dim_check (dim, 1, false) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_allocated (gfc_expr *array)
{
  symbol_attribute attr;

  if (variable_check (array, 0) == FAILURE)
    return FAILURE;

  attr = gfc_variable_attr (array, NULL);
  if (!attr.allocatable)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be ALLOCATABLE",
		 gfc_current_intrinsic_arg[0], gfc_current_intrinsic,
		 &array->where);
      return FAILURE;
    }

  if (array_check (array, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* Common check function where the first argument must be real or
   integer and the second argument must be the same as the first.  */

gfc_try
gfc_check_a_p (gfc_expr *a, gfc_expr *p)
{
  if (int_or_real_check (a, 0) == FAILURE)
    return FAILURE;

  if (a->ts.type != p->ts.type)
    {
      gfc_error ("'%s' and '%s' arguments of '%s' intrinsic at %L must "
		 "have the same type", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic_arg[1], gfc_current_intrinsic,
		 &p->where);
      return FAILURE;
    }

  if (a->ts.kind != p->ts.kind)
    {
      if (gfc_notify_std (GFC_STD_GNU, "Extension: Different type kinds at %L",
			  &p->where) == FAILURE)
       return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_x_yd (gfc_expr *x, gfc_expr *y)
{
  if (double_check (x, 0) == FAILURE || double_check (y, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_associated (gfc_expr *pointer, gfc_expr *target)
{
  symbol_attribute attr1, attr2;
  int i;
  gfc_try t;
  locus *where;

  where = &pointer->where;

  if (pointer->expr_type == EXPR_VARIABLE || pointer->expr_type == EXPR_FUNCTION)
    attr1 = gfc_expr_attr (pointer);
  else if (pointer->expr_type == EXPR_NULL)
    goto null_arg;
  else
    gcc_assert (0); /* Pointer must be a variable or a function.  */

  if (!attr1.pointer && !attr1.proc_pointer)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be a POINTER",
		 gfc_current_intrinsic_arg[0], gfc_current_intrinsic,
		 &pointer->where);
      return FAILURE;
    }

  /* Target argument is optional.  */
  if (target == NULL)
    return SUCCESS;

  where = &target->where;
  if (target->expr_type == EXPR_NULL)
    goto null_arg;

  if (target->expr_type == EXPR_VARIABLE || target->expr_type == EXPR_FUNCTION)
    attr2 = gfc_expr_attr (target);
  else
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be a pointer "
		 "or target VARIABLE or FUNCTION", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &target->where);
      return FAILURE;
    }

  if (attr1.pointer && !attr2.pointer && !attr2.target)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be a POINTER "
		 "or a TARGET", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &target->where);
      return FAILURE;
    }

  t = SUCCESS;
  if (same_type_check (pointer, 0, target, 1) == FAILURE)
    t = FAILURE;
  if (rank_check (target, 0, pointer->rank) == FAILURE)
    t = FAILURE;
  if (target->rank > 0)
    {
      for (i = 0; i < target->rank; i++)
	if (target->ref->u.ar.dimen_type[i] == DIMEN_VECTOR)
	  {
	    gfc_error ("Array section with a vector subscript at %L shall not "
		       "be the target of a pointer",
		       &target->where);
	    t = FAILURE;
	    break;
	  }
    }
  return t;

null_arg:

  gfc_error ("NULL pointer at %L is not permitted as actual argument "
	     "of '%s' intrinsic function", where, gfc_current_intrinsic);
  return FAILURE;

}


gfc_try
gfc_check_atan2 (gfc_expr *y, gfc_expr *x)
{
  if (type_check (y, 0, BT_REAL) == FAILURE)
    return FAILURE;
  if (same_type_check (y, 0, x, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* BESJN and BESYN functions.  */

gfc_try
gfc_check_besn (gfc_expr *n, gfc_expr *x)
{
  if (type_check (n, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (x, 1, BT_REAL) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_btest (gfc_expr *i, gfc_expr *pos)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (type_check (pos, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_char (gfc_expr *i, gfc_expr *kind)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind_check (kind, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_chdir (gfc_expr *dir)
{
  if (type_check (dir, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (dir, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_chdir_sub (gfc_expr *dir, gfc_expr *status)
{
  if (type_check (dir, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (dir, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 1, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (scalar_check (status, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_chmod (gfc_expr *name, gfc_expr *mode)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (mode, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (mode, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_chmod_sub (gfc_expr *name, gfc_expr *mode, gfc_expr *status)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (mode, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (mode, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_cmplx (gfc_expr *x, gfc_expr *y, gfc_expr *kind)
{
  if (numeric_check (x, 0) == FAILURE)
    return FAILURE;

  if (y != NULL)
    {
      if (numeric_check (y, 1) == FAILURE)
	return FAILURE;

      if (x->ts.type == BT_COMPLEX)
	{
	  gfc_error ("'%s' argument of '%s' intrinsic at %L must not be "
		     "present if 'x' is COMPLEX", gfc_current_intrinsic_arg[1],
		     gfc_current_intrinsic, &y->where);
	  return FAILURE;
	}

      if (y->ts.type == BT_COMPLEX)
	{
	  gfc_error ("'%s' argument of '%s' intrinsic at %L must have a type "
		     "of either REAL or INTEGER", gfc_current_intrinsic_arg[1],
		     gfc_current_intrinsic, &y->where);
	  return FAILURE;
	}

    }

  if (kind_check (kind, 2, BT_COMPLEX) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_complex (gfc_expr *x, gfc_expr *y)
{
  if (x->ts.type != BT_INTEGER && x->ts.type != BT_REAL)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be INTEGER "
		 "or REAL", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &x->where);
      return FAILURE;
    }
  if (scalar_check (x, 0) == FAILURE)
    return FAILURE;

  if (y->ts.type != BT_INTEGER && y->ts.type != BT_REAL)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be INTEGER "
		 "or REAL", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &y->where);
      return FAILURE;
    }
  if (scalar_check (y, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_count (gfc_expr *mask, gfc_expr *dim, gfc_expr *kind)
{
  if (logical_array_check (mask, 0) == FAILURE)
    return FAILURE;
  if (dim_check (dim, 1, false) == FAILURE)
    return FAILURE;
  if (kind_check (kind, 2, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_cshift (gfc_expr *array, gfc_expr *shift, gfc_expr *dim)
{
  if (array_check (array, 0) == FAILURE)
    return FAILURE;

  if (type_check (shift, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (array->rank == 1)
    {
      if (scalar_check (shift, 1) == FAILURE)
	return FAILURE;
    }
  else if (shift->rank != array->rank - 1 && shift->rank != 0)
    {
      gfc_error ("SHIFT argument at %L of CSHIFT must have rank %d or be a "
		 "scalar", &shift->where, array->rank - 1);
      return FAILURE;
    }

  /* TODO: Add shape conformance check between array (w/o dimension dim)
     and shift. */

  if (dim_check (dim, 2, true) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ctime (gfc_expr *time)
{
  if (scalar_check (time, 0) == FAILURE)
    return FAILURE;

  if (type_check (time, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try gfc_check_datan2 (gfc_expr *y, gfc_expr *x)
{
  if (double_check (y, 0) == FAILURE || double_check (x, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}

gfc_try
gfc_check_dcmplx (gfc_expr *x, gfc_expr *y)
{
  if (numeric_check (x, 0) == FAILURE)
    return FAILURE;

  if (y != NULL)
    {
      if (numeric_check (y, 1) == FAILURE)
	return FAILURE;

      if (x->ts.type == BT_COMPLEX)
	{
	  gfc_error ("'%s' argument of '%s' intrinsic at %L must not be "
		     "present if 'x' is COMPLEX", gfc_current_intrinsic_arg[1],
		     gfc_current_intrinsic, &y->where);
	  return FAILURE;
	}

      if (y->ts.type == BT_COMPLEX)
	{
	  gfc_error ("'%s' argument of '%s' intrinsic at %L must have a type "
		     "of either REAL or INTEGER", gfc_current_intrinsic_arg[1],
		     gfc_current_intrinsic, &y->where);
	  return FAILURE;
	}
    }

  return SUCCESS;
}


gfc_try
gfc_check_dble (gfc_expr *x)
{
  if (numeric_check (x, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_digits (gfc_expr *x)
{
  if (int_or_real_check (x, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_dot_product (gfc_expr *vector_a, gfc_expr *vector_b)
{
  switch (vector_a->ts.type)
    {
    case BT_LOGICAL:
      if (type_check (vector_b, 1, BT_LOGICAL) == FAILURE)
	return FAILURE;
      break;

    case BT_INTEGER:
    case BT_REAL:
    case BT_COMPLEX:
      if (numeric_check (vector_b, 1) == FAILURE)
	return FAILURE;
      break;

    default:
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be numeric "
		 "or LOGICAL", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &vector_a->where);
      return FAILURE;
    }

  if (rank_check (vector_a, 0, 1) == FAILURE)
    return FAILURE;

  if (rank_check (vector_b, 1, 1) == FAILURE)
    return FAILURE;

  if (! identical_dimen_shape (vector_a, 0, vector_b, 0))
    {
      gfc_error ("Different shape for arguments '%s' and '%s' at %L for "
		 "intrinsic 'dot_product'", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic_arg[1], &vector_a->where);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_dprod (gfc_expr *x, gfc_expr *y)
{
  if (type_check (x, 0, BT_REAL) == FAILURE
      || type_check (y, 1, BT_REAL) == FAILURE)
    return FAILURE;

  if (x->ts.kind != gfc_default_real_kind)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be default "
		 "real", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &x->where);
      return FAILURE;
    }

  if (y->ts.kind != gfc_default_real_kind)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be default "
		 "real", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &y->where);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_eoshift (gfc_expr *array, gfc_expr *shift, gfc_expr *boundary,
		   gfc_expr *dim)
{
  if (array_check (array, 0) == FAILURE)
    return FAILURE;

  if (type_check (shift, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (array->rank == 1)
    {
      if (scalar_check (shift, 2) == FAILURE)
	return FAILURE;
    }
  else if (shift->rank != array->rank - 1 && shift->rank != 0)
    {
      gfc_error ("SHIFT argument at %L of EOSHIFT must have rank %d or be a "
		 "scalar", &shift->where, array->rank - 1);
      return FAILURE;
    }

  /* TODO: Add shape conformance check between array (w/o dimension dim)
     and shift. */

  if (boundary != NULL)
    {
      if (same_type_check (array, 0, boundary, 2) == FAILURE)
	return FAILURE;

      if (array->rank == 1)
	{
	  if (scalar_check (boundary, 2) == FAILURE)
	    return FAILURE;
	}
      else if (boundary->rank != array->rank - 1 && boundary->rank != 0)
	{
	  gfc_error ("BOUNDARY argument at %L of EOSHIFT must have rank %d or be "
		     "a scalar", &boundary->where, array->rank - 1);
	  return FAILURE;
	}

      if (shift->rank == boundary->rank)
	{
	  int i;
	  for (i = 0; i < shift->rank; i++)
	    if (! identical_dimen_shape (shift, i, boundary, i))
	      {
		gfc_error ("Different shape in dimension %d for SHIFT and "
			   "BOUNDARY arguments of EOSHIFT at %L", shift->rank,
			   &boundary->where);
		return FAILURE;
	      }
	}
    }

  if (dim_check (dim, 4, true) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* A single complex argument.  */

gfc_try
gfc_check_fn_c (gfc_expr *a)
{
  if (type_check (a, 0, BT_COMPLEX) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* A single real argument.  */

gfc_try
gfc_check_fn_r (gfc_expr *a)
{
  if (type_check (a, 0, BT_REAL) == FAILURE)
    return FAILURE;

  return SUCCESS;
}

/* A single double argument.  */

gfc_try
gfc_check_fn_d (gfc_expr *a)
{
  if (double_check (a, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}

/* A single real or complex argument.  */

gfc_try
gfc_check_fn_rc (gfc_expr *a)
{
  if (real_or_complex_check (a, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_fnum (gfc_expr *unit)
{
  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_huge (gfc_expr *x)
{
  if (int_or_real_check (x, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_hypot (gfc_expr *x, gfc_expr *y)
{
  if (type_check (x, 0, BT_REAL) == FAILURE)
    return FAILURE;
  if (same_type_check (x, 0, y, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* Check that the single argument is an integer.  */

gfc_try
gfc_check_i (gfc_expr *i)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_iand (gfc_expr *i, gfc_expr *j)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (j, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (i->ts.kind != j->ts.kind)
    {
      if (gfc_notify_std (GFC_STD_GNU, "Extension: Different type kinds at %L",
			  &i->where) == FAILURE)
	return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_ibclr (gfc_expr *i, gfc_expr *pos)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (pos, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ibits (gfc_expr *i, gfc_expr *pos, gfc_expr *len)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (pos, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (len, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ibset (gfc_expr *i, gfc_expr *pos)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (pos, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ichar_iachar (gfc_expr *c, gfc_expr *kind)
{
  int i;

  if (type_check (c, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (kind_check (kind, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;

  if (c->expr_type == EXPR_VARIABLE || c->expr_type == EXPR_SUBSTRING)
    {
      gfc_expr *start;
      gfc_expr *end;
      gfc_ref *ref;

      /* Substring references don't have the charlength set.  */
      ref = c->ref;
      while (ref && ref->type != REF_SUBSTRING)
	ref = ref->next;

      gcc_assert (ref == NULL || ref->type == REF_SUBSTRING);

      if (!ref)
	{
	  /* Check that the argument is length one.  Non-constant lengths
	     can't be checked here, so assume they are ok.  */
	  if (c->ts.cl && c->ts.cl->length)
	    {
	      /* If we already have a length for this expression then use it.  */
	      if (c->ts.cl->length->expr_type != EXPR_CONSTANT)
		return SUCCESS;
	      i = mpz_get_si (c->ts.cl->length->value.integer);
	    }
	  else 
	    return SUCCESS;
	}
      else
	{
	  start = ref->u.ss.start;
	  end = ref->u.ss.end;

	  gcc_assert (start);
	  if (end == NULL || end->expr_type != EXPR_CONSTANT
	      || start->expr_type != EXPR_CONSTANT)
	    return SUCCESS;

	  i = mpz_get_si (end->value.integer) + 1
	    - mpz_get_si (start->value.integer);
	}
    }
  else
    return SUCCESS;

  if (i != 1)
    {
      gfc_error ("Argument of %s at %L must be of length one", 
		 gfc_current_intrinsic, &c->where);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_idnint (gfc_expr *a)
{
  if (double_check (a, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ieor (gfc_expr *i, gfc_expr *j)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (j, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (i->ts.kind != j->ts.kind)
    {
      if (gfc_notify_std (GFC_STD_GNU, "Extension: Different type kinds at %L",
			  &i->where) == FAILURE)
	return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_index (gfc_expr *string, gfc_expr *substring, gfc_expr *back,
		 gfc_expr *kind)
{
  if (type_check (string, 0, BT_CHARACTER) == FAILURE
      || type_check (substring, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (back != NULL && type_check (back, 2, BT_LOGICAL) == FAILURE)
    return FAILURE;

  if (kind_check (kind, 3, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;

  if (string->ts.kind != substring->ts.kind)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be the same "
		 "kind as '%s'", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &substring->where,
		 gfc_current_intrinsic_arg[0]);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_int (gfc_expr *x, gfc_expr *kind)
{
  if (numeric_check (x, 0) == FAILURE)
    return FAILURE;

  if (kind_check (kind, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_intconv (gfc_expr *x)
{
  if (numeric_check (x, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ior (gfc_expr *i, gfc_expr *j)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (j, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (i->ts.kind != j->ts.kind)
    {
      if (gfc_notify_std (GFC_STD_GNU, "Extension: Different type kinds at %L",
			  &i->where) == FAILURE)
	return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_ishft (gfc_expr *i, gfc_expr *shift)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE
      || type_check (shift, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ishftc (gfc_expr *i, gfc_expr *shift, gfc_expr *size)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE
      || type_check (shift, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (size != NULL && type_check (size, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_kill (gfc_expr *pid, gfc_expr *sig)
{
  if (type_check (pid, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (sig, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_kill_sub (gfc_expr *pid, gfc_expr *sig, gfc_expr *status)
{
  if (type_check (pid, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (pid, 0) == FAILURE)
    return FAILURE;

  if (type_check (sig, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (sig, 1) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_kind (gfc_expr *x)
{
  if (x->ts.type == BT_DERIVED)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be a "
		 "non-derived type", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &x->where);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_lbound (gfc_expr *array, gfc_expr *dim, gfc_expr *kind)
{
  if (array_check (array, 0) == FAILURE)
    return FAILURE;

  if (dim != NULL)
    {
      if (dim_check (dim, 1, false) == FAILURE)
	return FAILURE;

      if (dim_rank_check (dim, array, 1) == FAILURE)
	return FAILURE;
    }

  if (kind_check (kind, 2, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_len_lentrim (gfc_expr *s, gfc_expr *kind)
{
  if (type_check (s, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (kind_check (kind, 1, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_lge_lgt_lle_llt (gfc_expr *a, gfc_expr *b)
{
  if (type_check (a, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (a, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (b, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (b, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_link (gfc_expr *path1, gfc_expr *path2)
{
  if (type_check (path1, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path1, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (path2, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path2, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_link_sub (gfc_expr *path1, gfc_expr *path2, gfc_expr *status)
{
  if (type_check (path1, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path1, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (path2, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path2, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_loc (gfc_expr *expr)
{
  return variable_check (expr, 0);
}


gfc_try
gfc_check_symlnk (gfc_expr *path1, gfc_expr *path2)
{
  if (type_check (path1, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path1, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (path2, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path2, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_symlnk_sub (gfc_expr *path1, gfc_expr *path2, gfc_expr *status)
{
  if (type_check (path1, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path1, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (path2, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path2, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_logical (gfc_expr *a, gfc_expr *kind)
{
  if (type_check (a, 0, BT_LOGICAL) == FAILURE)
    return FAILURE;
  if (kind_check (kind, 1, BT_LOGICAL) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* Min/max family.  */

static gfc_try
min_max_args (gfc_actual_arglist *arg)
{
  if (arg == NULL || arg->next == NULL)
    {
      gfc_error ("Intrinsic '%s' at %L must have at least two arguments",
		 gfc_current_intrinsic, gfc_current_intrinsic_where);
      return FAILURE;
    }

  return SUCCESS;
}


static gfc_try
check_rest (bt type, int kind, gfc_actual_arglist *arglist)
{
  gfc_actual_arglist *arg, *tmp;

  gfc_expr *x;
  int m, n;

  if (min_max_args (arglist) == FAILURE)
    return FAILURE;

  for (arg = arglist, n=1; arg; arg = arg->next, n++)
    {
      x = arg->expr;
      if (x->ts.type != type || x->ts.kind != kind)
	{
	  if (x->ts.type == type)
	    {
	      if (gfc_notify_std (GFC_STD_GNU, "Extension: Different type "
				  "kinds at %L", &x->where) == FAILURE)
		return FAILURE;
	    }
	  else
	    {
	      gfc_error ("'a%d' argument of '%s' intrinsic at %L must be "
			 "%s(%d)", n, gfc_current_intrinsic, &x->where,
			 gfc_basic_typename (type), kind);
	      return FAILURE;
	    }
	}

      for (tmp = arglist, m=1; tmp != arg; tmp = tmp->next, m++)
        {
	  char buffer[80];
	  snprintf (buffer, 80, "arguments 'a%d' and 'a%d' for intrinsic '%s'",
		    m, n, gfc_current_intrinsic);
	  if (gfc_check_conformance (buffer, tmp->expr, x) == FAILURE)
	    return FAILURE;
	}
    }

  return SUCCESS;
}


gfc_try
gfc_check_min_max (gfc_actual_arglist *arg)
{
  gfc_expr *x;

  if (min_max_args (arg) == FAILURE)
    return FAILURE;

  x = arg->expr;

  if (x->ts.type == BT_CHARACTER)
    {
      if (gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			  "with CHARACTER argument at %L",
			  gfc_current_intrinsic, &x->where) == FAILURE)
	return FAILURE;
    }
  else if (x->ts.type != BT_INTEGER && x->ts.type != BT_REAL)
    {
      gfc_error ("'a1' argument of '%s' intrinsic at %L must be INTEGER, "
		 "REAL or CHARACTER", gfc_current_intrinsic, &x->where);
      return FAILURE;
    }

  return check_rest (x->ts.type, x->ts.kind, arg);
}


gfc_try
gfc_check_min_max_integer (gfc_actual_arglist *arg)
{
  return check_rest (BT_INTEGER, gfc_default_integer_kind, arg);
}


gfc_try
gfc_check_min_max_real (gfc_actual_arglist *arg)
{
  return check_rest (BT_REAL, gfc_default_real_kind, arg);
}


gfc_try
gfc_check_min_max_double (gfc_actual_arglist *arg)
{
  return check_rest (BT_REAL, gfc_default_double_kind, arg);
}


/* End of min/max family.  */

gfc_try
gfc_check_malloc (gfc_expr *size)
{
  if (type_check (size, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (size, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_matmul (gfc_expr *matrix_a, gfc_expr *matrix_b)
{
  if ((matrix_a->ts.type != BT_LOGICAL) && !gfc_numeric_ts (&matrix_a->ts))
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be numeric "
		 "or LOGICAL", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &matrix_a->where);
      return FAILURE;
    }

  if ((matrix_b->ts.type != BT_LOGICAL) && !gfc_numeric_ts (&matrix_b->ts))
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be numeric "
		 "or LOGICAL", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &matrix_b->where);
      return FAILURE;
    }

  if ((matrix_a->ts.type == BT_LOGICAL && gfc_numeric_ts (&matrix_b->ts))
      || (gfc_numeric_ts (&matrix_a->ts) && matrix_b->ts.type == BT_LOGICAL))
    {
      gfc_error ("Argument types of '%s' intrinsic at %L must match (%s/%s)",
		 gfc_current_intrinsic, &matrix_a->where,
		 gfc_typename(&matrix_a->ts), gfc_typename(&matrix_b->ts));
       return FAILURE;
    }

  switch (matrix_a->rank)
    {
    case 1:
      if (rank_check (matrix_b, 1, 2) == FAILURE)
	return FAILURE;
      /* Check for case matrix_a has shape(m), matrix_b has shape (m, k).  */
      if (!identical_dimen_shape (matrix_a, 0, matrix_b, 0))
	{
	  gfc_error ("Different shape on dimension 1 for arguments '%s' "
		     "and '%s' at %L for intrinsic matmul",
		     gfc_current_intrinsic_arg[0],
		     gfc_current_intrinsic_arg[1], &matrix_a->where);
	  return FAILURE;
	}
      break;

    case 2:
      if (matrix_b->rank != 2)
	{
	  if (rank_check (matrix_b, 1, 1) == FAILURE)
	    return FAILURE;
	}
      /* matrix_b has rank 1 or 2 here. Common check for the cases
	 - matrix_a has shape (n,m) and matrix_b has shape (m, k)
	 - matrix_a has shape (n,m) and matrix_b has shape (m).  */
      if (!identical_dimen_shape (matrix_a, 1, matrix_b, 0))
	{
	  gfc_error ("Different shape on dimension 2 for argument '%s' and "
		     "dimension 1 for argument '%s' at %L for intrinsic "
		     "matmul", gfc_current_intrinsic_arg[0],
		     gfc_current_intrinsic_arg[1], &matrix_a->where);
	  return FAILURE;
	}
      break;

    default:
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be of rank "
		 "1 or 2", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &matrix_a->where);
      return FAILURE;
    }

  return SUCCESS;
}


/* Whoever came up with this interface was probably on something.
   The possibilities for the occupation of the second and third
   parameters are:

	 Arg #2     Arg #3
	 NULL       NULL
	 DIM	NULL
	 MASK       NULL
	 NULL       MASK	     minloc(array, mask=m)
	 DIM	MASK

   I.e. in the case of minloc(array,mask), mask will be in the second
   position of the argument list and we'll have to fix that up.  */

gfc_try
gfc_check_minloc_maxloc (gfc_actual_arglist *ap)
{
  gfc_expr *a, *m, *d;

  a = ap->expr;
  if (int_or_real_check (a, 0) == FAILURE || array_check (a, 0) == FAILURE)
    return FAILURE;

  d = ap->next->expr;
  m = ap->next->next->expr;

  if (m == NULL && d != NULL && d->ts.type == BT_LOGICAL
      && ap->next->name == NULL)
    {
      m = d;
      d = NULL;
      ap->next->expr = NULL;
      ap->next->next->expr = m;
    }

  if (d && dim_check (d, 1, false) == FAILURE)
    return FAILURE;

  if (d && dim_rank_check (d, a, 0) == FAILURE)
    return FAILURE;

  if (m != NULL && type_check (m, 2, BT_LOGICAL) == FAILURE)
    return FAILURE;

  if (m != NULL)
    {
      char buffer[80];
      snprintf (buffer, 80, "arguments '%s' and '%s' for intrinsic %s",
		gfc_current_intrinsic_arg[0], gfc_current_intrinsic_arg[2],
		gfc_current_intrinsic);
      if (gfc_check_conformance (buffer, a, m) == FAILURE)
	return FAILURE;
    }

  return SUCCESS;
}


/* Similar to minloc/maxloc, the argument list might need to be
   reordered for the MINVAL, MAXVAL, PRODUCT, and SUM intrinsics.  The
   difference is that MINLOC/MAXLOC take an additional KIND argument.
   The possibilities are:

	 Arg #2     Arg #3
	 NULL       NULL
	 DIM	NULL
	 MASK       NULL
	 NULL       MASK	     minval(array, mask=m)
	 DIM	MASK

   I.e. in the case of minval(array,mask), mask will be in the second
   position of the argument list and we'll have to fix that up.  */

static gfc_try
check_reduction (gfc_actual_arglist *ap)
{
  gfc_expr *a, *m, *d;

  a = ap->expr;
  d = ap->next->expr;
  m = ap->next->next->expr;

  if (m == NULL && d != NULL && d->ts.type == BT_LOGICAL
      && ap->next->name == NULL)
    {
      m = d;
      d = NULL;
      ap->next->expr = NULL;
      ap->next->next->expr = m;
    }

  if (d && dim_check (d, 1, false) == FAILURE)
    return FAILURE;

  if (d && dim_rank_check (d, a, 0) == FAILURE)
    return FAILURE;

  if (m != NULL && type_check (m, 2, BT_LOGICAL) == FAILURE)
    return FAILURE;

  if (m != NULL)
    {
      char buffer[80];
      snprintf (buffer, 80, "arguments '%s' and '%s' for intrinsic %s",
		gfc_current_intrinsic_arg[0], gfc_current_intrinsic_arg[2],
		gfc_current_intrinsic);
      if (gfc_check_conformance (buffer, a, m) == FAILURE)
	return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_minval_maxval (gfc_actual_arglist *ap)
{
  if (int_or_real_check (ap->expr, 0) == FAILURE
      || array_check (ap->expr, 0) == FAILURE)
    return FAILURE;

  return check_reduction (ap);
}


gfc_try
gfc_check_product_sum (gfc_actual_arglist *ap)
{
  if (numeric_check (ap->expr, 0) == FAILURE
      || array_check (ap->expr, 0) == FAILURE)
    return FAILURE;

  return check_reduction (ap);
}


gfc_try
gfc_check_merge (gfc_expr *tsource, gfc_expr *fsource, gfc_expr *mask)
{
  if (same_type_check (tsource, 0, fsource, 1) == FAILURE)
    return FAILURE;

  if (type_check (mask, 2, BT_LOGICAL) == FAILURE)
    return FAILURE;

  if (tsource->ts.type == BT_CHARACTER)
    return gfc_check_same_strlen (tsource, fsource, "MERGE intrinsic");

  return SUCCESS;
}


gfc_try
gfc_check_move_alloc (gfc_expr *from, gfc_expr *to)
{
  symbol_attribute attr;

  if (variable_check (from, 0) == FAILURE)
    return FAILURE;

  if (array_check (from, 0) == FAILURE)
    return FAILURE;

  attr = gfc_variable_attr (from, NULL);
  if (!attr.allocatable)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be ALLOCATABLE",
		 gfc_current_intrinsic_arg[0], gfc_current_intrinsic,
		 &from->where);
      return FAILURE;
    }

  if (variable_check (to, 0) == FAILURE)
    return FAILURE;

  if (array_check (to, 0) == FAILURE)
    return FAILURE;

  attr = gfc_variable_attr (to, NULL);
  if (!attr.allocatable)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be ALLOCATABLE",
		 gfc_current_intrinsic_arg[0], gfc_current_intrinsic,
		 &to->where);
      return FAILURE;
    }

  if (same_type_check (from, 0, to, 1) == FAILURE)
    return FAILURE;

  if (to->rank != from->rank)
    {
      gfc_error ("the '%s' and '%s' arguments of '%s' intrinsic at %L must "
		 "have the same rank %d/%d", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic_arg[1], gfc_current_intrinsic,
		 &to->where,  from->rank, to->rank);
      return FAILURE;
    }

  if (to->ts.kind != from->ts.kind)
    {
      gfc_error ("the '%s' and '%s' arguments of '%s' intrinsic at %L must "
		 "be of the same kind %d/%d", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic_arg[1], gfc_current_intrinsic,
		 &to->where, from->ts.kind, to->ts.kind);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_nearest (gfc_expr *x, gfc_expr *s)
{
  if (type_check (x, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (type_check (s, 1, BT_REAL) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_new_line (gfc_expr *a)
{
  if (type_check (a, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_null (gfc_expr *mold)
{
  symbol_attribute attr;

  if (mold == NULL)
    return SUCCESS;

  if (variable_check (mold, 0) == FAILURE)
    return FAILURE;

  attr = gfc_variable_attr (mold, NULL);

  if (!attr.pointer && !attr.proc_pointer)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be a POINTER",
		 gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &mold->where);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_pack (gfc_expr *array, gfc_expr *mask, gfc_expr *vector)
{
  char buffer[80];

  if (array_check (array, 0) == FAILURE)
    return FAILURE;

  if (type_check (mask, 1, BT_LOGICAL) == FAILURE)
    return FAILURE;

  snprintf (buffer, 80, "arguments '%s' and '%s' for intrinsic '%s'",
	    gfc_current_intrinsic_arg[0], gfc_current_intrinsic_arg[1],
	    gfc_current_intrinsic);
  if (gfc_check_conformance (buffer, array, mask) == FAILURE)
    return FAILURE;

  if (vector != NULL)
    {
      if (same_type_check (array, 0, vector, 2) == FAILURE)
	return FAILURE;

      if (rank_check (vector, 2, 1) == FAILURE)
	return FAILURE;

      /* TODO: More constraints here.  */
    }

  return SUCCESS;
}


gfc_try
gfc_check_precision (gfc_expr *x)
{
  if (x->ts.type != BT_REAL && x->ts.type != BT_COMPLEX)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be of type "
		 "REAL or COMPLEX", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &x->where);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_present (gfc_expr *a)
{
  gfc_symbol *sym;

  if (variable_check (a, 0) == FAILURE)
    return FAILURE;

  sym = a->symtree->n.sym;
  if (!sym->attr.dummy)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be of a "
		 "dummy variable", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &a->where);
      return FAILURE;
    }

  if (!sym->attr.optional)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be of "
		 "an OPTIONAL dummy variable", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &a->where);
      return FAILURE;
    }

  /* 13.14.82  PRESENT(A)
     ......
     Argument.  A shall be the name of an optional dummy argument that is
     accessible in the subprogram in which the PRESENT function reference
     appears...  */

  if (a->ref != NULL
      && !(a->ref->next == NULL && a->ref->type == REF_ARRAY
	   && a->ref->u.ar.type == AR_FULL))
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must not be a "
		 "subobject of '%s'", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &a->where, sym->name);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_radix (gfc_expr *x)
{
  if (int_or_real_check (x, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_range (gfc_expr *x)
{
  if (numeric_check (x, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* real, float, sngl.  */
gfc_try
gfc_check_real (gfc_expr *a, gfc_expr *kind)
{
  if (numeric_check (a, 0) == FAILURE)
    return FAILURE;

  if (kind_check (kind, 1, BT_REAL) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_rename (gfc_expr *path1, gfc_expr *path2)
{
  if (type_check (path1, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path1, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (path2, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path2, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_rename_sub (gfc_expr *path1, gfc_expr *path2, gfc_expr *status)
{
  if (type_check (path1, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path1, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (path2, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (path2, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_repeat (gfc_expr *x, gfc_expr *y)
{
  if (type_check (x, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (scalar_check (x, 0) == FAILURE)
    return FAILURE;

  if (type_check (y, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (y, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_reshape (gfc_expr *source, gfc_expr *shape,
		   gfc_expr *pad, gfc_expr *order)
{
  mpz_t size;
  mpz_t nelems;
  int m;

  if (array_check (source, 0) == FAILURE)
    return FAILURE;

  if (rank_check (shape, 1, 1) == FAILURE)
    return FAILURE;

  if (type_check (shape, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (gfc_array_size (shape, &size) != SUCCESS)
    {
      gfc_error ("'shape' argument of 'reshape' intrinsic at %L must be an "
		 "array of constant size", &shape->where);
      return FAILURE;
    }

  m = mpz_cmp_ui (size, GFC_MAX_DIMENSIONS);
  mpz_clear (size);

  if (m > 0)
    {
      gfc_error ("'shape' argument of 'reshape' intrinsic at %L has more "
		 "than %d elements", &shape->where, GFC_MAX_DIMENSIONS);
      return FAILURE;
    }

  if (pad != NULL)
    {
      if (same_type_check (source, 0, pad, 2) == FAILURE)
	return FAILURE;
      if (array_check (pad, 2) == FAILURE)
	return FAILURE;
    }

  if (order != NULL && array_check (order, 3) == FAILURE)
    return FAILURE;

  if (pad == NULL && shape->expr_type == EXPR_ARRAY
      && gfc_is_constant_expr (shape)
      && !(source->expr_type == EXPR_VARIABLE && source->symtree->n.sym->as
	   && source->symtree->n.sym->as->type == AS_ASSUMED_SIZE))
    {
      /* Check the match in size between source and destination.  */
      if (gfc_array_size (source, &nelems) == SUCCESS)
	{
	  gfc_constructor *c;
	  bool test;

	  c = shape->value.constructor;
	  mpz_init_set_ui (size, 1);
	  for (; c; c = c->next)
	    mpz_mul (size, size, c->expr->value.integer);

	  test = mpz_cmp (nelems, size) < 0 && mpz_cmp_ui (size, 0) > 0;
	  mpz_clear (nelems);
	  mpz_clear (size);

	  if (test)
	    {
	      gfc_error ("Without padding, there are not enough elements "
			 "in the intrinsic RESHAPE source at %L to match "
			 "the shape", &source->where);
	      return FAILURE;
	    }
	}
    }

  return SUCCESS;
}


gfc_try
gfc_check_scale (gfc_expr *x, gfc_expr *i)
{
  if (type_check (x, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (type_check (i, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_scan (gfc_expr *x, gfc_expr *y, gfc_expr *z, gfc_expr *kind)
{
  if (type_check (x, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (type_check (y, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (z != NULL && type_check (z, 2, BT_LOGICAL) == FAILURE)
    return FAILURE;

  if (kind_check (kind, 3, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;

  if (same_type_check (x, 0, y, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_secnds (gfc_expr *r)
{
  if (type_check (r, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (kind_value_check (r, 0, 4) == FAILURE)
    return FAILURE;

  if (scalar_check (r, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_selected_char_kind (gfc_expr *name)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (scalar_check (name, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_selected_int_kind (gfc_expr *r)
{
  if (type_check (r, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (r, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_selected_real_kind (gfc_expr *p, gfc_expr *r)
{
  if (p == NULL && r == NULL)
    {
      gfc_error ("Missing arguments to %s intrinsic at %L",
		 gfc_current_intrinsic, gfc_current_intrinsic_where);

      return FAILURE;
    }

  if (p != NULL && type_check (p, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (r != NULL && type_check (r, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_set_exponent (gfc_expr *x, gfc_expr *i)
{
  if (type_check (x, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (type_check (i, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_shape (gfc_expr *source)
{
  gfc_array_ref *ar;

  if (source->rank == 0 || source->expr_type != EXPR_VARIABLE)
    return SUCCESS;

  ar = gfc_find_array_ref (source);

  if (ar->as && ar->as->type == AS_ASSUMED_SIZE && ar->type == AR_FULL)
    {
      gfc_error ("'source' argument of 'shape' intrinsic at %L must not be "
		 "an assumed size array", &source->where);
      return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_sign (gfc_expr *a, gfc_expr *b)
{
  if (int_or_real_check (a, 0) == FAILURE)
    return FAILURE;

  if (same_type_check (a, 0, b, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_size (gfc_expr *array, gfc_expr *dim, gfc_expr *kind)
{
  if (array_check (array, 0) == FAILURE)
    return FAILURE;

  if (dim != NULL)
    {
      if (dim_check (dim, 1, true) == FAILURE)
	return FAILURE;

      if (dim_rank_check (dim, array, 0) == FAILURE)
	return FAILURE;
    }

  if (kind_check (kind, 2, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;


  return SUCCESS;
}


gfc_try
gfc_check_sizeof (gfc_expr *arg ATTRIBUTE_UNUSED)
{
  return SUCCESS;
}


gfc_try
gfc_check_sleep_sub (gfc_expr *seconds)
{
  if (type_check (seconds, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (seconds, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_spread (gfc_expr *source, gfc_expr *dim, gfc_expr *ncopies)
{
  if (source->rank >= GFC_MAX_DIMENSIONS)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be less "
		 "than rank %d", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &source->where, GFC_MAX_DIMENSIONS);

      return FAILURE;
    }

  if (dim == NULL)
    return FAILURE;

  if (dim_check (dim, 1, false) == FAILURE)
    return FAILURE;

  if (type_check (ncopies, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (ncopies, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* Functions for checking FGETC, FPUTC, FGET and FPUT (subroutines and
   functions).  */

gfc_try
gfc_check_fgetputc_sub (gfc_expr *unit, gfc_expr *c, gfc_expr *status)
{
  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  if (type_check (c, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (c, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE
      || kind_value_check (status, 2, gfc_default_integer_kind) == FAILURE
      || scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_fgetputc (gfc_expr *unit, gfc_expr *c)
{
  return gfc_check_fgetputc_sub (unit, c, NULL);
}


gfc_try
gfc_check_fgetput_sub (gfc_expr *c, gfc_expr *status)
{
  if (type_check (c, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (c, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 1, BT_INTEGER) == FAILURE
      || kind_value_check (status, 1, gfc_default_integer_kind) == FAILURE
      || scalar_check (status, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_fgetput (gfc_expr *c)
{
  return gfc_check_fgetput_sub (c, NULL);
}


gfc_try
gfc_check_fseek_sub (gfc_expr *unit, gfc_expr *offset, gfc_expr *whence, gfc_expr *status)
{
  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  if (type_check (offset, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (offset, 1) == FAILURE)
    return FAILURE;

  if (type_check (whence, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (whence, 2) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 3, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check (status, 3, 4) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 3) == FAILURE)
    return FAILURE;

  return SUCCESS;
}



gfc_try
gfc_check_fstat (gfc_expr *unit, gfc_expr *array)
{
  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  if (type_check (array, 1, BT_INTEGER) == FAILURE
      || kind_value_check (unit, 0, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  if (array_check (array, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_fstat_sub (gfc_expr *unit, gfc_expr *array, gfc_expr *status)
{
  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  if (type_check (array, 1, BT_INTEGER) == FAILURE
      || kind_value_check (array, 1, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  if (array_check (array, 1) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE
      || kind_value_check (status, 2, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ftell (gfc_expr *unit)
{
  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ftell_sub (gfc_expr *unit, gfc_expr *offset)
{
  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  if (type_check (offset, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (offset, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_stat (gfc_expr *name, gfc_expr *array)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (array, 1, BT_INTEGER) == FAILURE
      || kind_value_check (array, 1, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  if (array_check (array, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_stat_sub (gfc_expr *name, gfc_expr *array, gfc_expr *status)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (type_check (array, 1, BT_INTEGER) == FAILURE
      || kind_value_check (array, 1, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  if (array_check (array, 1) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE
      || kind_value_check (array, 1, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_transfer (gfc_expr *source ATTRIBUTE_UNUSED,
		    gfc_expr *mold ATTRIBUTE_UNUSED, gfc_expr *size)
{
  if (mold->ts.type == BT_HOLLERITH)
    {
      gfc_error ("'MOLD' argument of 'TRANSFER' intrinsic at %L must not be %s",
		 &mold->where, gfc_basic_typename (BT_HOLLERITH));
      return FAILURE;
    }

  if (size != NULL)
    {
      if (type_check (size, 2, BT_INTEGER) == FAILURE)
	return FAILURE;

      if (scalar_check (size, 2) == FAILURE)
	return FAILURE;

      if (nonoptional_check (size, 2) == FAILURE)
	return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_transpose (gfc_expr *matrix)
{
  if (rank_check (matrix, 0, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ubound (gfc_expr *array, gfc_expr *dim, gfc_expr *kind)
{
  if (array_check (array, 0) == FAILURE)
    return FAILURE;

  if (dim != NULL)
    {
      if (dim_check (dim, 1, false) == FAILURE)
	return FAILURE;

      if (dim_rank_check (dim, array, 0) == FAILURE)
	return FAILURE;
    }

  if (kind_check (kind, 2, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_unpack (gfc_expr *vector, gfc_expr *mask, gfc_expr *field)
{
  if (rank_check (vector, 0, 1) == FAILURE)
    return FAILURE;

  if (array_check (mask, 1) == FAILURE)
    return FAILURE;

  if (type_check (mask, 1, BT_LOGICAL) == FAILURE)
    return FAILURE;

  if (same_type_check (vector, 0, field, 2) == FAILURE)
    return FAILURE;

  if (mask->rank != field->rank && field->rank != 0)
    {
      gfc_error ("FIELD argument at %L of UNPACK must have the same rank as "
		 "MASK or be a scalar", &field->where);
      return FAILURE;
    }

  if (mask->rank == field->rank)
    {
      int i;
      for (i = 0; i < field->rank; i++)
	if (! identical_dimen_shape (mask, i, field, i))
	{
	  gfc_error ("Different shape in dimension %d for MASK and FIELD "
		     "arguments of UNPACK at %L", mask->rank, &field->where);
	  return FAILURE;
	}
    }

  return SUCCESS;
}


gfc_try
gfc_check_verify (gfc_expr *x, gfc_expr *y, gfc_expr *z, gfc_expr *kind)
{
  if (type_check (x, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (same_type_check (x, 0, y, 1) == FAILURE)
    return FAILURE;

  if (z != NULL && type_check (z, 2, BT_LOGICAL) == FAILURE)
    return FAILURE;

  if (kind_check (kind, 3, BT_INTEGER) == FAILURE)
    return FAILURE;
  if (kind && gfc_notify_std (GFC_STD_F2003, "Fortran 2003: '%s' intrinsic "
			      "with KIND argument at %L",
			      gfc_current_intrinsic, &kind->where) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_trim (gfc_expr *x)
{
  if (type_check (x, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;

  if (scalar_check (x, 0) == FAILURE)
    return FAILURE;

   return SUCCESS;
}


gfc_try
gfc_check_ttynam (gfc_expr *unit)
{
  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* Common check function for the half a dozen intrinsics that have a
   single real argument.  */

gfc_try
gfc_check_x (gfc_expr *x)
{
  if (type_check (x, 0, BT_REAL) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/************* Check functions for intrinsic subroutines *************/

gfc_try
gfc_check_cpu_time (gfc_expr *time)
{
  if (scalar_check (time, 0) == FAILURE)
    return FAILURE;

  if (type_check (time, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (variable_check (time, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_date_and_time (gfc_expr *date, gfc_expr *time,
			 gfc_expr *zone, gfc_expr *values)
{
  if (date != NULL)
    {
      if (type_check (date, 0, BT_CHARACTER) == FAILURE)
	return FAILURE;
      if (kind_value_check (date, 0, gfc_default_character_kind) == FAILURE)
	return FAILURE;
      if (scalar_check (date, 0) == FAILURE)
	return FAILURE;
      if (variable_check (date, 0) == FAILURE)
	return FAILURE;
    }

  if (time != NULL)
    {
      if (type_check (time, 1, BT_CHARACTER) == FAILURE)
	return FAILURE;
      if (kind_value_check (time, 1, gfc_default_character_kind) == FAILURE)
	return FAILURE;
      if (scalar_check (time, 1) == FAILURE)
	return FAILURE;
      if (variable_check (time, 1) == FAILURE)
	return FAILURE;
    }

  if (zone != NULL)
    {
      if (type_check (zone, 2, BT_CHARACTER) == FAILURE)
	return FAILURE;
      if (kind_value_check (zone, 2, gfc_default_character_kind) == FAILURE)
	return FAILURE;
      if (scalar_check (zone, 2) == FAILURE)
	return FAILURE;
      if (variable_check (zone, 2) == FAILURE)
	return FAILURE;
    }

  if (values != NULL)
    {
      if (type_check (values, 3, BT_INTEGER) == FAILURE)
	return FAILURE;
      if (array_check (values, 3) == FAILURE)
	return FAILURE;
      if (rank_check (values, 3, 1) == FAILURE)
	return FAILURE;
      if (variable_check (values, 3) == FAILURE)
	return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_mvbits (gfc_expr *from, gfc_expr *frompos, gfc_expr *len,
		  gfc_expr *to, gfc_expr *topos)
{
  if (type_check (from, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (frompos, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (len, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (same_type_check (from, 0, to, 3) == FAILURE)
    return FAILURE;

  if (variable_check (to, 3) == FAILURE)
    return FAILURE;

  if (type_check (topos, 4, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_random_number (gfc_expr *harvest)
{
  if (type_check (harvest, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (variable_check (harvest, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_random_seed (gfc_expr *size, gfc_expr *put, gfc_expr *get)
{
  unsigned int nargs = 0, kiss_size;
  locus *where = NULL;
  mpz_t put_size, get_size;
  bool have_gfc_real_16; /* Try and mimic HAVE_GFC_REAL_16 in libgfortran.  */

  have_gfc_real_16 = gfc_validate_kind (BT_REAL, 16, true) != -1;

  /* Keep the number of bytes in sync with kiss_size in
     libgfortran/intrinsics/random.c.  */
  kiss_size = (have_gfc_real_16 ? 48 : 32) / gfc_default_integer_kind;

  if (size != NULL)
    {
      if (size->expr_type != EXPR_VARIABLE
	  || !size->symtree->n.sym->attr.optional)
	nargs++;

      if (scalar_check (size, 0) == FAILURE)
	return FAILURE;

      if (type_check (size, 0, BT_INTEGER) == FAILURE)
	return FAILURE;

      if (variable_check (size, 0) == FAILURE)
	return FAILURE;

      if (kind_value_check (size, 0, gfc_default_integer_kind) == FAILURE)
	return FAILURE;
    }

  if (put != NULL)
    {
      if (put->expr_type != EXPR_VARIABLE
	  || !put->symtree->n.sym->attr.optional)
	{
	  nargs++;
	  where = &put->where;
	}

      if (array_check (put, 1) == FAILURE)
	return FAILURE;

      if (rank_check (put, 1, 1) == FAILURE)
	return FAILURE;

      if (type_check (put, 1, BT_INTEGER) == FAILURE)
	return FAILURE;

      if (kind_value_check (put, 1, gfc_default_integer_kind) == FAILURE)
	return FAILURE;

      if (gfc_array_size (put, &put_size) == SUCCESS
	  && mpz_get_ui (put_size) < kiss_size)
	gfc_error ("Size of '%s' argument of '%s' intrinsic at %L "
		   "too small (%i/%i)",
		   gfc_current_intrinsic_arg[1], gfc_current_intrinsic, where, 
		   (int) mpz_get_ui (put_size), kiss_size);
    }

  if (get != NULL)
    {
      if (get->expr_type != EXPR_VARIABLE
	  || !get->symtree->n.sym->attr.optional)
	{
	  nargs++;
	  where = &get->where;
	}

      if (array_check (get, 2) == FAILURE)
	return FAILURE;

      if (rank_check (get, 2, 1) == FAILURE)
	return FAILURE;

      if (type_check (get, 2, BT_INTEGER) == FAILURE)
	return FAILURE;

      if (variable_check (get, 2) == FAILURE)
	return FAILURE;

      if (kind_value_check (get, 2, gfc_default_integer_kind) == FAILURE)
	return FAILURE;

       if (gfc_array_size (get, &get_size) == SUCCESS
 	  && mpz_get_ui (get_size) < kiss_size)
	gfc_error ("Size of '%s' argument of '%s' intrinsic at %L "
		   "too small (%i/%i)",
		   gfc_current_intrinsic_arg[2], gfc_current_intrinsic, where, 
		   (int) mpz_get_ui (get_size), kiss_size);
    }

  /* RANDOM_SEED may not have more than one non-optional argument.  */
  if (nargs > 1)
    gfc_error ("Too many arguments to %s at %L", gfc_current_intrinsic, where);

  return SUCCESS;
}


gfc_try
gfc_check_second_sub (gfc_expr *time)
{
  if (scalar_check (time, 0) == FAILURE)
    return FAILURE;

  if (type_check (time, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (kind_value_check(time, 0, 4) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* The arguments of SYSTEM_CLOCK are scalar, integer variables.  Note,
   count, count_rate, and count_max are all optional arguments */

gfc_try
gfc_check_system_clock (gfc_expr *count, gfc_expr *count_rate,
			gfc_expr *count_max)
{
  if (count != NULL)
    {
      if (scalar_check (count, 0) == FAILURE)
	return FAILURE;

      if (type_check (count, 0, BT_INTEGER) == FAILURE)
	return FAILURE;

      if (variable_check (count, 0) == FAILURE)
	return FAILURE;
    }

  if (count_rate != NULL)
    {
      if (scalar_check (count_rate, 1) == FAILURE)
	return FAILURE;

      if (type_check (count_rate, 1, BT_INTEGER) == FAILURE)
	return FAILURE;

      if (variable_check (count_rate, 1) == FAILURE)
	return FAILURE;

      if (count != NULL
	  && same_type_check (count, 0, count_rate, 1) == FAILURE)
	return FAILURE;

    }

  if (count_max != NULL)
    {
      if (scalar_check (count_max, 2) == FAILURE)
	return FAILURE;

      if (type_check (count_max, 2, BT_INTEGER) == FAILURE)
	return FAILURE;

      if (variable_check (count_max, 2) == FAILURE)
	return FAILURE;

      if (count != NULL
	  && same_type_check (count, 0, count_max, 2) == FAILURE)
	return FAILURE;

      if (count_rate != NULL
	  && same_type_check (count_rate, 1, count_max, 2) == FAILURE)
	return FAILURE;
    }

  return SUCCESS;
}


gfc_try
gfc_check_irand (gfc_expr *x)
{
  if (x == NULL)
    return SUCCESS;

  if (scalar_check (x, 0) == FAILURE)
    return FAILURE;

  if (type_check (x, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check(x, 0, 4) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_alarm_sub (gfc_expr *seconds, gfc_expr *handler, gfc_expr *status)
{
  if (scalar_check (seconds, 0) == FAILURE)
    return FAILURE;

  if (type_check (seconds, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (handler->ts.type != BT_INTEGER && handler->ts.type != BT_PROCEDURE)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be INTEGER "
		 "or PROCEDURE", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &handler->where);
      return FAILURE;
    }

  if (handler->ts.type == BT_INTEGER && scalar_check (handler, 1) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  if (type_check (status, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check (status, 2, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_rand (gfc_expr *x)
{
  if (x == NULL)
    return SUCCESS;

  if (scalar_check (x, 0) == FAILURE)
    return FAILURE;

  if (type_check (x, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check(x, 0, 4) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_srand (gfc_expr *x)
{
  if (scalar_check (x, 0) == FAILURE)
    return FAILURE;

  if (type_check (x, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check(x, 0, 4) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ctime_sub (gfc_expr *time, gfc_expr *result)
{
  if (scalar_check (time, 0) == FAILURE)
    return FAILURE;
  if (type_check (time, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (result, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (result, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_dtime_etime (gfc_expr *x)
{
  if (array_check (x, 0) == FAILURE)
    return FAILURE;

  if (rank_check (x, 0, 1) == FAILURE)
    return FAILURE;

  if (variable_check (x, 0) == FAILURE)
    return FAILURE;

  if (type_check (x, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (kind_value_check(x, 0, 4) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_dtime_etime_sub (gfc_expr *values, gfc_expr *time)
{
  if (array_check (values, 0) == FAILURE)
    return FAILURE;

  if (rank_check (values, 0, 1) == FAILURE)
    return FAILURE;

  if (variable_check (values, 0) == FAILURE)
    return FAILURE;

  if (type_check (values, 0, BT_REAL) == FAILURE)
    return FAILURE;

  if (kind_value_check(values, 0, 4) == FAILURE)
    return FAILURE;

  if (scalar_check (time, 1) == FAILURE)
    return FAILURE;

  if (type_check (time, 1, BT_REAL) == FAILURE)
    return FAILURE;

  if (kind_value_check(time, 1, 4) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_fdate_sub (gfc_expr *date)
{
  if (type_check (date, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (date, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_gerror (gfc_expr *msg)
{
  if (type_check (msg, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (msg, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_getcwd_sub (gfc_expr *cwd, gfc_expr *status)
{
  if (type_check (cwd, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (cwd, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (scalar_check (status, 1) == FAILURE)
    return FAILURE;

  if (type_check (status, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_getarg (gfc_expr *pos, gfc_expr *value)
{
  if (type_check (pos, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (pos->ts.kind > gfc_default_integer_kind)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be of a kind "
		 "not wider than the default kind (%d)",
		 gfc_current_intrinsic_arg[0], gfc_current_intrinsic,
		 &pos->where, gfc_default_integer_kind);
      return FAILURE;
    }

  if (type_check (value, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (value, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_getlog (gfc_expr *msg)
{
  if (type_check (msg, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (msg, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_exit (gfc_expr *status)
{
  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_flush (gfc_expr *unit)
{
  if (unit == NULL)
    return SUCCESS;

  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_free (gfc_expr *i)
{
  if (type_check (i, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (i, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_hostnm (gfc_expr *name)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_hostnm_sub (gfc_expr *name, gfc_expr *status)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (scalar_check (status, 1) == FAILURE)
    return FAILURE;

  if (type_check (status, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_itime_idate (gfc_expr *values)
{
  if (array_check (values, 0) == FAILURE)
    return FAILURE;

  if (rank_check (values, 0, 1) == FAILURE)
    return FAILURE;

  if (variable_check (values, 0) == FAILURE)
    return FAILURE;

  if (type_check (values, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check(values, 0, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ltime_gmtime (gfc_expr *time, gfc_expr *values)
{
  if (type_check (time, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check(time, 0, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  if (scalar_check (time, 0) == FAILURE)
    return FAILURE;

  if (array_check (values, 1) == FAILURE)
    return FAILURE;

  if (rank_check (values, 1, 1) == FAILURE)
    return FAILURE;

  if (variable_check (values, 1) == FAILURE)
    return FAILURE;

  if (type_check (values, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check(values, 1, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_ttynam_sub (gfc_expr *unit, gfc_expr *name)
{
  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (type_check (name, 1, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 1, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_isatty (gfc_expr *unit)
{
  if (unit == NULL)
    return FAILURE;

  if (type_check (unit, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (unit, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_isnan (gfc_expr *x)
{
  if (type_check (x, 0, BT_REAL) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_perror (gfc_expr *string)
{
  if (type_check (string, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (string, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_umask (gfc_expr *mask)
{
  if (type_check (mask, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (mask, 0) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_umask_sub (gfc_expr *mask, gfc_expr *old)
{
  if (type_check (mask, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (mask, 0) == FAILURE)
    return FAILURE;

  if (old == NULL)
    return SUCCESS;

  if (scalar_check (old, 1) == FAILURE)
    return FAILURE;

  if (type_check (old, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_unlink (gfc_expr *name)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_unlink_sub (gfc_expr *name, gfc_expr *status)
{
  if (type_check (name, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (name, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (scalar_check (status, 1) == FAILURE)
    return FAILURE;

  if (type_check (status, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_signal (gfc_expr *number, gfc_expr *handler)
{
  if (scalar_check (number, 0) == FAILURE)
    return FAILURE;

  if (type_check (number, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (handler->ts.type != BT_INTEGER && handler->ts.type != BT_PROCEDURE)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be INTEGER "
		 "or PROCEDURE", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &handler->where);
      return FAILURE;
    }

  if (handler->ts.type == BT_INTEGER && scalar_check (handler, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_signal_sub (gfc_expr *number, gfc_expr *handler, gfc_expr *status)
{
  if (scalar_check (number, 0) == FAILURE)
    return FAILURE;

  if (type_check (number, 0, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (handler->ts.type != BT_INTEGER && handler->ts.type != BT_PROCEDURE)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be INTEGER "
		 "or PROCEDURE", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &handler->where);
      return FAILURE;
    }

  if (handler->ts.type == BT_INTEGER && scalar_check (handler, 1) == FAILURE)
    return FAILURE;

  if (status == NULL)
    return SUCCESS;

  if (type_check (status, 2, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 2) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


gfc_try
gfc_check_system_sub (gfc_expr *cmd, gfc_expr *status)
{
  if (type_check (cmd, 0, BT_CHARACTER) == FAILURE)
    return FAILURE;
  if (kind_value_check (cmd, 0, gfc_default_character_kind) == FAILURE)
    return FAILURE;

  if (scalar_check (status, 1) == FAILURE)
    return FAILURE;

  if (type_check (status, 1, BT_INTEGER) == FAILURE)
    return FAILURE;

  if (kind_value_check (status, 1, gfc_default_integer_kind) == FAILURE)
    return FAILURE;

  return SUCCESS;
}


/* This is used for the GNU intrinsics AND, OR and XOR.  */
gfc_try
gfc_check_and (gfc_expr *i, gfc_expr *j)
{
  if (i->ts.type != BT_INTEGER && i->ts.type != BT_LOGICAL)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be INTEGER "
		 "or LOGICAL", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic, &i->where);
      return FAILURE;
    }

  if (j->ts.type != BT_INTEGER && j->ts.type != BT_LOGICAL)
    {
      gfc_error ("'%s' argument of '%s' intrinsic at %L must be INTEGER "
		 "or LOGICAL", gfc_current_intrinsic_arg[1],
		 gfc_current_intrinsic, &j->where);
      return FAILURE;
    }

  if (i->ts.type != j->ts.type)
    {
      gfc_error ("'%s' and '%s' arguments of '%s' intrinsic at %L must "
		 "have the same type", gfc_current_intrinsic_arg[0],
		 gfc_current_intrinsic_arg[1], gfc_current_intrinsic,
		 &j->where);
      return FAILURE;
    }

  if (scalar_check (i, 0) == FAILURE)
    return FAILURE;

  if (scalar_check (j, 1) == FAILURE)
    return FAILURE;

  return SUCCESS;
}
