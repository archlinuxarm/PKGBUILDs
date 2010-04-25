/* Definitions for describing one tree-ssa optimization pass.
   Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>

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


#ifndef GCC_TREE_PASS_H
#define GCC_TREE_PASS_H 1

/* In tree-dump.c */

/* Different tree dump places.  When you add new tree dump places,
   extend the DUMP_FILES array in tree-dump.c.  */
enum tree_dump_index
{
  TDI_none,			/* No dump */
  TDI_cgraph,                   /* dump function call graph.  */
  TDI_tu,			/* dump the whole translation unit.  */
  TDI_class,			/* dump class hierarchy.  */
  TDI_original,			/* dump each function before optimizing it */
  TDI_generic,			/* dump each function after genericizing it */
  TDI_nested,			/* dump each function after unnesting it */
  TDI_vcg,			/* create a VCG graph file for each
				   function's flowgraph.  */
  TDI_tree_all,                 /* enable all the GENERIC/GIMPLE dumps.  */
  TDI_rtl_all,                  /* enable all the RTL dumps.  */
  TDI_ipa_all,                  /* enable all the IPA dumps.  */

  TDI_end
};

/* Bit masks to control dumping. Not all values are applicable to
   all dumps. Add new ones at the end. When you define new
   values, extend the DUMP_OPTIONS array in tree-dump.c */
#define TDF_ADDRESS	(1 << 0)	/* dump node addresses */
#define TDF_SLIM	(1 << 1)	/* don't go wild following links */
#define TDF_RAW  	(1 << 2)	/* don't unparse the function */
#define TDF_DETAILS	(1 << 3)	/* show more detailed info about
					   each pass */
#define TDF_STATS	(1 << 4)	/* dump various statistics about
					   each pass */
#define TDF_BLOCKS	(1 << 5)	/* display basic block boundaries */
#define TDF_VOPS	(1 << 6)	/* display virtual operands */
#define TDF_LINENO	(1 << 7)	/* display statement line numbers */
#define TDF_UID		(1 << 8)	/* display decl UIDs */

#define TDF_TREE	(1 << 9)	/* is a tree dump */
#define TDF_RTL		(1 << 10)	/* is a RTL dump */
#define TDF_IPA		(1 << 11)	/* is an IPA dump */
#define TDF_STMTADDR	(1 << 12)	/* Address of stmt.  */

#define TDF_GRAPH	(1 << 13)	/* a graph dump is being emitted */
#define TDF_MEMSYMS	(1 << 14)	/* display memory symbols in expr.
                                           Implies TDF_VOPS.  */

#define TDF_DIAGNOSTIC	(1 << 15)	/* A dump to be put in a diagnostic
					   message.  */
#define TDF_VERBOSE     (1 << 16)       /* A dump that uses the full tree 
					   dumper to print stmts.  */
#define TDF_RHS_ONLY	(1 << 17)	/* a flag to only print the RHS of
					   a gimple stmt.  */

extern char *get_dump_file_name (enum tree_dump_index);
extern int dump_enabled_p (enum tree_dump_index);
extern int dump_initialized_p (enum tree_dump_index);
extern FILE *dump_begin (enum tree_dump_index, int *);
extern void dump_end (enum tree_dump_index, FILE *);
extern void dump_node (const_tree, int, FILE *);
extern int dump_switch_p (const char *);
extern const char *dump_flag_name (enum tree_dump_index);

/* Global variables used to communicate with passes.  */
extern FILE *dump_file;
extern int dump_flags;
extern const char *dump_file_name;

/* Return the dump_file_info for the given phase.  */
extern struct dump_file_info *get_dump_file_info (enum tree_dump_index);

/* Describe one pass; this is the common part shared across different pass
   types.  */
struct opt_pass
{
  /* Optimization pass type.  */
  enum opt_pass_type {
    GIMPLE_PASS,
    RTL_PASS,
    SIMPLE_IPA_PASS,
    IPA_PASS
  } type;
  /* Terse name of the pass used as a fragment of the dump file
     name.  If the name starts with a star, no dump happens. */
  const char *name;

  /* If non-null, this pass and all sub-passes are executed only if
     the function returns true.  */
  bool (*gate) (void);

  /* This is the code to run.  If null, then there should be sub-passes
     otherwise this pass does nothing.  The return value contains
     TODOs to execute in addition to those in TODO_flags_finish.   */
  unsigned int (*execute) (void);

  /* A list of sub-passes to run, dependent on gate predicate.  */
  struct opt_pass *sub;

  /* Next in the list of passes to run, independent of gate predicate.  */
  struct opt_pass *next;

  /* Static pass number, used as a fragment of the dump file name.  */
  int static_pass_number;

  /* The timevar id associated with this pass.  */
  /* ??? Ideally would be dynamically assigned.  */
  unsigned int tv_id;

  /* Sets of properties input and output from this pass.  */
  unsigned int properties_required;
  unsigned int properties_provided;
  unsigned int properties_destroyed;

  /* Flags indicating common sets things to do before and after.  */
  unsigned int todo_flags_start;
  unsigned int todo_flags_finish;
};

/* Description of GIMPLE pass.  */
struct gimple_opt_pass
{
  struct opt_pass pass;
};

/* Description of RTL pass.  */
struct rtl_opt_pass
{
  struct opt_pass pass;
};

struct varpool_node;
struct cgraph_node;

/* Description of IPA pass with generate summary, write, execute, read and
   transform stages.  */
struct ipa_opt_pass
{
  struct opt_pass pass;

  /* IPA passes can analyze function body and variable initializers
      using this hook and produce summary.  */
  void (*generate_summary) (void);

  /* This hook is used to serialize IPA summaries on disk.  */
  void (*write_summary) (void);

  /* For most ipa passes, the information can only be deserialized in
     one chunk.  However, function bodies are read function at a time
     as needed so both calls are necessary.  */
  void (*read_summary) (void);
  void (*function_read_summary) (struct cgraph_node *);
  
  /* Results of interprocedural propagation of an IPA pass is applied to
     function body via this hook.  */
  unsigned int function_transform_todo_flags_start;
  unsigned int (*function_transform) (struct cgraph_node *);
  void (*variable_transform) (struct varpool_node *);
};

/* Description of simple IPA pass.  Simple IPA passes have just one execute
   hook.  */
struct simple_ipa_opt_pass
{
  struct opt_pass pass;
};

/* Define a tree dump switch.  */
struct dump_file_info
{
  const char *suffix;           /* suffix to give output file.  */
  const char *swtch;            /* command line switch */
  const char *glob;             /* command line glob  */
  int flags;                    /* user flags */
  int state;                    /* state of play */
  int num;                      /* dump file number */
};

/* Pass properties.  */
#define PROP_gimple_any		(1 << 0)	/* entire gimple grammar */
#define PROP_gimple_lcf		(1 << 1)	/* lowered control flow */
#define PROP_gimple_leh		(1 << 2)	/* lowered eh */
#define PROP_cfg		(1 << 3)
#define PROP_referenced_vars	(1 << 4)
#define PROP_ssa		(1 << 5)
#define PROP_no_crit_edges      (1 << 6)
#define PROP_rtl		(1 << 7)
#define PROP_alias		(1 << 8)
#define PROP_gimple_lomp	(1 << 9)	/* lowered OpenMP directives */

#define PROP_trees \
  (PROP_gimple_any | PROP_gimple_lcf | PROP_gimple_leh | PROP_gimple_lomp)

/* To-do flags.  */
#define TODO_dump_func			(1 << 0)
#define TODO_ggc_collect		(1 << 1)
#define TODO_verify_ssa			(1 << 2) 
#define TODO_verify_flow		(1 << 3)
#define TODO_verify_stmts		(1 << 4)
#define TODO_cleanup_cfg        	(1 << 5)
#define TODO_verify_loops		(1 << 6)
#define TODO_dump_cgraph		(1 << 7)
#define TODO_remove_functions		(1 << 8)
#define TODO_rebuild_frequencies	(1 << 9)
#define TODO_verify_rtl_sharing         (1 << 10)

/* To-do flags for calls to update_ssa.  */

/* Update the SSA form inserting PHI nodes for newly exposed symbols
   and virtual names marked for updating.  When updating real names,
   only insert PHI nodes for a real name O_j in blocks reached by all
   the new and old definitions for O_j.  If the iterated dominance
   frontier for O_j is not pruned, we may end up inserting PHI nodes
   in blocks that have one or more edges with no incoming definition
   for O_j.  This would lead to uninitialized warnings for O_j's
   symbol.  */
#define TODO_update_ssa			(1 << 11)

/* Update the SSA form without inserting any new PHI nodes at all.
   This is used by passes that have either inserted all the PHI nodes
   themselves or passes that need only to patch use-def and def-def
   chains for virtuals (e.g., DCE).  */
#define TODO_update_ssa_no_phi		(1 << 12)

/* Insert PHI nodes everywhere they are needed.  No pruning of the
   IDF is done.  This is used by passes that need the PHI nodes for
   O_j even if it means that some arguments will come from the default
   definition of O_j's symbol (e.g., pass_linear_transform).
   
   WARNING: If you need to use this flag, chances are that your pass
   may be doing something wrong.  Inserting PHI nodes for an old name
   where not all edges carry a new replacement may lead to silent
   codegen errors or spurious uninitialized warnings.  */
#define TODO_update_ssa_full_phi	(1 << 13)

/* Passes that update the SSA form on their own may want to delegate
   the updating of virtual names to the generic updater.  Since FUD
   chains are easier to maintain, this simplifies the work they need
   to do.  NOTE: If this flag is used, any OLD->NEW mappings for real
   names are explicitly destroyed and only the symbols marked for
   renaming are processed.  */
#define TODO_update_ssa_only_virtuals	(1 << 14)

/* Some passes leave unused local variables that can be removed from
   cfun->local_decls.  This reduces the size of dump files
   and the memory footprint for VAR_DECLs.  */
#define TODO_remove_unused_locals	(1 << 15)

/* Internally used for the first in a sequence of passes.  It is set
   for the passes that are handed to register_dump_files.  */
#define TODO_set_props			(1 << 16)

/* Call df_finish at the end of the pass.  This is done after all of
   the dumpers have been allowed to run so that they have access to
   the instance before it is destroyed.  */
#define TODO_df_finish                  (1 << 17)

/* Call df_verify at the end of the pass if checking is enabled.  */
#define TODO_df_verify                  (1 << 18)

/* Internally used for the first instance of a pass.  */
#define TODO_mark_first_instance	(1 << 19)

/* Rebuild aliasing info.  */
#define TODO_rebuild_alias                (1 << 20)

#define TODO_update_ssa_any		\
    (TODO_update_ssa			\
     | TODO_update_ssa_no_phi		\
     | TODO_update_ssa_full_phi		\
     | TODO_update_ssa_only_virtuals)

#define TODO_verify_all \
  (TODO_verify_ssa | TODO_verify_flow | TODO_verify_stmts)

extern void tree_lowering_passes (tree decl);

extern struct gimple_opt_pass pass_mudflap_1;
extern struct gimple_opt_pass pass_mudflap_2;
extern struct gimple_opt_pass pass_remove_useless_stmts;
extern struct gimple_opt_pass pass_lower_cf;
extern struct gimple_opt_pass pass_refactor_eh;
extern struct gimple_opt_pass pass_lower_eh;
extern struct gimple_opt_pass pass_build_cfg;
extern struct gimple_opt_pass pass_tree_profile;
extern struct gimple_opt_pass pass_early_tree_profile;
extern struct gimple_opt_pass pass_cleanup_cfg;
extern struct gimple_opt_pass pass_referenced_vars;
extern struct gimple_opt_pass pass_sra;
extern struct gimple_opt_pass pass_sra_early;
extern struct gimple_opt_pass pass_tail_recursion;
extern struct gimple_opt_pass pass_tail_calls;
extern struct gimple_opt_pass pass_tree_loop;
extern struct gimple_opt_pass pass_tree_loop_init;
extern struct gimple_opt_pass pass_lim;
extern struct gimple_opt_pass pass_tree_unswitch;
extern struct gimple_opt_pass pass_predcom;
extern struct gimple_opt_pass pass_iv_canon;
extern struct gimple_opt_pass pass_scev_cprop;
extern struct gimple_opt_pass pass_empty_loop;
extern struct gimple_opt_pass pass_record_bounds;
extern struct gimple_opt_pass pass_graphite_transforms;
extern struct gimple_opt_pass pass_if_conversion;
extern struct gimple_opt_pass pass_loop_distribution;
extern struct gimple_opt_pass pass_vectorize;
extern struct gimple_opt_pass pass_complete_unroll;
extern struct gimple_opt_pass pass_complete_unrolli;
extern struct gimple_opt_pass pass_parallelize_loops;
extern struct gimple_opt_pass pass_loop_prefetch;
extern struct gimple_opt_pass pass_iv_optimize;
extern struct gimple_opt_pass pass_tree_loop_done;
extern struct gimple_opt_pass pass_ch;
extern struct gimple_opt_pass pass_ccp;
extern struct gimple_opt_pass pass_phi_only_cprop;
extern struct gimple_opt_pass pass_build_ssa;
extern struct gimple_opt_pass pass_del_ssa;
extern struct gimple_opt_pass pass_build_alias;
extern struct gimple_opt_pass pass_dominator;
extern struct gimple_opt_pass pass_dce;
extern struct gimple_opt_pass pass_dce_loop;
extern struct gimple_opt_pass pass_cd_dce;
extern struct gimple_opt_pass pass_call_cdce;
extern struct gimple_opt_pass pass_merge_phi;
extern struct gimple_opt_pass pass_split_crit_edges;
extern struct gimple_opt_pass pass_pre;
extern struct gimple_opt_pass pass_profile;
extern struct gimple_opt_pass pass_strip_predict_hints;
extern struct gimple_opt_pass pass_lower_complex_O0;
extern struct gimple_opt_pass pass_lower_complex;
extern struct gimple_opt_pass pass_lower_vector;
extern struct gimple_opt_pass pass_lower_vector_ssa;
extern struct gimple_opt_pass pass_lower_omp;
extern struct gimple_opt_pass pass_expand_omp;
extern struct gimple_opt_pass pass_expand_omp_ssa;
extern struct gimple_opt_pass pass_object_sizes;
extern struct gimple_opt_pass pass_fold_builtins;
extern struct gimple_opt_pass pass_stdarg;
extern struct gimple_opt_pass pass_early_warn_uninitialized;
extern struct gimple_opt_pass pass_late_warn_uninitialized;
extern struct gimple_opt_pass pass_cse_reciprocals;
extern struct gimple_opt_pass pass_cse_sincos;
extern struct gimple_opt_pass pass_convert_to_rsqrt;
extern struct gimple_opt_pass pass_warn_function_return;
extern struct gimple_opt_pass pass_warn_function_noreturn;
extern struct gimple_opt_pass pass_cselim;
extern struct gimple_opt_pass pass_phiopt;
extern struct gimple_opt_pass pass_forwprop;
extern struct gimple_opt_pass pass_phiprop;
extern struct gimple_opt_pass pass_tree_ifcombine;
extern struct gimple_opt_pass pass_dse;
extern struct gimple_opt_pass pass_simple_dse;
extern struct gimple_opt_pass pass_nrv;
extern struct gimple_opt_pass pass_mark_used_blocks;
extern struct gimple_opt_pass pass_rename_ssa_copies;
extern struct gimple_opt_pass pass_rest_of_compilation;
extern struct gimple_opt_pass pass_sink_code;
extern struct gimple_opt_pass pass_fre;
extern struct gimple_opt_pass pass_linear_transform;
extern struct gimple_opt_pass pass_check_data_deps;
extern struct gimple_opt_pass pass_copy_prop;
extern struct gimple_opt_pass pass_vrp;
extern struct gimple_opt_pass pass_uncprop;
extern struct gimple_opt_pass pass_return_slot;
extern struct gimple_opt_pass pass_reassoc;
extern struct gimple_opt_pass pass_rebuild_cgraph_edges;
extern struct gimple_opt_pass pass_build_cgraph_edges;
extern struct gimple_opt_pass pass_reset_cc_flags;

/* IPA Passes */
extern struct ipa_opt_pass pass_ipa_inline;
extern struct ipa_opt_pass pass_ipa_cp;
extern struct ipa_opt_pass pass_ipa_reference;
extern struct ipa_opt_pass pass_ipa_pure_const;

extern struct simple_ipa_opt_pass pass_ipa_matrix_reorg;
extern struct simple_ipa_opt_pass pass_ipa_early_inline;
extern struct simple_ipa_opt_pass pass_ipa_type_escape;
extern struct simple_ipa_opt_pass pass_ipa_pta;
extern struct simple_ipa_opt_pass pass_ipa_struct_reorg;
extern struct simple_ipa_opt_pass pass_early_local_passes;
extern struct simple_ipa_opt_pass pass_ipa_increase_alignment;
extern struct simple_ipa_opt_pass pass_ipa_function_and_variable_visibility;

extern struct gimple_opt_pass pass_all_optimizations;
extern struct gimple_opt_pass pass_cleanup_cfg_post_optimizing;
extern struct gimple_opt_pass pass_free_cfg_annotations;
extern struct gimple_opt_pass pass_free_datastructures;
extern struct gimple_opt_pass pass_init_datastructures;
extern struct gimple_opt_pass pass_fixup_cfg;

extern struct rtl_opt_pass pass_expand;
extern struct rtl_opt_pass pass_init_function;
extern struct rtl_opt_pass pass_jump;
extern struct rtl_opt_pass pass_rtl_eh;
extern struct rtl_opt_pass pass_initial_value_sets;
extern struct rtl_opt_pass pass_unshare_all_rtl;
extern struct rtl_opt_pass pass_instantiate_virtual_regs;
extern struct rtl_opt_pass pass_rtl_fwprop;
extern struct rtl_opt_pass pass_rtl_fwprop_addr;
extern struct rtl_opt_pass pass_jump2;
extern struct rtl_opt_pass pass_lower_subreg;
extern struct rtl_opt_pass pass_cse;
extern struct rtl_opt_pass pass_fast_rtl_dce;
extern struct rtl_opt_pass pass_ud_rtl_dce;
extern struct rtl_opt_pass pass_rtl_dce;
extern struct rtl_opt_pass pass_rtl_dse1;
extern struct rtl_opt_pass pass_rtl_dse2;
extern struct rtl_opt_pass pass_rtl_dse3;
extern struct rtl_opt_pass pass_gcse;
extern struct rtl_opt_pass pass_jump_bypass;
extern struct rtl_opt_pass pass_profiling;
extern struct rtl_opt_pass pass_rtl_ifcvt;
extern struct gimple_opt_pass pass_tracer;

extern struct rtl_opt_pass pass_into_cfg_layout_mode;
extern struct rtl_opt_pass pass_outof_cfg_layout_mode;

extern struct rtl_opt_pass pass_loop2;
extern struct rtl_opt_pass pass_rtl_loop_init;
extern struct rtl_opt_pass pass_rtl_move_loop_invariants;
extern struct rtl_opt_pass pass_rtl_unswitch;
extern struct rtl_opt_pass pass_rtl_unroll_and_peel_loops;
extern struct rtl_opt_pass pass_rtl_doloop;
extern struct rtl_opt_pass pass_rtl_loop_done;

extern struct rtl_opt_pass pass_web;
extern struct rtl_opt_pass pass_cse2;
extern struct rtl_opt_pass pass_df_initialize_opt;
extern struct rtl_opt_pass pass_df_initialize_no_opt;
extern struct rtl_opt_pass pass_reginfo_init;
extern struct rtl_opt_pass pass_subregs_of_mode_init;
extern struct rtl_opt_pass pass_subregs_of_mode_finish;
extern struct rtl_opt_pass pass_inc_dec;
extern struct rtl_opt_pass pass_stack_ptr_mod;
extern struct rtl_opt_pass pass_initialize_regs;
extern struct rtl_opt_pass pass_combine;
extern struct rtl_opt_pass pass_if_after_combine;
extern struct rtl_opt_pass pass_partition_blocks;
extern struct rtl_opt_pass pass_match_asm_constraints;
extern struct rtl_opt_pass pass_regmove;
extern struct rtl_opt_pass pass_split_all_insns;
extern struct rtl_opt_pass pass_fast_rtl_byte_dce;
extern struct rtl_opt_pass pass_lower_subreg2;
extern struct rtl_opt_pass pass_mode_switching;
extern struct rtl_opt_pass pass_see;
extern struct rtl_opt_pass pass_sms;
extern struct rtl_opt_pass pass_sched;
extern struct rtl_opt_pass pass_ira;
extern struct rtl_opt_pass pass_postreload;
extern struct rtl_opt_pass pass_clean_state;
extern struct rtl_opt_pass pass_branch_prob;
extern struct rtl_opt_pass pass_value_profile_transformations;
extern struct rtl_opt_pass pass_postreload_cse;
extern struct rtl_opt_pass pass_gcse2;
extern struct rtl_opt_pass pass_split_after_reload;
extern struct rtl_opt_pass pass_branch_target_load_optimize1;
extern struct rtl_opt_pass pass_thread_prologue_and_epilogue;
extern struct rtl_opt_pass pass_stack_adjustments;
extern struct rtl_opt_pass pass_peephole2;
extern struct rtl_opt_pass pass_if_after_reload;
extern struct rtl_opt_pass pass_regrename;
extern struct rtl_opt_pass pass_cprop_hardreg;
extern struct rtl_opt_pass pass_reorder_blocks;
extern struct rtl_opt_pass pass_branch_target_load_optimize2;
extern struct rtl_opt_pass pass_leaf_regs;
extern struct rtl_opt_pass pass_split_before_sched2;
extern struct rtl_opt_pass pass_sched2;
extern struct rtl_opt_pass pass_stack_regs;
extern struct rtl_opt_pass pass_stack_regs_run;
extern struct rtl_opt_pass pass_df_finish;
extern struct rtl_opt_pass pass_compute_alignments;
extern struct rtl_opt_pass pass_duplicate_computed_gotos;
extern struct rtl_opt_pass pass_variable_tracking;
extern struct rtl_opt_pass pass_free_cfg;
extern struct rtl_opt_pass pass_machine_reorg;
extern struct rtl_opt_pass pass_cleanup_barriers;
extern struct rtl_opt_pass pass_delay_slots;
extern struct rtl_opt_pass pass_split_for_shorten_branches;
extern struct rtl_opt_pass pass_split_before_regstack;
extern struct rtl_opt_pass pass_convert_to_eh_region_ranges;
extern struct rtl_opt_pass pass_shorten_branches;
extern struct rtl_opt_pass pass_set_nothrow_function_flags;
extern struct rtl_opt_pass pass_final;
extern struct rtl_opt_pass pass_rtl_seqabstr;
extern struct gimple_opt_pass pass_release_ssa_names;
extern struct gimple_opt_pass pass_early_inline;
extern struct gimple_opt_pass pass_inline_parameters;
extern struct gimple_opt_pass pass_all_early_optimizations;
extern struct gimple_opt_pass pass_update_address_taken;
extern struct gimple_opt_pass pass_convert_switch;

/* The root of the compilation pass tree, once constructed.  */
extern struct opt_pass *all_passes, *all_ipa_passes, *all_lowering_passes;

/* Current optimization pass.  */
extern struct opt_pass *current_pass;

extern struct opt_pass * get_pass_for_id (int);
extern void execute_pass_list (struct opt_pass *);
extern void execute_ipa_pass_list (struct opt_pass *);
extern void print_current_pass (FILE *);
extern void debug_pass (void);

/* Set to true if the pass is called the first time during compilation of the
   current function.  Note that using this information in the optimization
   passes is considered not to be clean, and it should be avoided if possible.
   This flag is currently used to prevent loops from being peeled repeatedly
   in jump threading; it will be removed once we preserve loop structures
   throughout the compilation -- we will be able to mark the affected loops
   directly in jump threading, and avoid peeling them next time.  */
extern bool first_pass_instance;

#endif /* GCC_TREE_PASS_H */
