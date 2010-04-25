/* Parse tree dumper
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008
   Free Software Foundation, Inc.
   Contributed by Steven Bosscher

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


/* Actually this is just a collection of routines that used to be
   scattered around the sources.  Now that they are all in a single
   file, almost all of them can be static, and the other files don't
   have this mess in them.

   As a nice side-effect, this file can act as documentation of the
   gfc_code and gfc_expr structures and all their friends and
   relatives.

   TODO: Dump DATA.  */

#include "config.h"
#include "gfortran.h"

/* Keep track of indentation for symbol tree dumps.  */
static int show_level = 0;

/* The file handle we're dumping to is kept in a static variable.  This
   is not too cool, but it avoids a lot of passing it around.  */
static FILE *dumpfile;

/* Forward declaration of some of the functions.  */
static void show_expr (gfc_expr *p);
static void show_code_node (int, gfc_code *);
static void show_namespace (gfc_namespace *ns);


/* Do indentation for a specific level.  */

static inline void
code_indent (int level, gfc_st_label *label)
{
  int i;

  if (label != NULL)
    fprintf (dumpfile, "%-5d ", label->value);
  else
    fputs ("      ", dumpfile);

  for (i = 0; i < 2 * level; i++)
    fputc (' ', dumpfile);
}


/* Simple indentation at the current level.  This one
   is used to show symbols.  */

static inline void
show_indent (void)
{
  fputc ('\n', dumpfile);
  code_indent (show_level, NULL);
}


/* Show type-specific information.  */

static void
show_typespec (gfc_typespec *ts)
{
  fprintf (dumpfile, "(%s ", gfc_basic_typename (ts->type));

  switch (ts->type)
    {
    case BT_DERIVED:
      fprintf (dumpfile, "%s", ts->derived->name);
      break;

    case BT_CHARACTER:
      show_expr (ts->cl->length);
      break;

    default:
      fprintf (dumpfile, "%d", ts->kind);
      break;
    }

  fputc (')', dumpfile);
}


/* Show an actual argument list.  */

static void
show_actual_arglist (gfc_actual_arglist *a)
{
  fputc ('(', dumpfile);

  for (; a; a = a->next)
    {
      fputc ('(', dumpfile);
      if (a->name != NULL)
	fprintf (dumpfile, "%s = ", a->name);
      if (a->expr != NULL)
	show_expr (a->expr);
      else
	fputs ("(arg not-present)", dumpfile);

      fputc (')', dumpfile);
      if (a->next != NULL)
	fputc (' ', dumpfile);
    }

  fputc (')', dumpfile);
}


/* Show a gfc_array_spec array specification structure.  */

static void
show_array_spec (gfc_array_spec *as)
{
  const char *c;
  int i;

  if (as == NULL)
    {
      fputs ("()", dumpfile);
      return;
    }

  fprintf (dumpfile, "(%d", as->rank);

  if (as->rank != 0)
    {
      switch (as->type)
      {
	case AS_EXPLICIT:      c = "AS_EXPLICIT";      break;
	case AS_DEFERRED:      c = "AS_DEFERRED";      break;
	case AS_ASSUMED_SIZE:  c = "AS_ASSUMED_SIZE";  break;
	case AS_ASSUMED_SHAPE: c = "AS_ASSUMED_SHAPE"; break;
	default:
	  gfc_internal_error ("show_array_spec(): Unhandled array shape "
			      "type.");
      }
      fprintf (dumpfile, " %s ", c);

      for (i = 0; i < as->rank; i++)
	{
	  show_expr (as->lower[i]);
	  fputc (' ', dumpfile);
	  show_expr (as->upper[i]);
	  fputc (' ', dumpfile);
	}
    }

  fputc (')', dumpfile);
}


/* Show a gfc_array_ref array reference structure.  */

static void
show_array_ref (gfc_array_ref * ar)
{
  int i;

  fputc ('(', dumpfile);

  switch (ar->type)
    {
    case AR_FULL:
      fputs ("FULL", dumpfile);
      break;

    case AR_SECTION:
      for (i = 0; i < ar->dimen; i++)
	{
	  /* There are two types of array sections: either the
	     elements are identified by an integer array ('vector'),
	     or by an index range. In the former case we only have to
	     print the start expression which contains the vector, in
	     the latter case we have to print any of lower and upper
	     bound and the stride, if they're present.  */
  
	  if (ar->start[i] != NULL)
	    show_expr (ar->start[i]);

	  if (ar->dimen_type[i] == DIMEN_RANGE)
	    {
	      fputc (':', dumpfile);

	      if (ar->end[i] != NULL)
		show_expr (ar->end[i]);

	      if (ar->stride[i] != NULL)
		{
		  fputc (':', dumpfile);
		  show_expr (ar->stride[i]);
		}
	    }

	  if (i != ar->dimen - 1)
	    fputs (" , ", dumpfile);
	}
      break;

    case AR_ELEMENT:
      for (i = 0; i < ar->dimen; i++)
	{
	  show_expr (ar->start[i]);
	  if (i != ar->dimen - 1)
	    fputs (" , ", dumpfile);
	}
      break;

    case AR_UNKNOWN:
      fputs ("UNKNOWN", dumpfile);
      break;

    default:
      gfc_internal_error ("show_array_ref(): Unknown array reference");
    }

  fputc (')', dumpfile);
}


/* Show a list of gfc_ref structures.  */

static void
show_ref (gfc_ref *p)
{
  for (; p; p = p->next)
    switch (p->type)
      {
      case REF_ARRAY:
	show_array_ref (&p->u.ar);
	break;

      case REF_COMPONENT:
	fprintf (dumpfile, " %% %s", p->u.c.component->name);
	break;

      case REF_SUBSTRING:
	fputc ('(', dumpfile);
	show_expr (p->u.ss.start);
	fputc (':', dumpfile);
	show_expr (p->u.ss.end);
	fputc (')', dumpfile);
	break;

      default:
	gfc_internal_error ("show_ref(): Bad component code");
      }
}


/* Display a constructor.  Works recursively for array constructors.  */

static void
show_constructor (gfc_constructor *c)
{
  for (; c; c = c->next)
    {
      if (c->iterator == NULL)
	show_expr (c->expr);
      else
	{
	  fputc ('(', dumpfile);
	  show_expr (c->expr);

	  fputc (' ', dumpfile);
	  show_expr (c->iterator->var);
	  fputc ('=', dumpfile);
	  show_expr (c->iterator->start);
	  fputc (',', dumpfile);
	  show_expr (c->iterator->end);
	  fputc (',', dumpfile);
	  show_expr (c->iterator->step);

	  fputc (')', dumpfile);
	}

      if (c->next != NULL)
	fputs (" , ", dumpfile);
    }
}


static void
show_char_const (const gfc_char_t *c, int length)
{
  int i;

  fputc ('\'', dumpfile);
  for (i = 0; i < length; i++)
    {
      if (c[i] == '\'')
	fputs ("''", dumpfile);
      else
	fputs (gfc_print_wide_char (c[i]), dumpfile);
    }
  fputc ('\'', dumpfile);
}


/* Show a component-call expression.  */

static void
show_compcall (gfc_expr* p)
{
  gcc_assert (p->expr_type == EXPR_COMPCALL);

  fprintf (dumpfile, "%s", p->symtree->n.sym->name);
  show_ref (p->ref);
  fprintf (dumpfile, "%s", p->value.compcall.name);

  show_actual_arglist (p->value.compcall.actual);
}


/* Show an expression.  */

static void
show_expr (gfc_expr *p)
{
  const char *c;
  int i;

  if (p == NULL)
    {
      fputs ("()", dumpfile);
      return;
    }

  switch (p->expr_type)
    {
    case EXPR_SUBSTRING:
      show_char_const (p->value.character.string, p->value.character.length);
      show_ref (p->ref);
      break;

    case EXPR_STRUCTURE:
      fprintf (dumpfile, "%s(", p->ts.derived->name);
      show_constructor (p->value.constructor);
      fputc (')', dumpfile);
      break;

    case EXPR_ARRAY:
      fputs ("(/ ", dumpfile);
      show_constructor (p->value.constructor);
      fputs (" /)", dumpfile);

      show_ref (p->ref);
      break;

    case EXPR_NULL:
      fputs ("NULL()", dumpfile);
      break;

    case EXPR_CONSTANT:
      switch (p->ts.type)
	{
	case BT_INTEGER:
	  mpz_out_str (stdout, 10, p->value.integer);

	  if (p->ts.kind != gfc_default_integer_kind)
	    fprintf (dumpfile, "_%d", p->ts.kind);
	  break;

	case BT_LOGICAL:
	  if (p->value.logical)
	    fputs (".true.", dumpfile);
	  else
	    fputs (".false.", dumpfile);
	  break;

	case BT_REAL:
	  mpfr_out_str (stdout, 10, 0, p->value.real, GFC_RND_MODE);
	  if (p->ts.kind != gfc_default_real_kind)
	    fprintf (dumpfile, "_%d", p->ts.kind);
	  break;

	case BT_CHARACTER:
	  show_char_const (p->value.character.string, 
			   p->value.character.length);
	  break;

	case BT_COMPLEX:
	  fputs ("(complex ", dumpfile);

	  mpfr_out_str (stdout, 10, 0, p->value.complex.r, GFC_RND_MODE);
	  if (p->ts.kind != gfc_default_complex_kind)
	    fprintf (dumpfile, "_%d", p->ts.kind);

	  fputc (' ', dumpfile);

	  mpfr_out_str (stdout, 10, 0, p->value.complex.i, GFC_RND_MODE);
	  if (p->ts.kind != gfc_default_complex_kind)
	    fprintf (dumpfile, "_%d", p->ts.kind);

	  fputc (')', dumpfile);
	  break;

	case BT_HOLLERITH:
	  fprintf (dumpfile, "%dH", p->representation.length);
	  c = p->representation.string;
	  for (i = 0; i < p->representation.length; i++, c++)
	    {
	      fputc (*c, dumpfile);
	    }
	  break;

	default:
	  fputs ("???", dumpfile);
	  break;
	}

      if (p->representation.string)
	{
	  fputs (" {", dumpfile);
	  c = p->representation.string;
	  for (i = 0; i < p->representation.length; i++, c++)
	    {
	      fprintf (dumpfile, "%.2x", (unsigned int) *c);
	      if (i < p->representation.length - 1)
		fputc (',', dumpfile);
	    }
	  fputc ('}', dumpfile);
	}

      break;

    case EXPR_VARIABLE:
      if (p->symtree->n.sym->ns && p->symtree->n.sym->ns->proc_name)
	fprintf (dumpfile, "%s:", p->symtree->n.sym->ns->proc_name->name);
      fprintf (dumpfile, "%s", p->symtree->n.sym->name);
      show_ref (p->ref);
      break;

    case EXPR_OP:
      fputc ('(', dumpfile);
      switch (p->value.op.op)
	{
	case INTRINSIC_UPLUS:
	  fputs ("U+ ", dumpfile);
	  break;
	case INTRINSIC_UMINUS:
	  fputs ("U- ", dumpfile);
	  break;
	case INTRINSIC_PLUS:
	  fputs ("+ ", dumpfile);
	  break;
	case INTRINSIC_MINUS:
	  fputs ("- ", dumpfile);
	  break;
	case INTRINSIC_TIMES:
	  fputs ("* ", dumpfile);
	  break;
	case INTRINSIC_DIVIDE:
	  fputs ("/ ", dumpfile);
	  break;
	case INTRINSIC_POWER:
	  fputs ("** ", dumpfile);
	  break;
	case INTRINSIC_CONCAT:
	  fputs ("// ", dumpfile);
	  break;
	case INTRINSIC_AND:
	  fputs ("AND ", dumpfile);
	  break;
	case INTRINSIC_OR:
	  fputs ("OR ", dumpfile);
	  break;
	case INTRINSIC_EQV:
	  fputs ("EQV ", dumpfile);
	  break;
	case INTRINSIC_NEQV:
	  fputs ("NEQV ", dumpfile);
	  break;
	case INTRINSIC_EQ:
	case INTRINSIC_EQ_OS:
	  fputs ("= ", dumpfile);
	  break;
	case INTRINSIC_NE:
	case INTRINSIC_NE_OS:
	  fputs ("/= ", dumpfile);
	  break;
	case INTRINSIC_GT:
	case INTRINSIC_GT_OS:
	  fputs ("> ", dumpfile);
	  break;
	case INTRINSIC_GE:
	case INTRINSIC_GE_OS:
	  fputs (">= ", dumpfile);
	  break;
	case INTRINSIC_LT:
	case INTRINSIC_LT_OS:
	  fputs ("< ", dumpfile);
	  break;
	case INTRINSIC_LE:
	case INTRINSIC_LE_OS:
	  fputs ("<= ", dumpfile);
	  break;
	case INTRINSIC_NOT:
	  fputs ("NOT ", dumpfile);
	  break;
	case INTRINSIC_PARENTHESES:
	  fputs ("parens", dumpfile);
	  break;

	default:
	  gfc_internal_error
	    ("show_expr(): Bad intrinsic in expression!");
	}

      show_expr (p->value.op.op1);

      if (p->value.op.op2)
	{
	  fputc (' ', dumpfile);
	  show_expr (p->value.op.op2);
	}

      fputc (')', dumpfile);
      break;

    case EXPR_FUNCTION:
      if (p->value.function.name == NULL)
	{
	  fprintf (dumpfile, "%s[", p->symtree->n.sym->name);
	  show_actual_arglist (p->value.function.actual);
	  fputc (']', dumpfile);
	}
      else
	{
	  fprintf (dumpfile, "%s[[", p->value.function.name);
	  show_actual_arglist (p->value.function.actual);
	  fputc (']', dumpfile);
	  fputc (']', dumpfile);
	}

      break;

    case EXPR_COMPCALL:
      show_compcall (p);
      break;

    default:
      gfc_internal_error ("show_expr(): Don't know how to show expr");
    }
}

/* Show symbol attributes.  The flavor and intent are followed by
   whatever single bit attributes are present.  */

static void
show_attr (symbol_attribute *attr)
{

  fprintf (dumpfile, "(%s %s %s %s %s",
	   gfc_code2string (flavors, attr->flavor),
	   gfc_intent_string (attr->intent),
	   gfc_code2string (access_types, attr->access),
	   gfc_code2string (procedures, attr->proc),
	   gfc_code2string (save_status, attr->save));

  if (attr->allocatable)
    fputs (" ALLOCATABLE", dumpfile);
  if (attr->dimension)
    fputs (" DIMENSION", dumpfile);
  if (attr->external)
    fputs (" EXTERNAL", dumpfile);
  if (attr->intrinsic)
    fputs (" INTRINSIC", dumpfile);
  if (attr->optional)
    fputs (" OPTIONAL", dumpfile);
  if (attr->pointer)
    fputs (" POINTER", dumpfile);
  if (attr->is_protected)
    fputs (" PROTECTED", dumpfile);
  if (attr->value)
    fputs (" VALUE", dumpfile);
  if (attr->volatile_)
    fputs (" VOLATILE", dumpfile);
  if (attr->threadprivate)
    fputs (" THREADPRIVATE", dumpfile);
  if (attr->target)
    fputs (" TARGET", dumpfile);
  if (attr->dummy)
    fputs (" DUMMY", dumpfile);
  if (attr->result)
    fputs (" RESULT", dumpfile);
  if (attr->entry)
    fputs (" ENTRY", dumpfile);
  if (attr->is_bind_c)
    fputs (" BIND(C)", dumpfile);

  if (attr->data)
    fputs (" DATA", dumpfile);
  if (attr->use_assoc)
    fputs (" USE-ASSOC", dumpfile);
  if (attr->in_namelist)
    fputs (" IN-NAMELIST", dumpfile);
  if (attr->in_common)
    fputs (" IN-COMMON", dumpfile);

  if (attr->abstract)
    fputs (" ABSTRACT", dumpfile);
  if (attr->function)
    fputs (" FUNCTION", dumpfile);
  if (attr->subroutine)
    fputs (" SUBROUTINE", dumpfile);
  if (attr->implicit_type)
    fputs (" IMPLICIT-TYPE", dumpfile);

  if (attr->sequence)
    fputs (" SEQUENCE", dumpfile);
  if (attr->elemental)
    fputs (" ELEMENTAL", dumpfile);
  if (attr->pure)
    fputs (" PURE", dumpfile);
  if (attr->recursive)
    fputs (" RECURSIVE", dumpfile);

  fputc (')', dumpfile);
}


/* Show components of a derived type.  */

static void
show_components (gfc_symbol *sym)
{
  gfc_component *c;

  for (c = sym->components; c; c = c->next)
    {
      fprintf (dumpfile, "(%s ", c->name);
      show_typespec (&c->ts);
      if (c->attr.pointer)
	fputs (" POINTER", dumpfile);
      if (c->attr.dimension)
	fputs (" DIMENSION", dumpfile);
      fputc (' ', dumpfile);
      show_array_spec (c->as);
      if (c->attr.access)
	fprintf (dumpfile, " %s", gfc_code2string (access_types, c->attr.access));
      fputc (')', dumpfile);
      if (c->next != NULL)
	fputc (' ', dumpfile);
    }
}


/* Show the f2k_derived namespace with procedure bindings.  */

static void
show_typebound (gfc_symtree* st)
{
  if (!st->typebound)
    return;

  show_indent ();

  if (st->typebound->is_generic)
    fputs ("GENERIC", dumpfile);
  else
    {
      fputs ("PROCEDURE, ", dumpfile);
      if (st->typebound->nopass)
	fputs ("NOPASS", dumpfile);
      else
	{
	  if (st->typebound->pass_arg)
	    fprintf (dumpfile, "PASS(%s)", st->typebound->pass_arg);
	  else
	    fputs ("PASS", dumpfile);
	}
      if (st->typebound->non_overridable)
	fputs (", NON_OVERRIDABLE", dumpfile);
    }

  if (st->typebound->access == ACCESS_PUBLIC)
    fputs (", PUBLIC", dumpfile);
  else
    fputs (", PRIVATE", dumpfile);

  fprintf (dumpfile, " :: %s => ", st->n.sym->name);

  if (st->typebound->is_generic)
    {
      gfc_tbp_generic* g;
      for (g = st->typebound->u.generic; g; g = g->next)
	{
	  fputs (g->specific_st->name, dumpfile);
	  if (g->next)
	    fputs (", ", dumpfile);
	}
    }
  else
    fputs (st->typebound->u.specific->n.sym->name, dumpfile);
}

static void
show_f2k_derived (gfc_namespace* f2k)
{
  gfc_finalizer* f;

  ++show_level;

  /* Finalizer bindings.  */
  for (f = f2k->finalizers; f; f = f->next)
    {
      show_indent ();
      fprintf (dumpfile, "FINAL %s", f->proc_sym->name);
    }

  /* Type-bound procedures.  */
  gfc_traverse_symtree (f2k->sym_root, &show_typebound);

  --show_level;
}


/* Show a symbol.  If a symbol is an ENTRY, SUBROUTINE or FUNCTION, we
   show the interface.  Information needed to reconstruct the list of
   specific interfaces associated with a generic symbol is done within
   that symbol.  */

static void
show_symbol (gfc_symbol *sym)
{
  gfc_formal_arglist *formal;
  gfc_interface *intr;

  if (sym == NULL)
    return;

  show_indent ();

  fprintf (dumpfile, "symbol %s ", sym->name);
  show_typespec (&sym->ts);
  show_attr (&sym->attr);

  if (sym->value)
    {
      show_indent ();
      fputs ("value: ", dumpfile);
      show_expr (sym->value);
    }

  if (sym->as)
    {
      show_indent ();
      fputs ("Array spec:", dumpfile);
      show_array_spec (sym->as);
    }

  if (sym->generic)
    {
      show_indent ();
      fputs ("Generic interfaces:", dumpfile);
      for (intr = sym->generic; intr; intr = intr->next)
	fprintf (dumpfile, " %s", intr->sym->name);
    }

  if (sym->result)
    {
      show_indent ();
      fprintf (dumpfile, "result: %s", sym->result->name);
    }

  if (sym->components)
    {
      show_indent ();
      fputs ("components: ", dumpfile);
      show_components (sym);
    }

  if (sym->f2k_derived)
    {
      show_indent ();
      fputs ("Procedure bindings:\n", dumpfile);
      show_f2k_derived (sym->f2k_derived);
    }

  if (sym->formal)
    {
      show_indent ();
      fputs ("Formal arglist:", dumpfile);

      for (formal = sym->formal; formal; formal = formal->next)
	{
	  if (formal->sym != NULL)
	    fprintf (dumpfile, " %s", formal->sym->name);
	  else
	    fputs (" [Alt Return]", dumpfile);
	}
    }

  if (sym->formal_ns)
    {
      show_indent ();
      fputs ("Formal namespace", dumpfile);
      show_namespace (sym->formal_ns);
    }

  fputc ('\n', dumpfile);
}


/* Show a user-defined operator.  Just prints an operator
   and the name of the associated subroutine, really.  */

static void
show_uop (gfc_user_op *uop)
{
  gfc_interface *intr;

  show_indent ();
  fprintf (dumpfile, "%s:", uop->name);

  for (intr = uop->op; intr; intr = intr->next)
    fprintf (dumpfile, " %s", intr->sym->name);
}


/* Workhorse function for traversing the user operator symtree.  */

static void
traverse_uop (gfc_symtree *st, void (*func) (gfc_user_op *))
{
  if (st == NULL)
    return;

  (*func) (st->n.uop);

  traverse_uop (st->left, func);
  traverse_uop (st->right, func);
}


/* Traverse the tree of user operator nodes.  */

void
gfc_traverse_user_op (gfc_namespace *ns, void (*func) (gfc_user_op *))
{
  traverse_uop (ns->uop_root, func);
}


/* Function to display a common block.  */

static void
show_common (gfc_symtree *st)
{
  gfc_symbol *s;

  show_indent ();
  fprintf (dumpfile, "common: /%s/ ", st->name);

  s = st->n.common->head;
  while (s)
    {
      fprintf (dumpfile, "%s", s->name);
      s = s->common_next;
      if (s)
	fputs (", ", dumpfile);
    }
  fputc ('\n', dumpfile);
}    


/* Worker function to display the symbol tree.  */

static void
show_symtree (gfc_symtree *st)
{
  show_indent ();
  fprintf (dumpfile, "symtree: %s  Ambig %d", st->name, st->ambiguous);

  if (st->n.sym->ns != gfc_current_ns)
    fprintf (dumpfile, " from namespace %s", st->n.sym->ns->proc_name->name);
  else
    show_symbol (st->n.sym);
}


/******************* Show gfc_code structures **************/


/* Show a list of code structures.  Mutually recursive with
   show_code_node().  */

static void
show_code (int level, gfc_code *c)
{
  for (; c; c = c->next)
    show_code_node (level, c);
}

static void
show_namelist (gfc_namelist *n)
{
  for (; n->next; n = n->next)
    fprintf (dumpfile, "%s,", n->sym->name);
  fprintf (dumpfile, "%s", n->sym->name);
}

/* Show a single OpenMP directive node and everything underneath it
   if necessary.  */

static void
show_omp_node (int level, gfc_code *c)
{
  gfc_omp_clauses *omp_clauses = NULL;
  const char *name = NULL;

  switch (c->op)
    {
    case EXEC_OMP_ATOMIC: name = "ATOMIC"; break;
    case EXEC_OMP_BARRIER: name = "BARRIER"; break;
    case EXEC_OMP_CRITICAL: name = "CRITICAL"; break;
    case EXEC_OMP_FLUSH: name = "FLUSH"; break;
    case EXEC_OMP_DO: name = "DO"; break;
    case EXEC_OMP_MASTER: name = "MASTER"; break;
    case EXEC_OMP_ORDERED: name = "ORDERED"; break;
    case EXEC_OMP_PARALLEL: name = "PARALLEL"; break;
    case EXEC_OMP_PARALLEL_DO: name = "PARALLEL DO"; break;
    case EXEC_OMP_PARALLEL_SECTIONS: name = "PARALLEL SECTIONS"; break;
    case EXEC_OMP_PARALLEL_WORKSHARE: name = "PARALLEL WORKSHARE"; break;
    case EXEC_OMP_SECTIONS: name = "SECTIONS"; break;
    case EXEC_OMP_SINGLE: name = "SINGLE"; break;
    case EXEC_OMP_TASK: name = "TASK"; break;
    case EXEC_OMP_TASKWAIT: name = "TASKWAIT"; break;
    case EXEC_OMP_WORKSHARE: name = "WORKSHARE"; break;
    default:
      gcc_unreachable ();
    }
  fprintf (dumpfile, "!$OMP %s", name);
  switch (c->op)
    {
    case EXEC_OMP_DO:
    case EXEC_OMP_PARALLEL:
    case EXEC_OMP_PARALLEL_DO:
    case EXEC_OMP_PARALLEL_SECTIONS:
    case EXEC_OMP_SECTIONS:
    case EXEC_OMP_SINGLE:
    case EXEC_OMP_WORKSHARE:
    case EXEC_OMP_PARALLEL_WORKSHARE:
    case EXEC_OMP_TASK:
      omp_clauses = c->ext.omp_clauses;
      break;
    case EXEC_OMP_CRITICAL:
      if (c->ext.omp_name)
	fprintf (dumpfile, " (%s)", c->ext.omp_name);
      break;
    case EXEC_OMP_FLUSH:
      if (c->ext.omp_namelist)
	{
	  fputs (" (", dumpfile);
	  show_namelist (c->ext.omp_namelist);
	  fputc (')', dumpfile);
	}
      return;
    case EXEC_OMP_BARRIER:
    case EXEC_OMP_TASKWAIT:
      return;
    default:
      break;
    }
  if (omp_clauses)
    {
      int list_type;

      if (omp_clauses->if_expr)
	{
	  fputs (" IF(", dumpfile);
	  show_expr (omp_clauses->if_expr);
	  fputc (')', dumpfile);
	}
      if (omp_clauses->num_threads)
	{
	  fputs (" NUM_THREADS(", dumpfile);
	  show_expr (omp_clauses->num_threads);
	  fputc (')', dumpfile);
	}
      if (omp_clauses->sched_kind != OMP_SCHED_NONE)
	{
	  const char *type;
	  switch (omp_clauses->sched_kind)
	    {
	    case OMP_SCHED_STATIC: type = "STATIC"; break;
	    case OMP_SCHED_DYNAMIC: type = "DYNAMIC"; break;
	    case OMP_SCHED_GUIDED: type = "GUIDED"; break;
	    case OMP_SCHED_RUNTIME: type = "RUNTIME"; break;
	    case OMP_SCHED_AUTO: type = "AUTO"; break;
	    default:
	      gcc_unreachable ();
	    }
	  fprintf (dumpfile, " SCHEDULE (%s", type);
	  if (omp_clauses->chunk_size)
	    {
	      fputc (',', dumpfile);
	      show_expr (omp_clauses->chunk_size);
	    }
	  fputc (')', dumpfile);
	}
      if (omp_clauses->default_sharing != OMP_DEFAULT_UNKNOWN)
	{
	  const char *type;
	  switch (omp_clauses->default_sharing)
	    {
	    case OMP_DEFAULT_NONE: type = "NONE"; break;
	    case OMP_DEFAULT_PRIVATE: type = "PRIVATE"; break;
	    case OMP_DEFAULT_SHARED: type = "SHARED"; break;
	    case OMP_DEFAULT_FIRSTPRIVATE: type = "FIRSTPRIVATE"; break;
	    default:
	      gcc_unreachable ();
	    }
	  fprintf (dumpfile, " DEFAULT(%s)", type);
	}
      if (omp_clauses->ordered)
	fputs (" ORDERED", dumpfile);
      if (omp_clauses->untied)
	fputs (" UNTIED", dumpfile);
      if (omp_clauses->collapse)
	fprintf (dumpfile, " COLLAPSE(%d)", omp_clauses->collapse);
      for (list_type = 0; list_type < OMP_LIST_NUM; list_type++)
	if (omp_clauses->lists[list_type] != NULL
	    && list_type != OMP_LIST_COPYPRIVATE)
	  {
	    const char *type;
	    if (list_type >= OMP_LIST_REDUCTION_FIRST)
	      {
		switch (list_type)
		  {
		  case OMP_LIST_PLUS: type = "+"; break;
		  case OMP_LIST_MULT: type = "*"; break;
		  case OMP_LIST_SUB: type = "-"; break;
		  case OMP_LIST_AND: type = ".AND."; break;
		  case OMP_LIST_OR: type = ".OR."; break;
		  case OMP_LIST_EQV: type = ".EQV."; break;
		  case OMP_LIST_NEQV: type = ".NEQV."; break;
		  case OMP_LIST_MAX: type = "MAX"; break;
		  case OMP_LIST_MIN: type = "MIN"; break;
		  case OMP_LIST_IAND: type = "IAND"; break;
		  case OMP_LIST_IOR: type = "IOR"; break;
		  case OMP_LIST_IEOR: type = "IEOR"; break;
		  default:
		    gcc_unreachable ();
		  }
		fprintf (dumpfile, " REDUCTION(%s:", type);
	      }
	    else
	      {
		switch (list_type)
		  {
		  case OMP_LIST_PRIVATE: type = "PRIVATE"; break;
		  case OMP_LIST_FIRSTPRIVATE: type = "FIRSTPRIVATE"; break;
		  case OMP_LIST_LASTPRIVATE: type = "LASTPRIVATE"; break;
		  case OMP_LIST_SHARED: type = "SHARED"; break;
		  case OMP_LIST_COPYIN: type = "COPYIN"; break;
		  default:
		    gcc_unreachable ();
		  }
		fprintf (dumpfile, " %s(", type);
	      }
	    show_namelist (omp_clauses->lists[list_type]);
	    fputc (')', dumpfile);
	  }
    }
  fputc ('\n', dumpfile);
  if (c->op == EXEC_OMP_SECTIONS || c->op == EXEC_OMP_PARALLEL_SECTIONS)
    {
      gfc_code *d = c->block;
      while (d != NULL)
	{
	  show_code (level + 1, d->next);
	  if (d->block == NULL)
	    break;
	  code_indent (level, 0);
	  fputs ("!$OMP SECTION\n", dumpfile);
	  d = d->block;
	}
    }
  else
    show_code (level + 1, c->block->next);
  if (c->op == EXEC_OMP_ATOMIC)
    return;
  code_indent (level, 0);
  fprintf (dumpfile, "!$OMP END %s", name);
  if (omp_clauses != NULL)
    {
      if (omp_clauses->lists[OMP_LIST_COPYPRIVATE])
	{
	  fputs (" COPYPRIVATE(", dumpfile);
	  show_namelist (omp_clauses->lists[OMP_LIST_COPYPRIVATE]);
	  fputc (')', dumpfile);
	}
      else if (omp_clauses->nowait)
	fputs (" NOWAIT", dumpfile);
    }
  else if (c->op == EXEC_OMP_CRITICAL && c->ext.omp_name)
    fprintf (dumpfile, " (%s)", c->ext.omp_name);
}


/* Show a single code node and everything underneath it if necessary.  */

static void
show_code_node (int level, gfc_code *c)
{
  gfc_forall_iterator *fa;
  gfc_open *open;
  gfc_case *cp;
  gfc_alloc *a;
  gfc_code *d;
  gfc_close *close;
  gfc_filepos *fp;
  gfc_inquire *i;
  gfc_dt *dt;

  code_indent (level, c->here);

  switch (c->op)
    {
    case EXEC_NOP:
      fputs ("NOP", dumpfile);
      break;

    case EXEC_CONTINUE:
      fputs ("CONTINUE", dumpfile);
      break;

    case EXEC_ENTRY:
      fprintf (dumpfile, "ENTRY %s", c->ext.entry->sym->name);
      break;

    case EXEC_INIT_ASSIGN:
    case EXEC_ASSIGN:
      fputs ("ASSIGN ", dumpfile);
      show_expr (c->expr);
      fputc (' ', dumpfile);
      show_expr (c->expr2);
      break;

    case EXEC_LABEL_ASSIGN:
      fputs ("LABEL ASSIGN ", dumpfile);
      show_expr (c->expr);
      fprintf (dumpfile, " %d", c->label->value);
      break;

    case EXEC_POINTER_ASSIGN:
      fputs ("POINTER ASSIGN ", dumpfile);
      show_expr (c->expr);
      fputc (' ', dumpfile);
      show_expr (c->expr2);
      break;

    case EXEC_GOTO:
      fputs ("GOTO ", dumpfile);
      if (c->label)
	fprintf (dumpfile, "%d", c->label->value);
      else
	{
	  show_expr (c->expr);
	  d = c->block;
	  if (d != NULL)
	    {
	      fputs (", (", dumpfile);
	      for (; d; d = d ->block)
		{
		  code_indent (level, d->label);
		  if (d->block != NULL)
		    fputc (',', dumpfile);
		  else
		    fputc (')', dumpfile);
		}
	    }
	}
      break;

    case EXEC_CALL:
    case EXEC_ASSIGN_CALL:
      if (c->resolved_sym)
	fprintf (dumpfile, "CALL %s ", c->resolved_sym->name);
      else if (c->symtree)
	fprintf (dumpfile, "CALL %s ", c->symtree->name);
      else
	fputs ("CALL ?? ", dumpfile);

      show_actual_arglist (c->ext.actual);
      break;

    case EXEC_COMPCALL:
      fputs ("CALL ", dumpfile);
      show_compcall (c->expr);
      break;

    case EXEC_RETURN:
      fputs ("RETURN ", dumpfile);
      if (c->expr)
	show_expr (c->expr);
      break;

    case EXEC_PAUSE:
      fputs ("PAUSE ", dumpfile);

      if (c->expr != NULL)
	show_expr (c->expr);
      else
	fprintf (dumpfile, "%d", c->ext.stop_code);

      break;

    case EXEC_STOP:
      fputs ("STOP ", dumpfile);

      if (c->expr != NULL)
	show_expr (c->expr);
      else
	fprintf (dumpfile, "%d", c->ext.stop_code);

      break;

    case EXEC_ARITHMETIC_IF:
      fputs ("IF ", dumpfile);
      show_expr (c->expr);
      fprintf (dumpfile, " %d, %d, %d",
		  c->label->value, c->label2->value, c->label3->value);
      break;

    case EXEC_IF:
      d = c->block;
      fputs ("IF ", dumpfile);
      show_expr (d->expr);
      fputc ('\n', dumpfile);
      show_code (level + 1, d->next);

      d = d->block;
      for (; d; d = d->block)
	{
	  code_indent (level, 0);

	  if (d->expr == NULL)
	    fputs ("ELSE\n", dumpfile);
	  else
	    {
	      fputs ("ELSE IF ", dumpfile);
	      show_expr (d->expr);
	      fputc ('\n', dumpfile);
	    }

	  show_code (level + 1, d->next);
	}

      code_indent (level, c->label);

      fputs ("ENDIF", dumpfile);
      break;

    case EXEC_SELECT:
      d = c->block;
      fputs ("SELECT CASE ", dumpfile);
      show_expr (c->expr);
      fputc ('\n', dumpfile);

      for (; d; d = d->block)
	{
	  code_indent (level, 0);

	  fputs ("CASE ", dumpfile);
	  for (cp = d->ext.case_list; cp; cp = cp->next)
	    {
	      fputc ('(', dumpfile);
	      show_expr (cp->low);
	      fputc (' ', dumpfile);
	      show_expr (cp->high);
	      fputc (')', dumpfile);
	      fputc (' ', dumpfile);
	    }
	  fputc ('\n', dumpfile);

	  show_code (level + 1, d->next);
	}

      code_indent (level, c->label);
      fputs ("END SELECT", dumpfile);
      break;

    case EXEC_WHERE:
      fputs ("WHERE ", dumpfile);

      d = c->block;
      show_expr (d->expr);
      fputc ('\n', dumpfile);

      show_code (level + 1, d->next);

      for (d = d->block; d; d = d->block)
	{
	  code_indent (level, 0);
	  fputs ("ELSE WHERE ", dumpfile);
	  show_expr (d->expr);
	  fputc ('\n', dumpfile);
	  show_code (level + 1, d->next);
	}

      code_indent (level, 0);
      fputs ("END WHERE", dumpfile);
      break;


    case EXEC_FORALL:
      fputs ("FORALL ", dumpfile);
      for (fa = c->ext.forall_iterator; fa; fa = fa->next)
	{
	  show_expr (fa->var);
	  fputc (' ', dumpfile);
	  show_expr (fa->start);
	  fputc (':', dumpfile);
	  show_expr (fa->end);
	  fputc (':', dumpfile);
	  show_expr (fa->stride);

	  if (fa->next != NULL)
	    fputc (',', dumpfile);
	}

      if (c->expr != NULL)
	{
	  fputc (',', dumpfile);
	  show_expr (c->expr);
	}
      fputc ('\n', dumpfile);

      show_code (level + 1, c->block->next);

      code_indent (level, 0);
      fputs ("END FORALL", dumpfile);
      break;

    case EXEC_DO:
      fputs ("DO ", dumpfile);

      show_expr (c->ext.iterator->var);
      fputc ('=', dumpfile);
      show_expr (c->ext.iterator->start);
      fputc (' ', dumpfile);
      show_expr (c->ext.iterator->end);
      fputc (' ', dumpfile);
      show_expr (c->ext.iterator->step);
      fputc ('\n', dumpfile);

      show_code (level + 1, c->block->next);

      code_indent (level, 0);
      fputs ("END DO", dumpfile);
      break;

    case EXEC_DO_WHILE:
      fputs ("DO WHILE ", dumpfile);
      show_expr (c->expr);
      fputc ('\n', dumpfile);

      show_code (level + 1, c->block->next);

      code_indent (level, c->label);
      fputs ("END DO", dumpfile);
      break;

    case EXEC_CYCLE:
      fputs ("CYCLE", dumpfile);
      if (c->symtree)
	fprintf (dumpfile, " %s", c->symtree->n.sym->name);
      break;

    case EXEC_EXIT:
      fputs ("EXIT", dumpfile);
      if (c->symtree)
	fprintf (dumpfile, " %s", c->symtree->n.sym->name);
      break;

    case EXEC_ALLOCATE:
      fputs ("ALLOCATE ", dumpfile);
      if (c->expr)
	{
	  fputs (" STAT=", dumpfile);
	  show_expr (c->expr);
	}

      for (a = c->ext.alloc_list; a; a = a->next)
	{
	  fputc (' ', dumpfile);
	  show_expr (a->expr);
	}

      break;

    case EXEC_DEALLOCATE:
      fputs ("DEALLOCATE ", dumpfile);
      if (c->expr)
	{
	  fputs (" STAT=", dumpfile);
	  show_expr (c->expr);
	}

      for (a = c->ext.alloc_list; a; a = a->next)
	{
	  fputc (' ', dumpfile);
	  show_expr (a->expr);
	}

      break;

    case EXEC_OPEN:
      fputs ("OPEN", dumpfile);
      open = c->ext.open;

      if (open->unit)
	{
	  fputs (" UNIT=", dumpfile);
	  show_expr (open->unit);
	}
      if (open->iomsg)
	{
	  fputs (" IOMSG=", dumpfile);
	  show_expr (open->iomsg);
	}
      if (open->iostat)
	{
	  fputs (" IOSTAT=", dumpfile);
	  show_expr (open->iostat);
	}
      if (open->file)
	{
	  fputs (" FILE=", dumpfile);
	  show_expr (open->file);
	}
      if (open->status)
	{
	  fputs (" STATUS=", dumpfile);
	  show_expr (open->status);
	}
      if (open->access)
	{
	  fputs (" ACCESS=", dumpfile);
	  show_expr (open->access);
	}
      if (open->form)
	{
	  fputs (" FORM=", dumpfile);
	  show_expr (open->form);
	}
      if (open->recl)
	{
	  fputs (" RECL=", dumpfile);
	  show_expr (open->recl);
	}
      if (open->blank)
	{
	  fputs (" BLANK=", dumpfile);
	  show_expr (open->blank);
	}
      if (open->position)
	{
	  fputs (" POSITION=", dumpfile);
	  show_expr (open->position);
	}
      if (open->action)
	{
	  fputs (" ACTION=", dumpfile);
	  show_expr (open->action);
	}
      if (open->delim)
	{
	  fputs (" DELIM=", dumpfile);
	  show_expr (open->delim);
	}
      if (open->pad)
	{
	  fputs (" PAD=", dumpfile);
	  show_expr (open->pad);
	}
      if (open->decimal)
	{
	  fputs (" DECIMAL=", dumpfile);
	  show_expr (open->decimal);
	}
      if (open->encoding)
	{
	  fputs (" ENCODING=", dumpfile);
	  show_expr (open->encoding);
	}
      if (open->round)
	{
	  fputs (" ROUND=", dumpfile);
	  show_expr (open->round);
	}
      if (open->sign)
	{
	  fputs (" SIGN=", dumpfile);
	  show_expr (open->sign);
	}
      if (open->convert)
	{
	  fputs (" CONVERT=", dumpfile);
	  show_expr (open->convert);
	}
      if (open->asynchronous)
	{
	  fputs (" ASYNCHRONOUS=", dumpfile);
	  show_expr (open->asynchronous);
	}
      if (open->err != NULL)
	fprintf (dumpfile, " ERR=%d", open->err->value);

      break;

    case EXEC_CLOSE:
      fputs ("CLOSE", dumpfile);
      close = c->ext.close;

      if (close->unit)
	{
	  fputs (" UNIT=", dumpfile);
	  show_expr (close->unit);
	}
      if (close->iomsg)
	{
	  fputs (" IOMSG=", dumpfile);
	  show_expr (close->iomsg);
	}
      if (close->iostat)
	{
	  fputs (" IOSTAT=", dumpfile);
	  show_expr (close->iostat);
	}
      if (close->status)
	{
	  fputs (" STATUS=", dumpfile);
	  show_expr (close->status);
	}
      if (close->err != NULL)
	fprintf (dumpfile, " ERR=%d", close->err->value);
      break;

    case EXEC_BACKSPACE:
      fputs ("BACKSPACE", dumpfile);
      goto show_filepos;

    case EXEC_ENDFILE:
      fputs ("ENDFILE", dumpfile);
      goto show_filepos;

    case EXEC_REWIND:
      fputs ("REWIND", dumpfile);
      goto show_filepos;

    case EXEC_FLUSH:
      fputs ("FLUSH", dumpfile);

    show_filepos:
      fp = c->ext.filepos;

      if (fp->unit)
	{
	  fputs (" UNIT=", dumpfile);
	  show_expr (fp->unit);
	}
      if (fp->iomsg)
	{
	  fputs (" IOMSG=", dumpfile);
	  show_expr (fp->iomsg);
	}
      if (fp->iostat)
	{
	  fputs (" IOSTAT=", dumpfile);
	  show_expr (fp->iostat);
	}
      if (fp->err != NULL)
	fprintf (dumpfile, " ERR=%d", fp->err->value);
      break;

    case EXEC_INQUIRE:
      fputs ("INQUIRE", dumpfile);
      i = c->ext.inquire;

      if (i->unit)
	{
	  fputs (" UNIT=", dumpfile);
	  show_expr (i->unit);
	}
      if (i->file)
	{
	  fputs (" FILE=", dumpfile);
	  show_expr (i->file);
	}

      if (i->iomsg)
	{
	  fputs (" IOMSG=", dumpfile);
	  show_expr (i->iomsg);
	}
      if (i->iostat)
	{
	  fputs (" IOSTAT=", dumpfile);
	  show_expr (i->iostat);
	}
      if (i->exist)
	{
	  fputs (" EXIST=", dumpfile);
	  show_expr (i->exist);
	}
      if (i->opened)
	{
	  fputs (" OPENED=", dumpfile);
	  show_expr (i->opened);
	}
      if (i->number)
	{
	  fputs (" NUMBER=", dumpfile);
	  show_expr (i->number);
	}
      if (i->named)
	{
	  fputs (" NAMED=", dumpfile);
	  show_expr (i->named);
	}
      if (i->name)
	{
	  fputs (" NAME=", dumpfile);
	  show_expr (i->name);
	}
      if (i->access)
	{
	  fputs (" ACCESS=", dumpfile);
	  show_expr (i->access);
	}
      if (i->sequential)
	{
	  fputs (" SEQUENTIAL=", dumpfile);
	  show_expr (i->sequential);
	}

      if (i->direct)
	{
	  fputs (" DIRECT=", dumpfile);
	  show_expr (i->direct);
	}
      if (i->form)
	{
	  fputs (" FORM=", dumpfile);
	  show_expr (i->form);
	}
      if (i->formatted)
	{
	  fputs (" FORMATTED", dumpfile);
	  show_expr (i->formatted);
	}
      if (i->unformatted)
	{
	  fputs (" UNFORMATTED=", dumpfile);
	  show_expr (i->unformatted);
	}
      if (i->recl)
	{
	  fputs (" RECL=", dumpfile);
	  show_expr (i->recl);
	}
      if (i->nextrec)
	{
	  fputs (" NEXTREC=", dumpfile);
	  show_expr (i->nextrec);
	}
      if (i->blank)
	{
	  fputs (" BLANK=", dumpfile);
	  show_expr (i->blank);
	}
      if (i->position)
	{
	  fputs (" POSITION=", dumpfile);
	  show_expr (i->position);
	}
      if (i->action)
	{
	  fputs (" ACTION=", dumpfile);
	  show_expr (i->action);
	}
      if (i->read)
	{
	  fputs (" READ=", dumpfile);
	  show_expr (i->read);
	}
      if (i->write)
	{
	  fputs (" WRITE=", dumpfile);
	  show_expr (i->write);
	}
      if (i->readwrite)
	{
	  fputs (" READWRITE=", dumpfile);
	  show_expr (i->readwrite);
	}
      if (i->delim)
	{
	  fputs (" DELIM=", dumpfile);
	  show_expr (i->delim);
	}
      if (i->pad)
	{
	  fputs (" PAD=", dumpfile);
	  show_expr (i->pad);
	}
      if (i->convert)
	{
	  fputs (" CONVERT=", dumpfile);
	  show_expr (i->convert);
	}
      if (i->asynchronous)
	{
	  fputs (" ASYNCHRONOUS=", dumpfile);
	  show_expr (i->asynchronous);
	}
      if (i->decimal)
	{
	  fputs (" DECIMAL=", dumpfile);
	  show_expr (i->decimal);
	}
      if (i->encoding)
	{
	  fputs (" ENCODING=", dumpfile);
	  show_expr (i->encoding);
	}
      if (i->pending)
	{
	  fputs (" PENDING=", dumpfile);
	  show_expr (i->pending);
	}
      if (i->round)
	{
	  fputs (" ROUND=", dumpfile);
	  show_expr (i->round);
	}
      if (i->sign)
	{
	  fputs (" SIGN=", dumpfile);
	  show_expr (i->sign);
	}
      if (i->size)
	{
	  fputs (" SIZE=", dumpfile);
	  show_expr (i->size);
	}
      if (i->id)
	{
	  fputs (" ID=", dumpfile);
	  show_expr (i->id);
	}

      if (i->err != NULL)
	fprintf (dumpfile, " ERR=%d", i->err->value);
      break;

    case EXEC_IOLENGTH:
      fputs ("IOLENGTH ", dumpfile);
      show_expr (c->expr);
      goto show_dt_code;
      break;

    case EXEC_READ:
      fputs ("READ", dumpfile);
      goto show_dt;

    case EXEC_WRITE:
      fputs ("WRITE", dumpfile);

    show_dt:
      dt = c->ext.dt;
      if (dt->io_unit)
	{
	  fputs (" UNIT=", dumpfile);
	  show_expr (dt->io_unit);
	}

      if (dt->format_expr)
	{
	  fputs (" FMT=", dumpfile);
	  show_expr (dt->format_expr);
	}

      if (dt->format_label != NULL)
	fprintf (dumpfile, " FMT=%d", dt->format_label->value);
      if (dt->namelist)
	fprintf (dumpfile, " NML=%s", dt->namelist->name);

      if (dt->iomsg)
	{
	  fputs (" IOMSG=", dumpfile);
	  show_expr (dt->iomsg);
	}
      if (dt->iostat)
	{
	  fputs (" IOSTAT=", dumpfile);
	  show_expr (dt->iostat);
	}
      if (dt->size)
	{
	  fputs (" SIZE=", dumpfile);
	  show_expr (dt->size);
	}
      if (dt->rec)
	{
	  fputs (" REC=", dumpfile);
	  show_expr (dt->rec);
	}
      if (dt->advance)
	{
	  fputs (" ADVANCE=", dumpfile);
	  show_expr (dt->advance);
	}
      if (dt->id)
	{
	  fputs (" ID=", dumpfile);
	  show_expr (dt->id);
	}
      if (dt->pos)
	{
	  fputs (" POS=", dumpfile);
	  show_expr (dt->pos);
	}
      if (dt->asynchronous)
	{
	  fputs (" ASYNCHRONOUS=", dumpfile);
	  show_expr (dt->asynchronous);
	}
      if (dt->blank)
	{
	  fputs (" BLANK=", dumpfile);
	  show_expr (dt->blank);
	}
      if (dt->decimal)
	{
	  fputs (" DECIMAL=", dumpfile);
	  show_expr (dt->decimal);
	}
      if (dt->delim)
	{
	  fputs (" DELIM=", dumpfile);
	  show_expr (dt->delim);
	}
      if (dt->pad)
	{
	  fputs (" PAD=", dumpfile);
	  show_expr (dt->pad);
	}
      if (dt->round)
	{
	  fputs (" ROUND=", dumpfile);
	  show_expr (dt->round);
	}
      if (dt->sign)
	{
	  fputs (" SIGN=", dumpfile);
	  show_expr (dt->sign);
	}

    show_dt_code:
      fputc ('\n', dumpfile);
      for (c = c->block->next; c; c = c->next)
	show_code_node (level + (c->next != NULL), c);
      return;

    case EXEC_TRANSFER:
      fputs ("TRANSFER ", dumpfile);
      show_expr (c->expr);
      break;

    case EXEC_DT_END:
      fputs ("DT_END", dumpfile);
      dt = c->ext.dt;

      if (dt->err != NULL)
	fprintf (dumpfile, " ERR=%d", dt->err->value);
      if (dt->end != NULL)
	fprintf (dumpfile, " END=%d", dt->end->value);
      if (dt->eor != NULL)
	fprintf (dumpfile, " EOR=%d", dt->eor->value);
      break;

    case EXEC_OMP_ATOMIC:
    case EXEC_OMP_BARRIER:
    case EXEC_OMP_CRITICAL:
    case EXEC_OMP_FLUSH:
    case EXEC_OMP_DO:
    case EXEC_OMP_MASTER:
    case EXEC_OMP_ORDERED:
    case EXEC_OMP_PARALLEL:
    case EXEC_OMP_PARALLEL_DO:
    case EXEC_OMP_PARALLEL_SECTIONS:
    case EXEC_OMP_PARALLEL_WORKSHARE:
    case EXEC_OMP_SECTIONS:
    case EXEC_OMP_SINGLE:
    case EXEC_OMP_TASK:
    case EXEC_OMP_TASKWAIT:
    case EXEC_OMP_WORKSHARE:
      show_omp_node (level, c);
      break;

    default:
      gfc_internal_error ("show_code_node(): Bad statement code");
    }

  fputc ('\n', dumpfile);
}


/* Show an equivalence chain.  */

static void
show_equiv (gfc_equiv *eq)
{
  show_indent ();
  fputs ("Equivalence: ", dumpfile);
  while (eq)
    {
      show_expr (eq->expr);
      eq = eq->eq;
      if (eq)
	fputs (", ", dumpfile);
    }
}


/* Show a freakin' whole namespace.  */

static void
show_namespace (gfc_namespace *ns)
{
  gfc_interface *intr;
  gfc_namespace *save;
  gfc_intrinsic_op op;
  gfc_equiv *eq;
  int i;

  save = gfc_current_ns;
  show_level++;

  show_indent ();
  fputs ("Namespace:", dumpfile);

  if (ns != NULL)
    {
      i = 0;
      do
	{
	  int l = i;
	  while (i < GFC_LETTERS - 1
		 && gfc_compare_types(&ns->default_type[i+1],
				      &ns->default_type[l]))
	    i++;

	  if (i > l)
	    fprintf (dumpfile, " %c-%c: ", l+'A', i+'A');
	  else
	    fprintf (dumpfile, " %c: ", l+'A');

	  show_typespec(&ns->default_type[l]);
	  i++;
      } while (i < GFC_LETTERS);

      if (ns->proc_name != NULL)
	{
	  show_indent ();
	  fprintf (dumpfile, "procedure name = %s", ns->proc_name->name);
	}

      gfc_current_ns = ns;
      gfc_traverse_symtree (ns->common_root, show_common);

      gfc_traverse_symtree (ns->sym_root, show_symtree);

      for (op = GFC_INTRINSIC_BEGIN; op != GFC_INTRINSIC_END; op++)
	{
	  /* User operator interfaces */
	  intr = ns->op[op];
	  if (intr == NULL)
	    continue;

	  show_indent ();
	  fprintf (dumpfile, "Operator interfaces for %s:",
		   gfc_op2string (op));

	  for (; intr; intr = intr->next)
	    fprintf (dumpfile, " %s", intr->sym->name);
	}

      if (ns->uop_root != NULL)
	{
	  show_indent ();
	  fputs ("User operators:\n", dumpfile);
	  gfc_traverse_user_op (ns, show_uop);
	}
    }
  
  for (eq = ns->equiv; eq; eq = eq->next)
    show_equiv (eq);

  fputc ('\n', dumpfile);
  fputc ('\n', dumpfile);

  show_code (0, ns->code);

  for (ns = ns->contained; ns; ns = ns->sibling)
    {
      show_indent ();
      fputs ("CONTAINS\n", dumpfile);
      show_namespace (ns);
    }

  show_level--;
  fputc ('\n', dumpfile);
  gfc_current_ns = save;
}


/* Main function for dumping a parse tree.  */

void
gfc_dump_parse_tree (gfc_namespace *ns, FILE *file)
{
  dumpfile = file;
  show_namespace (ns);
}
