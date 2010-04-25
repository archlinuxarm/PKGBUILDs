/* Induction variable optimizations.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.
   
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

/* This pass tries to find the optimal set of induction variables for the loop.
   It optimizes just the basic linear induction variables (although adding
   support for other types should not be too hard).  It includes the
   optimizations commonly known as strength reduction, induction variable
   coalescing and induction variable elimination.  It does it in the
   following steps:

   1) The interesting uses of induction variables are found.  This includes

      -- uses of induction variables in non-linear expressions
      -- addresses of arrays
      -- comparisons of induction variables

   2) Candidates for the induction variables are found.  This includes

      -- old induction variables
      -- the variables defined by expressions derived from the "interesting
	 uses" above

   3) The optimal (w.r. to a cost function) set of variables is chosen.  The
      cost function assigns a cost to sets of induction variables and consists
      of three parts:

      -- The use costs.  Each of the interesting uses chooses the best induction
	 variable in the set and adds its cost to the sum.  The cost reflects
	 the time spent on modifying the induction variables value to be usable
	 for the given purpose (adding base and offset for arrays, etc.).
      -- The variable costs.  Each of the variables has a cost assigned that
	 reflects the costs associated with incrementing the value of the
	 variable.  The original variables are somewhat preferred.
      -- The set cost.  Depending on the size of the set, extra cost may be
	 added to reflect register pressure.

      All the costs are defined in a machine-specific way, using the target
      hooks and machine descriptions to determine them.

   4) The trees are transformed to use the new variables, the dead code is
      removed.
   
   All of this is done loop by loop.  Doing it globally is theoretically
   possible, it might give a better performance and it might enable us
   to decide costs more precisely, but getting all the interactions right
   would be complicated.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "rtl.h"
#include "tm_p.h"
#include "hard-reg-set.h"
#include "basic-block.h"
#include "output.h"
#include "diagnostic.h"
#include "tree-flow.h"
#include "tree-dump.h"
#include "timevar.h"
#include "cfgloop.h"
#include "varray.h"
#include "expr.h"
#include "tree-pass.h"
#include "ggc.h"
#include "insn-config.h"
#include "recog.h"
#include "pointer-set.h"
#include "hashtab.h"
#include "tree-chrec.h"
#include "tree-scalar-evolution.h"
#include "cfgloop.h"
#include "params.h"
#include "langhooks.h"
#include "tree-affine.h"
#include "target.h"

/* The infinite cost.  */
#define INFTY 10000000

/* The expected number of loop iterations.  TODO -- use profiling instead of
   this.  */
#define AVG_LOOP_NITER(LOOP) 5


/* Representation of the induction variable.  */
struct iv
{
  tree base;		/* Initial value of the iv.  */
  tree base_object;	/* A memory object to that the induction variable points.  */
  tree step;		/* Step of the iv (constant only).  */
  tree ssa_name;	/* The ssa name with the value.  */
  bool biv_p;		/* Is it a biv?  */
  bool have_use_for;	/* Do we already have a use for it?  */
  unsigned use_id;	/* The identifier in the use if it is the case.  */
};

/* Per-ssa version information (induction variable descriptions, etc.).  */
struct version_info
{
  tree name;		/* The ssa name.  */
  struct iv *iv;	/* Induction variable description.  */
  bool has_nonlin_use;	/* For a loop-level invariant, whether it is used in
			   an expression that is not an induction variable.  */
  unsigned inv_id;	/* Id of an invariant.  */
  bool preserve_biv;	/* For the original biv, whether to preserve it.  */
};

/* Types of uses.  */
enum use_type
{
  USE_NONLINEAR_EXPR,	/* Use in a nonlinear expression.  */
  USE_ADDRESS,		/* Use in an address.  */
  USE_COMPARE		/* Use is a compare.  */
};

/* Cost of a computation.  */
typedef struct
{
  unsigned cost;	/* The runtime cost.  */
  unsigned complexity;	/* The estimate of the complexity of the code for
			   the computation (in no concrete units --
			   complexity field should be larger for more
			   complex expressions and addressing modes).  */
} comp_cost;

static const comp_cost zero_cost = {0, 0};
static const comp_cost infinite_cost = {INFTY, INFTY};

/* The candidate - cost pair.  */
struct cost_pair
{
  struct iv_cand *cand;	/* The candidate.  */
  comp_cost cost;	/* The cost.  */
  bitmap depends_on;	/* The list of invariants that have to be
			   preserved.  */
  tree value;		/* For final value elimination, the expression for
			   the final value of the iv.  For iv elimination,
			   the new bound to compare with.  */
};

/* Use.  */
struct iv_use
{
  unsigned id;		/* The id of the use.  */
  enum use_type type;	/* Type of the use.  */
  struct iv *iv;	/* The induction variable it is based on.  */
  gimple stmt;		/* Statement in that it occurs.  */
  tree *op_p;		/* The place where it occurs.  */
  bitmap related_cands;	/* The set of "related" iv candidates, plus the common
			   important ones.  */

  unsigned n_map_members; /* Number of candidates in the cost_map list.  */
  struct cost_pair *cost_map;
			/* The costs wrto the iv candidates.  */

  struct iv_cand *selected;
			/* The selected candidate.  */
};

/* The position where the iv is computed.  */
enum iv_position
{
  IP_NORMAL,		/* At the end, just before the exit condition.  */
  IP_END,		/* At the end of the latch block.  */
  IP_ORIGINAL		/* The original biv.  */
};

/* The induction variable candidate.  */
struct iv_cand
{
  unsigned id;		/* The number of the candidate.  */
  bool important;	/* Whether this is an "important" candidate, i.e. such
			   that it should be considered by all uses.  */
  enum iv_position pos;	/* Where it is computed.  */
  gimple incremented_at;/* For original biv, the statement where it is
			   incremented.  */
  tree var_before;	/* The variable used for it before increment.  */
  tree var_after;	/* The variable used for it after increment.  */
  struct iv *iv;	/* The value of the candidate.  NULL for
			   "pseudocandidate" used to indicate the possibility
			   to replace the final value of an iv by direct
			   computation of the value.  */
  unsigned cost;	/* Cost of the candidate.  */
  bitmap depends_on;	/* The list of invariants that are used in step of the
			   biv.  */
};

/* The data used by the induction variable optimizations.  */

typedef struct iv_use *iv_use_p;
DEF_VEC_P(iv_use_p);
DEF_VEC_ALLOC_P(iv_use_p,heap);

typedef struct iv_cand *iv_cand_p;
DEF_VEC_P(iv_cand_p);
DEF_VEC_ALLOC_P(iv_cand_p,heap);

struct ivopts_data
{
  /* The currently optimized loop.  */
  struct loop *current_loop;

  /* Numbers of iterations for all exits of the current loop.  */
  struct pointer_map_t *niters;

  /* Number of registers used in it.  */
  unsigned regs_used;

  /* The size of version_info array allocated.  */
  unsigned version_info_size;

  /* The array of information for the ssa names.  */
  struct version_info *version_info;

  /* The bitmap of indices in version_info whose value was changed.  */
  bitmap relevant;

  /* The uses of induction variables.  */
  VEC(iv_use_p,heap) *iv_uses;

  /* The candidates.  */
  VEC(iv_cand_p,heap) *iv_candidates;

  /* A bitmap of important candidates.  */
  bitmap important_candidates;

  /* The maximum invariant id.  */
  unsigned max_inv_id;

  /* Whether to consider just related and important candidates when replacing a
     use.  */
  bool consider_all_candidates;

  /* Are we optimizing for speed?  */
  bool speed;
};

/* An assignment of iv candidates to uses.  */

struct iv_ca
{
  /* The number of uses covered by the assignment.  */
  unsigned upto;

  /* Number of uses that cannot be expressed by the candidates in the set.  */
  unsigned bad_uses;

  /* Candidate assigned to a use, together with the related costs.  */
  struct cost_pair **cand_for_use;

  /* Number of times each candidate is used.  */
  unsigned *n_cand_uses;

  /* The candidates used.  */
  bitmap cands;

  /* The number of candidates in the set.  */
  unsigned n_cands;

  /* Total number of registers needed.  */
  unsigned n_regs;

  /* Total cost of expressing uses.  */
  comp_cost cand_use_cost;

  /* Total cost of candidates.  */
  unsigned cand_cost;

  /* Number of times each invariant is used.  */
  unsigned *n_invariant_uses;

  /* Total cost of the assignment.  */
  comp_cost cost;
};

/* Difference of two iv candidate assignments.  */

struct iv_ca_delta
{
  /* Changed use.  */
  struct iv_use *use;

  /* An old assignment (for rollback purposes).  */
  struct cost_pair *old_cp;

  /* A new assignment.  */
  struct cost_pair *new_cp;

  /* Next change in the list.  */
  struct iv_ca_delta *next_change;
};

/* Bound on number of candidates below that all candidates are considered.  */

#define CONSIDER_ALL_CANDIDATES_BOUND \
  ((unsigned) PARAM_VALUE (PARAM_IV_CONSIDER_ALL_CANDIDATES_BOUND))

/* If there are more iv occurrences, we just give up (it is quite unlikely that
   optimizing such a loop would help, and it would take ages).  */

#define MAX_CONSIDERED_USES \
  ((unsigned) PARAM_VALUE (PARAM_IV_MAX_CONSIDERED_USES))

/* If there are at most this number of ivs in the set, try removing unnecessary
   ivs from the set always.  */

#define ALWAYS_PRUNE_CAND_SET_BOUND \
  ((unsigned) PARAM_VALUE (PARAM_IV_ALWAYS_PRUNE_CAND_SET_BOUND))

/* The list of trees for that the decl_rtl field must be reset is stored
   here.  */

static VEC(tree,heap) *decl_rtl_to_reset;

/* Number of uses recorded in DATA.  */

static inline unsigned
n_iv_uses (struct ivopts_data *data)
{
  return VEC_length (iv_use_p, data->iv_uses);
}

/* Ith use recorded in DATA.  */

static inline struct iv_use *
iv_use (struct ivopts_data *data, unsigned i)
{
  return VEC_index (iv_use_p, data->iv_uses, i);
}

/* Number of candidates recorded in DATA.  */

static inline unsigned
n_iv_cands (struct ivopts_data *data)
{
  return VEC_length (iv_cand_p, data->iv_candidates);
}

/* Ith candidate recorded in DATA.  */

static inline struct iv_cand *
iv_cand (struct ivopts_data *data, unsigned i)
{
  return VEC_index (iv_cand_p, data->iv_candidates, i);
}

/* The single loop exit if it dominates the latch, NULL otherwise.  */

edge
single_dom_exit (struct loop *loop)
{
  edge exit = single_exit (loop);

  if (!exit)
    return NULL;

  if (!just_once_each_iteration_p (loop, exit->src))
    return NULL;

  return exit;
}

/* Dumps information about the induction variable IV to FILE.  */

extern void dump_iv (FILE *, struct iv *);
void
dump_iv (FILE *file, struct iv *iv)
{
  if (iv->ssa_name)
    {
      fprintf (file, "ssa name ");
      print_generic_expr (file, iv->ssa_name, TDF_SLIM);
      fprintf (file, "\n");
    }

  fprintf (file, "  type ");
  print_generic_expr (file, TREE_TYPE (iv->base), TDF_SLIM);
  fprintf (file, "\n");

  if (iv->step)
    {
      fprintf (file, "  base ");
      print_generic_expr (file, iv->base, TDF_SLIM);
      fprintf (file, "\n");

      fprintf (file, "  step ");
      print_generic_expr (file, iv->step, TDF_SLIM);
      fprintf (file, "\n");
    }
  else
    {
      fprintf (file, "  invariant ");
      print_generic_expr (file, iv->base, TDF_SLIM);
      fprintf (file, "\n");
    }

  if (iv->base_object)
    {
      fprintf (file, "  base object ");
      print_generic_expr (file, iv->base_object, TDF_SLIM);
      fprintf (file, "\n");
    }

  if (iv->biv_p)
    fprintf (file, "  is a biv\n");
}

/* Dumps information about the USE to FILE.  */

extern void dump_use (FILE *, struct iv_use *);
void
dump_use (FILE *file, struct iv_use *use)
{
  fprintf (file, "use %d\n", use->id);

  switch (use->type)
    {
    case USE_NONLINEAR_EXPR:
      fprintf (file, "  generic\n");
      break;

    case USE_ADDRESS:
      fprintf (file, "  address\n");
      break;

    case USE_COMPARE:
      fprintf (file, "  compare\n");
      break;

    default:
      gcc_unreachable ();
    }

  fprintf (file, "  in statement ");
  print_gimple_stmt (file, use->stmt, 0, 0);
  fprintf (file, "\n");

  fprintf (file, "  at position ");
  if (use->op_p)
    print_generic_expr (file, *use->op_p, TDF_SLIM);
  fprintf (file, "\n");

  dump_iv (file, use->iv);

  if (use->related_cands)
    {
      fprintf (file, "  related candidates ");
      dump_bitmap (file, use->related_cands);
    }
}

/* Dumps information about the uses to FILE.  */

extern void dump_uses (FILE *, struct ivopts_data *);
void
dump_uses (FILE *file, struct ivopts_data *data)
{
  unsigned i;
  struct iv_use *use;

  for (i = 0; i < n_iv_uses (data); i++)
    {
      use = iv_use (data, i);

      dump_use (file, use);
      fprintf (file, "\n");
    }
}

/* Dumps information about induction variable candidate CAND to FILE.  */

extern void dump_cand (FILE *, struct iv_cand *);
void
dump_cand (FILE *file, struct iv_cand *cand)
{
  struct iv *iv = cand->iv;

  fprintf (file, "candidate %d%s\n",
	   cand->id, cand->important ? " (important)" : "");

  if (cand->depends_on)
    {
      fprintf (file, "  depends on ");
      dump_bitmap (file, cand->depends_on);
    }

  if (!iv)
    {
      fprintf (file, "  final value replacement\n");
      return;
    }

  switch (cand->pos)
    {
    case IP_NORMAL:
      fprintf (file, "  incremented before exit test\n");
      break;

    case IP_END:
      fprintf (file, "  incremented at end\n");
      break;

    case IP_ORIGINAL:
      fprintf (file, "  original biv\n");
      break;
    }

  dump_iv (file, iv);
}

/* Returns the info for ssa version VER.  */

static inline struct version_info *
ver_info (struct ivopts_data *data, unsigned ver)
{
  return data->version_info + ver;
}

/* Returns the info for ssa name NAME.  */

static inline struct version_info *
name_info (struct ivopts_data *data, tree name)
{
  return ver_info (data, SSA_NAME_VERSION (name));
}

/* Returns true if STMT is after the place where the IP_NORMAL ivs will be
   emitted in LOOP.  */

static bool
stmt_after_ip_normal_pos (struct loop *loop, gimple stmt)
{
  basic_block bb = ip_normal_pos (loop), sbb = gimple_bb (stmt);

  gcc_assert (bb);

  if (sbb == loop->latch)
    return true;

  if (sbb != bb)
    return false;

  return stmt == last_stmt (bb);
}

/* Returns true if STMT if after the place where the original induction
   variable CAND is incremented.  */

static bool
stmt_after_ip_original_pos (struct iv_cand *cand, gimple stmt)
{
  basic_block cand_bb = gimple_bb (cand->incremented_at);
  basic_block stmt_bb = gimple_bb (stmt);
  gimple_stmt_iterator bsi;

  if (!dominated_by_p (CDI_DOMINATORS, stmt_bb, cand_bb))
    return false;

  if (stmt_bb != cand_bb)
    return true;

  /* Scan the block from the end, since the original ivs are usually
     incremented at the end of the loop body.  */
  for (bsi = gsi_last_bb (stmt_bb); ; gsi_prev (&bsi))
    {
      if (gsi_stmt (bsi) == cand->incremented_at)
	return false;
      if (gsi_stmt (bsi) == stmt)
	return true;
    }
}

/* Returns true if STMT if after the place where the induction variable
   CAND is incremented in LOOP.  */

static bool
stmt_after_increment (struct loop *loop, struct iv_cand *cand, gimple stmt)
{
  switch (cand->pos)
    {
    case IP_END:
      return false;

    case IP_NORMAL:
      return stmt_after_ip_normal_pos (loop, stmt);

    case IP_ORIGINAL:
      return stmt_after_ip_original_pos (cand, stmt);

    default:
      gcc_unreachable ();
    }
}

/* Returns true if EXP is a ssa name that occurs in an abnormal phi node.  */

static bool
abnormal_ssa_name_p (tree exp)
{
  if (!exp)
    return false;

  if (TREE_CODE (exp) != SSA_NAME)
    return false;

  return SSA_NAME_OCCURS_IN_ABNORMAL_PHI (exp) != 0;
}

/* Returns false if BASE or INDEX contains a ssa name that occurs in an
   abnormal phi node.  Callback for for_each_index.  */

static bool
idx_contains_abnormal_ssa_name_p (tree base, tree *index,
				  void *data ATTRIBUTE_UNUSED)
{
  if (TREE_CODE (base) == ARRAY_REF || TREE_CODE (base) == ARRAY_RANGE_REF)
    {
      if (abnormal_ssa_name_p (TREE_OPERAND (base, 2)))
	return false;
      if (abnormal_ssa_name_p (TREE_OPERAND (base, 3)))
	return false;
    }

  return !abnormal_ssa_name_p (*index);
}

/* Returns true if EXPR contains a ssa name that occurs in an
   abnormal phi node.  */

bool
contains_abnormal_ssa_name_p (tree expr)
{
  enum tree_code code;
  enum tree_code_class codeclass;

  if (!expr)
    return false;

  code = TREE_CODE (expr);
  codeclass = TREE_CODE_CLASS (code);

  if (code == SSA_NAME)
    return SSA_NAME_OCCURS_IN_ABNORMAL_PHI (expr) != 0;

  if (code == INTEGER_CST
      || is_gimple_min_invariant (expr))
    return false;

  if (code == ADDR_EXPR)
    return !for_each_index (&TREE_OPERAND (expr, 0),
			    idx_contains_abnormal_ssa_name_p,
			    NULL);

  switch (codeclass)
    {
    case tcc_binary:
    case tcc_comparison:
      if (contains_abnormal_ssa_name_p (TREE_OPERAND (expr, 1)))
	return true;

      /* Fallthru.  */
    case tcc_unary:
      if (contains_abnormal_ssa_name_p (TREE_OPERAND (expr, 0)))
	return true;

      break;

    default:
      gcc_unreachable ();
    }

  return false;
}

/*  Returns tree describing number of iterations determined from
    EXIT of DATA->current_loop, or NULL if something goes wrong.  */

static tree
niter_for_exit (struct ivopts_data *data, edge exit)
{
  struct tree_niter_desc desc;
  tree niter;
  void **slot;

  if (!data->niters)
    {
      data->niters = pointer_map_create ();
      slot = NULL;
    }
  else
    slot = pointer_map_contains (data->niters, exit);

  if (!slot)
    {
      /* Try to determine number of iterations.  We must know it
	 unconditionally (i.e., without possibility of # of iterations
	 being zero).  Also, we cannot safely work with ssa names that
	 appear in phi nodes on abnormal edges, so that we do not create
	 overlapping life ranges for them (PR 27283).  */
      if (number_of_iterations_exit (data->current_loop,
				     exit, &desc, true)
	  && integer_zerop (desc.may_be_zero)
     	  && !contains_abnormal_ssa_name_p (desc.niter))
	niter = desc.niter;
      else
	niter = NULL_TREE;

      *pointer_map_insert (data->niters, exit) = niter;
    }
  else
    niter = (tree) *slot;

  return niter;
}

/* Returns tree describing number of iterations determined from
   single dominating exit of DATA->current_loop, or NULL if something
   goes wrong.  */

static tree
niter_for_single_dom_exit (struct ivopts_data *data)
{
  edge exit = single_dom_exit (data->current_loop);

  if (!exit)
    return NULL;

  return niter_for_exit (data, exit);
}

/* Initializes data structures used by the iv optimization pass, stored
   in DATA.  */

static void
tree_ssa_iv_optimize_init (struct ivopts_data *data)
{
  data->version_info_size = 2 * num_ssa_names;
  data->version_info = XCNEWVEC (struct version_info, data->version_info_size);
  data->relevant = BITMAP_ALLOC (NULL);
  data->important_candidates = BITMAP_ALLOC (NULL);
  data->max_inv_id = 0;
  data->niters = NULL;
  data->iv_uses = VEC_alloc (iv_use_p, heap, 20);
  data->iv_candidates = VEC_alloc (iv_cand_p, heap, 20);
  decl_rtl_to_reset = VEC_alloc (tree, heap, 20);
}

/* Returns a memory object to that EXPR points.  In case we are able to
   determine that it does not point to any such object, NULL is returned.  */

static tree
determine_base_object (tree expr)
{
  enum tree_code code = TREE_CODE (expr);
  tree base, obj;

  /* If this is a pointer casted to any type, we need to determine
     the base object for the pointer; so handle conversions before
     throwing away non-pointer expressions.  */
  if (CONVERT_EXPR_P (expr))
    return determine_base_object (TREE_OPERAND (expr, 0));

  if (!POINTER_TYPE_P (TREE_TYPE (expr)))
    return NULL_TREE;

  switch (code)
    {
    case INTEGER_CST:
      return NULL_TREE;

    case ADDR_EXPR:
      obj = TREE_OPERAND (expr, 0);
      base = get_base_address (obj);

      if (!base)
	return expr;

      if (TREE_CODE (base) == INDIRECT_REF)
	return determine_base_object (TREE_OPERAND (base, 0));

      return fold_convert (ptr_type_node,
		           build_fold_addr_expr (base));

    case POINTER_PLUS_EXPR:
      return determine_base_object (TREE_OPERAND (expr, 0));

    case PLUS_EXPR:
    case MINUS_EXPR:
      /* Pointer addition is done solely using POINTER_PLUS_EXPR.  */
      gcc_unreachable ();

    default:
      return fold_convert (ptr_type_node, expr);
    }
}

/* Allocates an induction variable with given initial value BASE and step STEP
   for loop LOOP.  */

static struct iv *
alloc_iv (tree base, tree step)
{
  struct iv *iv = XCNEW (struct iv);
  gcc_assert (step != NULL_TREE);

  iv->base = base;
  iv->base_object = determine_base_object (base);
  iv->step = step;
  iv->biv_p = false;
  iv->have_use_for = false;
  iv->use_id = 0;
  iv->ssa_name = NULL_TREE;

  return iv;
}

/* Sets STEP and BASE for induction variable IV.  */

static void
set_iv (struct ivopts_data *data, tree iv, tree base, tree step)
{
  struct version_info *info = name_info (data, iv);

  gcc_assert (!info->iv);

  bitmap_set_bit (data->relevant, SSA_NAME_VERSION (iv));
  info->iv = alloc_iv (base, step);
  info->iv->ssa_name = iv;
}

/* Finds induction variable declaration for VAR.  */

static struct iv *
get_iv (struct ivopts_data *data, tree var)
{
  basic_block bb;
  tree type = TREE_TYPE (var);

  if (!POINTER_TYPE_P (type)
      && !INTEGRAL_TYPE_P (type))
    return NULL;

  if (!name_info (data, var)->iv)
    {
      bb = gimple_bb (SSA_NAME_DEF_STMT (var));

      if (!bb
	  || !flow_bb_inside_loop_p (data->current_loop, bb))
	set_iv (data, var, var, build_int_cst (type, 0));
    }

  return name_info (data, var)->iv;
}

/* Determines the step of a biv defined in PHI.  Returns NULL if PHI does
   not define a simple affine biv with nonzero step.  */

static tree
determine_biv_step (gimple phi)
{
  struct loop *loop = gimple_bb (phi)->loop_father;
  tree name = PHI_RESULT (phi);
  affine_iv iv;

  if (!is_gimple_reg (name))
    return NULL_TREE;

  if (!simple_iv (loop, loop, name, &iv, true))
    return NULL_TREE;

  return integer_zerop (iv.step) ? NULL_TREE : iv.step;
}

/* Finds basic ivs.  */

static bool
find_bivs (struct ivopts_data *data)
{
  gimple phi;
  tree step, type, base;
  bool found = false;
  struct loop *loop = data->current_loop;
  gimple_stmt_iterator psi;

  for (psi = gsi_start_phis (loop->header); !gsi_end_p (psi); gsi_next (&psi))
    {
      phi = gsi_stmt (psi);

      if (SSA_NAME_OCCURS_IN_ABNORMAL_PHI (PHI_RESULT (phi)))
	continue;

      step = determine_biv_step (phi);
      if (!step)
	continue;

      base = PHI_ARG_DEF_FROM_EDGE (phi, loop_preheader_edge (loop));
      base = expand_simple_operations (base);
      if (contains_abnormal_ssa_name_p (base)
	  || contains_abnormal_ssa_name_p (step))
	continue;

      type = TREE_TYPE (PHI_RESULT (phi));
      base = fold_convert (type, base);
      if (step)
	{
	  if (POINTER_TYPE_P (type))
	    step = fold_convert (sizetype, step);
	  else
	    step = fold_convert (type, step);
	}

      set_iv (data, PHI_RESULT (phi), base, step);
      found = true;
    }

  return found;
}

/* Marks basic ivs.  */

static void
mark_bivs (struct ivopts_data *data)
{
  gimple phi;
  tree var;
  struct iv *iv, *incr_iv;
  struct loop *loop = data->current_loop;
  basic_block incr_bb;
  gimple_stmt_iterator psi;

  for (psi = gsi_start_phis (loop->header); !gsi_end_p (psi); gsi_next (&psi))
    {
      phi = gsi_stmt (psi);

      iv = get_iv (data, PHI_RESULT (phi));
      if (!iv)
	continue;

      var = PHI_ARG_DEF_FROM_EDGE (phi, loop_latch_edge (loop));
      incr_iv = get_iv (data, var);
      if (!incr_iv)
	continue;

      /* If the increment is in the subloop, ignore it.  */
      incr_bb = gimple_bb (SSA_NAME_DEF_STMT (var));
      if (incr_bb->loop_father != data->current_loop
	  || (incr_bb->flags & BB_IRREDUCIBLE_LOOP))
	continue;

      iv->biv_p = true;
      incr_iv->biv_p = true;
    }
}

/* Checks whether STMT defines a linear induction variable and stores its
   parameters to IV.  */

static bool
find_givs_in_stmt_scev (struct ivopts_data *data, gimple stmt, affine_iv *iv)
{
  tree lhs;
  struct loop *loop = data->current_loop;

  iv->base = NULL_TREE;
  iv->step = NULL_TREE;

  if (gimple_code (stmt) != GIMPLE_ASSIGN)
    return false;

  lhs = gimple_assign_lhs (stmt);
  if (TREE_CODE (lhs) != SSA_NAME)
    return false;

  if (!simple_iv (loop, loop_containing_stmt (stmt), lhs, iv, true))
    return false;
  iv->base = expand_simple_operations (iv->base);

  if (contains_abnormal_ssa_name_p (iv->base)
      || contains_abnormal_ssa_name_p (iv->step))
    return false;

  return true;
}

/* Finds general ivs in statement STMT.  */

static void
find_givs_in_stmt (struct ivopts_data *data, gimple stmt)
{
  affine_iv iv;

  if (!find_givs_in_stmt_scev (data, stmt, &iv))
    return;

  set_iv (data, gimple_assign_lhs (stmt), iv.base, iv.step);
}

/* Finds general ivs in basic block BB.  */

static void
find_givs_in_bb (struct ivopts_data *data, basic_block bb)
{
  gimple_stmt_iterator bsi;

  for (bsi = gsi_start_bb (bb); !gsi_end_p (bsi); gsi_next (&bsi))
    find_givs_in_stmt (data, gsi_stmt (bsi));
}

/* Finds general ivs.  */

static void
find_givs (struct ivopts_data *data)
{
  struct loop *loop = data->current_loop;
  basic_block *body = get_loop_body_in_dom_order (loop);
  unsigned i;

  for (i = 0; i < loop->num_nodes; i++)
    find_givs_in_bb (data, body[i]);
  free (body);
}

/* For each ssa name defined in LOOP determines whether it is an induction
   variable and if so, its initial value and step.  */

static bool
find_induction_variables (struct ivopts_data *data)
{
  unsigned i;
  bitmap_iterator bi;

  if (!find_bivs (data))
    return false;

  find_givs (data);
  mark_bivs (data);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      tree niter = niter_for_single_dom_exit (data);

      if (niter)
	{
	  fprintf (dump_file, "  number of iterations ");
	  print_generic_expr (dump_file, niter, TDF_SLIM);
	  fprintf (dump_file, "\n\n");
    	};
 
      fprintf (dump_file, "Induction variables:\n\n");

      EXECUTE_IF_SET_IN_BITMAP (data->relevant, 0, i, bi)
	{
	  if (ver_info (data, i)->iv)
	    dump_iv (dump_file, ver_info (data, i)->iv);
	}
    }

  return true;
}

/* Records a use of type USE_TYPE at *USE_P in STMT whose value is IV.  */

static struct iv_use *
record_use (struct ivopts_data *data, tree *use_p, struct iv *iv,
	    gimple stmt, enum use_type use_type)
{
  struct iv_use *use = XCNEW (struct iv_use);

  use->id = n_iv_uses (data);
  use->type = use_type;
  use->iv = iv;
  use->stmt = stmt;
  use->op_p = use_p;
  use->related_cands = BITMAP_ALLOC (NULL);

  /* To avoid showing ssa name in the dumps, if it was not reset by the
     caller.  */
  iv->ssa_name = NULL_TREE;

  if (dump_file && (dump_flags & TDF_DETAILS))
    dump_use (dump_file, use);

  VEC_safe_push (iv_use_p, heap, data->iv_uses, use);

  return use;
}

/* Checks whether OP is a loop-level invariant and if so, records it.
   NONLINEAR_USE is true if the invariant is used in a way we do not
   handle specially.  */

static void
record_invariant (struct ivopts_data *data, tree op, bool nonlinear_use)
{
  basic_block bb;
  struct version_info *info;

  if (TREE_CODE (op) != SSA_NAME
      || !is_gimple_reg (op))
    return;

  bb = gimple_bb (SSA_NAME_DEF_STMT (op));
  if (bb
      && flow_bb_inside_loop_p (data->current_loop, bb))
    return;

  info = name_info (data, op);
  info->name = op;
  info->has_nonlin_use |= nonlinear_use;
  if (!info->inv_id)
    info->inv_id = ++data->max_inv_id;
  bitmap_set_bit (data->relevant, SSA_NAME_VERSION (op));
}

/* Checks whether the use OP is interesting and if so, records it.  */

static struct iv_use *
find_interesting_uses_op (struct ivopts_data *data, tree op)
{
  struct iv *iv;
  struct iv *civ;
  gimple stmt;
  struct iv_use *use;

  if (TREE_CODE (op) != SSA_NAME)
    return NULL;

  iv = get_iv (data, op);
  if (!iv)
    return NULL;
  
  if (iv->have_use_for)
    {
      use = iv_use (data, iv->use_id);

      gcc_assert (use->type == USE_NONLINEAR_EXPR);
      return use;
    }

  if (integer_zerop (iv->step))
    {
      record_invariant (data, op, true);
      return NULL;
    }
  iv->have_use_for = true;

  civ = XNEW (struct iv);
  *civ = *iv;

  stmt = SSA_NAME_DEF_STMT (op);
  gcc_assert (gimple_code (stmt) == GIMPLE_PHI
	      || is_gimple_assign (stmt));

  use = record_use (data, NULL, civ, stmt, USE_NONLINEAR_EXPR);
  iv->use_id = use->id;

  return use;
}

/* Given a condition in statement STMT, checks whether it is a compare
   of an induction variable and an invariant.  If this is the case,
   CONTROL_VAR is set to location of the iv, BOUND to the location of
   the invariant, IV_VAR and IV_BOUND are set to the corresponding
   induction variable descriptions, and true is returned.  If this is not
   the case, CONTROL_VAR and BOUND are set to the arguments of the
   condition and false is returned.  */

static bool
extract_cond_operands (struct ivopts_data *data, gimple stmt,
		       tree **control_var, tree **bound,
		       struct iv **iv_var, struct iv **iv_bound)
{
  /* The objects returned when COND has constant operands.  */
  static struct iv const_iv;
  static tree zero;
  tree *op0 = &zero, *op1 = &zero, *tmp_op;
  struct iv *iv0 = &const_iv, *iv1 = &const_iv, *tmp_iv;
  bool ret = false;

  if (gimple_code (stmt) == GIMPLE_COND)
    {
      op0 = gimple_cond_lhs_ptr (stmt);
      op1 = gimple_cond_rhs_ptr (stmt);
    }
  else
    {
      op0 = gimple_assign_rhs1_ptr (stmt);
      op1 = gimple_assign_rhs2_ptr (stmt);
    }

  zero = integer_zero_node;
  const_iv.step = integer_zero_node;

  if (TREE_CODE (*op0) == SSA_NAME)
    iv0 = get_iv (data, *op0);
  if (TREE_CODE (*op1) == SSA_NAME)
    iv1 = get_iv (data, *op1);

  /* Exactly one of the compared values must be an iv, and the other one must
     be an invariant.  */
  if (!iv0 || !iv1)
    goto end;

  if (integer_zerop (iv0->step))
    {
      /* Control variable may be on the other side.  */
      tmp_op = op0; op0 = op1; op1 = tmp_op;
      tmp_iv = iv0; iv0 = iv1; iv1 = tmp_iv;
    }
  ret = !integer_zerop (iv0->step) && integer_zerop (iv1->step);

end:
  if (control_var)
    *control_var = op0;;
  if (iv_var)
    *iv_var = iv0;;
  if (bound)
    *bound = op1;
  if (iv_bound)
    *iv_bound = iv1;

  return ret;
}

/* Checks whether the condition in STMT is interesting and if so,
   records it.  */

static void
find_interesting_uses_cond (struct ivopts_data *data, gimple stmt)
{
  tree *var_p, *bound_p;
  struct iv *var_iv, *civ;

  if (!extract_cond_operands (data, stmt, &var_p, &bound_p, &var_iv, NULL))
    {
      find_interesting_uses_op (data, *var_p);
      find_interesting_uses_op (data, *bound_p);
      return;
    }

  civ = XNEW (struct iv);
  *civ = *var_iv;
  record_use (data, NULL, civ, stmt, USE_COMPARE);
}

/* Returns true if expression EXPR is obviously invariant in LOOP,
   i.e. if all its operands are defined outside of the LOOP.  LOOP
   should not be the function body.  */

bool
expr_invariant_in_loop_p (struct loop *loop, tree expr)
{
  basic_block def_bb;
  unsigned i, len;

  gcc_assert (loop_depth (loop) > 0);

  if (is_gimple_min_invariant (expr))
    return true;

  if (TREE_CODE (expr) == SSA_NAME)
    {
      def_bb = gimple_bb (SSA_NAME_DEF_STMT (expr));
      if (def_bb
	  && flow_bb_inside_loop_p (loop, def_bb))
	return false;

      return true;
    }

  if (!EXPR_P (expr))
    return false;

  len = TREE_OPERAND_LENGTH (expr);
  for (i = 0; i < len; i++)
    if (!expr_invariant_in_loop_p (loop, TREE_OPERAND (expr, i)))
      return false;

  return true;
}

/* Returns true if statement STMT is obviously invariant in LOOP,
   i.e. if all its operands on the RHS are defined outside of the LOOP.
   LOOP should not be the function body.  */

bool
stmt_invariant_in_loop_p (struct loop *loop, gimple stmt)
{
  unsigned i;
  tree lhs;

  gcc_assert (loop_depth (loop) > 0);

  lhs = gimple_get_lhs (stmt);
  for (i = 0; i < gimple_num_ops (stmt); i++)
    {
      tree op = gimple_op (stmt, i);
      if (op != lhs && !expr_invariant_in_loop_p (loop, op))
	return false;
    }

  return true;
}

/* Cumulates the steps of indices into DATA and replaces their values with the
   initial ones.  Returns false when the value of the index cannot be determined.
   Callback for for_each_index.  */

struct ifs_ivopts_data
{
  struct ivopts_data *ivopts_data;
  gimple stmt;
  tree step;
};

static bool
idx_find_step (tree base, tree *idx, void *data)
{
  struct ifs_ivopts_data *dta = (struct ifs_ivopts_data *) data;
  struct iv *iv;
  tree step, iv_base, iv_step, lbound, off;
  struct loop *loop = dta->ivopts_data->current_loop;

  if (TREE_CODE (base) == MISALIGNED_INDIRECT_REF
      || TREE_CODE (base) == ALIGN_INDIRECT_REF)
    return false;

  /* If base is a component ref, require that the offset of the reference
     be invariant.  */
  if (TREE_CODE (base) == COMPONENT_REF)
    {
      off = component_ref_field_offset (base);
      return expr_invariant_in_loop_p (loop, off);
    }

  /* If base is array, first check whether we will be able to move the
     reference out of the loop (in order to take its address in strength
     reduction).  In order for this to work we need both lower bound
     and step to be loop invariants.  */
  if (TREE_CODE (base) == ARRAY_REF || TREE_CODE (base) == ARRAY_RANGE_REF)
    {
      /* Moreover, for a range, the size needs to be invariant as well.  */
      if (TREE_CODE (base) == ARRAY_RANGE_REF
	  && !expr_invariant_in_loop_p (loop, TYPE_SIZE (TREE_TYPE (base))))
	return false;

      step = array_ref_element_size (base);
      lbound = array_ref_low_bound (base);

      if (!expr_invariant_in_loop_p (loop, step)
	  || !expr_invariant_in_loop_p (loop, lbound))
	return false;
    }

  if (TREE_CODE (*idx) != SSA_NAME)
    return true;

  iv = get_iv (dta->ivopts_data, *idx);
  if (!iv)
    return false;

  /* XXX  We produce for a base of *D42 with iv->base being &x[0]
	  *&x[0], which is not folded and does not trigger the
	  ARRAY_REF path below.  */
  *idx = iv->base;

  if (integer_zerop (iv->step))
    return true;

  if (TREE_CODE (base) == ARRAY_REF || TREE_CODE (base) == ARRAY_RANGE_REF)
    {
      step = array_ref_element_size (base);

      /* We only handle addresses whose step is an integer constant.  */
      if (TREE_CODE (step) != INTEGER_CST)
	return false;
    }
  else
    /* The step for pointer arithmetics already is 1 byte.  */
    step = build_int_cst (sizetype, 1);

  iv_base = iv->base;
  iv_step = iv->step;
  if (!convert_affine_scev (dta->ivopts_data->current_loop,
			    sizetype, &iv_base, &iv_step, dta->stmt,
			    false))
    {
      /* The index might wrap.  */
      return false;
    }

  step = fold_build2 (MULT_EXPR, sizetype, step, iv_step);
  dta->step = fold_build2 (PLUS_EXPR, sizetype, dta->step, step);

  return true;
}

/* Records use in index IDX.  Callback for for_each_index.  Ivopts data
   object is passed to it in DATA.  */

static bool
idx_record_use (tree base, tree *idx,
		void *vdata)
{
  struct ivopts_data *data = (struct ivopts_data *) vdata;
  find_interesting_uses_op (data, *idx);
  if (TREE_CODE (base) == ARRAY_REF || TREE_CODE (base) == ARRAY_RANGE_REF)
    {
      find_interesting_uses_op (data, array_ref_element_size (base));
      find_interesting_uses_op (data, array_ref_low_bound (base));
    }
  return true;
}

/* If we can prove that TOP = cst * BOT for some constant cst,
   store cst to MUL and return true.  Otherwise return false.
   The returned value is always sign-extended, regardless of the
   signedness of TOP and BOT.  */

static bool
constant_multiple_of (tree top, tree bot, double_int *mul)
{
  tree mby;
  enum tree_code code;
  double_int res, p0, p1;
  unsigned precision = TYPE_PRECISION (TREE_TYPE (top));

  STRIP_NOPS (top);
  STRIP_NOPS (bot);

  if (operand_equal_p (top, bot, 0))
    {
      *mul = double_int_one;
      return true;
    }

  code = TREE_CODE (top);
  switch (code)
    {
    case MULT_EXPR:
      mby = TREE_OPERAND (top, 1);
      if (TREE_CODE (mby) != INTEGER_CST)
	return false;

      if (!constant_multiple_of (TREE_OPERAND (top, 0), bot, &res))
	return false;

      *mul = double_int_sext (double_int_mul (res, tree_to_double_int (mby)),
			      precision);
      return true;

    case PLUS_EXPR:
    case MINUS_EXPR:
      if (!constant_multiple_of (TREE_OPERAND (top, 0), bot, &p0)
	  || !constant_multiple_of (TREE_OPERAND (top, 1), bot, &p1))
	return false;

      if (code == MINUS_EXPR)
	p1 = double_int_neg (p1);
      *mul = double_int_sext (double_int_add (p0, p1), precision);
      return true;

    case INTEGER_CST:
      if (TREE_CODE (bot) != INTEGER_CST)
	return false;

      p0 = double_int_sext (tree_to_double_int (top), precision);
      p1 = double_int_sext (tree_to_double_int (bot), precision);
      if (double_int_zero_p (p1))
	return false;
      *mul = double_int_sext (double_int_sdivmod (p0, p1, FLOOR_DIV_EXPR, &res),
			      precision);
      return double_int_zero_p (res);

    default:
      return false;
    }
}

/* Returns true if memory reference REF with step STEP may be unaligned.  */

static bool
may_be_unaligned_p (tree ref, tree step)
{
  tree base;
  tree base_type;
  HOST_WIDE_INT bitsize;
  HOST_WIDE_INT bitpos;
  tree toffset;
  enum machine_mode mode;
  int unsignedp, volatilep;
  unsigned base_align;

  /* TARGET_MEM_REFs are translated directly to valid MEMs on the target,
     thus they are not misaligned.  */
  if (TREE_CODE (ref) == TARGET_MEM_REF)
    return false;

  /* The test below is basically copy of what expr.c:normal_inner_ref
     does to check whether the object must be loaded by parts when
     STRICT_ALIGNMENT is true.  */
  base = get_inner_reference (ref, &bitsize, &bitpos, &toffset, &mode,
			      &unsignedp, &volatilep, true);
  base_type = TREE_TYPE (base);
  base_align = TYPE_ALIGN (base_type);

  if (mode != BLKmode)
    {
      double_int mul;
      tree al = build_int_cst (TREE_TYPE (step),
			       GET_MODE_ALIGNMENT (mode) / BITS_PER_UNIT);

      if (base_align < GET_MODE_ALIGNMENT (mode)
	  || bitpos % GET_MODE_ALIGNMENT (mode) != 0
	  || bitpos % BITS_PER_UNIT != 0)
	return true;
    
      if (!constant_multiple_of (step, al, &mul))
	return true;
    }

  return false;
}

/* Return true if EXPR may be non-addressable.   */

static bool
may_be_nonaddressable_p (tree expr)
{
  switch (TREE_CODE (expr))
    {
    case TARGET_MEM_REF:
      /* TARGET_MEM_REFs are translated directly to valid MEMs on the
	 target, thus they are always addressable.  */
      return false;

    case COMPONENT_REF:
      return DECL_NONADDRESSABLE_P (TREE_OPERAND (expr, 1))
	     || may_be_nonaddressable_p (TREE_OPERAND (expr, 0));

    case VIEW_CONVERT_EXPR:
      /* This kind of view-conversions may wrap non-addressable objects
	 and make them look addressable.  After some processing the
	 non-addressability may be uncovered again, causing ADDR_EXPRs
	 of inappropriate objects to be built.  */
      if (is_gimple_reg (TREE_OPERAND (expr, 0))
	  || !is_gimple_addressable (TREE_OPERAND (expr, 0)))
	return true;

      /* ... fall through ... */

    case ARRAY_REF:
    case ARRAY_RANGE_REF:
      return may_be_nonaddressable_p (TREE_OPERAND (expr, 0));

    CASE_CONVERT:
      return true;

    default:
      break;
    }

  return false;
}

/* Finds addresses in *OP_P inside STMT.  */

static void
find_interesting_uses_address (struct ivopts_data *data, gimple stmt, tree *op_p)
{
  tree base = *op_p, step = build_int_cst (sizetype, 0);
  struct iv *civ;
  struct ifs_ivopts_data ifs_ivopts_data;

  /* Do not play with volatile memory references.  A bit too conservative,
     perhaps, but safe.  */
  if (gimple_has_volatile_ops (stmt))
    goto fail;

  /* Ignore bitfields for now.  Not really something terribly complicated
     to handle.  TODO.  */
  if (TREE_CODE (base) == BIT_FIELD_REF)
    goto fail;

  base = unshare_expr (base);

  if (TREE_CODE (base) == TARGET_MEM_REF)
    {
      tree type = build_pointer_type (TREE_TYPE (base));
      tree astep;

      if (TMR_BASE (base)
	  && TREE_CODE (TMR_BASE (base)) == SSA_NAME)
	{
	  civ = get_iv (data, TMR_BASE (base));
	  if (!civ)
	    goto fail;

	  TMR_BASE (base) = civ->base;
	  step = civ->step;
	}
      if (TMR_INDEX (base)
	  && TREE_CODE (TMR_INDEX (base)) == SSA_NAME)
	{
	  civ = get_iv (data, TMR_INDEX (base));
	  if (!civ)
	    goto fail;

	  TMR_INDEX (base) = civ->base;
	  astep = civ->step;

	  if (astep)
	    {
	      if (TMR_STEP (base))
		astep = fold_build2 (MULT_EXPR, type, TMR_STEP (base), astep);

	      step = fold_build2 (PLUS_EXPR, type, step, astep);
	    }
	}

      if (integer_zerop (step))
	goto fail;
      base = tree_mem_ref_addr (type, base);
    }
  else
    {
      ifs_ivopts_data.ivopts_data = data;
      ifs_ivopts_data.stmt = stmt;
      ifs_ivopts_data.step = build_int_cst (sizetype, 0);
      if (!for_each_index (&base, idx_find_step, &ifs_ivopts_data)
	  || integer_zerop (ifs_ivopts_data.step))
	goto fail;
      step = ifs_ivopts_data.step;

      gcc_assert (TREE_CODE (base) != ALIGN_INDIRECT_REF);
      gcc_assert (TREE_CODE (base) != MISALIGNED_INDIRECT_REF);

      /* Check that the base expression is addressable.  This needs
	 to be done after substituting bases of IVs into it.  */
      if (may_be_nonaddressable_p (base))
	goto fail;

      /* Moreover, on strict alignment platforms, check that it is
	 sufficiently aligned.  */
      if (STRICT_ALIGNMENT && may_be_unaligned_p (base, step))
	goto fail;

      base = build_fold_addr_expr (base);

      /* Substituting bases of IVs into the base expression might
	 have caused folding opportunities.  */
      if (TREE_CODE (base) == ADDR_EXPR)
	{
	  tree *ref = &TREE_OPERAND (base, 0);
	  while (handled_component_p (*ref))
	    ref = &TREE_OPERAND (*ref, 0);
	  if (TREE_CODE (*ref) == INDIRECT_REF)
	    *ref = fold_indirect_ref (*ref);
	}
    }

  civ = alloc_iv (base, step);
  record_use (data, op_p, civ, stmt, USE_ADDRESS);
  return;

fail:
  for_each_index (op_p, idx_record_use, data);
}

/* Finds and records invariants used in STMT.  */

static void
find_invariants_stmt (struct ivopts_data *data, gimple stmt)
{
  ssa_op_iter iter;
  use_operand_p use_p;
  tree op;

  FOR_EACH_PHI_OR_STMT_USE (use_p, stmt, iter, SSA_OP_USE)
    {
      op = USE_FROM_PTR (use_p);
      record_invariant (data, op, false);
    }
}

/* Finds interesting uses of induction variables in the statement STMT.  */

static void
find_interesting_uses_stmt (struct ivopts_data *data, gimple stmt)
{
  struct iv *iv;
  tree op, *lhs, *rhs;
  ssa_op_iter iter;
  use_operand_p use_p;
  enum tree_code code;

  find_invariants_stmt (data, stmt);

  if (gimple_code (stmt) == GIMPLE_COND)
    {
      find_interesting_uses_cond (data, stmt);
      return;
    }

  if (is_gimple_assign (stmt))
    {
      lhs = gimple_assign_lhs_ptr (stmt);
      rhs = gimple_assign_rhs1_ptr (stmt);

      if (TREE_CODE (*lhs) == SSA_NAME)
	{
	  /* If the statement defines an induction variable, the uses are not
	     interesting by themselves.  */

	  iv = get_iv (data, *lhs);

	  if (iv && !integer_zerop (iv->step))
	    return;
	}

      code = gimple_assign_rhs_code (stmt);
      if (get_gimple_rhs_class (code) == GIMPLE_SINGLE_RHS
	  && (REFERENCE_CLASS_P (*rhs)
	      || is_gimple_val (*rhs)))
	{
	  if (REFERENCE_CLASS_P (*rhs))
	    find_interesting_uses_address (data, stmt, rhs);
	  else
	    find_interesting_uses_op (data, *rhs);

	  if (REFERENCE_CLASS_P (*lhs))
	    find_interesting_uses_address (data, stmt, lhs);
	  return;
	}
      else if (TREE_CODE_CLASS (code) == tcc_comparison)
	{
	  find_interesting_uses_cond (data, stmt);
	  return;
	}

      /* TODO -- we should also handle address uses of type

	 memory = call (whatever);

	 and

	 call (memory).  */
    }

  if (gimple_code (stmt) == GIMPLE_PHI
      && gimple_bb (stmt) == data->current_loop->header)
    {
      iv = get_iv (data, PHI_RESULT (stmt));

      if (iv && !integer_zerop (iv->step))
	return;
    }

  FOR_EACH_PHI_OR_STMT_USE (use_p, stmt, iter, SSA_OP_USE)
    {
      op = USE_FROM_PTR (use_p);

      if (TREE_CODE (op) != SSA_NAME)
	continue;

      iv = get_iv (data, op);
      if (!iv)
	continue;

      find_interesting_uses_op (data, op);
    }
}

/* Finds interesting uses of induction variables outside of loops
   on loop exit edge EXIT.  */

static void
find_interesting_uses_outside (struct ivopts_data *data, edge exit)
{
  gimple phi;
  gimple_stmt_iterator psi;
  tree def;

  for (psi = gsi_start_phis (exit->dest); !gsi_end_p (psi); gsi_next (&psi))
    {
      phi = gsi_stmt (psi);
      def = PHI_ARG_DEF_FROM_EDGE (phi, exit);
      if (is_gimple_reg (def))
	find_interesting_uses_op (data, def);
    }
}

/* Finds uses of the induction variables that are interesting.  */

static void
find_interesting_uses (struct ivopts_data *data)
{
  basic_block bb;
  gimple_stmt_iterator bsi;
  basic_block *body = get_loop_body (data->current_loop);
  unsigned i;
  struct version_info *info;
  edge e;

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "Uses:\n\n");

  for (i = 0; i < data->current_loop->num_nodes; i++)
    {
      edge_iterator ei;
      bb = body[i];

      FOR_EACH_EDGE (e, ei, bb->succs)
	if (e->dest != EXIT_BLOCK_PTR
	    && !flow_bb_inside_loop_p (data->current_loop, e->dest))
	  find_interesting_uses_outside (data, e);

      for (bsi = gsi_start_phis (bb); !gsi_end_p (bsi); gsi_next (&bsi))
	find_interesting_uses_stmt (data, gsi_stmt (bsi));
      for (bsi = gsi_start_bb (bb); !gsi_end_p (bsi); gsi_next (&bsi))
	find_interesting_uses_stmt (data, gsi_stmt (bsi));
    }

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      bitmap_iterator bi;

      fprintf (dump_file, "\n");

      EXECUTE_IF_SET_IN_BITMAP (data->relevant, 0, i, bi)
	{
	  info = ver_info (data, i);
	  if (info->inv_id)
	    {
	      fprintf (dump_file, "  ");
	      print_generic_expr (dump_file, info->name, TDF_SLIM);
	      fprintf (dump_file, " is invariant (%d)%s\n",
		       info->inv_id, info->has_nonlin_use ? "" : ", eliminable");
	    }
	}

      fprintf (dump_file, "\n");
    }

  free (body);
}

/* Strips constant offsets from EXPR and stores them to OFFSET.  If INSIDE_ADDR
   is true, assume we are inside an address.  If TOP_COMPREF is true, assume
   we are at the top-level of the processed address.  */

static tree
strip_offset_1 (tree expr, bool inside_addr, bool top_compref,
		unsigned HOST_WIDE_INT *offset)
{
  tree op0 = NULL_TREE, op1 = NULL_TREE, tmp, step;
  enum tree_code code;
  tree type, orig_type = TREE_TYPE (expr);
  unsigned HOST_WIDE_INT off0, off1, st;
  tree orig_expr = expr;

  STRIP_NOPS (expr);

  type = TREE_TYPE (expr);
  code = TREE_CODE (expr);
  *offset = 0;

  switch (code)
    {
    case INTEGER_CST:
      if (!cst_and_fits_in_hwi (expr)
	  || integer_zerop (expr))
	return orig_expr;

      *offset = int_cst_value (expr);
      return build_int_cst (orig_type, 0);

    case POINTER_PLUS_EXPR:
    case PLUS_EXPR:
    case MINUS_EXPR:
      op0 = TREE_OPERAND (expr, 0);
      op1 = TREE_OPERAND (expr, 1);

      op0 = strip_offset_1 (op0, false, false, &off0);
      op1 = strip_offset_1 (op1, false, false, &off1);

      *offset = (code == MINUS_EXPR ? off0 - off1 : off0 + off1);
      if (op0 == TREE_OPERAND (expr, 0)
	  && op1 == TREE_OPERAND (expr, 1))
	return orig_expr;

      if (integer_zerop (op1))
	expr = op0;
      else if (integer_zerop (op0))
	{
	  if (code == MINUS_EXPR)
	    expr = fold_build1 (NEGATE_EXPR, type, op1);
	  else
	    expr = op1;
	}
      else
	expr = fold_build2 (code, type, op0, op1);

      return fold_convert (orig_type, expr);

    case ARRAY_REF:
    case ARRAY_RANGE_REF:
      if (!inside_addr)
	return orig_expr;

      step = array_ref_element_size (expr);
      if (!cst_and_fits_in_hwi (step))
	break;

      st = int_cst_value (step);
      op1 = TREE_OPERAND (expr, 1);
      op1 = strip_offset_1 (op1, false, false, &off1);
      *offset = off1 * st;

      if (top_compref
	  && integer_zerop (op1))
	{
	  /* Strip the component reference completely.  */
	  op0 = TREE_OPERAND (expr, 0);
	  op0 = strip_offset_1 (op0, inside_addr, top_compref, &off0);
	  *offset += off0;
	  return op0;
	}
      break;

    case COMPONENT_REF:
      if (!inside_addr)
	return orig_expr;

      tmp = component_ref_field_offset (expr);
      if (top_compref
	  && cst_and_fits_in_hwi (tmp))
	{
	  /* Strip the component reference completely.  */
	  op0 = TREE_OPERAND (expr, 0);
	  op0 = strip_offset_1 (op0, inside_addr, top_compref, &off0);
	  *offset = off0 + int_cst_value (tmp);
	  return op0;
	}
      break;

    case ADDR_EXPR:
      op0 = TREE_OPERAND (expr, 0);
      op0 = strip_offset_1 (op0, true, true, &off0);
      *offset += off0;

      if (op0 == TREE_OPERAND (expr, 0))
	return orig_expr;

      expr = build_fold_addr_expr (op0);
      return fold_convert (orig_type, expr);

    case INDIRECT_REF:
      inside_addr = false;
      break;

    default:
      return orig_expr;
    }

  /* Default handling of expressions for that we want to recurse into
     the first operand.  */
  op0 = TREE_OPERAND (expr, 0);
  op0 = strip_offset_1 (op0, inside_addr, false, &off0);
  *offset += off0;

  if (op0 == TREE_OPERAND (expr, 0)
      && (!op1 || op1 == TREE_OPERAND (expr, 1)))
    return orig_expr;

  expr = copy_node (expr);
  TREE_OPERAND (expr, 0) = op0;
  if (op1)
    TREE_OPERAND (expr, 1) = op1;

  /* Inside address, we might strip the top level component references,
     thus changing type of the expression.  Handling of ADDR_EXPR
     will fix that.  */
  expr = fold_convert (orig_type, expr);

  return expr;
}

/* Strips constant offsets from EXPR and stores them to OFFSET.  */

static tree
strip_offset (tree expr, unsigned HOST_WIDE_INT *offset)
{
  return strip_offset_1 (expr, false, false, offset);
}

/* Returns variant of TYPE that can be used as base for different uses.
   We return unsigned type with the same precision, which avoids problems
   with overflows.  */

static tree
generic_type_for (tree type)
{
  if (POINTER_TYPE_P (type))
    return unsigned_type_for (type);

  if (TYPE_UNSIGNED (type))
    return type;

  return unsigned_type_for (type);
}

/* Records invariants in *EXPR_P.  Callback for walk_tree.  DATA contains
   the bitmap to that we should store it.  */

static struct ivopts_data *fd_ivopts_data;
static tree
find_depends (tree *expr_p, int *ws ATTRIBUTE_UNUSED, void *data)
{
  bitmap *depends_on = (bitmap *) data;
  struct version_info *info;

  if (TREE_CODE (*expr_p) != SSA_NAME)
    return NULL_TREE;
  info = name_info (fd_ivopts_data, *expr_p);

  if (!info->inv_id || info->has_nonlin_use)
    return NULL_TREE;

  if (!*depends_on)
    *depends_on = BITMAP_ALLOC (NULL);
  bitmap_set_bit (*depends_on, info->inv_id);

  return NULL_TREE;
}

/* Adds a candidate BASE + STEP * i.  Important field is set to IMPORTANT and
   position to POS.  If USE is not NULL, the candidate is set as related to
   it.  If both BASE and STEP are NULL, we add a pseudocandidate for the
   replacement of the final value of the iv by a direct computation.  */

static struct iv_cand *
add_candidate_1 (struct ivopts_data *data,
		 tree base, tree step, bool important, enum iv_position pos,
		 struct iv_use *use, gimple incremented_at)
{
  unsigned i;
  struct iv_cand *cand = NULL;
  tree type, orig_type;
  
  if (base)
    {
      orig_type = TREE_TYPE (base);
      type = generic_type_for (orig_type);
      if (type != orig_type)
	{
	  base = fold_convert (type, base);
	  step = fold_convert (type, step);
	}
    }

  for (i = 0; i < n_iv_cands (data); i++)
    {
      cand = iv_cand (data, i);

      if (cand->pos != pos)
	continue;

      if (cand->incremented_at != incremented_at)
	continue;

      if (!cand->iv)
	{
	  if (!base && !step)
	    break;

	  continue;
	}

      if (!base && !step)
	continue;

      if (operand_equal_p (base, cand->iv->base, 0)
	  && operand_equal_p (step, cand->iv->step, 0))
	break;
    }

  if (i == n_iv_cands (data))
    {
      cand = XCNEW (struct iv_cand);
      cand->id = i;

      if (!base && !step)
	cand->iv = NULL;
      else
	cand->iv = alloc_iv (base, step);

      cand->pos = pos;
      if (pos != IP_ORIGINAL && cand->iv)
	{
	  cand->var_before = create_tmp_var_raw (TREE_TYPE (base), "ivtmp");
	  cand->var_after = cand->var_before;
	}
      cand->important = important;
      cand->incremented_at = incremented_at;
      VEC_safe_push (iv_cand_p, heap, data->iv_candidates, cand);

      if (step
	  && TREE_CODE (step) != INTEGER_CST)
	{
	  fd_ivopts_data = data;
	  walk_tree (&step, find_depends, &cand->depends_on, NULL);
	}

      if (dump_file && (dump_flags & TDF_DETAILS))
	dump_cand (dump_file, cand);
    }

  if (important && !cand->important)
    {
      cand->important = true;
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "Candidate %d is important\n", cand->id);
    }

  if (use)
    {
      bitmap_set_bit (use->related_cands, i);
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "Candidate %d is related to use %d\n",
		 cand->id, use->id);
    }

  return cand;
}

/* Returns true if incrementing the induction variable at the end of the LOOP
   is allowed.

   The purpose is to avoid splitting latch edge with a biv increment, thus
   creating a jump, possibly confusing other optimization passes and leaving
   less freedom to scheduler.  So we allow IP_END_POS only if IP_NORMAL_POS
   is not available (so we do not have a better alternative), or if the latch
   edge is already nonempty.  */

static bool
allow_ip_end_pos_p (struct loop *loop)
{
  if (!ip_normal_pos (loop))
    return true;

  if (!empty_block_p (ip_end_pos (loop)))
    return true;

  return false;
}

/* Adds a candidate BASE + STEP * i.  Important field is set to IMPORTANT and
   position to POS.  If USE is not NULL, the candidate is set as related to
   it.  The candidate computation is scheduled on all available positions.  */

static void
add_candidate (struct ivopts_data *data, 
	       tree base, tree step, bool important, struct iv_use *use)
{
  if (ip_normal_pos (data->current_loop))
    add_candidate_1 (data, base, step, important, IP_NORMAL, use, NULL);
  if (ip_end_pos (data->current_loop)
      && allow_ip_end_pos_p (data->current_loop))
    add_candidate_1 (data, base, step, important, IP_END, use, NULL);
}

/* Add a standard "0 + 1 * iteration" iv candidate for a
   type with SIZE bits.  */

static void
add_standard_iv_candidates_for_size (struct ivopts_data *data,
				     unsigned int size)
{
  tree type = lang_hooks.types.type_for_size (size, true);
  add_candidate (data, build_int_cst (type, 0), build_int_cst (type, 1),
		 true, NULL);
}

/* Adds standard iv candidates.  */

static void
add_standard_iv_candidates (struct ivopts_data *data)
{
  add_standard_iv_candidates_for_size (data, INT_TYPE_SIZE);

  /* The same for a double-integer type if it is still fast enough.  */
  if (BITS_PER_WORD >= INT_TYPE_SIZE * 2)
    add_standard_iv_candidates_for_size (data, INT_TYPE_SIZE * 2);
}


/* Adds candidates bases on the old induction variable IV.  */

static void
add_old_iv_candidates (struct ivopts_data *data, struct iv *iv)
{
  gimple phi;
  tree def;
  struct iv_cand *cand;

  add_candidate (data, iv->base, iv->step, true, NULL);

  /* The same, but with initial value zero.  */
  if (POINTER_TYPE_P (TREE_TYPE (iv->base)))
    add_candidate (data, size_int (0), iv->step, true, NULL);
  else
    add_candidate (data, build_int_cst (TREE_TYPE (iv->base), 0),
		   iv->step, true, NULL);

  phi = SSA_NAME_DEF_STMT (iv->ssa_name);
  if (gimple_code (phi) == GIMPLE_PHI)
    {
      /* Additionally record the possibility of leaving the original iv
	 untouched.  */
      def = PHI_ARG_DEF_FROM_EDGE (phi, loop_latch_edge (data->current_loop));
      cand = add_candidate_1 (data,
			      iv->base, iv->step, true, IP_ORIGINAL, NULL,
			      SSA_NAME_DEF_STMT (def));
      cand->var_before = iv->ssa_name;
      cand->var_after = def;
    }
}

/* Adds candidates based on the old induction variables.  */

static void
add_old_ivs_candidates (struct ivopts_data *data)
{
  unsigned i;
  struct iv *iv;
  bitmap_iterator bi;

  EXECUTE_IF_SET_IN_BITMAP (data->relevant, 0, i, bi)
    {
      iv = ver_info (data, i)->iv;
      if (iv && iv->biv_p && !integer_zerop (iv->step))
	add_old_iv_candidates (data, iv);
    }
}

/* Adds candidates based on the value of the induction variable IV and USE.  */

static void
add_iv_value_candidates (struct ivopts_data *data,
			 struct iv *iv, struct iv_use *use)
{
  unsigned HOST_WIDE_INT offset;
  tree base;
  tree basetype;

  add_candidate (data, iv->base, iv->step, false, use);

  /* The same, but with initial value zero.  Make such variable important,
     since it is generic enough so that possibly many uses may be based
     on it.  */
  basetype = TREE_TYPE (iv->base);
  if (POINTER_TYPE_P (basetype))
    basetype = sizetype;
  add_candidate (data, build_int_cst (basetype, 0),
		 iv->step, true, use);

  /* Third, try removing the constant offset.  Make sure to even
     add a candidate for &a[0] vs. (T *)&a.  */
  base = strip_offset (iv->base, &offset);
  if (offset
      || base != iv->base)
    add_candidate (data, base, iv->step, false, use);
}

/* Adds candidates based on the uses.  */

static void
add_derived_ivs_candidates (struct ivopts_data *data)
{
  unsigned i;

  for (i = 0; i < n_iv_uses (data); i++)
    {
      struct iv_use *use = iv_use (data, i);

      if (!use)
	continue;

      switch (use->type)
	{
	case USE_NONLINEAR_EXPR:
	case USE_COMPARE:
	case USE_ADDRESS:
	  /* Just add the ivs based on the value of the iv used here.  */
	  add_iv_value_candidates (data, use->iv, use);
	  break;

	default:
	  gcc_unreachable ();
	}
    }
}

/* Record important candidates and add them to related_cands bitmaps
   if needed.  */

static void
record_important_candidates (struct ivopts_data *data)
{
  unsigned i;
  struct iv_use *use;

  for (i = 0; i < n_iv_cands (data); i++)
    {
      struct iv_cand *cand = iv_cand (data, i);

      if (cand->important)
	bitmap_set_bit (data->important_candidates, i);
    }

  data->consider_all_candidates = (n_iv_cands (data)
				   <= CONSIDER_ALL_CANDIDATES_BOUND);

  if (data->consider_all_candidates)
    {
      /* We will not need "related_cands" bitmaps in this case,
	 so release them to decrease peak memory consumption.  */
      for (i = 0; i < n_iv_uses (data); i++)
	{
	  use = iv_use (data, i);
	  BITMAP_FREE (use->related_cands);
	}
    }
  else
    {
      /* Add important candidates to the related_cands bitmaps.  */
      for (i = 0; i < n_iv_uses (data); i++)
	bitmap_ior_into (iv_use (data, i)->related_cands,
			 data->important_candidates);
    }
}

/* Finds the candidates for the induction variables.  */

static void
find_iv_candidates (struct ivopts_data *data)
{
  /* Add commonly used ivs.  */
  add_standard_iv_candidates (data);

  /* Add old induction variables.  */
  add_old_ivs_candidates (data);

  /* Add induction variables derived from uses.  */
  add_derived_ivs_candidates (data);

  /* Record the important candidates.  */
  record_important_candidates (data);
}

/* Allocates the data structure mapping the (use, candidate) pairs to costs.
   If consider_all_candidates is true, we use a two-dimensional array, otherwise
   we allocate a simple list to every use.  */

static void
alloc_use_cost_map (struct ivopts_data *data)
{
  unsigned i, size, s, j;

  for (i = 0; i < n_iv_uses (data); i++)
    {
      struct iv_use *use = iv_use (data, i);
      bitmap_iterator bi;

      if (data->consider_all_candidates)
	size = n_iv_cands (data);
      else
	{
	  s = 0;
	  EXECUTE_IF_SET_IN_BITMAP (use->related_cands, 0, j, bi)
	    {
	      s++;
	    }

	  /* Round up to the power of two, so that moduling by it is fast.  */
	  for (size = 1; size < s; size <<= 1)
	    continue;
	}

      use->n_map_members = size;
      use->cost_map = XCNEWVEC (struct cost_pair, size);
    }
}

/* Returns description of computation cost of expression whose runtime
   cost is RUNTIME and complexity corresponds to COMPLEXITY.  */

static comp_cost
new_cost (unsigned runtime, unsigned complexity)
{
  comp_cost cost;

  cost.cost = runtime;
  cost.complexity = complexity;

  return cost;
}

/* Adds costs COST1 and COST2.  */

static comp_cost
add_costs (comp_cost cost1, comp_cost cost2)
{
  cost1.cost += cost2.cost;
  cost1.complexity += cost2.complexity;

  return cost1;
}
/* Subtracts costs COST1 and COST2.  */

static comp_cost
sub_costs (comp_cost cost1, comp_cost cost2)
{
  cost1.cost -= cost2.cost;
  cost1.complexity -= cost2.complexity;

  return cost1;
}

/* Returns a negative number if COST1 < COST2, a positive number if
   COST1 > COST2, and 0 if COST1 = COST2.  */

static int
compare_costs (comp_cost cost1, comp_cost cost2)
{
  if (cost1.cost == cost2.cost)
    return cost1.complexity - cost2.complexity;

  return cost1.cost - cost2.cost;
}

/* Returns true if COST is infinite.  */

static bool
infinite_cost_p (comp_cost cost)
{
  return cost.cost == INFTY;
}

/* Sets cost of (USE, CANDIDATE) pair to COST and record that it depends
   on invariants DEPENDS_ON and that the value used in expressing it
   is VALUE.*/

static void
set_use_iv_cost (struct ivopts_data *data,
		 struct iv_use *use, struct iv_cand *cand,
		 comp_cost cost, bitmap depends_on, tree value)
{
  unsigned i, s;

  if (infinite_cost_p (cost))
    {
      BITMAP_FREE (depends_on);
      return;
    }

  if (data->consider_all_candidates)
    {
      use->cost_map[cand->id].cand = cand;
      use->cost_map[cand->id].cost = cost;
      use->cost_map[cand->id].depends_on = depends_on;
      use->cost_map[cand->id].value = value;
      return;
    }

  /* n_map_members is a power of two, so this computes modulo.  */
  s = cand->id & (use->n_map_members - 1);
  for (i = s; i < use->n_map_members; i++)
    if (!use->cost_map[i].cand)
      goto found;
  for (i = 0; i < s; i++)
    if (!use->cost_map[i].cand)
      goto found;

  gcc_unreachable ();

found:
  use->cost_map[i].cand = cand;
  use->cost_map[i].cost = cost;
  use->cost_map[i].depends_on = depends_on;
  use->cost_map[i].value = value;
}

/* Gets cost of (USE, CANDIDATE) pair.  */

static struct cost_pair *
get_use_iv_cost (struct ivopts_data *data, struct iv_use *use,
		 struct iv_cand *cand)
{
  unsigned i, s;
  struct cost_pair *ret;

  if (!cand)
    return NULL;

  if (data->consider_all_candidates)
    {
      ret = use->cost_map + cand->id;
      if (!ret->cand)
	return NULL;

      return ret;
    }
      
  /* n_map_members is a power of two, so this computes modulo.  */
  s = cand->id & (use->n_map_members - 1);
  for (i = s; i < use->n_map_members; i++)
    if (use->cost_map[i].cand == cand)
      return use->cost_map + i;

  for (i = 0; i < s; i++)
    if (use->cost_map[i].cand == cand)
      return use->cost_map + i;

  return NULL;
}

/* Returns estimate on cost of computing SEQ.  */

static unsigned
seq_cost (rtx seq, bool speed)
{
  unsigned cost = 0;
  rtx set;

  for (; seq; seq = NEXT_INSN (seq))
    {
      set = single_set (seq);
      if (set)
	cost += rtx_cost (set, SET,speed);
      else
	cost++;
    }

  return cost;
}

/* Produce DECL_RTL for object obj so it looks like it is stored in memory.  */
static rtx
produce_memory_decl_rtl (tree obj, int *regno)
{
  rtx x;
  
  gcc_assert (obj);
  if (TREE_STATIC (obj) || DECL_EXTERNAL (obj))
    {
      const char *name = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (obj));
      x = gen_rtx_SYMBOL_REF (Pmode, name);
      SET_SYMBOL_REF_DECL (x, obj);
      x = gen_rtx_MEM (DECL_MODE (obj), x);
      targetm.encode_section_info (obj, x, true);
    }
  else
    {
      x = gen_raw_REG (Pmode, (*regno)++);
      x = gen_rtx_MEM (DECL_MODE (obj), x);
    }

  return x;
}

/* Prepares decl_rtl for variables referred in *EXPR_P.  Callback for
   walk_tree.  DATA contains the actual fake register number.  */

static tree
prepare_decl_rtl (tree *expr_p, int *ws, void *data)
{
  tree obj = NULL_TREE;
  rtx x = NULL_RTX;
  int *regno = (int *) data;

  switch (TREE_CODE (*expr_p))
    {
    case ADDR_EXPR:
      for (expr_p = &TREE_OPERAND (*expr_p, 0);
	   handled_component_p (*expr_p);
	   expr_p = &TREE_OPERAND (*expr_p, 0))
	continue;
      obj = *expr_p;
      if (DECL_P (obj) && !DECL_RTL_SET_P (obj))
        x = produce_memory_decl_rtl (obj, regno);
      break;

    case SSA_NAME:
      *ws = 0;
      obj = SSA_NAME_VAR (*expr_p);
      if (!DECL_RTL_SET_P (obj))
	x = gen_raw_REG (DECL_MODE (obj), (*regno)++);
      break;

    case VAR_DECL:
    case PARM_DECL:
    case RESULT_DECL:
      *ws = 0;
      obj = *expr_p;

      if (DECL_RTL_SET_P (obj))
	break;

      if (DECL_MODE (obj) == BLKmode)
	x = produce_memory_decl_rtl (obj, regno);
      else
	x = gen_raw_REG (DECL_MODE (obj), (*regno)++);

      break;

    default:
      break;
    }

  if (x)
    {
      VEC_safe_push (tree, heap, decl_rtl_to_reset, obj);
      SET_DECL_RTL (obj, x);
    }

  return NULL_TREE;
}

/* Determines cost of the computation of EXPR.  */

static unsigned
computation_cost (tree expr, bool speed)
{
  rtx seq, rslt;
  tree type = TREE_TYPE (expr);
  unsigned cost;
  /* Avoid using hard regs in ways which may be unsupported.  */
  int regno = LAST_VIRTUAL_REGISTER + 1;
  enum function_frequency real_frequency = cfun->function_frequency;

  cfun->function_frequency = FUNCTION_FREQUENCY_NORMAL;
  crtl->maybe_hot_insn_p = speed;
  walk_tree (&expr, prepare_decl_rtl, &regno, NULL);
  start_sequence ();
  rslt = expand_expr (expr, NULL_RTX, TYPE_MODE (type), EXPAND_NORMAL);
  seq = get_insns ();
  end_sequence ();
  default_rtl_profile ();
  cfun->function_frequency = real_frequency;

  cost = seq_cost (seq, speed);
  if (MEM_P (rslt))
    cost += address_cost (XEXP (rslt, 0), TYPE_MODE (type), speed);

  return cost;
}

/* Returns variable containing the value of candidate CAND at statement AT.  */

static tree
var_at_stmt (struct loop *loop, struct iv_cand *cand, gimple stmt)
{
  if (stmt_after_increment (loop, cand, stmt))
    return cand->var_after;
  else
    return cand->var_before;
}

/* Return the most significant (sign) bit of T.  Similar to tree_int_cst_msb,
   but the bit is determined from TYPE_PRECISION, not MODE_BITSIZE.  */

int
tree_int_cst_sign_bit (const_tree t)
{
  unsigned bitno = TYPE_PRECISION (TREE_TYPE (t)) - 1;
  unsigned HOST_WIDE_INT w;

  if (bitno < HOST_BITS_PER_WIDE_INT)
    w = TREE_INT_CST_LOW (t);
  else
    {
      w = TREE_INT_CST_HIGH (t);
      bitno -= HOST_BITS_PER_WIDE_INT;
    }

  return (w >> bitno) & 1;
}

/* If A is (TYPE) BA and B is (TYPE) BB, and the types of BA and BB have the
   same precision that is at least as wide as the precision of TYPE, stores
   BA to A and BB to B, and returns the type of BA.  Otherwise, returns the
   type of A and B.  */

static tree
determine_common_wider_type (tree *a, tree *b)
{
  tree wider_type = NULL;
  tree suba, subb;
  tree atype = TREE_TYPE (*a);

  if (CONVERT_EXPR_P (*a))
    {
      suba = TREE_OPERAND (*a, 0);
      wider_type = TREE_TYPE (suba);
      if (TYPE_PRECISION (wider_type) < TYPE_PRECISION (atype))
	return atype;
    }
  else
    return atype;

  if (CONVERT_EXPR_P (*b))
    {
      subb = TREE_OPERAND (*b, 0);
      if (TYPE_PRECISION (wider_type) != TYPE_PRECISION (TREE_TYPE (subb)))
	return atype;
    }
  else
    return atype;

  *a = suba;
  *b = subb;
  return wider_type;
}

/* Determines the expression by that USE is expressed from induction variable
   CAND at statement AT in LOOP.  The expression is stored in a decomposed
   form into AFF.  Returns false if USE cannot be expressed using CAND.  */

static bool
get_computation_aff (struct loop *loop,
		     struct iv_use *use, struct iv_cand *cand, gimple at,
		     struct affine_tree_combination *aff)
{
  tree ubase = use->iv->base;
  tree ustep = use->iv->step;
  tree cbase = cand->iv->base;
  tree cstep = cand->iv->step, cstep_common;
  tree utype = TREE_TYPE (ubase), ctype = TREE_TYPE (cbase);
  tree common_type, var;
  tree uutype;
  aff_tree cbase_aff, var_aff;
  double_int rat;

  if (TYPE_PRECISION (utype) > TYPE_PRECISION (ctype))
    {
      /* We do not have a precision to express the values of use.  */
      return false;
    }

  var = var_at_stmt (loop, cand, at);
  uutype = unsigned_type_for (utype);

  /* If the conversion is not noop, perform it.  */
  if (TYPE_PRECISION (utype) < TYPE_PRECISION (ctype))
    {
      cstep = fold_convert (uutype, cstep);
      cbase = fold_convert (uutype, cbase);
      var = fold_convert (uutype, var);
    }

  if (!constant_multiple_of (ustep, cstep, &rat))
    return false;

  /* In case both UBASE and CBASE are shortened to UUTYPE from some common
     type, we achieve better folding by computing their difference in this
     wider type, and cast the result to UUTYPE.  We do not need to worry about
     overflows, as all the arithmetics will in the end be performed in UUTYPE
     anyway.  */
  common_type = determine_common_wider_type (&ubase, &cbase);

  /* use = ubase - ratio * cbase + ratio * var.  */
  tree_to_aff_combination (ubase, common_type, aff);
  tree_to_aff_combination (cbase, common_type, &cbase_aff);
  tree_to_aff_combination (var, uutype, &var_aff);

  /* We need to shift the value if we are after the increment.  */
  if (stmt_after_increment (loop, cand, at))
    {
      aff_tree cstep_aff;
  
      if (common_type != uutype)
	cstep_common = fold_convert (common_type, cstep);
      else
	cstep_common = cstep;

      tree_to_aff_combination (cstep_common, common_type, &cstep_aff);
      aff_combination_add (&cbase_aff, &cstep_aff);
    }

  aff_combination_scale (&cbase_aff, double_int_neg (rat));
  aff_combination_add (aff, &cbase_aff);
  if (common_type != uutype)
    aff_combination_convert (aff, uutype);

  aff_combination_scale (&var_aff, rat);
  aff_combination_add (aff, &var_aff);

  return true;
}

/* Determines the expression by that USE is expressed from induction variable
   CAND at statement AT in LOOP.  The computation is unshared.  */

static tree
get_computation_at (struct loop *loop,
		    struct iv_use *use, struct iv_cand *cand, gimple at)
{
  aff_tree aff;
  tree type = TREE_TYPE (use->iv->base);

  if (!get_computation_aff (loop, use, cand, at, &aff))
    return NULL_TREE;
  unshare_aff_combination (&aff);
  return fold_convert (type, aff_combination_to_tree (&aff));
}

/* Determines the expression by that USE is expressed from induction variable
   CAND in LOOP.  The computation is unshared.  */

static tree
get_computation (struct loop *loop, struct iv_use *use, struct iv_cand *cand)
{
  return get_computation_at (loop, use, cand, use->stmt);
}

/* Returns cost of addition in MODE.  */

static unsigned
add_cost (enum machine_mode mode, bool speed)
{
  static unsigned costs[NUM_MACHINE_MODES];
  rtx seq;
  unsigned cost;

  if (costs[mode])
    return costs[mode];

  start_sequence ();
  force_operand (gen_rtx_fmt_ee (PLUS, mode,
				 gen_raw_REG (mode, LAST_VIRTUAL_REGISTER + 1),
				 gen_raw_REG (mode, LAST_VIRTUAL_REGISTER + 2)),
		 NULL_RTX);
  seq = get_insns ();
  end_sequence ();

  cost = seq_cost (seq, speed);
  if (!cost)
    cost = 1;

  costs[mode] = cost;
      
  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "Addition in %s costs %d\n",
	     GET_MODE_NAME (mode), cost);
  return cost;
}

/* Entry in a hashtable of already known costs for multiplication.  */
struct mbc_entry
{
  HOST_WIDE_INT cst;		/* The constant to multiply by.  */
  enum machine_mode mode;	/* In mode.  */
  unsigned cost;		/* The cost.  */
};

/* Counts hash value for the ENTRY.  */

static hashval_t
mbc_entry_hash (const void *entry)
{
  const struct mbc_entry *e = (const struct mbc_entry *) entry;

  return 57 * (hashval_t) e->mode + (hashval_t) (e->cst % 877);
}

/* Compares the hash table entries ENTRY1 and ENTRY2.  */

static int
mbc_entry_eq (const void *entry1, const void *entry2)
{
  const struct mbc_entry *e1 = (const struct mbc_entry *) entry1;
  const struct mbc_entry *e2 = (const struct mbc_entry *) entry2;

  return (e1->mode == e2->mode
	  && e1->cst == e2->cst);
}

/* Returns cost of multiplication by constant CST in MODE.  */

unsigned
multiply_by_cost (HOST_WIDE_INT cst, enum machine_mode mode, bool speed)
{
  static htab_t costs;
  struct mbc_entry **cached, act;
  rtx seq;
  unsigned cost;

  if (!costs)
    costs = htab_create (100, mbc_entry_hash, mbc_entry_eq, free);

  act.mode = mode;
  act.cst = cst;
  cached = (struct mbc_entry **) htab_find_slot (costs, &act, INSERT);
  if (*cached)
    return (*cached)->cost;

  *cached = XNEW (struct mbc_entry);
  (*cached)->mode = mode;
  (*cached)->cst = cst;

  start_sequence ();
  expand_mult (mode, gen_raw_REG (mode, LAST_VIRTUAL_REGISTER + 1),
	       gen_int_mode (cst, mode), NULL_RTX, 0);
  seq = get_insns ();
  end_sequence ();
  
  cost = seq_cost (seq, speed);

  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "Multiplication by %d in %s costs %d\n",
	     (int) cst, GET_MODE_NAME (mode), cost);

  (*cached)->cost = cost;

  return cost;
}

/* Returns true if multiplying by RATIO is allowed in an address.  Test the
   validity for a memory reference accessing memory of mode MODE.  */

bool
multiplier_allowed_in_address_p (HOST_WIDE_INT ratio, enum machine_mode mode)
{
#define MAX_RATIO 128
  static sbitmap valid_mult[MAX_MACHINE_MODE];
  
  if (!valid_mult[mode])
    {
      rtx reg1 = gen_raw_REG (Pmode, LAST_VIRTUAL_REGISTER + 1);
      rtx addr;
      HOST_WIDE_INT i;

      valid_mult[mode] = sbitmap_alloc (2 * MAX_RATIO + 1);
      sbitmap_zero (valid_mult[mode]);
      addr = gen_rtx_fmt_ee (MULT, Pmode, reg1, NULL_RTX);
      for (i = -MAX_RATIO; i <= MAX_RATIO; i++)
	{
	  XEXP (addr, 1) = gen_int_mode (i, Pmode);
	  if (memory_address_p (mode, addr))
	    SET_BIT (valid_mult[mode], i + MAX_RATIO);
	}

      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "  allowed multipliers:");
	  for (i = -MAX_RATIO; i <= MAX_RATIO; i++)
	    if (TEST_BIT (valid_mult[mode], i + MAX_RATIO))
	      fprintf (dump_file, " %d", (int) i);
	  fprintf (dump_file, "\n");
	  fprintf (dump_file, "\n");
	}
    }

  if (ratio > MAX_RATIO || ratio < -MAX_RATIO)
    return false;

  return TEST_BIT (valid_mult[mode], ratio + MAX_RATIO);
}

/* Returns cost of address in shape symbol + var + OFFSET + RATIO * index.
   If SYMBOL_PRESENT is false, symbol is omitted.  If VAR_PRESENT is false,
   variable is omitted.  Compute the cost for a memory reference that accesses
   a memory location of mode MEM_MODE.

   TODO -- there must be some better way.  This all is quite crude.  */

static comp_cost
get_address_cost (bool symbol_present, bool var_present,
		  unsigned HOST_WIDE_INT offset, HOST_WIDE_INT ratio,
		  enum machine_mode mem_mode,
		  bool speed)
{
  static bool initialized[MAX_MACHINE_MODE];
  static HOST_WIDE_INT rat[MAX_MACHINE_MODE], off[MAX_MACHINE_MODE];
  static HOST_WIDE_INT min_offset[MAX_MACHINE_MODE], max_offset[MAX_MACHINE_MODE];
  static unsigned costs[MAX_MACHINE_MODE][2][2][2][2];
  unsigned cost, acost, complexity;
  bool offset_p, ratio_p;
  HOST_WIDE_INT s_offset;
  unsigned HOST_WIDE_INT mask;
  unsigned bits;

  if (!initialized[mem_mode])
    {
      HOST_WIDE_INT i;
      HOST_WIDE_INT start = BIGGEST_ALIGNMENT / BITS_PER_UNIT;
      int old_cse_not_expected;
      unsigned sym_p, var_p, off_p, rat_p, add_c;
      rtx seq, addr, base;
      rtx reg0, reg1;

      initialized[mem_mode] = true;

      reg1 = gen_raw_REG (Pmode, LAST_VIRTUAL_REGISTER + 1);

      addr = gen_rtx_fmt_ee (PLUS, Pmode, reg1, NULL_RTX);
      for (i = start; i <= 1 << 20; i <<= 1)
	{
	  XEXP (addr, 1) = gen_int_mode (i, Pmode);
	  if (!memory_address_p (mem_mode, addr))
	    break;
	}
      max_offset[mem_mode] = i == start ? 0 : i >> 1;
      off[mem_mode] = max_offset[mem_mode];

      for (i = start; i <= 1 << 20; i <<= 1)
	{
	  XEXP (addr, 1) = gen_int_mode (-i, Pmode);
	  if (!memory_address_p (mem_mode, addr))
	    break;
	}
      min_offset[mem_mode] = i == start ? 0 : -(i >> 1);

      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "get_address_cost:\n");
	  fprintf (dump_file, "  min offset %s %d\n",
		   GET_MODE_NAME (mem_mode),
		   (int) min_offset[mem_mode]);
	  fprintf (dump_file, "  max offset %s %d\n",
		   GET_MODE_NAME (mem_mode),
		   (int) max_offset[mem_mode]);
	}

      rat[mem_mode] = 1;
      for (i = 2; i <= MAX_RATIO; i++)
	if (multiplier_allowed_in_address_p (i, mem_mode))
	  {
	    rat[mem_mode] = i;
	    break;
	  }

      /* Compute the cost of various addressing modes.  */
      acost = 0;
      reg0 = gen_raw_REG (Pmode, LAST_VIRTUAL_REGISTER + 1);
      reg1 = gen_raw_REG (Pmode, LAST_VIRTUAL_REGISTER + 2);

      for (i = 0; i < 16; i++)
	{
	  sym_p = i & 1;
	  var_p = (i >> 1) & 1;
	  off_p = (i >> 2) & 1;
	  rat_p = (i >> 3) & 1;

	  addr = reg0;
	  if (rat_p)
	    addr = gen_rtx_fmt_ee (MULT, Pmode, addr,
				   gen_int_mode (rat[mem_mode], Pmode));

	  if (var_p)
	    addr = gen_rtx_fmt_ee (PLUS, Pmode, addr, reg1);

	  if (sym_p)
	    {
	      base = gen_rtx_SYMBOL_REF (Pmode, ggc_strdup (""));
	      /* ??? We can run into trouble with some backends by presenting
		 it with symbols which haven't been properly passed through
		 targetm.encode_section_info.  By setting the local bit, we
		 enhance the probability of things working.  */
	      SYMBOL_REF_FLAGS (base) = SYMBOL_FLAG_LOCAL;

	      if (off_p)
		base = gen_rtx_fmt_e (CONST, Pmode,
				      gen_rtx_fmt_ee (PLUS, Pmode,
						      base,
						      gen_int_mode (off[mem_mode],
								    Pmode)));
	    }
	  else if (off_p)
	    base = gen_int_mode (off[mem_mode], Pmode);
	  else
	    base = NULL_RTX;
    
	  if (base)
	    addr = gen_rtx_fmt_ee (PLUS, Pmode, addr, base);
  
	  start_sequence ();
	  /* To avoid splitting addressing modes, pretend that no cse will
	     follow.  */
	  old_cse_not_expected = cse_not_expected;
	  cse_not_expected = true;
	  addr = memory_address (mem_mode, addr);
	  cse_not_expected = old_cse_not_expected;
	  seq = get_insns ();
	  end_sequence ();

	  acost = seq_cost (seq, speed);
	  acost += address_cost (addr, mem_mode, speed);

	  if (!acost)
	    acost = 1;
	  costs[mem_mode][sym_p][var_p][off_p][rat_p] = acost;
	}

      /* On some targets, it is quite expensive to load symbol to a register,
	 which makes addresses that contain symbols look much more expensive.
	 However, the symbol will have to be loaded in any case before the
	 loop (and quite likely we have it in register already), so it does not
	 make much sense to penalize them too heavily.  So make some final
         tweaks for the SYMBOL_PRESENT modes:

         If VAR_PRESENT is false, and the mode obtained by changing symbol to
	 var is cheaper, use this mode with small penalty.
	 If VAR_PRESENT is true, try whether the mode with
	 SYMBOL_PRESENT = false is cheaper even with cost of addition, and
	 if this is the case, use it.  */
      add_c = add_cost (Pmode, speed);
      for (i = 0; i < 8; i++)
	{
	  var_p = i & 1;
	  off_p = (i >> 1) & 1;
	  rat_p = (i >> 2) & 1;

	  acost = costs[mem_mode][0][1][off_p][rat_p] + 1;
	  if (var_p)
	    acost += add_c;

	  if (acost < costs[mem_mode][1][var_p][off_p][rat_p])
	    costs[mem_mode][1][var_p][off_p][rat_p] = acost;
	}
  
      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "Address costs:\n");
      
	  for (i = 0; i < 16; i++)
	    {
	      sym_p = i & 1;
	      var_p = (i >> 1) & 1;
	      off_p = (i >> 2) & 1;
	      rat_p = (i >> 3) & 1;

	      fprintf (dump_file, "  ");
	      if (sym_p)
		fprintf (dump_file, "sym + ");
	      if (var_p)
		fprintf (dump_file, "var + ");
	      if (off_p)
		fprintf (dump_file, "cst + ");
	      if (rat_p)
		fprintf (dump_file, "rat * ");

	      acost = costs[mem_mode][sym_p][var_p][off_p][rat_p];
	      fprintf (dump_file, "index costs %d\n", acost);
	    }
	  fprintf (dump_file, "\n");
	}
    }

  bits = GET_MODE_BITSIZE (Pmode);
  mask = ~(~(unsigned HOST_WIDE_INT) 0 << (bits - 1) << 1);
  offset &= mask;
  if ((offset >> (bits - 1) & 1))
    offset |= ~mask;
  s_offset = offset;

  cost = 0;
  offset_p = (s_offset != 0
	      && min_offset[mem_mode] <= s_offset
	      && s_offset <= max_offset[mem_mode]);
  ratio_p = (ratio != 1
	     && multiplier_allowed_in_address_p (ratio, mem_mode));

  if (ratio != 1 && !ratio_p)
    cost += multiply_by_cost (ratio, Pmode, speed);

  if (s_offset && !offset_p && !symbol_present)
    cost += add_cost (Pmode, speed);

  acost = costs[mem_mode][symbol_present][var_present][offset_p][ratio_p];
  complexity = (symbol_present != 0) + (var_present != 0) + offset_p + ratio_p;
  return new_cost (cost + acost, complexity);
}

/* Estimates cost of forcing expression EXPR into a variable.  */

static comp_cost
force_expr_to_var_cost (tree expr, bool speed)
{
  static bool costs_initialized = false;
  static unsigned integer_cost [2];
  static unsigned symbol_cost [2];
  static unsigned address_cost [2];
  tree op0, op1;
  comp_cost cost0, cost1, cost;
  enum machine_mode mode;

  if (!costs_initialized)
    {
      tree type = build_pointer_type (integer_type_node);
      tree var, addr;
      rtx x;
      int i;

      var = create_tmp_var_raw (integer_type_node, "test_var");
      TREE_STATIC (var) = 1;
      x = produce_memory_decl_rtl (var, NULL);
      SET_DECL_RTL (var, x);

      addr = build1 (ADDR_EXPR, type, var);


      for (i = 0; i < 2; i++)
	{
	  integer_cost[i] = computation_cost (build_int_cst (integer_type_node,
							     2000), i);

	  symbol_cost[i] = computation_cost (addr, i) + 1;

	  address_cost[i]
	    = computation_cost (build2 (POINTER_PLUS_EXPR, type,
					addr,
					build_int_cst (sizetype, 2000)), i) + 1;
	  if (dump_file && (dump_flags & TDF_DETAILS))
	    {
	      fprintf (dump_file, "force_expr_to_var_cost %s costs:\n", i ? "speed" : "size");
	      fprintf (dump_file, "  integer %d\n", (int) integer_cost[i]);
	      fprintf (dump_file, "  symbol %d\n", (int) symbol_cost[i]);
	      fprintf (dump_file, "  address %d\n", (int) address_cost[i]);
	      fprintf (dump_file, "  other %d\n", (int) target_spill_cost[i]);
	      fprintf (dump_file, "\n");
	    }
	}

      costs_initialized = true;
    }

  STRIP_NOPS (expr);

  if (SSA_VAR_P (expr))
    return zero_cost;

  if (is_gimple_min_invariant (expr))
    {
      if (TREE_CODE (expr) == INTEGER_CST)
	return new_cost (integer_cost [speed], 0);

      if (TREE_CODE (expr) == ADDR_EXPR)
	{
	  tree obj = TREE_OPERAND (expr, 0);

	  if (TREE_CODE (obj) == VAR_DECL
	      || TREE_CODE (obj) == PARM_DECL
	      || TREE_CODE (obj) == RESULT_DECL)
	    return new_cost (symbol_cost [speed], 0);
	}

      return new_cost (address_cost [speed], 0);
    }

  switch (TREE_CODE (expr))
    {
    case POINTER_PLUS_EXPR:
    case PLUS_EXPR:
    case MINUS_EXPR:
    case MULT_EXPR:
      op0 = TREE_OPERAND (expr, 0);
      op1 = TREE_OPERAND (expr, 1);
      STRIP_NOPS (op0);
      STRIP_NOPS (op1);

      if (is_gimple_val (op0))
	cost0 = zero_cost;
      else
	cost0 = force_expr_to_var_cost (op0, speed);

      if (is_gimple_val (op1))
	cost1 = zero_cost;
      else
	cost1 = force_expr_to_var_cost (op1, speed);

      break;

    default:
      /* Just an arbitrary value, FIXME.  */
      return new_cost (target_spill_cost[speed], 0);
    }

  mode = TYPE_MODE (TREE_TYPE (expr));
  switch (TREE_CODE (expr))
    {
    case POINTER_PLUS_EXPR:
    case PLUS_EXPR:
    case MINUS_EXPR:
      cost = new_cost (add_cost (mode, speed), 0);
      break;

    case MULT_EXPR:
      if (cst_and_fits_in_hwi (op0))
	cost = new_cost (multiply_by_cost (int_cst_value (op0), mode, speed), 0);
      else if (cst_and_fits_in_hwi (op1))                                  
	cost = new_cost (multiply_by_cost (int_cst_value (op1), mode, speed), 0);
      else
	return new_cost (target_spill_cost [speed], 0);
      break;

    default:
      gcc_unreachable ();
    }

  cost = add_costs (cost, cost0);
  cost = add_costs (cost, cost1);

  /* Bound the cost by target_spill_cost.  The parts of complicated
     computations often are either loop invariant or at least can
     be shared between several iv uses, so letting this grow without
     limits would not give reasonable results.  */
  if (cost.cost > target_spill_cost [speed])
    cost.cost = target_spill_cost [speed];

  return cost;
}

/* Estimates cost of forcing EXPR into a variable.  DEPENDS_ON is a set of the
   invariants the computation depends on.  */

static comp_cost
force_var_cost (struct ivopts_data *data,
		tree expr, bitmap *depends_on)
{
  if (depends_on)
    {
      fd_ivopts_data = data;
      walk_tree (&expr, find_depends, depends_on, NULL);
    }

  return force_expr_to_var_cost (expr, data->speed);
}

/* Estimates cost of expressing address ADDR  as var + symbol + offset.  The
   value of offset is added to OFFSET, SYMBOL_PRESENT and VAR_PRESENT are set
   to false if the corresponding part is missing.  DEPENDS_ON is a set of the
   invariants the computation depends on.  */

static comp_cost
split_address_cost (struct ivopts_data *data,
		    tree addr, bool *symbol_present, bool *var_present,
		    unsigned HOST_WIDE_INT *offset, bitmap *depends_on)
{
  tree core;
  HOST_WIDE_INT bitsize;
  HOST_WIDE_INT bitpos;
  tree toffset;
  enum machine_mode mode;
  int unsignedp, volatilep;
  
  core = get_inner_reference (addr, &bitsize, &bitpos, &toffset, &mode,
			      &unsignedp, &volatilep, false);

  if (toffset != 0
      || bitpos % BITS_PER_UNIT != 0
      || TREE_CODE (core) != VAR_DECL)
    {
      *symbol_present = false;
      *var_present = true;
      fd_ivopts_data = data;
      walk_tree (&addr, find_depends, depends_on, NULL);
      return new_cost (target_spill_cost[data->speed], 0);
    }

  *offset += bitpos / BITS_PER_UNIT;
  if (TREE_STATIC (core)
      || DECL_EXTERNAL (core))
    {
      *symbol_present = true;
      *var_present = false;
      return zero_cost;
    }
      
  *symbol_present = false;
  *var_present = true;
  return zero_cost;
}

/* Estimates cost of expressing difference of addresses E1 - E2 as
   var + symbol + offset.  The value of offset is added to OFFSET,
   SYMBOL_PRESENT and VAR_PRESENT are set to false if the corresponding
   part is missing.  DEPENDS_ON is a set of the invariants the computation
   depends on.  */

static comp_cost
ptr_difference_cost (struct ivopts_data *data,
		     tree e1, tree e2, bool *symbol_present, bool *var_present,
		     unsigned HOST_WIDE_INT *offset, bitmap *depends_on)
{
  HOST_WIDE_INT diff = 0;
  comp_cost cost;
  bool speed = optimize_loop_for_speed_p (data->current_loop);

  gcc_assert (TREE_CODE (e1) == ADDR_EXPR);

  if (ptr_difference_const (e1, e2, &diff))
    {
      *offset += diff;
      *symbol_present = false;
      *var_present = false;
      return zero_cost;
    }

  if (integer_zerop (e2))
    return split_address_cost (data, TREE_OPERAND (e1, 0),
			       symbol_present, var_present, offset, depends_on);

  *symbol_present = false;
  *var_present = true;
  
  cost = force_var_cost (data, e1, depends_on);
  cost = add_costs (cost, force_var_cost (data, e2, depends_on));
  cost.cost += add_cost (Pmode, speed);

  return cost;
}

/* Estimates cost of expressing difference E1 - E2 as
   var + symbol + offset.  The value of offset is added to OFFSET,
   SYMBOL_PRESENT and VAR_PRESENT are set to false if the corresponding
   part is missing.  DEPENDS_ON is a set of the invariants the computation
   depends on.  */

static comp_cost
difference_cost (struct ivopts_data *data,
		 tree e1, tree e2, bool *symbol_present, bool *var_present,
		 unsigned HOST_WIDE_INT *offset, bitmap *depends_on)
{
  comp_cost cost;
  enum machine_mode mode = TYPE_MODE (TREE_TYPE (e1));
  unsigned HOST_WIDE_INT off1, off2;

  e1 = strip_offset (e1, &off1);
  e2 = strip_offset (e2, &off2);
  *offset += off1 - off2;

  STRIP_NOPS (e1);
  STRIP_NOPS (e2);

  if (TREE_CODE (e1) == ADDR_EXPR)
    return ptr_difference_cost (data, e1, e2, symbol_present, var_present, offset,
				depends_on);
  *symbol_present = false;

  if (operand_equal_p (e1, e2, 0))
    {
      *var_present = false;
      return zero_cost;
    }
  *var_present = true;
  if (integer_zerop (e2))
    return force_var_cost (data, e1, depends_on);

  if (integer_zerop (e1))
    {
      cost = force_var_cost (data, e2, depends_on);
      cost.cost += multiply_by_cost (-1, mode, data->speed);

      return cost;
    }

  cost = force_var_cost (data, e1, depends_on);
  cost = add_costs (cost, force_var_cost (data, e2, depends_on));
  cost.cost += add_cost (mode, data->speed);

  return cost;
}

/* Determines the cost of the computation by that USE is expressed
   from induction variable CAND.  If ADDRESS_P is true, we just need
   to create an address from it, otherwise we want to get it into
   register.  A set of invariants we depend on is stored in
   DEPENDS_ON.  AT is the statement at that the value is computed.  */

static comp_cost
get_computation_cost_at (struct ivopts_data *data,
			 struct iv_use *use, struct iv_cand *cand,
			 bool address_p, bitmap *depends_on, gimple at)
{
  tree ubase = use->iv->base, ustep = use->iv->step;
  tree cbase, cstep;
  tree utype = TREE_TYPE (ubase), ctype;
  unsigned HOST_WIDE_INT cstepi, offset = 0;
  HOST_WIDE_INT ratio, aratio;
  bool var_present, symbol_present;
  comp_cost cost;
  unsigned n_sums;
  double_int rat;
  bool speed = optimize_bb_for_speed_p (gimple_bb (at));

  *depends_on = NULL;

  /* Only consider real candidates.  */
  if (!cand->iv)
    return infinite_cost;

  cbase = cand->iv->base;
  cstep = cand->iv->step;
  ctype = TREE_TYPE (cbase);

  if (TYPE_PRECISION (utype) > TYPE_PRECISION (ctype))
    {
      /* We do not have a precision to express the values of use.  */
      return infinite_cost;
    }

  if (address_p)
    {
      /* Do not try to express address of an object with computation based
	 on address of a different object.  This may cause problems in rtl
	 level alias analysis (that does not expect this to be happening,
	 as this is illegal in C), and would be unlikely to be useful
	 anyway.  */
      if (use->iv->base_object
	  && cand->iv->base_object
	  && !operand_equal_p (use->iv->base_object, cand->iv->base_object, 0))
	return infinite_cost;
    }

  if (TYPE_PRECISION (utype) != TYPE_PRECISION (ctype))
    {
      /* TODO -- add direct handling of this case.  */
      goto fallback;
    }

  /* CSTEPI is removed from the offset in case statement is after the
     increment.  If the step is not constant, we use zero instead.
     This is a bit imprecise (there is the extra addition), but
     redundancy elimination is likely to transform the code so that
     it uses value of the variable before increment anyway,
     so it is not that much unrealistic.  */
  if (cst_and_fits_in_hwi (cstep))
    cstepi = int_cst_value (cstep);
  else
    cstepi = 0;

  if (!constant_multiple_of (ustep, cstep, &rat))
    return infinite_cost;
    
  if (double_int_fits_in_shwi_p (rat))
    ratio = double_int_to_shwi (rat);
  else
    return infinite_cost;

  /* use = ubase + ratio * (var - cbase).  If either cbase is a constant
     or ratio == 1, it is better to handle this like
     
     ubase - ratio * cbase + ratio * var
     
     (also holds in the case ratio == -1, TODO.  */

  if (cst_and_fits_in_hwi (cbase))
    {
      offset = - ratio * int_cst_value (cbase); 
      cost = difference_cost (data,
			      ubase, build_int_cst (utype, 0),
			      &symbol_present, &var_present, &offset,
			      depends_on);
    }
  else if (ratio == 1)
    {
      cost = difference_cost (data,
			      ubase, cbase,
			      &symbol_present, &var_present, &offset,
			      depends_on);
    }
  else
    {
      cost = force_var_cost (data, cbase, depends_on);
      cost.cost += add_cost (TYPE_MODE (ctype), data->speed);
      cost = add_costs (cost,
			difference_cost (data,
					 ubase, build_int_cst (utype, 0),
					 &symbol_present, &var_present,
					 &offset, depends_on));
    }

  /* If we are after the increment, the value of the candidate is higher by
     one iteration.  */
  if (stmt_after_increment (data->current_loop, cand, at))
    offset -= ratio * cstepi;

  /* Now the computation is in shape symbol + var1 + const + ratio * var2.
     (symbol/var/const parts may be omitted).  If we are looking for an address,
     find the cost of addressing this.  */
  if (address_p)
    return add_costs (cost, get_address_cost (symbol_present, var_present,
				offset, ratio,
				TYPE_MODE (TREE_TYPE (*use->op_p)), speed));

  /* Otherwise estimate the costs for computing the expression.  */
  aratio = ratio > 0 ? ratio : -ratio;
  if (!symbol_present && !var_present && !offset)
    {
      if (ratio != 1)
	cost.cost += multiply_by_cost (ratio, TYPE_MODE (ctype), speed);

      return cost;
    }

  if (aratio != 1)
    cost.cost += multiply_by_cost (aratio, TYPE_MODE (ctype), speed);

  n_sums = 1;
  if (var_present
      /* Symbol + offset should be compile-time computable.  */
      && (symbol_present || offset))
    n_sums++;

  /* Having offset does not affect runtime cost in case it is added to
     symbol, but it increases complexity.  */
  if (offset)
    cost.complexity++;

  cost.cost += n_sums * add_cost (TYPE_MODE (ctype), speed);
  return cost;

fallback:
  {
    /* Just get the expression, expand it and measure the cost.  */
    tree comp = get_computation_at (data->current_loop, use, cand, at);

    if (!comp)
      return infinite_cost;

    if (address_p)
      comp = build1 (INDIRECT_REF, TREE_TYPE (TREE_TYPE (comp)), comp);

    return new_cost (computation_cost (comp, speed), 0);
  }
}

/* Determines the cost of the computation by that USE is expressed
   from induction variable CAND.  If ADDRESS_P is true, we just need
   to create an address from it, otherwise we want to get it into
   register.  A set of invariants we depend on is stored in
   DEPENDS_ON.  */

static comp_cost
get_computation_cost (struct ivopts_data *data,
		      struct iv_use *use, struct iv_cand *cand,
		      bool address_p, bitmap *depends_on)
{
  return get_computation_cost_at (data,
				  use, cand, address_p, depends_on, use->stmt);
}

/* Determines cost of basing replacement of USE on CAND in a generic
   expression.  */

static bool
determine_use_iv_cost_generic (struct ivopts_data *data,
			       struct iv_use *use, struct iv_cand *cand)
{
  bitmap depends_on;
  comp_cost cost;

  /* The simple case first -- if we need to express value of the preserved
     original biv, the cost is 0.  This also prevents us from counting the
     cost of increment twice -- once at this use and once in the cost of
     the candidate.  */
  if (cand->pos == IP_ORIGINAL
      && cand->incremented_at == use->stmt)
    {
      set_use_iv_cost (data, use, cand, zero_cost, NULL, NULL_TREE);
      return true;
    }

  cost = get_computation_cost (data, use, cand, false, &depends_on);
  set_use_iv_cost (data, use, cand, cost, depends_on, NULL_TREE);

  return !infinite_cost_p (cost);
}

/* Determines cost of basing replacement of USE on CAND in an address.  */

static bool
determine_use_iv_cost_address (struct ivopts_data *data,
			       struct iv_use *use, struct iv_cand *cand)
{
  bitmap depends_on;
  comp_cost cost = get_computation_cost (data, use, cand, true, &depends_on);

  set_use_iv_cost (data, use, cand, cost, depends_on, NULL_TREE);

  return !infinite_cost_p (cost);
}

/* Computes value of candidate CAND at position AT in iteration NITER, and
   stores it to VAL.  */

static void
cand_value_at (struct loop *loop, struct iv_cand *cand, gimple at, tree niter,
	       aff_tree *val)
{
  aff_tree step, delta, nit;
  struct iv *iv = cand->iv;
  tree type = TREE_TYPE (iv->base);
  tree steptype = type;
  if (POINTER_TYPE_P (type))
    steptype = sizetype;

  tree_to_aff_combination (iv->step, steptype, &step);
  tree_to_aff_combination (niter, TREE_TYPE (niter), &nit);
  aff_combination_convert (&nit, steptype);
  aff_combination_mult (&nit, &step, &delta);
  if (stmt_after_increment (loop, cand, at))
    aff_combination_add (&delta, &step);

  tree_to_aff_combination (iv->base, type, val);
  aff_combination_add (val, &delta);
}

/* Returns period of induction variable iv.  */

static tree
iv_period (struct iv *iv)
{
  tree step = iv->step, period, type;
  tree pow2div;

  gcc_assert (step && TREE_CODE (step) == INTEGER_CST);

  /* Period of the iv is gcd (step, type range).  Since type range is power
     of two, it suffices to determine the maximum power of two that divides
     step.  */
  pow2div = num_ending_zeros (step);
  type = unsigned_type_for (TREE_TYPE (step));

  period = build_low_bits_mask (type,
				(TYPE_PRECISION (type)
				 - tree_low_cst (pow2div, 1)));

  return period;
}

/* Returns the comparison operator used when eliminating the iv USE.  */

static enum tree_code
iv_elimination_compare (struct ivopts_data *data, struct iv_use *use)
{
  struct loop *loop = data->current_loop;
  basic_block ex_bb;
  edge exit;

  ex_bb = gimple_bb (use->stmt);
  exit = EDGE_SUCC (ex_bb, 0);
  if (flow_bb_inside_loop_p (loop, exit->dest))
    exit = EDGE_SUCC (ex_bb, 1);

  return (exit->flags & EDGE_TRUE_VALUE ? EQ_EXPR : NE_EXPR);
}

/* Check whether it is possible to express the condition in USE by comparison
   of candidate CAND.  If so, store the value compared with to BOUND.  */

static bool
may_eliminate_iv (struct ivopts_data *data,
		  struct iv_use *use, struct iv_cand *cand, tree *bound)
{
  basic_block ex_bb;
  edge exit;
  tree nit, period;
  struct loop *loop = data->current_loop;
  aff_tree bnd;

  if (TREE_CODE (cand->iv->step) != INTEGER_CST)
    return false;

  /* For now works only for exits that dominate the loop latch.
     TODO: extend to other conditions inside loop body.  */
  ex_bb = gimple_bb (use->stmt);
  if (use->stmt != last_stmt (ex_bb)
      || gimple_code (use->stmt) != GIMPLE_COND
      || !dominated_by_p (CDI_DOMINATORS, loop->latch, ex_bb))
    return false;

  exit = EDGE_SUCC (ex_bb, 0);
  if (flow_bb_inside_loop_p (loop, exit->dest))
    exit = EDGE_SUCC (ex_bb, 1);
  if (flow_bb_inside_loop_p (loop, exit->dest))
    return false;

  nit = niter_for_exit (data, exit);
  if (!nit)
    return false;

  /* Determine whether we can use the variable to test the exit condition.
     This is the case iff the period of the induction variable is greater
     than the number of iterations for which the exit condition is true.  */
  period = iv_period (cand->iv);

  /* If the number of iterations is constant, compare against it directly.  */
  if (TREE_CODE (nit) == INTEGER_CST)
    {
      if (!tree_int_cst_lt (nit, period))
	return false;
    }

  /* If not, and if this is the only possible exit of the loop, see whether
     we can get a conservative estimate on the number of iterations of the
     entire loop and compare against that instead.  */
  else if (loop_only_exit_p (loop, exit))
    {
      double_int period_value, max_niter;
      if (!estimated_loop_iterations (loop, true, &max_niter))
	return false;
      period_value = tree_to_double_int (period);
      if (double_int_ucmp (max_niter, period_value) >= 0)
	return false;
    }

  /* Otherwise, punt.  */
  else
    return false;

  cand_value_at (loop, cand, use->stmt, nit, &bnd);

  *bound = aff_combination_to_tree (&bnd);
  /* It is unlikely that computing the number of iterations using division
     would be more profitable than keeping the original induction variable.  */
  if (expression_expensive_p (*bound))
    return false;
  return true;
}

/* Determines cost of basing replacement of USE on CAND in a condition.  */

static bool
determine_use_iv_cost_condition (struct ivopts_data *data,
				 struct iv_use *use, struct iv_cand *cand)
{
  tree bound = NULL_TREE;
  struct iv *cmp_iv;
  bitmap depends_on_elim = NULL, depends_on_express = NULL, depends_on;
  comp_cost elim_cost, express_cost, cost;
  bool ok;

  /* Only consider real candidates.  */
  if (!cand->iv)
    {
      set_use_iv_cost (data, use, cand, infinite_cost, NULL, NULL_TREE);
      return false;
    }

  /* Try iv elimination.  */
  if (may_eliminate_iv (data, use, cand, &bound))
    {
      elim_cost = force_var_cost (data, bound, &depends_on_elim);
      /* The bound is a loop invariant, so it will be only computed
	 once.  */
      elim_cost.cost /= AVG_LOOP_NITER (data->current_loop);
    }
  else
    elim_cost = infinite_cost;

  /* Try expressing the original giv.  If it is compared with an invariant,
     note that we cannot get rid of it.  */
  ok = extract_cond_operands (data, use->stmt, NULL, NULL, NULL, &cmp_iv);
  gcc_assert (ok);

  express_cost = get_computation_cost (data, use, cand, false,
				       &depends_on_express);
  fd_ivopts_data = data;
  walk_tree (&cmp_iv->base, find_depends, &depends_on_express, NULL);

  /* Choose the better approach, preferring the eliminated IV. */
  if (compare_costs (elim_cost, express_cost) <= 0)
    {
      cost = elim_cost;
      depends_on = depends_on_elim;
      depends_on_elim = NULL;
    }
  else
    {
      cost = express_cost;
      depends_on = depends_on_express;
      depends_on_express = NULL;
      bound = NULL_TREE;
    }

  set_use_iv_cost (data, use, cand, cost, depends_on, bound);

  if (depends_on_elim)
    BITMAP_FREE (depends_on_elim);
  if (depends_on_express)
    BITMAP_FREE (depends_on_express);

  return !infinite_cost_p (cost);
}

/* Determines cost of basing replacement of USE on CAND.  Returns false
   if USE cannot be based on CAND.  */

static bool
determine_use_iv_cost (struct ivopts_data *data,
		       struct iv_use *use, struct iv_cand *cand)
{
  switch (use->type)
    {
    case USE_NONLINEAR_EXPR:
      return determine_use_iv_cost_generic (data, use, cand);

    case USE_ADDRESS:
      return determine_use_iv_cost_address (data, use, cand);

    case USE_COMPARE:
      return determine_use_iv_cost_condition (data, use, cand);

    default:
      gcc_unreachable ();
    }
}

/* Determines costs of basing the use of the iv on an iv candidate.  */

static void
determine_use_iv_costs (struct ivopts_data *data)
{
  unsigned i, j;
  struct iv_use *use;
  struct iv_cand *cand;
  bitmap to_clear = BITMAP_ALLOC (NULL);

  alloc_use_cost_map (data);

  for (i = 0; i < n_iv_uses (data); i++)
    {
      use = iv_use (data, i);

      if (data->consider_all_candidates)
	{
	  for (j = 0; j < n_iv_cands (data); j++)
	    {
	      cand = iv_cand (data, j);
	      determine_use_iv_cost (data, use, cand);
	    }
	}
      else
	{
	  bitmap_iterator bi;

	  EXECUTE_IF_SET_IN_BITMAP (use->related_cands, 0, j, bi)
	    {
	      cand = iv_cand (data, j);
	      if (!determine_use_iv_cost (data, use, cand))
		bitmap_set_bit (to_clear, j);
	    }

	  /* Remove the candidates for that the cost is infinite from
	     the list of related candidates.  */
	  bitmap_and_compl_into (use->related_cands, to_clear);
	  bitmap_clear (to_clear);
	}
    }

  BITMAP_FREE (to_clear);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Use-candidate costs:\n");

      for (i = 0; i < n_iv_uses (data); i++)
	{
	  use = iv_use (data, i);

	  fprintf (dump_file, "Use %d:\n", i);
	  fprintf (dump_file, "  cand\tcost\tcompl.\tdepends on\n");
	  for (j = 0; j < use->n_map_members; j++)
	    {
	      if (!use->cost_map[j].cand
		  || infinite_cost_p (use->cost_map[j].cost))
		continue;

	      fprintf (dump_file, "  %d\t%d\t%d\t",
		       use->cost_map[j].cand->id,
		       use->cost_map[j].cost.cost,
		       use->cost_map[j].cost.complexity);
	      if (use->cost_map[j].depends_on)
		bitmap_print (dump_file,
			      use->cost_map[j].depends_on, "","");
	      fprintf (dump_file, "\n");
	    }

	  fprintf (dump_file, "\n");
	}
      fprintf (dump_file, "\n");
    }
}

/* Determines cost of the candidate CAND.  */

static void
determine_iv_cost (struct ivopts_data *data, struct iv_cand *cand)
{
  comp_cost cost_base;
  unsigned cost, cost_step;
  tree base;

  if (!cand->iv)
    {
      cand->cost = 0;
      return;
    }

  /* There are two costs associated with the candidate -- its increment
     and its initialization.  The second is almost negligible for any loop
     that rolls enough, so we take it just very little into account.  */

  base = cand->iv->base;
  cost_base = force_var_cost (data, base, NULL);
  cost_step = add_cost (TYPE_MODE (TREE_TYPE (base)), data->speed);

  cost = cost_step + cost_base.cost / AVG_LOOP_NITER (current_loop);

  /* Prefer the original ivs unless we may gain something by replacing it.
     The reason is to make debugging simpler; so this is not relevant for
     artificial ivs created by other optimization passes.  */
  if (cand->pos != IP_ORIGINAL
      || DECL_ARTIFICIAL (SSA_NAME_VAR (cand->var_before)))
    cost++;
  
  /* Prefer not to insert statements into latch unless there are some
     already (so that we do not create unnecessary jumps).  */
  if (cand->pos == IP_END
      && empty_block_p (ip_end_pos (data->current_loop)))
    cost++;

  cand->cost = cost;
}

/* Determines costs of computation of the candidates.  */

static void
determine_iv_costs (struct ivopts_data *data)
{
  unsigned i;

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Candidate costs:\n");
      fprintf (dump_file, "  cand\tcost\n");
    }

  for (i = 0; i < n_iv_cands (data); i++)
    {
      struct iv_cand *cand = iv_cand (data, i);

      determine_iv_cost (data, cand);

      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "  %d\t%d\n", i, cand->cost);
    }
  
  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "\n");
}

/* Calculates cost for having SIZE induction variables.  */

static unsigned
ivopts_global_cost_for_size (struct ivopts_data *data, unsigned size)
{
  /* We add size to the cost, so that we prefer eliminating ivs
     if possible.  */
  return size + estimate_reg_pressure_cost (size, data->regs_used, data->speed);
}

/* For each size of the induction variable set determine the penalty.  */

static void
determine_set_costs (struct ivopts_data *data)
{
  unsigned j, n;
  gimple phi;
  gimple_stmt_iterator psi;
  tree op;
  struct loop *loop = data->current_loop;
  bitmap_iterator bi;

  /* We use the following model (definitely improvable, especially the
     cost function -- TODO):

     We estimate the number of registers available (using MD data), name it A.

     We estimate the number of registers used by the loop, name it U.  This
     number is obtained as the number of loop phi nodes (not counting virtual
     registers and bivs) + the number of variables from outside of the loop.

     We set a reserve R (free regs that are used for temporary computations,
     etc.).  For now the reserve is a constant 3.

     Let I be the number of induction variables.
     
     -- if U + I + R <= A, the cost is I * SMALL_COST (just not to encourage
	make a lot of ivs without a reason).
     -- if A - R < U + I <= A, the cost is I * PRES_COST
     -- if U + I > A, the cost is I * PRES_COST and
        number of uses * SPILL_COST * (U + I - A) / (U + I) is added.  */

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Global costs:\n");
      fprintf (dump_file, "  target_avail_regs %d\n", target_avail_regs);
      fprintf (dump_file, "  target_reg_cost %d\n", target_reg_cost[data->speed]);
      fprintf (dump_file, "  target_spill_cost %d\n", target_spill_cost[data->speed]);
    }

  n = 0;
  for (psi = gsi_start_phis (loop->header); !gsi_end_p (psi); gsi_next (&psi))
    {
      phi = gsi_stmt (psi);
      op = PHI_RESULT (phi);

      if (!is_gimple_reg (op))
	continue;

      if (get_iv (data, op))
	continue;

      n++;
    }

  EXECUTE_IF_SET_IN_BITMAP (data->relevant, 0, j, bi)
    {
      struct version_info *info = ver_info (data, j);

      if (info->inv_id && info->has_nonlin_use)
	n++;
    }

  data->regs_used = n;
  if (dump_file && (dump_flags & TDF_DETAILS))
    fprintf (dump_file, "  regs_used %d\n", n);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "  cost for size:\n");
      fprintf (dump_file, "  ivs\tcost\n");
      for (j = 0; j <= 2 * target_avail_regs; j++)
	fprintf (dump_file, "  %d\t%d\n", j,
		 ivopts_global_cost_for_size (data, j));
      fprintf (dump_file, "\n");
    }
}

/* Returns true if A is a cheaper cost pair than B.  */

static bool
cheaper_cost_pair (struct cost_pair *a, struct cost_pair *b)
{
  int cmp;

  if (!a)
    return false;

  if (!b)
    return true;

  cmp = compare_costs (a->cost, b->cost);
  if (cmp < 0)
    return true;

  if (cmp > 0)
    return false;

  /* In case the costs are the same, prefer the cheaper candidate.  */
  if (a->cand->cost < b->cand->cost)
    return true;

  return false;
}

/* Computes the cost field of IVS structure.  */

static void
iv_ca_recount_cost (struct ivopts_data *data, struct iv_ca *ivs)
{
  comp_cost cost = ivs->cand_use_cost;
  cost.cost += ivs->cand_cost;
  cost.cost += ivopts_global_cost_for_size (data, ivs->n_regs);

  ivs->cost = cost;
}

/* Remove invariants in set INVS to set IVS.  */

static void
iv_ca_set_remove_invariants (struct iv_ca *ivs, bitmap invs)
{
  bitmap_iterator bi;
  unsigned iid;

  if (!invs)
    return;

  EXECUTE_IF_SET_IN_BITMAP (invs, 0, iid, bi)
    {
      ivs->n_invariant_uses[iid]--;
      if (ivs->n_invariant_uses[iid] == 0)
	ivs->n_regs--;
    }
}

/* Set USE not to be expressed by any candidate in IVS.  */

static void
iv_ca_set_no_cp (struct ivopts_data *data, struct iv_ca *ivs,
		 struct iv_use *use)
{
  unsigned uid = use->id, cid;
  struct cost_pair *cp;

  cp = ivs->cand_for_use[uid];
  if (!cp)
    return;
  cid = cp->cand->id;

  ivs->bad_uses++;
  ivs->cand_for_use[uid] = NULL;
  ivs->n_cand_uses[cid]--;

  if (ivs->n_cand_uses[cid] == 0)
    {
      bitmap_clear_bit (ivs->cands, cid);
      /* Do not count the pseudocandidates.  */
      if (cp->cand->iv)
	ivs->n_regs--;
      ivs->n_cands--;
      ivs->cand_cost -= cp->cand->cost;

      iv_ca_set_remove_invariants (ivs, cp->cand->depends_on);
    }

  ivs->cand_use_cost = sub_costs (ivs->cand_use_cost, cp->cost);

  iv_ca_set_remove_invariants (ivs, cp->depends_on);
  iv_ca_recount_cost (data, ivs);
}

/* Add invariants in set INVS to set IVS.  */

static void
iv_ca_set_add_invariants (struct iv_ca *ivs, bitmap invs)
{
  bitmap_iterator bi;
  unsigned iid;

  if (!invs)
    return;

  EXECUTE_IF_SET_IN_BITMAP (invs, 0, iid, bi)
    {
      ivs->n_invariant_uses[iid]++;
      if (ivs->n_invariant_uses[iid] == 1)
	ivs->n_regs++;
    }
}

/* Set cost pair for USE in set IVS to CP.  */

static void
iv_ca_set_cp (struct ivopts_data *data, struct iv_ca *ivs,
	      struct iv_use *use, struct cost_pair *cp)
{
  unsigned uid = use->id, cid;

  if (ivs->cand_for_use[uid] == cp)
    return;

  if (ivs->cand_for_use[uid])
    iv_ca_set_no_cp (data, ivs, use);

  if (cp)
    {
      cid = cp->cand->id;

      ivs->bad_uses--;
      ivs->cand_for_use[uid] = cp;
      ivs->n_cand_uses[cid]++;
      if (ivs->n_cand_uses[cid] == 1)
	{
	  bitmap_set_bit (ivs->cands, cid);
	  /* Do not count the pseudocandidates.  */
	  if (cp->cand->iv)
	    ivs->n_regs++;
	  ivs->n_cands++;
	  ivs->cand_cost += cp->cand->cost;

	  iv_ca_set_add_invariants (ivs, cp->cand->depends_on);
	}

      ivs->cand_use_cost = add_costs (ivs->cand_use_cost, cp->cost);
      iv_ca_set_add_invariants (ivs, cp->depends_on);
      iv_ca_recount_cost (data, ivs);
    }
}

/* Extend set IVS by expressing USE by some of the candidates in it
   if possible.  */

static void
iv_ca_add_use (struct ivopts_data *data, struct iv_ca *ivs,
	       struct iv_use *use)
{
  struct cost_pair *best_cp = NULL, *cp;
  bitmap_iterator bi;
  unsigned i;

  gcc_assert (ivs->upto >= use->id);

  if (ivs->upto == use->id)
    {
      ivs->upto++;
      ivs->bad_uses++;
    }

  EXECUTE_IF_SET_IN_BITMAP (ivs->cands, 0, i, bi)
    {
      cp = get_use_iv_cost (data, use, iv_cand (data, i));

      if (cheaper_cost_pair (cp, best_cp))
	best_cp = cp;
    }

  iv_ca_set_cp (data, ivs, use, best_cp);
}

/* Get cost for assignment IVS.  */

static comp_cost
iv_ca_cost (struct iv_ca *ivs)
{
  /* This was a conditional expression but it triggered a bug in
     Sun C 5.5.  */
  if (ivs->bad_uses)
    return infinite_cost;
  else
    return ivs->cost;
}

/* Returns true if all dependences of CP are among invariants in IVS.  */

static bool
iv_ca_has_deps (struct iv_ca *ivs, struct cost_pair *cp)
{
  unsigned i;
  bitmap_iterator bi;

  if (!cp->depends_on)
    return true;

  EXECUTE_IF_SET_IN_BITMAP (cp->depends_on, 0, i, bi)
    {
      if (ivs->n_invariant_uses[i] == 0)
	return false;
    }

  return true;
}

/* Creates change of expressing USE by NEW_CP instead of OLD_CP and chains
   it before NEXT_CHANGE.  */

static struct iv_ca_delta *
iv_ca_delta_add (struct iv_use *use, struct cost_pair *old_cp,
		 struct cost_pair *new_cp, struct iv_ca_delta *next_change)
{
  struct iv_ca_delta *change = XNEW (struct iv_ca_delta);

  change->use = use;
  change->old_cp = old_cp;
  change->new_cp = new_cp;
  change->next_change = next_change;

  return change;
}

/* Joins two lists of changes L1 and L2.  Destructive -- old lists
   are rewritten.  */

static struct iv_ca_delta *
iv_ca_delta_join (struct iv_ca_delta *l1, struct iv_ca_delta *l2)
{
  struct iv_ca_delta *last;

  if (!l2)
    return l1;

  if (!l1)
    return l2;

  for (last = l1; last->next_change; last = last->next_change)
    continue;
  last->next_change = l2;

  return l1;
}

/* Returns candidate by that USE is expressed in IVS.  */

static struct cost_pair *
iv_ca_cand_for_use (struct iv_ca *ivs, struct iv_use *use)
{
  return ivs->cand_for_use[use->id];
}

/* Reverse the list of changes DELTA, forming the inverse to it.  */

static struct iv_ca_delta *
iv_ca_delta_reverse (struct iv_ca_delta *delta)
{
  struct iv_ca_delta *act, *next, *prev = NULL;
  struct cost_pair *tmp;

  for (act = delta; act; act = next)
    {
      next = act->next_change;
      act->next_change = prev;
      prev = act;

      tmp = act->old_cp;
      act->old_cp = act->new_cp;
      act->new_cp = tmp;
    }

  return prev;
}

/* Commit changes in DELTA to IVS.  If FORWARD is false, the changes are
   reverted instead.  */

static void
iv_ca_delta_commit (struct ivopts_data *data, struct iv_ca *ivs,
		    struct iv_ca_delta *delta, bool forward)
{
  struct cost_pair *from, *to;
  struct iv_ca_delta *act;

  if (!forward)
    delta = iv_ca_delta_reverse (delta);

  for (act = delta; act; act = act->next_change)
    {
      from = act->old_cp;
      to = act->new_cp;
      gcc_assert (iv_ca_cand_for_use (ivs, act->use) == from);
      iv_ca_set_cp (data, ivs, act->use, to);
    }

  if (!forward)
    iv_ca_delta_reverse (delta);
}

/* Returns true if CAND is used in IVS.  */

static bool
iv_ca_cand_used_p (struct iv_ca *ivs, struct iv_cand *cand)
{
  return ivs->n_cand_uses[cand->id] > 0;
}

/* Returns number of induction variable candidates in the set IVS.  */

static unsigned
iv_ca_n_cands (struct iv_ca *ivs)
{
  return ivs->n_cands;
}

/* Free the list of changes DELTA.  */

static void
iv_ca_delta_free (struct iv_ca_delta **delta)
{
  struct iv_ca_delta *act, *next;

  for (act = *delta; act; act = next)
    {
      next = act->next_change;
      free (act);
    }

  *delta = NULL;
}

/* Allocates new iv candidates assignment.  */

static struct iv_ca *
iv_ca_new (struct ivopts_data *data)
{
  struct iv_ca *nw = XNEW (struct iv_ca);

  nw->upto = 0;
  nw->bad_uses = 0;
  nw->cand_for_use = XCNEWVEC (struct cost_pair *, n_iv_uses (data));
  nw->n_cand_uses = XCNEWVEC (unsigned, n_iv_cands (data));
  nw->cands = BITMAP_ALLOC (NULL);
  nw->n_cands = 0;
  nw->n_regs = 0;
  nw->cand_use_cost = zero_cost;
  nw->cand_cost = 0;
  nw->n_invariant_uses = XCNEWVEC (unsigned, data->max_inv_id + 1);
  nw->cost = zero_cost;

  return nw;
}

/* Free memory occupied by the set IVS.  */

static void
iv_ca_free (struct iv_ca **ivs)
{
  free ((*ivs)->cand_for_use);
  free ((*ivs)->n_cand_uses);
  BITMAP_FREE ((*ivs)->cands);
  free ((*ivs)->n_invariant_uses);
  free (*ivs);
  *ivs = NULL;
}

/* Dumps IVS to FILE.  */

static void
iv_ca_dump (struct ivopts_data *data, FILE *file, struct iv_ca *ivs)
{
  const char *pref = "  invariants ";
  unsigned i;
  comp_cost cost = iv_ca_cost (ivs);

  fprintf (file, "  cost %d (complexity %d)\n", cost.cost, cost.complexity);
  bitmap_print (file, ivs->cands, "  candidates ","\n");

  for (i = 1; i <= data->max_inv_id; i++)
    if (ivs->n_invariant_uses[i])
      {
	fprintf (file, "%s%d", pref, i);
	pref = ", ";
      }
  fprintf (file, "\n");
}

/* Try changing candidate in IVS to CAND for each use.  Return cost of the
   new set, and store differences in DELTA.  Number of induction variables
   in the new set is stored to N_IVS.  */

static comp_cost
iv_ca_extend (struct ivopts_data *data, struct iv_ca *ivs,
	      struct iv_cand *cand, struct iv_ca_delta **delta,
	      unsigned *n_ivs)
{
  unsigned i;
  comp_cost cost;
  struct iv_use *use;
  struct cost_pair *old_cp, *new_cp;

  *delta = NULL;
  for (i = 0; i < ivs->upto; i++)
    {
      use = iv_use (data, i);
      old_cp = iv_ca_cand_for_use (ivs, use);

      if (old_cp
	  && old_cp->cand == cand)
	continue;

      new_cp = get_use_iv_cost (data, use, cand);
      if (!new_cp)
	continue;

      if (!iv_ca_has_deps (ivs, new_cp))
	continue;
      
      if (!cheaper_cost_pair (new_cp, old_cp))
	continue;

      *delta = iv_ca_delta_add (use, old_cp, new_cp, *delta);
    }

  iv_ca_delta_commit (data, ivs, *delta, true);
  cost = iv_ca_cost (ivs);
  if (n_ivs)
    *n_ivs = iv_ca_n_cands (ivs);
  iv_ca_delta_commit (data, ivs, *delta, false);

  return cost;
}

/* Try narrowing set IVS by removing CAND.  Return the cost of
   the new set and store the differences in DELTA.  */

static comp_cost
iv_ca_narrow (struct ivopts_data *data, struct iv_ca *ivs,
	      struct iv_cand *cand, struct iv_ca_delta **delta)
{
  unsigned i, ci;
  struct iv_use *use;
  struct cost_pair *old_cp, *new_cp, *cp;
  bitmap_iterator bi;
  struct iv_cand *cnd;
  comp_cost cost;

  *delta = NULL;
  for (i = 0; i < n_iv_uses (data); i++)
    {
      use = iv_use (data, i);

      old_cp = iv_ca_cand_for_use (ivs, use);
      if (old_cp->cand != cand)
	continue;

      new_cp = NULL;

      if (data->consider_all_candidates)
	{
	  EXECUTE_IF_SET_IN_BITMAP (ivs->cands, 0, ci, bi)
	    {
	      if (ci == cand->id)
		continue;

	      cnd = iv_cand (data, ci);

	      cp = get_use_iv_cost (data, use, cnd);
	      if (!cp)
		continue;
	      if (!iv_ca_has_deps (ivs, cp))
		continue;
      
	      if (!cheaper_cost_pair (cp, new_cp))
		continue;

	      new_cp = cp;
	    }
	}
      else
	{
	  EXECUTE_IF_AND_IN_BITMAP (use->related_cands, ivs->cands, 0, ci, bi)
	    {
	      if (ci == cand->id)
		continue;

	      cnd = iv_cand (data, ci);

	      cp = get_use_iv_cost (data, use, cnd);
	      if (!cp)
		continue;
	      if (!iv_ca_has_deps (ivs, cp))
		continue;
      
	      if (!cheaper_cost_pair (cp, new_cp))
		continue;

	      new_cp = cp;
	    }
	}

      if (!new_cp)
	{
	  iv_ca_delta_free (delta);
	  return infinite_cost;
	}

      *delta = iv_ca_delta_add (use, old_cp, new_cp, *delta);
    }

  iv_ca_delta_commit (data, ivs, *delta, true);
  cost = iv_ca_cost (ivs);
  iv_ca_delta_commit (data, ivs, *delta, false);

  return cost;
}

/* Try optimizing the set of candidates IVS by removing candidates different
   from to EXCEPT_CAND from it.  Return cost of the new set, and store
   differences in DELTA.  */

static comp_cost
iv_ca_prune (struct ivopts_data *data, struct iv_ca *ivs,
	     struct iv_cand *except_cand, struct iv_ca_delta **delta)
{
  bitmap_iterator bi;
  struct iv_ca_delta *act_delta, *best_delta;
  unsigned i;
  comp_cost best_cost, acost;
  struct iv_cand *cand;

  best_delta = NULL;
  best_cost = iv_ca_cost (ivs);

  EXECUTE_IF_SET_IN_BITMAP (ivs->cands, 0, i, bi)
    {
      cand = iv_cand (data, i);

      if (cand == except_cand)
	continue;

      acost = iv_ca_narrow (data, ivs, cand, &act_delta);

      if (compare_costs (acost, best_cost) < 0)
	{
	  best_cost = acost;
	  iv_ca_delta_free (&best_delta);
	  best_delta = act_delta;
	}
      else
	iv_ca_delta_free (&act_delta);
    }

  if (!best_delta)
    {
      *delta = NULL;
      return best_cost;
    }

  /* Recurse to possibly remove other unnecessary ivs.  */
  iv_ca_delta_commit (data, ivs, best_delta, true);
  best_cost = iv_ca_prune (data, ivs, except_cand, delta);
  iv_ca_delta_commit (data, ivs, best_delta, false);
  *delta = iv_ca_delta_join (best_delta, *delta);
  return best_cost;
}

/* Tries to extend the sets IVS in the best possible way in order
   to express the USE.  */

static bool
try_add_cand_for (struct ivopts_data *data, struct iv_ca *ivs,
		  struct iv_use *use)
{
  comp_cost best_cost, act_cost;
  unsigned i;
  bitmap_iterator bi;
  struct iv_cand *cand;
  struct iv_ca_delta *best_delta = NULL, *act_delta;
  struct cost_pair *cp;

  iv_ca_add_use (data, ivs, use);
  best_cost = iv_ca_cost (ivs);

  cp = iv_ca_cand_for_use (ivs, use);
  if (cp)
    {
      best_delta = iv_ca_delta_add (use, NULL, cp, NULL);
      iv_ca_set_no_cp (data, ivs, use);
    }

  /* First try important candidates not based on any memory object.  Only if
     this fails, try the specific ones.  Rationale -- in loops with many
     variables the best choice often is to use just one generic biv.  If we
     added here many ivs specific to the uses, the optimization algorithm later
     would be likely to get stuck in a local minimum, thus causing us to create
     too many ivs.  The approach from few ivs to more seems more likely to be
     successful -- starting from few ivs, replacing an expensive use by a
     specific iv should always be a win.  */
  EXECUTE_IF_SET_IN_BITMAP (data->important_candidates, 0, i, bi)
    {
      cand = iv_cand (data, i);

      if (cand->iv->base_object != NULL_TREE)
	continue;

      if (iv_ca_cand_used_p (ivs, cand))
	continue;

      cp = get_use_iv_cost (data, use, cand);
      if (!cp)
	continue;

      iv_ca_set_cp (data, ivs, use, cp);
      act_cost = iv_ca_extend (data, ivs, cand, &act_delta, NULL);
      iv_ca_set_no_cp (data, ivs, use);
      act_delta = iv_ca_delta_add (use, NULL, cp, act_delta);

      if (compare_costs (act_cost, best_cost) < 0)
	{
	  best_cost = act_cost;

	  iv_ca_delta_free (&best_delta);
	  best_delta = act_delta;
	}
      else
	iv_ca_delta_free (&act_delta);
    }

  if (infinite_cost_p (best_cost))
    {
      for (i = 0; i < use->n_map_members; i++)
	{
	  cp = use->cost_map + i;
	  cand = cp->cand;
	  if (!cand)
	    continue;

	  /* Already tried this.  */
	  if (cand->important && cand->iv->base_object == NULL_TREE)
	    continue;
      
	  if (iv_ca_cand_used_p (ivs, cand))
	    continue;

	  act_delta = NULL;
	  iv_ca_set_cp (data, ivs, use, cp);
	  act_cost = iv_ca_extend (data, ivs, cand, &act_delta, NULL);
	  iv_ca_set_no_cp (data, ivs, use);
	  act_delta = iv_ca_delta_add (use, iv_ca_cand_for_use (ivs, use),
				       cp, act_delta);

	  if (compare_costs (act_cost, best_cost) < 0)
	    {
	      best_cost = act_cost;

	      if (best_delta)
		iv_ca_delta_free (&best_delta);
	      best_delta = act_delta;
	    }
	  else
	    iv_ca_delta_free (&act_delta);
	}
    }

  iv_ca_delta_commit (data, ivs, best_delta, true);
  iv_ca_delta_free (&best_delta);

  return !infinite_cost_p (best_cost);
}

/* Finds an initial assignment of candidates to uses.  */

static struct iv_ca *
get_initial_solution (struct ivopts_data *data)
{
  struct iv_ca *ivs = iv_ca_new (data);
  unsigned i;

  for (i = 0; i < n_iv_uses (data); i++)
    if (!try_add_cand_for (data, ivs, iv_use (data, i)))
      {
	iv_ca_free (&ivs);
	return NULL;
      }

  return ivs;
}

/* Tries to improve set of induction variables IVS.  */

static bool
try_improve_iv_set (struct ivopts_data *data, struct iv_ca *ivs)
{
  unsigned i, n_ivs;
  comp_cost acost, best_cost = iv_ca_cost (ivs);
  struct iv_ca_delta *best_delta = NULL, *act_delta, *tmp_delta;
  struct iv_cand *cand;

  /* Try extending the set of induction variables by one.  */
  for (i = 0; i < n_iv_cands (data); i++)
    {
      cand = iv_cand (data, i);
      
      if (iv_ca_cand_used_p (ivs, cand))
	continue;

      acost = iv_ca_extend (data, ivs, cand, &act_delta, &n_ivs);
      if (!act_delta)
	continue;

      /* If we successfully added the candidate and the set is small enough,
	 try optimizing it by removing other candidates.  */
      if (n_ivs <= ALWAYS_PRUNE_CAND_SET_BOUND)
      	{
	  iv_ca_delta_commit (data, ivs, act_delta, true);
	  acost = iv_ca_prune (data, ivs, cand, &tmp_delta);
	  iv_ca_delta_commit (data, ivs, act_delta, false);
	  act_delta = iv_ca_delta_join (act_delta, tmp_delta);
	}

      if (compare_costs (acost, best_cost) < 0)
	{
	  best_cost = acost;
	  iv_ca_delta_free (&best_delta);
	  best_delta = act_delta;
	}
      else
	iv_ca_delta_free (&act_delta);
    }

  if (!best_delta)
    {
      /* Try removing the candidates from the set instead.  */
      best_cost = iv_ca_prune (data, ivs, NULL, &best_delta);

      /* Nothing more we can do.  */
      if (!best_delta)
	return false;
    }

  iv_ca_delta_commit (data, ivs, best_delta, true);
  gcc_assert (compare_costs (best_cost, iv_ca_cost (ivs)) == 0);
  iv_ca_delta_free (&best_delta);
  return true;
}

/* Attempts to find the optimal set of induction variables.  We do simple
   greedy heuristic -- we try to replace at most one candidate in the selected
   solution and remove the unused ivs while this improves the cost.  */

static struct iv_ca *
find_optimal_iv_set (struct ivopts_data *data)
{
  unsigned i;
  struct iv_ca *set;
  struct iv_use *use;

  /* Get the initial solution.  */
  set = get_initial_solution (data);
  if (!set)
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	fprintf (dump_file, "Unable to substitute for ivs, failed.\n");
      return NULL;
    }

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Initial set of candidates:\n");
      iv_ca_dump (data, dump_file, set);
    }

  while (try_improve_iv_set (data, set))
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	{
	  fprintf (dump_file, "Improved to:\n");
	  iv_ca_dump (data, dump_file, set);
	}
    }

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      comp_cost cost = iv_ca_cost (set);
      fprintf (dump_file, "Final cost %d (complexity %d)\n\n", cost.cost, cost.complexity);
    }

  for (i = 0; i < n_iv_uses (data); i++)
    {
      use = iv_use (data, i);
      use->selected = iv_ca_cand_for_use (set, use)->cand;
    }

  return set;
}

/* Creates a new induction variable corresponding to CAND.  */

static void
create_new_iv (struct ivopts_data *data, struct iv_cand *cand)
{
  gimple_stmt_iterator incr_pos;
  tree base;
  bool after = false;

  if (!cand->iv)
    return;

  switch (cand->pos)
    {
    case IP_NORMAL:
      incr_pos = gsi_last_bb (ip_normal_pos (data->current_loop));
      break;

    case IP_END:
      incr_pos = gsi_last_bb (ip_end_pos (data->current_loop));
      after = true;
      break;

    case IP_ORIGINAL:
      /* Mark that the iv is preserved.  */
      name_info (data, cand->var_before)->preserve_biv = true;
      name_info (data, cand->var_after)->preserve_biv = true;

      /* Rewrite the increment so that it uses var_before directly.  */
      find_interesting_uses_op (data, cand->var_after)->selected = cand;
      
      return;
    }
 
  gimple_add_tmp_var (cand->var_before);
  add_referenced_var (cand->var_before);

  base = unshare_expr (cand->iv->base);

  create_iv (base, unshare_expr (cand->iv->step),
	     cand->var_before, data->current_loop,
	     &incr_pos, after, &cand->var_before, &cand->var_after);
}

/* Creates new induction variables described in SET.  */

static void
create_new_ivs (struct ivopts_data *data, struct iv_ca *set)
{
  unsigned i;
  struct iv_cand *cand;
  bitmap_iterator bi;

  EXECUTE_IF_SET_IN_BITMAP (set->cands, 0, i, bi)
    {
      cand = iv_cand (data, i);
      create_new_iv (data, cand);
    }
}

/* Returns the phi-node in BB with result RESULT.  */

static gimple
get_phi_with_result (basic_block bb, tree result)
{
  gimple_stmt_iterator i = gsi_start_phis (bb);

  for (; !gsi_end_p (i); gsi_next (&i))
    if (gimple_phi_result (gsi_stmt (i)) == result)
      return gsi_stmt (i);

  gcc_unreachable ();
  return NULL;
}


/* Removes statement STMT (real or a phi node).  If INCLUDING_DEFINED_NAME
   is true, remove also the ssa name defined by the statement.  */

static void
remove_statement (gimple stmt, bool including_defined_name)
{
  if (gimple_code (stmt) == GIMPLE_PHI)
    {
      gimple bb_phi = get_phi_with_result (gimple_bb (stmt), 
					 gimple_phi_result (stmt));
      gimple_stmt_iterator bsi = gsi_for_stmt (bb_phi);
      remove_phi_node (&bsi, including_defined_name);
    }
  else
    {
      gimple_stmt_iterator bsi = gsi_for_stmt (stmt);
      gsi_remove (&bsi, true);
      release_defs (stmt); 
    }
}

/* Rewrites USE (definition of iv used in a nonlinear expression)
   using candidate CAND.  */

static void
rewrite_use_nonlinear_expr (struct ivopts_data *data,
			    struct iv_use *use, struct iv_cand *cand)
{
  tree comp;
  tree op, tgt;
  gimple ass;
  gimple_stmt_iterator bsi;

  /* An important special case -- if we are asked to express value of
     the original iv by itself, just exit; there is no need to
     introduce a new computation (that might also need casting the
     variable to unsigned and back).  */
  if (cand->pos == IP_ORIGINAL
      && cand->incremented_at == use->stmt)
    {
      tree step, ctype, utype;
      enum tree_code incr_code = PLUS_EXPR, old_code;

      gcc_assert (is_gimple_assign (use->stmt));
      gcc_assert (gimple_assign_lhs (use->stmt) == cand->var_after);

      step = cand->iv->step;
      ctype = TREE_TYPE (step);
      utype = TREE_TYPE (cand->var_after);
      if (TREE_CODE (step) == NEGATE_EXPR)
	{
	  incr_code = MINUS_EXPR;
	  step = TREE_OPERAND (step, 0);
	}

      /* Check whether we may leave the computation unchanged.
	 This is the case only if it does not rely on other
	 computations in the loop -- otherwise, the computation
	 we rely upon may be removed in remove_unused_ivs,
	 thus leading to ICE.  */
      old_code = gimple_assign_rhs_code (use->stmt);
      if (old_code == PLUS_EXPR
	  || old_code == MINUS_EXPR
	  || old_code == POINTER_PLUS_EXPR)
	{
	  if (gimple_assign_rhs1 (use->stmt) == cand->var_before)
	    op = gimple_assign_rhs2 (use->stmt);
	  else if (old_code != MINUS_EXPR
		   && gimple_assign_rhs2 (use->stmt) == cand->var_before)
	    op = gimple_assign_rhs1 (use->stmt);
	  else
	    op = NULL_TREE;
	}
      else
	op = NULL_TREE;

      if (op
	  && (TREE_CODE (op) == INTEGER_CST
	      || operand_equal_p (op, step, 0)))
	return;

      /* Otherwise, add the necessary computations to express
	 the iv.  */
      op = fold_convert (ctype, cand->var_before);
      comp = fold_convert (utype,
			   build2 (incr_code, ctype, op,
				   unshare_expr (step)));
    }
  else
    {
      comp = get_computation (data->current_loop, use, cand);
      gcc_assert (comp != NULL_TREE);
    }

  switch (gimple_code (use->stmt))
    {
    case GIMPLE_PHI:
      tgt = PHI_RESULT (use->stmt);

      /* If we should keep the biv, do not replace it.  */
      if (name_info (data, tgt)->preserve_biv)
	return;

      bsi = gsi_after_labels (gimple_bb (use->stmt));
      break;

    case GIMPLE_ASSIGN:
      tgt = gimple_assign_lhs (use->stmt);
      bsi = gsi_for_stmt (use->stmt);
      break;

    default:
      gcc_unreachable ();
    }

  op = force_gimple_operand_gsi (&bsi, comp, false, SSA_NAME_VAR (tgt),
				 true, GSI_SAME_STMT);

  if (gimple_code (use->stmt) == GIMPLE_PHI)
    {
      ass = gimple_build_assign (tgt, op);
      gsi_insert_before (&bsi, ass, GSI_SAME_STMT);
      remove_statement (use->stmt, false);
    }
  else
    {
      gimple_assign_set_rhs_from_tree (&bsi, op);
      use->stmt = gsi_stmt (bsi);
    }
}

/* Replaces ssa name in index IDX by its basic variable.  Callback for
   for_each_index.  */

static bool
idx_remove_ssa_names (tree base, tree *idx,
		      void *data ATTRIBUTE_UNUSED)
{
  tree *op;

  if (TREE_CODE (*idx) == SSA_NAME)
    *idx = SSA_NAME_VAR (*idx);

  if (TREE_CODE (base) == ARRAY_REF || TREE_CODE (base) == ARRAY_RANGE_REF)
    {
      op = &TREE_OPERAND (base, 2);
      if (*op
	  && TREE_CODE (*op) == SSA_NAME)
	*op = SSA_NAME_VAR (*op);
      op = &TREE_OPERAND (base, 3);
      if (*op
	  && TREE_CODE (*op) == SSA_NAME)
	*op = SSA_NAME_VAR (*op);
    }

  return true;
}

/* Unshares REF and replaces ssa names inside it by their basic variables.  */

static tree
unshare_and_remove_ssa_names (tree ref)
{
  ref = unshare_expr (ref);
  for_each_index (&ref, idx_remove_ssa_names, NULL);

  return ref;
}

/* Extract the alias analysis info for the memory reference REF.  There are
   several ways how this information may be stored and what precisely is
   its semantics depending on the type of the reference, but there always is
   somewhere hidden one _DECL node that is used to determine the set of
   virtual operands for the reference.  The code below deciphers this jungle
   and extracts this single useful piece of information.  */

static tree
get_ref_tag (tree ref, tree orig)
{
  tree var = get_base_address (ref);
  tree aref = NULL_TREE, tag, sv;
  HOST_WIDE_INT offset, size, maxsize;

  for (sv = orig; handled_component_p (sv); sv = TREE_OPERAND (sv, 0))
    {
      aref = get_ref_base_and_extent (sv, &offset, &size, &maxsize);
      if (ref)
	break;
    }

  if (!var)
    return NULL_TREE;

  if (TREE_CODE (var) == INDIRECT_REF)
    {
      /* If the base is a dereference of a pointer, first check its name memory
	 tag.  If it does not have one, use its symbol memory tag.  */
      var = TREE_OPERAND (var, 0);
      if (TREE_CODE (var) != SSA_NAME)
	return NULL_TREE;

      if (SSA_NAME_PTR_INFO (var))
	{
	  tag = SSA_NAME_PTR_INFO (var)->name_mem_tag;
	  if (tag)
	    return tag;
	}
 
      var = SSA_NAME_VAR (var);
      tag = symbol_mem_tag (var);
      gcc_assert (tag != NULL_TREE);
      return tag;
    }
  else
    { 
      if (!DECL_P (var))
	return NULL_TREE;

      tag = symbol_mem_tag (var);
      if (tag)
	return tag;

      return var;
    }
}

/* Copies the reference information from OLD_REF to NEW_REF.  */

static void
copy_ref_info (tree new_ref, tree old_ref)
{
  if (TREE_CODE (old_ref) == TARGET_MEM_REF)
    copy_mem_ref_info (new_ref, old_ref);
  else
    {
      TMR_ORIGINAL (new_ref) = unshare_and_remove_ssa_names (old_ref);
      TMR_TAG (new_ref) = get_ref_tag (old_ref, TMR_ORIGINAL (new_ref));
    }
}

/* Rewrites USE (address that is an iv) using candidate CAND.  */

static void
rewrite_use_address (struct ivopts_data *data,
		     struct iv_use *use, struct iv_cand *cand)
{
  aff_tree aff;
  gimple_stmt_iterator bsi = gsi_for_stmt (use->stmt);
  tree ref;
  bool ok;

  ok = get_computation_aff (data->current_loop, use, cand, use->stmt, &aff);
  gcc_assert (ok);
  unshare_aff_combination (&aff);

  ref = create_mem_ref (&bsi, TREE_TYPE (*use->op_p), &aff, data->speed);
  copy_ref_info (ref, *use->op_p);
  *use->op_p = ref;
}

/* Rewrites USE (the condition such that one of the arguments is an iv) using
   candidate CAND.  */

static void
rewrite_use_compare (struct ivopts_data *data,
		     struct iv_use *use, struct iv_cand *cand)
{
  tree comp, *var_p, op, bound;
  gimple_stmt_iterator bsi = gsi_for_stmt (use->stmt);
  enum tree_code compare;
  struct cost_pair *cp = get_use_iv_cost (data, use, cand);
  bool ok;

  bound = cp->value;
  if (bound)
    {
      tree var = var_at_stmt (data->current_loop, cand, use->stmt);
      tree var_type = TREE_TYPE (var);
      gimple_seq stmts;

      compare = iv_elimination_compare (data, use);
      bound = unshare_expr (fold_convert (var_type, bound));
      op = force_gimple_operand (bound, &stmts, true, NULL_TREE);
      if (stmts)
	gsi_insert_seq_on_edge_immediate (
		loop_preheader_edge (data->current_loop),
		stmts);

      gimple_cond_set_lhs (use->stmt, var);
      gimple_cond_set_code (use->stmt, compare);
      gimple_cond_set_rhs (use->stmt, op);
      return;
    }

  /* The induction variable elimination failed; just express the original
     giv.  */
  comp = get_computation (data->current_loop, use, cand);
  gcc_assert (comp != NULL_TREE);

  ok = extract_cond_operands (data, use->stmt, &var_p, NULL, NULL, NULL);
  gcc_assert (ok);

  *var_p = force_gimple_operand_gsi (&bsi, comp, true, SSA_NAME_VAR (*var_p),
				     true, GSI_SAME_STMT);
}

/* Rewrites USE using candidate CAND.  */

static void
rewrite_use (struct ivopts_data *data, struct iv_use *use, struct iv_cand *cand)
{
  push_stmt_changes (&use->stmt);

  switch (use->type)
    {
      case USE_NONLINEAR_EXPR:
	rewrite_use_nonlinear_expr (data, use, cand);
	break;

      case USE_ADDRESS:
	rewrite_use_address (data, use, cand);
	break;

      case USE_COMPARE:
	rewrite_use_compare (data, use, cand);
	break;

      default:
	gcc_unreachable ();
    }

  pop_stmt_changes (&use->stmt);
}

/* Rewrite the uses using the selected induction variables.  */

static void
rewrite_uses (struct ivopts_data *data)
{
  unsigned i;
  struct iv_cand *cand;
  struct iv_use *use;

  for (i = 0; i < n_iv_uses (data); i++)
    {
      use = iv_use (data, i);
      cand = use->selected;
      gcc_assert (cand);

      rewrite_use (data, use, cand);
    }
}

/* Removes the ivs that are not used after rewriting.  */

static void
remove_unused_ivs (struct ivopts_data *data)
{
  unsigned j;
  bitmap_iterator bi;

  EXECUTE_IF_SET_IN_BITMAP (data->relevant, 0, j, bi)
    {
      struct version_info *info;

      info = ver_info (data, j);
      if (info->iv
	  && !integer_zerop (info->iv->step)
	  && !info->inv_id
	  && !info->iv->have_use_for
	  && !info->preserve_biv)
	remove_statement (SSA_NAME_DEF_STMT (info->iv->ssa_name), true);
    }
}

/* Frees data allocated by the optimization of a single loop.  */

static void
free_loop_data (struct ivopts_data *data)
{
  unsigned i, j;
  bitmap_iterator bi;
  tree obj;

  if (data->niters)
    {
      pointer_map_destroy (data->niters);
      data->niters = NULL;
    }

  EXECUTE_IF_SET_IN_BITMAP (data->relevant, 0, i, bi)
    {
      struct version_info *info;

      info = ver_info (data, i);
      if (info->iv)
	free (info->iv);
      info->iv = NULL;
      info->has_nonlin_use = false;
      info->preserve_biv = false;
      info->inv_id = 0;
    }
  bitmap_clear (data->relevant);
  bitmap_clear (data->important_candidates);

  for (i = 0; i < n_iv_uses (data); i++)
    {
      struct iv_use *use = iv_use (data, i);

      free (use->iv);
      BITMAP_FREE (use->related_cands);
      for (j = 0; j < use->n_map_members; j++)
	if (use->cost_map[j].depends_on)
	  BITMAP_FREE (use->cost_map[j].depends_on);
      free (use->cost_map);
      free (use);
    }
  VEC_truncate (iv_use_p, data->iv_uses, 0);

  for (i = 0; i < n_iv_cands (data); i++)
    {
      struct iv_cand *cand = iv_cand (data, i);

      if (cand->iv)
	free (cand->iv);
      if (cand->depends_on)
	BITMAP_FREE (cand->depends_on);
      free (cand);
    }
  VEC_truncate (iv_cand_p, data->iv_candidates, 0);

  if (data->version_info_size < num_ssa_names)
    {
      data->version_info_size = 2 * num_ssa_names;
      free (data->version_info);
      data->version_info = XCNEWVEC (struct version_info, data->version_info_size);
    }

  data->max_inv_id = 0;

  for (i = 0; VEC_iterate (tree, decl_rtl_to_reset, i, obj); i++)
    SET_DECL_RTL (obj, NULL_RTX);

  VEC_truncate (tree, decl_rtl_to_reset, 0);
}

/* Finalizes data structures used by the iv optimization pass.  LOOPS is the
   loop tree.  */

static void
tree_ssa_iv_optimize_finalize (struct ivopts_data *data)
{
  free_loop_data (data);
  free (data->version_info);
  BITMAP_FREE (data->relevant);
  BITMAP_FREE (data->important_candidates);

  VEC_free (tree, heap, decl_rtl_to_reset);
  VEC_free (iv_use_p, heap, data->iv_uses);
  VEC_free (iv_cand_p, heap, data->iv_candidates);
}

/* Optimizes the LOOP.  Returns true if anything changed.  */

static bool
tree_ssa_iv_optimize_loop (struct ivopts_data *data, struct loop *loop)
{
  bool changed = false;
  struct iv_ca *iv_ca;
  edge exit;

  gcc_assert (!data->niters);
  data->current_loop = loop;
  data->speed = optimize_loop_for_speed_p (loop);

  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "Processing loop %d\n", loop->num);
      
      exit = single_dom_exit (loop);
      if (exit)
	{
	  fprintf (dump_file, "  single exit %d -> %d, exit condition ",
		   exit->src->index, exit->dest->index);
	  print_gimple_stmt (dump_file, last_stmt (exit->src), 0, TDF_SLIM);
	  fprintf (dump_file, "\n");
	}

      fprintf (dump_file, "\n");
    }

  /* For each ssa name determines whether it behaves as an induction variable
     in some loop.  */
  if (!find_induction_variables (data))
    goto finish;

  /* Finds interesting uses (item 1).  */
  find_interesting_uses (data);
  if (n_iv_uses (data) > MAX_CONSIDERED_USES)
    goto finish;

  /* Finds candidates for the induction variables (item 2).  */
  find_iv_candidates (data);

  /* Calculates the costs (item 3, part 1).  */
  determine_use_iv_costs (data);
  determine_iv_costs (data);
  determine_set_costs (data);

  /* Find the optimal set of induction variables (item 3, part 2).  */
  iv_ca = find_optimal_iv_set (data);
  if (!iv_ca)
    goto finish;
  changed = true;

  /* Create the new induction variables (item 4, part 1).  */
  create_new_ivs (data, iv_ca);
  iv_ca_free (&iv_ca);
  
  /* Rewrite the uses (item 4, part 2).  */
  rewrite_uses (data);

  /* Remove the ivs that are unused after rewriting.  */
  remove_unused_ivs (data);

  /* We have changed the structure of induction variables; it might happen
     that definitions in the scev database refer to some of them that were
     eliminated.  */
  scev_reset ();

finish:
  free_loop_data (data);

  return changed;
}

/* Main entry point.  Optimizes induction variables in loops.  */

void
tree_ssa_iv_optimize (void)
{
  struct loop *loop;
  struct ivopts_data data;
  loop_iterator li;

  tree_ssa_iv_optimize_init (&data);

  /* Optimize the loops starting with the innermost ones.  */
  FOR_EACH_LOOP (li, loop, LI_FROM_INNERMOST)
    {
      if (dump_file && (dump_flags & TDF_DETAILS))
	flow_loop_dump (loop, dump_file, NULL, 1);

      tree_ssa_iv_optimize_loop (&data, loop);
    }

  tree_ssa_iv_optimize_finalize (&data);
}
