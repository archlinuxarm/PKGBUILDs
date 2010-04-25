/* Transformations based on profile information for values.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Free Software
   Foundation, Inc.

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
#include "rtl.h"
#include "expr.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "value-prof.h"
#include "output.h"
#include "flags.h"
#include "insn-config.h"
#include "recog.h"
#include "optabs.h"
#include "regs.h"
#include "ggc.h"
#include "tree-flow.h"
#include "tree-flow-inline.h"
#include "diagnostic.h"
#include "coverage.h"
#include "tree.h"
#include "gcov-io.h"
#include "cgraph.h"
#include "timevar.h"
#include "tree-pass.h"
#include "toplev.h"
#include "pointer-set.h"

static struct value_prof_hooks *value_prof_hooks;

/* In this file value profile based optimizations are placed.  Currently the
   following optimizations are implemented (for more detailed descriptions
   see comments at value_profile_transformations):

   1) Division/modulo specialization.  Provided that we can determine that the
      operands of the division have some special properties, we may use it to
      produce more effective code.
   2) Speculative prefetching.  If we are able to determine that the difference
      between addresses accessed by a memory reference is usually constant, we
      may add the prefetch instructions.
      FIXME: This transformation was removed together with RTL based value
      profiling.

   3) Indirect/virtual call specialization. If we can determine most
      common function callee in indirect/virtual call. We can use this
      information to improve code effectiveness (especially info for
      inliner).

   Every such optimization should add its requirements for profiled values to
   insn_values_to_profile function.  This function is called from branch_prob
   in profile.c and the requested values are instrumented by it in the first
   compilation with -fprofile-arcs.  The optimization may then read the
   gathered data in the second compilation with -fbranch-probabilities.

   The measured data is pointed to from the histograms
   field of the statement annotation of the instrumented insns.  It is
   kept as a linked list of struct histogram_value_t's, which contain the
   same information as above.  */


static tree gimple_divmod_fixed_value (gimple, tree, int, gcov_type, gcov_type);
static tree gimple_mod_pow2 (gimple, int, gcov_type, gcov_type);
static tree gimple_mod_subtract (gimple, int, int, int, gcov_type, gcov_type,
				 gcov_type);
static bool gimple_divmod_fixed_value_transform (gimple_stmt_iterator *);
static bool gimple_mod_pow2_value_transform (gimple_stmt_iterator *);
static bool gimple_mod_subtract_transform (gimple_stmt_iterator *);
static bool gimple_stringops_transform (gimple_stmt_iterator *);
static bool gimple_ic_transform (gimple);

/* Allocate histogram value.  */

static histogram_value
gimple_alloc_histogram_value (struct function *fun ATTRIBUTE_UNUSED,
			      enum hist_type type, gimple stmt, tree value)
{
   histogram_value hist = (histogram_value) xcalloc (1, sizeof (*hist));
   hist->hvalue.value = value;
   hist->hvalue.stmt = stmt;
   hist->type = type;
   return hist;
}

/* Hash value for histogram.  */

static hashval_t
histogram_hash (const void *x)
{
  return htab_hash_pointer (((const_histogram_value)x)->hvalue.stmt);
}

/* Return nonzero if decl_id of die_struct X is the same as UID of decl *Y.  */

static int
histogram_eq (const void *x, const void *y)
{
  return ((const_histogram_value) x)->hvalue.stmt == (const_gimple) y;
}

/* Set histogram for STMT.  */

static void
set_histogram_value (struct function *fun, gimple stmt, histogram_value hist)
{
  void **loc;
  if (!hist && !VALUE_HISTOGRAMS (fun))
    return;
  if (!VALUE_HISTOGRAMS (fun))
    VALUE_HISTOGRAMS (fun) = htab_create (1, histogram_hash,
				           histogram_eq, NULL);
  loc = htab_find_slot_with_hash (VALUE_HISTOGRAMS (fun), stmt,
                                  htab_hash_pointer (stmt),
				  hist ? INSERT : NO_INSERT);
  if (!hist)
    {
      if (loc)
	htab_clear_slot (VALUE_HISTOGRAMS (fun), loc);
      return;
    }
  *loc = hist;
}

/* Get histogram list for STMT.  */

histogram_value
gimple_histogram_value (struct function *fun, gimple stmt)
{
  if (!VALUE_HISTOGRAMS (fun))
    return NULL;
  return (histogram_value) htab_find_with_hash (VALUE_HISTOGRAMS (fun), stmt,
						htab_hash_pointer (stmt));
}

/* Add histogram for STMT.  */

void
gimple_add_histogram_value (struct function *fun, gimple stmt,
			    histogram_value hist)
{
  hist->hvalue.next = gimple_histogram_value (fun, stmt);
  set_histogram_value (fun, stmt, hist);
}


/* Remove histogram HIST from STMT's histogram list.  */

void
gimple_remove_histogram_value (struct function *fun, gimple stmt,
			       histogram_value hist)
{
  histogram_value hist2 = gimple_histogram_value (fun, stmt);
  if (hist == hist2)
    {
      set_histogram_value (fun, stmt, hist->hvalue.next);
    }
  else
    {
      while (hist2->hvalue.next != hist)
	hist2 = hist2->hvalue.next;
      hist2->hvalue.next = hist->hvalue.next;
    }
  free (hist->hvalue.counters);
#ifdef ENABLE_CHECKING
  memset (hist, 0xab, sizeof (*hist));
#endif
  free (hist);
}


/* Lookup histogram of type TYPE in the STMT.  */

histogram_value
gimple_histogram_value_of_type (struct function *fun, gimple stmt,
				enum hist_type type)
{
  histogram_value hist;
  for (hist = gimple_histogram_value (fun, stmt); hist;
       hist = hist->hvalue.next)
    if (hist->type == type)
      return hist;
  return NULL;
}

/* Dump information about HIST to DUMP_FILE.  */

static void
dump_histogram_value (FILE *dump_file, histogram_value hist)
{
  switch (hist->type)
    {
    case HIST_TYPE_INTERVAL:
      fprintf (dump_file, "Interval counter range %d -- %d",
	       hist->hdata.intvl.int_start,
	       (hist->hdata.intvl.int_start
	        + hist->hdata.intvl.steps - 1));
      if (hist->hvalue.counters)
	{
	   unsigned int i;
	   fprintf(dump_file, " [");
           for (i = 0; i < hist->hdata.intvl.steps; i++)
	     fprintf (dump_file, " %d:"HOST_WIDEST_INT_PRINT_DEC,
		      hist->hdata.intvl.int_start + i,
		      (HOST_WIDEST_INT) hist->hvalue.counters[i]);
	   fprintf (dump_file, " ] outside range:"HOST_WIDEST_INT_PRINT_DEC,
		    (HOST_WIDEST_INT) hist->hvalue.counters[i]);
	}
      fprintf (dump_file, ".\n");
      break;

    case HIST_TYPE_POW2:
      fprintf (dump_file, "Pow2 counter ");
      if (hist->hvalue.counters)
	{
	   fprintf (dump_file, "pow2:"HOST_WIDEST_INT_PRINT_DEC
		    " nonpow2:"HOST_WIDEST_INT_PRINT_DEC,
		    (HOST_WIDEST_INT) hist->hvalue.counters[0],
		    (HOST_WIDEST_INT) hist->hvalue.counters[1]);
	}
      fprintf (dump_file, ".\n");
      break;

    case HIST_TYPE_SINGLE_VALUE:
      fprintf (dump_file, "Single value ");
      if (hist->hvalue.counters)
	{
	   fprintf (dump_file, "value:"HOST_WIDEST_INT_PRINT_DEC
		    " match:"HOST_WIDEST_INT_PRINT_DEC
		    " wrong:"HOST_WIDEST_INT_PRINT_DEC,
		    (HOST_WIDEST_INT) hist->hvalue.counters[0],
		    (HOST_WIDEST_INT) hist->hvalue.counters[1],
		    (HOST_WIDEST_INT) hist->hvalue.counters[2]);
	}
      fprintf (dump_file, ".\n");
      break;

    case HIST_TYPE_AVERAGE:
      fprintf (dump_file, "Average value ");
      if (hist->hvalue.counters)
	{
	   fprintf (dump_file, "sum:"HOST_WIDEST_INT_PRINT_DEC
		    " times:"HOST_WIDEST_INT_PRINT_DEC,
		    (HOST_WIDEST_INT) hist->hvalue.counters[0],
		    (HOST_WIDEST_INT) hist->hvalue.counters[1]);
	}
      fprintf (dump_file, ".\n");
      break;

    case HIST_TYPE_IOR:
      fprintf (dump_file, "IOR value ");
      if (hist->hvalue.counters)
	{
	   fprintf (dump_file, "ior:"HOST_WIDEST_INT_PRINT_DEC,
		    (HOST_WIDEST_INT) hist->hvalue.counters[0]);
	}
      fprintf (dump_file, ".\n");
      break;

    case HIST_TYPE_CONST_DELTA:
      fprintf (dump_file, "Constant delta ");
      if (hist->hvalue.counters)
	{
	   fprintf (dump_file, "value:"HOST_WIDEST_INT_PRINT_DEC
		    " match:"HOST_WIDEST_INT_PRINT_DEC
		    " wrong:"HOST_WIDEST_INT_PRINT_DEC,
		    (HOST_WIDEST_INT) hist->hvalue.counters[0],
		    (HOST_WIDEST_INT) hist->hvalue.counters[1],
		    (HOST_WIDEST_INT) hist->hvalue.counters[2]);
	}
      fprintf (dump_file, ".\n");
      break;
    case HIST_TYPE_INDIR_CALL:
      fprintf (dump_file, "Indirect call ");
      if (hist->hvalue.counters)
	{
	   fprintf (dump_file, "value:"HOST_WIDEST_INT_PRINT_DEC
		    " match:"HOST_WIDEST_INT_PRINT_DEC
		    " all:"HOST_WIDEST_INT_PRINT_DEC,
		    (HOST_WIDEST_INT) hist->hvalue.counters[0],
		    (HOST_WIDEST_INT) hist->hvalue.counters[1],
		    (HOST_WIDEST_INT) hist->hvalue.counters[2]);
	}
      fprintf (dump_file, ".\n");
      break;
   }
}

/* Dump all histograms attached to STMT to DUMP_FILE.  */

void
dump_histograms_for_stmt (struct function *fun, FILE *dump_file, gimple stmt)
{
  histogram_value hist;
  for (hist = gimple_histogram_value (fun, stmt); hist; hist = hist->hvalue.next)
   dump_histogram_value (dump_file, hist);
}

/* Remove all histograms associated with STMT.  */

void
gimple_remove_stmt_histograms (struct function *fun, gimple stmt)
{
  histogram_value val;
  while ((val = gimple_histogram_value (fun, stmt)) != NULL)
    gimple_remove_histogram_value (fun, stmt, val);
}

/* Duplicate all histograms associates with OSTMT to STMT.  */

void
gimple_duplicate_stmt_histograms (struct function *fun, gimple stmt,
				  struct function *ofun, gimple ostmt)
{
  histogram_value val;
  for (val = gimple_histogram_value (ofun, ostmt); val != NULL; val = val->hvalue.next)
    {
      histogram_value new_val = gimple_alloc_histogram_value (fun, val->type, NULL, NULL);
      memcpy (new_val, val, sizeof (*val));
      new_val->hvalue.stmt = stmt;
      new_val->hvalue.counters = XNEWVAR (gcov_type, sizeof (*new_val->hvalue.counters) * new_val->n_counters);
      memcpy (new_val->hvalue.counters, val->hvalue.counters, sizeof (*new_val->hvalue.counters) * new_val->n_counters);
      gimple_add_histogram_value (fun, stmt, new_val);
    }
}


/* Move all histograms associated with OSTMT to STMT.  */

void
gimple_move_stmt_histograms (struct function *fun, gimple stmt, gimple ostmt)
{
  histogram_value val = gimple_histogram_value (fun, ostmt);
  if (val)
    {
      /* The following three statements can't be reordered,
         because histogram hashtab relies on stmt field value
	 for finding the exact slot. */
      set_histogram_value (fun, ostmt, NULL);
      for (; val != NULL; val = val->hvalue.next)
	val->hvalue.stmt = stmt;
      set_histogram_value (fun, stmt, val);
    }
}

static bool error_found = false;

/* Helper function for verify_histograms.  For each histogram reachable via htab
   walk verify that it was reached via statement walk.  */

static int
visit_hist (void **slot, void *data)
{
  struct pointer_set_t *visited = (struct pointer_set_t *) data;
  histogram_value hist = *(histogram_value *) slot;
  if (!pointer_set_contains (visited, hist))
    {
      error ("Dead histogram");
      dump_histogram_value (stderr, hist);
      debug_gimple_stmt (hist->hvalue.stmt);
      error_found = true;
    }
  return 1;
}


/* Verify sanity of the histograms.  */

void
verify_histograms (void)
{
  basic_block bb;
  gimple_stmt_iterator gsi;
  histogram_value hist;
  struct pointer_set_t *visited_hists;

  error_found = false;
  visited_hists = pointer_set_create ();
  FOR_EACH_BB (bb)
    for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
      {
	gimple stmt = gsi_stmt (gsi);

	for (hist = gimple_histogram_value (cfun, stmt); hist;
	     hist = hist->hvalue.next)
	  {
	    if (hist->hvalue.stmt != stmt)
	      {
		error ("Histogram value statement does not correspond to "
		       "the statement it is associated with");
		debug_gimple_stmt (stmt);
		dump_histogram_value (stderr, hist);
		error_found = true;
	      }
            pointer_set_insert (visited_hists, hist);
	  }
      }
  if (VALUE_HISTOGRAMS (cfun))
    htab_traverse (VALUE_HISTOGRAMS (cfun), visit_hist, visited_hists);
  pointer_set_destroy (visited_hists);
  if (error_found)
    internal_error ("verify_histograms failed");
}

/* Helper function for verify_histograms.  For each histogram reachable via htab
   walk verify that it was reached via statement walk.  */

static int
free_hist (void **slot, void *data ATTRIBUTE_UNUSED)
{
  histogram_value hist = *(histogram_value *) slot;
  free (hist->hvalue.counters);
#ifdef ENABLE_CHECKING
  memset (hist, 0xab, sizeof (*hist));
#endif
  free (hist);
  return 1;
}

void
free_histograms (void)
{
  if (VALUE_HISTOGRAMS (cfun))
    {
      htab_traverse (VALUE_HISTOGRAMS (cfun), free_hist, NULL);
      htab_delete (VALUE_HISTOGRAMS (cfun));
      VALUE_HISTOGRAMS (cfun) = NULL;
    }
}


/* The overall number of invocations of the counter should match
   execution count of basic block.  Report it as error rather than
   internal error as it might mean that user has misused the profile
   somehow.  */

static bool
check_counter (gimple stmt, const char * name,
	       gcov_type *count, gcov_type *all, gcov_type bb_count)
{
  if (*all != bb_count || *count > *all)
    {
      location_t locus;
      locus = (stmt != NULL)
              ? gimple_location (stmt)
              : DECL_SOURCE_LOCATION (current_function_decl);
      if (flag_profile_correction)
        {
	  inform (locus, "Correcting inconsistent value profile: "
		  "%s profiler overall count (%d) does not match BB count "
                  "(%d)", name, (int)*all, (int)bb_count);
	  *all = bb_count;
	  if (*count > *all)
            *count = *all;
	  return false;
	}
      else
	{
	  error ("%HCorrupted value profile: %s profiler overall count (%d) "
                 "does not match BB count (%d)", &locus, name, (int)*all,
                 (int)bb_count);
	  return true;
	}
    }

  return false;
}


/* GIMPLE based transformations. */

static bool
gimple_value_profile_transformations (void)
{
  basic_block bb;
  gimple_stmt_iterator gsi;
  bool changed = false;

  FOR_EACH_BB (bb)
    {
      for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	{
	  gimple stmt = gsi_stmt (gsi);
	  histogram_value th = gimple_histogram_value (cfun, stmt);
	  if (!th)
	    continue;

	  if (dump_file)
	    {
	      fprintf (dump_file, "Trying transformations on stmt ");
	      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
	      dump_histograms_for_stmt (cfun, dump_file, stmt);
	    }

	  /* Transformations:  */
	  /* The order of things in this conditional controls which
	     transformation is used when more than one is applicable.  */
	  /* It is expected that any code added by the transformations
	     will be added before the current statement, and that the
	     current statement remain valid (although possibly
	     modified) upon return.  */
	  if (flag_value_profile_transformations
	      && (gimple_mod_subtract_transform (&gsi)
		  || gimple_divmod_fixed_value_transform (&gsi)
		  || gimple_mod_pow2_value_transform (&gsi)
		  || gimple_stringops_transform (&gsi)
		  || gimple_ic_transform (stmt)))
	    {
	      stmt = gsi_stmt (gsi);
	      changed = true;
	      /* Original statement may no longer be in the same block. */
	      if (bb != gimple_bb (stmt))
		{
	          bb = gimple_bb (stmt);
		  gsi = gsi_for_stmt (stmt);
		}
	    }
        }
    }

  if (changed)
    {
      counts_to_freqs ();
    }

  return changed;
}


/* Generate code for transformation 1 (with parent gimple assignment
   STMT and probability of taking the optimal path PROB, which is
   equivalent to COUNT/ALL within roundoff error).  This generates the
   result into a temp and returns the temp; it does not replace or
   alter the original STMT.  */

static tree
gimple_divmod_fixed_value (gimple stmt, tree value, int prob, gcov_type count,
			   gcov_type all)
{
  gimple stmt1, stmt2, stmt3;
  tree tmp1, tmp2, tmpv;
  gimple bb1end, bb2end, bb3end;
  basic_block bb, bb2, bb3, bb4;
  tree optype, op1, op2;
  edge e12, e13, e23, e24, e34;
  gimple_stmt_iterator gsi;

  gcc_assert (is_gimple_assign (stmt)
	      && (gimple_assign_rhs_code (stmt) == TRUNC_DIV_EXPR
		  || gimple_assign_rhs_code (stmt) == TRUNC_MOD_EXPR));

  optype = TREE_TYPE (gimple_assign_lhs (stmt));
  op1 = gimple_assign_rhs1 (stmt);
  op2 = gimple_assign_rhs2 (stmt);

  bb = gimple_bb (stmt);
  gsi = gsi_for_stmt (stmt);

  tmpv = create_tmp_var (optype, "PROF");
  tmp1 = create_tmp_var (optype, "PROF");
  stmt1 = gimple_build_assign (tmpv, fold_convert (optype, value));
  stmt2 = gimple_build_assign (tmp1, op2);
  stmt3 = gimple_build_cond (NE_EXPR, tmp1, tmpv, NULL_TREE, NULL_TREE);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt2, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt3, GSI_SAME_STMT);
  bb1end = stmt3;

  tmp2 = create_tmp_var (optype, "PROF");
  stmt1 = gimple_build_assign_with_ops (gimple_assign_rhs_code (stmt), tmp2,
					op1, tmpv);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  bb2end = stmt1;

  stmt1 = gimple_build_assign_with_ops (gimple_assign_rhs_code (stmt), tmp2,
					op1, op2);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  bb3end = stmt1;

  /* Fix CFG. */
  /* Edge e23 connects bb2 to bb3, etc. */
  e12 = split_block (bb, bb1end);
  bb2 = e12->dest;
  bb2->count = count;
  e23 = split_block (bb2, bb2end);
  bb3 = e23->dest;
  bb3->count = all - count;
  e34 = split_block (bb3, bb3end);
  bb4 = e34->dest;
  bb4->count = all;

  e12->flags &= ~EDGE_FALLTHRU;
  e12->flags |= EDGE_FALSE_VALUE;
  e12->probability = prob;
  e12->count = count;

  e13 = make_edge (bb, bb3, EDGE_TRUE_VALUE);
  e13->probability = REG_BR_PROB_BASE - prob;
  e13->count = all - count;

  remove_edge (e23);
  
  e24 = make_edge (bb2, bb4, EDGE_FALLTHRU);
  e24->probability = REG_BR_PROB_BASE;
  e24->count = count;

  e34->probability = REG_BR_PROB_BASE;
  e34->count = all - count;

  return tmp2;
}


/* Do transform 1) on INSN if applicable.  */

static bool
gimple_divmod_fixed_value_transform (gimple_stmt_iterator *si)
{
  histogram_value histogram;
  enum tree_code code;
  gcov_type val, count, all;
  tree result, value, tree_val;
  gcov_type prob;
  gimple stmt;

  stmt = gsi_stmt (*si);
  if (gimple_code (stmt) != GIMPLE_ASSIGN)
    return false;

  if (!INTEGRAL_TYPE_P (TREE_TYPE (gimple_assign_lhs (stmt))))
    return false;

  code = gimple_assign_rhs_code (stmt);
  
  if (code != TRUNC_DIV_EXPR && code != TRUNC_MOD_EXPR)
    return false;

  histogram = gimple_histogram_value_of_type (cfun, stmt,
					      HIST_TYPE_SINGLE_VALUE);
  if (!histogram)
    return false;

  value = histogram->hvalue.value;
  val = histogram->hvalue.counters[0];
  count = histogram->hvalue.counters[1];
  all = histogram->hvalue.counters[2];
  gimple_remove_histogram_value (cfun, stmt, histogram);

  /* We require that count is at least half of all; this means
     that for the transformation to fire the value must be constant
     at least 50% of time (and 75% gives the guarantee of usage).  */
  if (simple_cst_equal (gimple_assign_rhs2 (stmt), value) != 1
      || 2 * count < all
      || optimize_bb_for_size_p (gimple_bb (stmt)))
    return false;

  if (check_counter (stmt, "value", &count, &all, gimple_bb (stmt)->count))
    return false;

  /* Compute probability of taking the optimal path.  */
  if (all > 0)
    prob = (count * REG_BR_PROB_BASE + all / 2) / all;
  else
    prob = 0;

  tree_val = build_int_cst_wide (get_gcov_type (),
				 (unsigned HOST_WIDE_INT) val,
				 val >> (HOST_BITS_PER_WIDE_INT - 1) >> 1);
  result = gimple_divmod_fixed_value (stmt, tree_val, prob, count, all);

  if (dump_file)
    {
      fprintf (dump_file, "Div/mod by constant ");
      print_generic_expr (dump_file, value, TDF_SLIM);
      fprintf (dump_file, "=");
      print_generic_expr (dump_file, tree_val, TDF_SLIM);
      fprintf (dump_file, " transformation on insn ");
      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
    }

  gimple_assign_set_rhs_from_tree (si, result);

  return true;
}

/* Generate code for transformation 2 (with parent gimple assign STMT and
   probability of taking the optimal path PROB, which is equivalent to COUNT/ALL
   within roundoff error).  This generates the result into a temp and returns 
   the temp; it does not replace or alter the original STMT.  */
static tree
gimple_mod_pow2 (gimple stmt, int prob, gcov_type count, gcov_type all)
{
  gimple stmt1, stmt2, stmt3, stmt4;
  tree tmp2, tmp3;
  gimple bb1end, bb2end, bb3end;
  basic_block bb, bb2, bb3, bb4;
  tree optype, op1, op2;
  edge e12, e13, e23, e24, e34;
  gimple_stmt_iterator gsi;
  tree result;

  gcc_assert (is_gimple_assign (stmt)
	      && gimple_assign_rhs_code (stmt) == TRUNC_MOD_EXPR);

  optype = TREE_TYPE (gimple_assign_lhs (stmt));
  op1 = gimple_assign_rhs1 (stmt);
  op2 = gimple_assign_rhs2 (stmt);

  bb = gimple_bb (stmt);
  gsi = gsi_for_stmt (stmt);

  result = create_tmp_var (optype, "PROF");
  tmp2 = create_tmp_var (optype, "PROF");
  tmp3 = create_tmp_var (optype, "PROF");
  stmt2 = gimple_build_assign_with_ops (PLUS_EXPR, tmp2, op2,
					build_int_cst (optype, -1));
  stmt3 = gimple_build_assign_with_ops (BIT_AND_EXPR, tmp3, tmp2, op2);
  stmt4 = gimple_build_cond (NE_EXPR, tmp3, build_int_cst (optype, 0),
			     NULL_TREE, NULL_TREE);
  gsi_insert_before (&gsi, stmt2, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt3, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt4, GSI_SAME_STMT);
  bb1end = stmt4;

  /* tmp2 == op2-1 inherited from previous block.  */
  stmt1 = gimple_build_assign_with_ops (BIT_AND_EXPR, result, op1, tmp2);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  bb2end = stmt1;

  stmt1 = gimple_build_assign_with_ops (gimple_assign_rhs_code (stmt), result,
					op1, op2);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  bb3end = stmt1;

  /* Fix CFG. */
  /* Edge e23 connects bb2 to bb3, etc. */
  e12 = split_block (bb, bb1end);
  bb2 = e12->dest;
  bb2->count = count;
  e23 = split_block (bb2, bb2end);
  bb3 = e23->dest;
  bb3->count = all - count;
  e34 = split_block (bb3, bb3end);
  bb4 = e34->dest;
  bb4->count = all;

  e12->flags &= ~EDGE_FALLTHRU;
  e12->flags |= EDGE_FALSE_VALUE;
  e12->probability = prob;
  e12->count = count;

  e13 = make_edge (bb, bb3, EDGE_TRUE_VALUE);
  e13->probability = REG_BR_PROB_BASE - prob;
  e13->count = all - count;

  remove_edge (e23);
  
  e24 = make_edge (bb2, bb4, EDGE_FALLTHRU);
  e24->probability = REG_BR_PROB_BASE;
  e24->count = count;

  e34->probability = REG_BR_PROB_BASE;
  e34->count = all - count;

  return result;
}

/* Do transform 2) on INSN if applicable.  */
static bool
gimple_mod_pow2_value_transform (gimple_stmt_iterator *si)
{
  histogram_value histogram;
  enum tree_code code;
  gcov_type count, wrong_values, all;
  tree lhs_type, result, value;
  gcov_type prob;
  gimple stmt;

  stmt = gsi_stmt (*si);
  if (gimple_code (stmt) != GIMPLE_ASSIGN)
    return false;

  lhs_type = TREE_TYPE (gimple_assign_lhs (stmt));
  if (!INTEGRAL_TYPE_P (lhs_type))
    return false;

  code = gimple_assign_rhs_code (stmt);
  
  if (code != TRUNC_MOD_EXPR || !TYPE_UNSIGNED (lhs_type))
    return false;

  histogram = gimple_histogram_value_of_type (cfun, stmt, HIST_TYPE_POW2);
  if (!histogram)
    return false;

  value = histogram->hvalue.value;
  wrong_values = histogram->hvalue.counters[0];
  count = histogram->hvalue.counters[1];

  gimple_remove_histogram_value (cfun, stmt, histogram);

  /* We require that we hit a power of 2 at least half of all evaluations.  */
  if (simple_cst_equal (gimple_assign_rhs2 (stmt), value) != 1
      || count < wrong_values
      || optimize_bb_for_size_p (gimple_bb (stmt)))
    return false;

  if (dump_file)
    {
      fprintf (dump_file, "Mod power of 2 transformation on insn ");
      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
    }

  /* Compute probability of taking the optimal path.  */
  all = count + wrong_values;

  if (check_counter (stmt, "pow2", &count, &all, gimple_bb (stmt)->count))
    return false;

  if (all > 0)
    prob = (count * REG_BR_PROB_BASE + all / 2) / all;
  else
    prob = 0;

  result = gimple_mod_pow2 (stmt, prob, count, all);

  gimple_assign_set_rhs_from_tree (si, result);

  return true;
}

/* Generate code for transformations 3 and 4 (with parent gimple assign STMT, and
   NCOUNTS the number of cases to support.  Currently only NCOUNTS==0 or 1 is
   supported and this is built into this interface.  The probabilities of taking
   the optimal paths are PROB1 and PROB2, which are equivalent to COUNT1/ALL and
   COUNT2/ALL respectively within roundoff error).  This generates the 
   result into a temp and returns the temp; it does not replace or alter 
   the original STMT.  */
/* FIXME: Generalize the interface to handle NCOUNTS > 1.  */

static tree
gimple_mod_subtract (gimple stmt, int prob1, int prob2, int ncounts,
		     gcov_type count1, gcov_type count2, gcov_type all)
{
  gimple stmt1, stmt2, stmt3;
  tree tmp1;
  gimple bb1end, bb2end = NULL, bb3end;
  basic_block bb, bb2, bb3, bb4;
  tree optype, op1, op2;
  edge e12, e23 = 0, e24, e34, e14;
  gimple_stmt_iterator gsi;
  tree result;

  gcc_assert (is_gimple_assign (stmt)
	      && gimple_assign_rhs_code (stmt) == TRUNC_MOD_EXPR);

  optype = TREE_TYPE (gimple_assign_lhs (stmt));
  op1 = gimple_assign_rhs1 (stmt);
  op2 = gimple_assign_rhs2 (stmt);

  bb = gimple_bb (stmt);
  gsi = gsi_for_stmt (stmt);

  result = create_tmp_var (optype, "PROF");
  tmp1 = create_tmp_var (optype, "PROF");
  stmt1 = gimple_build_assign (result, op1);
  stmt2 = gimple_build_assign (tmp1, op2);
  stmt3 = gimple_build_cond (LT_EXPR, result, tmp1, NULL_TREE, NULL_TREE);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt2, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt3, GSI_SAME_STMT);
  bb1end = stmt3;

  if (ncounts)	/* Assumed to be 0 or 1 */
    {
      stmt1 = gimple_build_assign_with_ops (MINUS_EXPR, result, result, tmp1);
      stmt2 = gimple_build_cond (LT_EXPR, result, tmp1, NULL_TREE, NULL_TREE);
      gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
      gsi_insert_before (&gsi, stmt2, GSI_SAME_STMT);
      bb2end = stmt2;
    }

  /* Fallback case. */
  stmt1 = gimple_build_assign_with_ops (gimple_assign_rhs_code (stmt), result,
					result, tmp1);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  bb3end = stmt1;

  /* Fix CFG. */
  /* Edge e23 connects bb2 to bb3, etc. */
  /* However block 3 is optional; if it is not there, references
     to 3 really refer to block 2. */
  e12 = split_block (bb, bb1end);
  bb2 = e12->dest;
  bb2->count = all - count1;
    
  if (ncounts)	/* Assumed to be 0 or 1.  */
    {
      e23 = split_block (bb2, bb2end);
      bb3 = e23->dest;
      bb3->count = all - count1 - count2;
    }

  e34 = split_block (ncounts ? bb3 : bb2, bb3end);
  bb4 = e34->dest;
  bb4->count = all;

  e12->flags &= ~EDGE_FALLTHRU;
  e12->flags |= EDGE_FALSE_VALUE;
  e12->probability = REG_BR_PROB_BASE - prob1;
  e12->count = all - count1;

  e14 = make_edge (bb, bb4, EDGE_TRUE_VALUE);
  e14->probability = prob1;
  e14->count = count1;

  if (ncounts)  /* Assumed to be 0 or 1.  */
    {
      e23->flags &= ~EDGE_FALLTHRU;
      e23->flags |= EDGE_FALSE_VALUE;
      e23->count = all - count1 - count2;
      e23->probability = REG_BR_PROB_BASE - prob2;

      e24 = make_edge (bb2, bb4, EDGE_TRUE_VALUE);
      e24->probability = prob2;
      e24->count = count2;
    }

  e34->probability = REG_BR_PROB_BASE;
  e34->count = all - count1 - count2;

  return result;
}


/* Do transforms 3) and 4) on the statement pointed-to by SI if applicable.  */

static bool
gimple_mod_subtract_transform (gimple_stmt_iterator *si)
{
  histogram_value histogram;
  enum tree_code code;
  gcov_type count, wrong_values, all;
  tree lhs_type, result, value;
  gcov_type prob1, prob2;
  unsigned int i, steps;
  gcov_type count1, count2;
  gimple stmt;

  stmt = gsi_stmt (*si);
  if (gimple_code (stmt) != GIMPLE_ASSIGN)
    return false;

  lhs_type = TREE_TYPE (gimple_assign_lhs (stmt));
  if (!INTEGRAL_TYPE_P (lhs_type))
    return false;

  code = gimple_assign_rhs_code (stmt);
  
  if (code != TRUNC_MOD_EXPR || !TYPE_UNSIGNED (lhs_type))
    return false;

  histogram = gimple_histogram_value_of_type (cfun, stmt, HIST_TYPE_INTERVAL);
  if (!histogram)
    return false;

  value = histogram->hvalue.value;
  all = 0;
  wrong_values = 0;
  for (i = 0; i < histogram->hdata.intvl.steps; i++)
    all += histogram->hvalue.counters[i];

  wrong_values += histogram->hvalue.counters[i];
  wrong_values += histogram->hvalue.counters[i+1];
  steps = histogram->hdata.intvl.steps;
  all += wrong_values;
  count1 = histogram->hvalue.counters[0];
  count2 = histogram->hvalue.counters[1];

  /* Compute probability of taking the optimal path.  */
  if (check_counter (stmt, "interval", &count1, &all, gimple_bb (stmt)->count))
    {
      gimple_remove_histogram_value (cfun, stmt, histogram);
      return false;
    }

  if (flag_profile_correction && count1 + count2 > all)
      all = count1 + count2;

  gcc_assert (count1 + count2 <= all);

  /* We require that we use just subtractions in at least 50% of all
     evaluations.  */
  count = 0;
  for (i = 0; i < histogram->hdata.intvl.steps; i++)
    {
      count += histogram->hvalue.counters[i];
      if (count * 2 >= all)
	break;
    }
  if (i == steps
      || optimize_bb_for_size_p (gimple_bb (stmt)))
    return false;

  gimple_remove_histogram_value (cfun, stmt, histogram);
  if (dump_file)
    {
      fprintf (dump_file, "Mod subtract transformation on insn ");
      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
    }

  /* Compute probability of taking the optimal path(s).  */
  if (all > 0)
    {
      prob1 = (count1 * REG_BR_PROB_BASE + all / 2) / all;
      prob2 = (count2 * REG_BR_PROB_BASE + all / 2) / all;
    }
  else
    {
      prob1 = prob2 = 0;
    }

  /* In practice, "steps" is always 2.  This interface reflects this,
     and will need to be changed if "steps" can change.  */
  result = gimple_mod_subtract (stmt, prob1, prob2, i, count1, count2, all);

  gimple_assign_set_rhs_from_tree (si, result);

  return true;
}

static struct cgraph_node** pid_map = NULL;

/* Initialize map of pids (pid -> cgraph node) */

static void 
init_pid_map (void)
{
  struct cgraph_node *n;

  if (pid_map != NULL)
    return;

  pid_map 
    = (struct cgraph_node**) xmalloc (sizeof (struct cgraph_node*) * cgraph_max_pid);

  for (n = cgraph_nodes; n; n = n->next)
    {
      if (n->pid != -1)
	pid_map [n->pid] = n;
    }
}

/* Return cgraph node for function with pid */

static inline struct cgraph_node*
find_func_by_pid (int	pid)
{
  init_pid_map ();

  return pid_map [pid];
}

/* Do transformation

  if (actual_callee_address == address_of_most_common_function/method)
    do direct call
  else
    old call
 */

static gimple
gimple_ic (gimple stmt, gimple call, struct cgraph_node *direct_call, 
	   int prob, gcov_type count, gcov_type all)
{
  gimple stmt1, stmt2, stmt3;
  tree tmp1, tmpv, tmp;
  gimple bb1end, bb2end, bb3end;
  basic_block bb, bb2, bb3, bb4;
  tree optype = build_pointer_type (void_type_node);
  edge e12, e13, e23, e24, e34;
  gimple_stmt_iterator gsi;
  int region;

  bb = gimple_bb (stmt);
  gsi = gsi_for_stmt (stmt);

  tmpv = create_tmp_var (optype, "PROF");
  tmp1 = create_tmp_var (optype, "PROF");
  stmt1 = gimple_build_assign (tmpv, unshare_expr (gimple_call_fn (call)));

  tmp = fold_convert (optype, build_addr (direct_call->decl, 
					  current_function_decl));
  stmt2 = gimple_build_assign (tmp1, tmp);
  stmt3 = gimple_build_cond (NE_EXPR, tmp1, tmpv, NULL_TREE, NULL_TREE);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt2, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt3, GSI_SAME_STMT);
  bb1end = stmt3;

  stmt1 = gimple_copy (stmt);
  gimple_call_set_fndecl (stmt1, direct_call->decl);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  bb2end = stmt1;
  bb3end = stmt;

  /* Fix CFG. */
  /* Edge e23 connects bb2 to bb3, etc. */
  e12 = split_block (bb, bb1end);
  bb2 = e12->dest;
  bb2->count = count;
  e23 = split_block (bb2, bb2end);
  bb3 = e23->dest;
  bb3->count = all - count;
  e34 = split_block (bb3, bb3end);
  bb4 = e34->dest;
  bb4->count = all;

  e12->flags &= ~EDGE_FALLTHRU;
  e12->flags |= EDGE_FALSE_VALUE;
  e12->probability = prob;
  e12->count = count;

  e13 = make_edge (bb, bb3, EDGE_TRUE_VALUE);
  e13->probability = REG_BR_PROB_BASE - prob;
  e13->count = all - count;

  remove_edge (e23);
  
  e24 = make_edge (bb2, bb4, EDGE_FALLTHRU);
  e24->probability = REG_BR_PROB_BASE;
  e24->count = count;
  e34->probability = REG_BR_PROB_BASE;
  e34->count = all - count;

  /* Fix eh edges */
  region = lookup_stmt_eh_region (stmt);
  if (region >= 0 && stmt_could_throw_p (stmt1))
    {
      add_stmt_to_eh_region (stmt1, region);
      make_eh_edges (stmt1);
    }

  if (region >= 0 && stmt_could_throw_p (stmt))
    {
      gimple_purge_dead_eh_edges (bb4);
      make_eh_edges (stmt);
    }

  return stmt1;
}

/*
  For every checked indirect/virtual call determine if most common pid of
  function/class method has probability more than 50%. If yes modify code of
  this call to:
 */

static bool
gimple_ic_transform (gimple stmt)
{
  histogram_value histogram;
  gcov_type val, count, all, bb_all;
  gcov_type prob;
  tree callee;
  gimple modify;
  struct cgraph_node *direct_call;
  
  if (gimple_code (stmt) != GIMPLE_CALL)
    return false;

  callee = gimple_call_fn (stmt);

  if (TREE_CODE (callee) == FUNCTION_DECL)
    return false;

  histogram = gimple_histogram_value_of_type (cfun, stmt, HIST_TYPE_INDIR_CALL);
  if (!histogram)
    return false;

  val = histogram->hvalue.counters [0];
  count = histogram->hvalue.counters [1];
  all = histogram->hvalue.counters [2];
  gimple_remove_histogram_value (cfun, stmt, histogram);

  if (4 * count <= 3 * all)
    return false;

  bb_all = gimple_bb (stmt)->count;
  /* The order of CHECK_COUNTER calls is important - 
     since check_counter can correct the third parameter
     and we want to make count <= all <= bb_all. */
  if ( check_counter (stmt, "ic", &all, &bb_all, bb_all)
      || check_counter (stmt, "ic", &count, &all, all))
    return false;

  if (all > 0)
    prob = (count * REG_BR_PROB_BASE + all / 2) / all;
  else
    prob = 0;
  direct_call = find_func_by_pid ((int)val);

  if (direct_call == NULL)
    return false;

  modify = gimple_ic (stmt, stmt, direct_call, prob, count, all);

  if (dump_file)
    {
      fprintf (dump_file, "Indirect call -> direct call ");
      print_generic_expr (dump_file, gimple_call_fn (stmt), TDF_SLIM);
      fprintf (dump_file, "=> ");
      print_generic_expr (dump_file, direct_call->decl, TDF_SLIM);
      fprintf (dump_file, " transformation on insn ");
      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
      fprintf (dump_file, " to ");
      print_gimple_stmt (dump_file, modify, 0, TDF_SLIM);
      fprintf (dump_file, "hist->count "HOST_WIDEST_INT_PRINT_DEC
	       " hist->all "HOST_WIDEST_INT_PRINT_DEC"\n", count, all);
    }

  return true;
}

/* Return true if the stringop CALL with FNDECL shall be profiled.  */
static bool
interesting_stringop_to_profile_p (tree fndecl, gimple call)
{
  enum built_in_function fcode = DECL_FUNCTION_CODE (fndecl);

  if (fcode != BUILT_IN_MEMCPY && fcode != BUILT_IN_MEMPCPY
      && fcode != BUILT_IN_MEMSET && fcode != BUILT_IN_BZERO)
    return false;

  switch (fcode)
    {
     case BUILT_IN_MEMCPY:
     case BUILT_IN_MEMPCPY:
       return validate_gimple_arglist (call, POINTER_TYPE, POINTER_TYPE,
				       INTEGER_TYPE, VOID_TYPE);
     case BUILT_IN_MEMSET:
       return validate_gimple_arglist (call, POINTER_TYPE, INTEGER_TYPE,
				      INTEGER_TYPE, VOID_TYPE);
     case BUILT_IN_BZERO:
       return validate_gimple_arglist (call, POINTER_TYPE, INTEGER_TYPE,
				       VOID_TYPE);
     default:
       gcc_unreachable ();
    }
}

/* Convert   stringop (..., size)
   into 
   if (size == VALUE)
     stringop (...., VALUE);
   else
     stringop (...., size);
   assuming constant propagation of VALUE will happen later.
*/
static void
gimple_stringop_fixed_value (gimple stmt, tree value, int prob, gcov_type count,
			   gcov_type all)
{
  gimple stmt1, stmt2, stmt3;
  tree tmp1, tmpv;
  gimple bb1end, bb2end;
  basic_block bb, bb2, bb3, bb4;
  edge e12, e13, e23, e24, e34;
  gimple_stmt_iterator gsi;
  tree blck_size = gimple_call_arg (stmt, 2);
  tree optype = TREE_TYPE (blck_size);
  int region;

  bb = gimple_bb (stmt);
  gsi = gsi_for_stmt (stmt);

  if (gsi_end_p (gsi))
    {
      edge_iterator ei;
      for (ei = ei_start (bb->succs); (e34 = ei_safe_edge (ei)); )
	if (!(e34->flags & EDGE_ABNORMAL))
	  break;
    }
  else
    {
      e34 = split_block (bb, stmt);
      gsi = gsi_for_stmt (stmt);
    }
  bb4 = e34->dest;

  tmpv = create_tmp_var (optype, "PROF");
  tmp1 = create_tmp_var (optype, "PROF");
  stmt1 = gimple_build_assign (tmpv, fold_convert (optype, value));
  stmt2 = gimple_build_assign (tmp1, blck_size);
  stmt3 = gimple_build_cond (NE_EXPR, tmp1, tmpv, NULL_TREE, NULL_TREE);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt2, GSI_SAME_STMT);
  gsi_insert_before (&gsi, stmt3, GSI_SAME_STMT);
  bb1end = stmt3;

  stmt1 = gimple_copy (stmt);
  gimple_call_set_arg (stmt1, 2, value);
  gsi_insert_before (&gsi, stmt1, GSI_SAME_STMT);
  region = lookup_stmt_eh_region (stmt);
  if (region >= 0)
    add_stmt_to_eh_region (stmt1, region);
  bb2end = stmt1;

  /* Fix CFG. */
  /* Edge e23 connects bb2 to bb3, etc. */
  e12 = split_block (bb, bb1end);
  bb2 = e12->dest;
  bb2->count = count;
  e23 = split_block (bb2, bb2end);
  bb3 = e23->dest;
  bb3->count = all - count;

  e12->flags &= ~EDGE_FALLTHRU;
  e12->flags |= EDGE_FALSE_VALUE;
  e12->probability = prob;
  e12->count = count;

  e13 = make_edge (bb, bb3, EDGE_TRUE_VALUE);
  e13->probability = REG_BR_PROB_BASE - prob;
  e13->count = all - count;

  remove_edge (e23);
  
  e24 = make_edge (bb2, bb4, EDGE_FALLTHRU);
  e24->probability = REG_BR_PROB_BASE;
  e24->count = count;

  e34->probability = REG_BR_PROB_BASE;
  e34->count = all - count;
}

/* Find values inside STMT for that we want to measure histograms for
   division/modulo optimization.  */
static bool
gimple_stringops_transform (gimple_stmt_iterator *gsi)
{
  gimple stmt = gsi_stmt (*gsi);
  tree fndecl;
  tree blck_size;
  enum built_in_function fcode;
  histogram_value histogram;
  gcov_type count, all, val;
  tree value;
  tree dest, src;
  unsigned int dest_align, src_align;
  gcov_type prob;
  tree tree_val;

  if (gimple_code (stmt) != GIMPLE_CALL)
    return false;
  fndecl = gimple_call_fndecl (stmt);
  if (!fndecl)
    return false;
  fcode = DECL_FUNCTION_CODE (fndecl);
  if (!interesting_stringop_to_profile_p (fndecl, stmt))
    return false;

  if (fcode == BUILT_IN_BZERO)
    blck_size = gimple_call_arg (stmt, 1);
  else
    blck_size = gimple_call_arg (stmt, 2);
  if (TREE_CODE (blck_size) == INTEGER_CST)
    return false;

  histogram = gimple_histogram_value_of_type (cfun, stmt, HIST_TYPE_SINGLE_VALUE);
  if (!histogram)
    return false;
  value = histogram->hvalue.value;
  val = histogram->hvalue.counters[0];
  count = histogram->hvalue.counters[1];
  all = histogram->hvalue.counters[2];
  gimple_remove_histogram_value (cfun, stmt, histogram);
  /* We require that count is at least half of all; this means
     that for the transformation to fire the value must be constant
     at least 80% of time.  */
  if ((6 * count / 5) < all || optimize_bb_for_size_p (gimple_bb (stmt)))
    return false;
  if (check_counter (stmt, "value", &count, &all, gimple_bb (stmt)->count))
    return false;
  if (all > 0)
    prob = (count * REG_BR_PROB_BASE + all / 2) / all;
  else
    prob = 0;
  dest = gimple_call_arg (stmt, 0);
  dest_align = get_pointer_alignment (dest, BIGGEST_ALIGNMENT);
  switch (fcode)
    {
    case BUILT_IN_MEMCPY:
    case BUILT_IN_MEMPCPY:
      src = gimple_call_arg (stmt, 1);
      src_align = get_pointer_alignment (src, BIGGEST_ALIGNMENT);
      if (!can_move_by_pieces (val, MIN (dest_align, src_align)))
	return false;
      break;
    case BUILT_IN_MEMSET:
      if (!can_store_by_pieces (val, builtin_memset_read_str,
				gimple_call_arg (stmt, 1),
				dest_align, true))
	return false;
      break;
    case BUILT_IN_BZERO:
      if (!can_store_by_pieces (val, builtin_memset_read_str,
				integer_zero_node,
				dest_align, true))
	return false;
      break;
    default:
      gcc_unreachable ();
    }
  tree_val = build_int_cst_wide (get_gcov_type (),
				 (unsigned HOST_WIDE_INT) val,
				 val >> (HOST_BITS_PER_WIDE_INT - 1) >> 1);
  if (dump_file)
    {
      fprintf (dump_file, "Single value %i stringop transformation on ",
	       (int)val);
      print_gimple_stmt (dump_file, stmt, 0, TDF_SLIM);
    }
  gimple_stringop_fixed_value (stmt, tree_val, prob, count, all);
  
  return true;
}

void
stringop_block_profile (gimple stmt, unsigned int *expected_align,
			HOST_WIDE_INT *expected_size)
{
  histogram_value histogram;
  histogram = gimple_histogram_value_of_type (cfun, stmt, HIST_TYPE_AVERAGE);
  if (!histogram)
    *expected_size = -1;
  else if (!histogram->hvalue.counters[1])
    {
      *expected_size = -1;
      gimple_remove_histogram_value (cfun, stmt, histogram);
    }
  else
    {
      gcov_type size;
      size = ((histogram->hvalue.counters[0]
	      + histogram->hvalue.counters[1] / 2)
	       / histogram->hvalue.counters[1]);
      /* Even if we can hold bigger value in SIZE, INT_MAX
	 is safe "infinity" for code generation strategies.  */
      if (size > INT_MAX)
	size = INT_MAX;
      *expected_size = size;
      gimple_remove_histogram_value (cfun, stmt, histogram);
    }
  histogram = gimple_histogram_value_of_type (cfun, stmt, HIST_TYPE_IOR);
  if (!histogram)
    *expected_align = 0;
  else if (!histogram->hvalue.counters[0])
    {
      gimple_remove_histogram_value (cfun, stmt, histogram);
      *expected_align = 0;
    }
  else
    {
      gcov_type count;
      int alignment;

      count = histogram->hvalue.counters[0];
      alignment = 1;
      while (!(count & alignment)
	     && (alignment * 2 * BITS_PER_UNIT))
	alignment <<= 1;
      *expected_align = alignment * BITS_PER_UNIT;
      gimple_remove_histogram_value (cfun, stmt, histogram);
    }
}

struct value_prof_hooks {
  /* Find list of values for which we want to measure histograms.  */
  void (*find_values_to_profile) (histogram_values *);

  /* Identify and exploit properties of values that are hard to analyze
     statically.  See value-prof.c for more detail.  */
  bool (*value_profile_transformations) (void);  
};

/* Find values inside STMT for that we want to measure histograms for
   division/modulo optimization.  */
static void
gimple_divmod_values_to_profile (gimple stmt, histogram_values *values)
{
  tree lhs, divisor, op0, type;
  histogram_value hist;

  if (gimple_code (stmt) != GIMPLE_ASSIGN)
    return;

  lhs = gimple_assign_lhs (stmt);
  type = TREE_TYPE (lhs);
  if (!INTEGRAL_TYPE_P (type))
    return;

  switch (gimple_assign_rhs_code (stmt))
    {
    case TRUNC_DIV_EXPR:
    case TRUNC_MOD_EXPR:
      divisor = gimple_assign_rhs2 (stmt);
      op0 = gimple_assign_rhs1 (stmt);

      VEC_reserve (histogram_value, heap, *values, 3);

      if (is_gimple_reg (divisor))
	/* Check for the case where the divisor is the same value most
	   of the time.  */
	VEC_quick_push (histogram_value, *values,
			gimple_alloc_histogram_value (cfun,
						      HIST_TYPE_SINGLE_VALUE,
						      stmt, divisor));

      /* For mod, check whether it is not often a noop (or replaceable by
	 a few subtractions).  */
      if (gimple_assign_rhs_code (stmt) == TRUNC_MOD_EXPR
	  && TYPE_UNSIGNED (type))
	{
          tree val;
          /* Check for a special case where the divisor is power of 2.  */
	  VEC_quick_push (histogram_value, *values,
			  gimple_alloc_histogram_value (cfun, HIST_TYPE_POW2,
							stmt, divisor));

	  val = build2 (TRUNC_DIV_EXPR, type, op0, divisor);
	  hist = gimple_alloc_histogram_value (cfun, HIST_TYPE_INTERVAL,
					       stmt, val);
	  hist->hdata.intvl.int_start = 0;
	  hist->hdata.intvl.steps = 2;
	  VEC_quick_push (histogram_value, *values, hist);
	}
      return;

    default:
      return;
    }
}

/* Find calls inside STMT for that we want to measure histograms for 
   indirect/virtual call optimization. */ 

static void
gimple_indirect_call_to_profile (gimple stmt, histogram_values *values)
{
  tree callee;

  if (gimple_code (stmt) != GIMPLE_CALL
      || gimple_call_fndecl (stmt) != NULL_TREE)
    return;

  callee = gimple_call_fn (stmt);

  VEC_reserve (histogram_value, heap, *values, 3);

  VEC_quick_push (histogram_value, *values, 
		  gimple_alloc_histogram_value (cfun, HIST_TYPE_INDIR_CALL,
						stmt, callee));

  return;
}

/* Find values inside STMT for that we want to measure histograms for
   string operations.  */
static void
gimple_stringops_values_to_profile (gimple stmt, histogram_values *values)
{
  tree fndecl;
  tree blck_size;
  tree dest;
  enum built_in_function fcode;

  if (gimple_code (stmt) != GIMPLE_CALL)
    return;
  fndecl = gimple_call_fndecl (stmt);
  if (!fndecl)
    return;
  fcode = DECL_FUNCTION_CODE (fndecl);

  if (!interesting_stringop_to_profile_p (fndecl, stmt))
    return;

  dest = gimple_call_arg (stmt, 0);
  if (fcode == BUILT_IN_BZERO)
    blck_size = gimple_call_arg (stmt, 1);
  else
    blck_size = gimple_call_arg (stmt, 2);

  if (TREE_CODE (blck_size) != INTEGER_CST)
    {
      VEC_safe_push (histogram_value, heap, *values,
		     gimple_alloc_histogram_value (cfun, HIST_TYPE_SINGLE_VALUE,
						   stmt, blck_size));
      VEC_safe_push (histogram_value, heap, *values,
		     gimple_alloc_histogram_value (cfun, HIST_TYPE_AVERAGE,
						   stmt, blck_size));
    }
  if (TREE_CODE (blck_size) != INTEGER_CST)
    VEC_safe_push (histogram_value, heap, *values,
		   gimple_alloc_histogram_value (cfun, HIST_TYPE_IOR,
						 stmt, dest));
}

/* Find values inside STMT for that we want to measure histograms and adds
   them to list VALUES.  */

static void
gimple_values_to_profile (gimple stmt, histogram_values *values)
{
  if (flag_value_profile_transformations)
    {
      gimple_divmod_values_to_profile (stmt, values);
      gimple_stringops_values_to_profile (stmt, values);
      gimple_indirect_call_to_profile (stmt, values);
    }
}

static void
gimple_find_values_to_profile (histogram_values *values)
{
  basic_block bb;
  gimple_stmt_iterator gsi;
  unsigned i;
  histogram_value hist = NULL;

  *values = NULL;
  FOR_EACH_BB (bb)
    for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
      gimple_values_to_profile (gsi_stmt (gsi), values);
  
  for (i = 0; VEC_iterate (histogram_value, *values, i, hist); i++)
    {
      switch (hist->type)
        {
	case HIST_TYPE_INTERVAL:
	  hist->n_counters = hist->hdata.intvl.steps + 2;
	  break;

	case HIST_TYPE_POW2:
	  hist->n_counters = 2;
	  break;

	case HIST_TYPE_SINGLE_VALUE:
	  hist->n_counters = 3;
	  break;

	case HIST_TYPE_CONST_DELTA:
	  hist->n_counters = 4;
	  break;

 	case HIST_TYPE_INDIR_CALL:
 	  hist->n_counters = 3;
	  break;

	case HIST_TYPE_AVERAGE:
	  hist->n_counters = 2;
	  break;

	case HIST_TYPE_IOR:
	  hist->n_counters = 1;
	  break;

	default:
	  gcc_unreachable ();
	}
      if (dump_file)
        {
	  fprintf (dump_file, "Stmt ");
          print_gimple_stmt (dump_file, hist->hvalue.stmt, 0, TDF_SLIM);
	  dump_histogram_value (dump_file, hist);
        }
    }
}

static struct value_prof_hooks gimple_value_prof_hooks = {
  gimple_find_values_to_profile,
  gimple_value_profile_transformations
};

void
gimple_register_value_prof_hooks (void)
{
  gcc_assert (current_ir_type () == IR_GIMPLE);
  value_prof_hooks = &gimple_value_prof_hooks;
}

/* IR-independent entry points.  */
void
find_values_to_profile (histogram_values *values)
{
  (value_prof_hooks->find_values_to_profile) (values);
}

bool
value_profile_transformations (void)
{
  return (value_prof_hooks->value_profile_transformations) ();
}

