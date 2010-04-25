/* Data structures and function declarations for the SSA value propagation
   engine.
   Copyright (C) 2004, 2005, 2007, 2008 Free Software Foundation, Inc.
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

#ifndef _TREE_SSA_PROPAGATE_H
#define _TREE_SSA_PROPAGATE_H 1

/* If SIM_P is true, statement S will be simulated again.  */

static inline void
prop_set_simulate_again (gimple s, bool visit_p)
{
  gimple_set_visited (s, visit_p);
}

/* Return true if statement T should be simulated again.  */

static inline bool
prop_simulate_again_p (gimple s)
{
  return gimple_visited_p (s);
}

/* Lattice values used for propagation purposes.  Specific instances
   of a propagation engine must return these values from the statement
   and PHI visit functions to direct the engine.  */
enum ssa_prop_result {
    /* The statement produces nothing of interest.  No edges will be
       added to the work lists.  */
    SSA_PROP_NOT_INTERESTING,

    /* The statement produces an interesting value.  The set SSA_NAMEs
       returned by SSA_PROP_VISIT_STMT should be added to
       INTERESTING_SSA_EDGES.  If the statement being visited is a
       conditional jump, SSA_PROP_VISIT_STMT should indicate which edge
       out of the basic block should be marked executable.  */
    SSA_PROP_INTERESTING,

    /* The statement produces a varying (i.e., useless) value and
       should not be simulated again.  If the statement being visited
       is a conditional jump, all the edges coming out of the block
       will be considered executable.  */
    SSA_PROP_VARYING
};


struct prop_value_d {
    /* Lattice value.  Each propagator is free to define its own
       lattice and this field is only meaningful while propagating.
       It will not be used by substitute_and_fold.  */
    unsigned lattice_val;

    /* Propagated value.  */
    tree value;
};

typedef struct prop_value_d prop_value_t;


/* Type of value ranges.  See value_range_d for a description of these
   types.  */
enum value_range_type { VR_UNDEFINED, VR_RANGE, VR_ANTI_RANGE, VR_VARYING };

/* Range of values that can be associated with an SSA_NAME after VRP
   has executed.  */
struct value_range_d
{
  /* Lattice value represented by this range.  */
  enum value_range_type type;

  /* Minimum and maximum values represented by this range.  These
     values should be interpreted as follows:

	- If TYPE is VR_UNDEFINED or VR_VARYING then MIN and MAX must
	  be NULL.

	- If TYPE == VR_RANGE then MIN holds the minimum value and
	  MAX holds the maximum value of the range [MIN, MAX].

	- If TYPE == ANTI_RANGE the variable is known to NOT
	  take any values in the range [MIN, MAX].  */
  tree min;
  tree max;

  /* Set of SSA names whose value ranges are equivalent to this one.
     This set is only valid when TYPE is VR_RANGE or VR_ANTI_RANGE.  */
  bitmap equiv;
};

typedef struct value_range_d value_range_t;


/* Call-back functions used by the value propagation engine.  */
typedef enum ssa_prop_result (*ssa_prop_visit_stmt_fn) (gimple, edge *, tree *);
typedef enum ssa_prop_result (*ssa_prop_visit_phi_fn) (gimple);


/* In tree-ssa-propagate.c  */
void ssa_propagate (ssa_prop_visit_stmt_fn, ssa_prop_visit_phi_fn);
bool valid_gimple_rhs_p (tree);
bool valid_gimple_call_p (tree);
void move_ssa_defining_stmt_for_defs (gimple, gimple);
bool update_call_from_tree (gimple_stmt_iterator *, tree);
bool stmt_makes_single_load (gimple);
bool stmt_makes_single_store (gimple);
bool substitute_and_fold (prop_value_t *, bool);

#endif /* _TREE_SSA_PROPAGATE_H  */
