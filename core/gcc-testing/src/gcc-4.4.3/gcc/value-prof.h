/* Definitions for transformations based on profile information for values.
   Copyright (C) 2003, 2004, 2005, 2007, 2008 Free Software Foundation, Inc.

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

#ifndef GCC_VALUE_PROF_H
#define GCC_VALUE_PROF_H

/* Supported histogram types.  */
enum hist_type
{
  HIST_TYPE_INTERVAL,	/* Measures histogram of values inside a specified
			   interval.  */
  HIST_TYPE_POW2,	/* Histogram of power of 2 values.  */
  HIST_TYPE_SINGLE_VALUE, /* Tries to identify the value that is (almost)
			   always constant.  */
  HIST_TYPE_CONST_DELTA, /* Tries to identify the (almost) always constant
			   difference between two evaluations of a value.  */
  HIST_TYPE_INDIR_CALL,   /* Tries to identify the function that is (almost) 
			    called in indirect call */
  HIST_TYPE_AVERAGE,	/* Compute average value (sum of all values).  */
  HIST_TYPE_IOR		/* Used to compute expected alignment.  */
};

#define COUNTER_FOR_HIST_TYPE(TYPE) ((int) (TYPE) + GCOV_FIRST_VALUE_COUNTER)
#define HIST_TYPE_FOR_COUNTER(COUNTER) \
  ((enum hist_type) ((COUNTER) - GCOV_FIRST_VALUE_COUNTER))

/* The value to measure.  */
struct histogram_value_t
{
  struct
    {
      tree value;		/* The value to profile.  */
      gimple stmt;		/* Insn containing the value.  */
      gcov_type *counters;		        /* Pointer to first counter.  */
      struct histogram_value_t *next;		/* Linked list pointer.  */
    } hvalue;
  enum hist_type type;			/* Type of information to measure.  */
  unsigned n_counters;			/* Number of required counters.  */
  union
    {
      struct
	{
	  int int_start;	/* First value in interval.  */
	  unsigned int steps;	/* Number of values in it.  */
	} intvl;	/* Interval histogram data.  */
    } hdata;		/* Profiled information specific data.  */
};

typedef struct histogram_value_t *histogram_value;
typedef const struct histogram_value_t *const_histogram_value;

DEF_VEC_P(histogram_value);
DEF_VEC_ALLOC_P(histogram_value,heap);

typedef VEC(histogram_value,heap) *histogram_values;

/* Hooks registration.  */
extern void gimple_register_value_prof_hooks (void);

/* IR-independent entry points.  */
extern void find_values_to_profile (histogram_values *);
extern bool value_profile_transformations (void);

/* External declarations for edge-based profiling.  */
struct profile_hooks {

  /* Insert code to initialize edge profiler.  */
  void (*init_edge_profiler) (void);

  /* Insert code to increment an edge count.  */
  void (*gen_edge_profiler) (int, edge);

  /* Insert code to increment the interval histogram counter.  */
  void (*gen_interval_profiler) (histogram_value, unsigned, unsigned);

  /* Insert code to increment the power of two histogram counter.  */
  void (*gen_pow2_profiler) (histogram_value, unsigned, unsigned);

  /* Insert code to find the most common value.  */
  void (*gen_one_value_profiler) (histogram_value, unsigned, unsigned);

  /* Insert code to find the most common value of a difference between two
     evaluations of an expression.  */
  void (*gen_const_delta_profiler) (histogram_value, unsigned, unsigned);

  /* Insert code to find the most common indirect call */
  void (*gen_ic_profiler) (histogram_value, unsigned, unsigned);

  /* Insert code to find the average value of an expression.  */
  void (*gen_average_profiler) (histogram_value, unsigned, unsigned);

  /* Insert code to ior value of an expression.  */
  void (*gen_ior_profiler) (histogram_value, unsigned, unsigned);
};

histogram_value gimple_histogram_value (struct function *, gimple);
histogram_value gimple_histogram_value_of_type (struct function *, gimple,
						enum hist_type);
void gimple_add_histogram_value (struct function *, gimple, histogram_value);
void dump_histograms_for_stmt (struct function *, FILE *, gimple);
void gimple_remove_histogram_value (struct function *, gimple, histogram_value);
void gimple_remove_stmt_histograms (struct function *, gimple);
void gimple_duplicate_stmt_histograms (struct function *, gimple,
				       struct function *, gimple);
void gimple_move_stmt_histograms (struct function *, gimple, gimple);
void verify_histograms (void);
void free_histograms (void);
void stringop_block_profile (gimple, unsigned int *, HOST_WIDE_INT *);

/* In profile.c.  */
extern void init_branch_prob (void);
extern void branch_prob (void);
extern void end_branch_prob (void);
extern void tree_register_profile_hooks (void);

/* In tree-profile.c.  */
extern struct profile_hooks tree_profile_hooks;

#endif	/* GCC_VALUE_PROF_H */

