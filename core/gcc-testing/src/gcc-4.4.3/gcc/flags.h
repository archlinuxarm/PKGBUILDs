/* Compilation switch flag definitions for GCC.
   Copyright (C) 1987, 1988, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2002,
   2003, 2004, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.

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

#ifndef GCC_FLAGS_H
#define GCC_FLAGS_H

#include "coretypes.h"
#include "options.h"
#include "real.h"

enum debug_info_type
{
  NO_DEBUG,	    /* Write no debug info.  */
  DBX_DEBUG,	    /* Write BSD .stabs for DBX (using dbxout.c).  */
  SDB_DEBUG,	    /* Write COFF for (old) SDB (using sdbout.c).  */
  DWARF2_DEBUG,	    /* Write Dwarf v2 debug info (using dwarf2out.c).  */
  XCOFF_DEBUG,	    /* Write IBM/Xcoff debug info (using dbxout.c).  */
  VMS_DEBUG,        /* Write VMS debug info (using vmsdbgout.c).  */
  VMS_AND_DWARF2_DEBUG /* Write VMS debug info (using vmsdbgout.c).
                          and DWARF v2 debug info (using dwarf2out.c).  */
};

/* Specify which kind of debugging info to generate.  */
extern enum debug_info_type write_symbols;

/* Names of debug_info_type, for error messages.  */
extern const char *const debug_type_names[];

enum debug_info_level
{
  DINFO_LEVEL_NONE,	/* Write no debugging info.  */
  DINFO_LEVEL_TERSE,	/* Write minimal info to support tracebacks only.  */
  DINFO_LEVEL_NORMAL,	/* Write info for all declarations (and line table).  */
  DINFO_LEVEL_VERBOSE	/* Write normal info plus #define/#undef info.  */
};

/* Specify how much debugging info to generate.  */
extern enum debug_info_level debug_info_level;

/* A major contribution to object and executable size is debug
   information size.  A major contribution to debug information
   size is struct descriptions replicated in several object files.
   The following function determines whether or not debug information
   should be generated for a given struct.  The indirect parameter
   indicates that the struct is being handled indirectly, via
   a pointer.  See opts.c for the implementation. */

enum debug_info_usage
{
  DINFO_USAGE_DFN,	/* A struct definition. */
  DINFO_USAGE_DIR_USE,	/* A direct use, such as the type of a variable. */
  DINFO_USAGE_IND_USE,	/* An indirect use, such as through a pointer. */
  DINFO_USAGE_NUM_ENUMS	/* The number of enumerators. */
};

extern bool should_emit_struct_debug (tree type_decl, enum debug_info_usage);
extern void set_struct_debug_option (const char *value);

/* Nonzero means use GNU-only extensions in the generated symbolic
   debugging information.  */
extern bool use_gnu_debug_info_extensions;

/* Enumerate visibility settings.  This is deliberately ordered from most
   to least visibility.  */
#ifndef SYMBOL_VISIBILITY_DEFINED
#define SYMBOL_VISIBILITY_DEFINED
enum symbol_visibility
{
  VISIBILITY_DEFAULT,
  VISIBILITY_PROTECTED,
  VISIBILITY_HIDDEN,
  VISIBILITY_INTERNAL
};
#endif

/* The default visibility for all symbols (unless overridden).  */
extern enum symbol_visibility default_visibility;

struct visibility_flags
{
  unsigned inpragma : 1;	/* True when in #pragma GCC visibility.  */
  unsigned inlines_hidden : 1;	/* True when -finlineshidden in effect.  */
};

/* Global visibility options.  */
extern struct visibility_flags visibility_options;

/* Nonzero means do optimizations.  -opt.  */

extern int optimize;

/* Nonzero means optimize for size.  -Os.  */

extern int optimize_size;

/* Do print extra warnings (such as for uninitialized variables).
   -W/-Wextra.  */

extern bool extra_warnings;

/* Used to set the level of -Wstrict-aliasing, when no level is specified.  
   The external way to set the default level is to use
   -Wstrict-aliasing=level.  
   ONOFF is assumed to take value 1 when -Wstrict-aliasing is specified,
   and 0 otherwise.  After calling this function, wstrict_aliasing will be
   set to the default value of -Wstrict_aliasing=level.  */

extern void set_Wstrict_aliasing (int onoff);

/* Nonzero means warn about any objects definitions whose size is larger
   than N bytes.  Also want about function definitions whose returned
   values are larger than N bytes. The value N is in `larger_than_size'.  */

extern bool warn_larger_than;
extern HOST_WIDE_INT larger_than_size;

/* Nonzero means warn about any function whose frame size is larger
   than N bytes. */

extern bool warn_frame_larger_than;
extern HOST_WIDE_INT frame_larger_than_size;

/* Nonzero for -dp: annotate the assembly with a comment describing the
   pattern and alternative used.  */

extern int flag_print_asm_name;

/* Now the symbols that are set with `-f' switches.  */

/* Nonzero means `char' should be signed.  */

extern int flag_signed_char;

/* Nonzero means give an enum type only as many bytes as it needs.  A value
   of 2 means it has not yet been initialized.  */

extern int flag_short_enums;

/* Nonzero for -fpcc-struct-return: return values the same way PCC does.  */

extern int flag_pcc_struct_return;

/* 0 means straightforward implementation of complex divide acceptable.
   1 means wide ranges of inputs must work for complex divide.
   2 means C99-like requirements for complex multiply and divide.  */

extern int flag_complex_method;

/* Nonzero if we are only using compiler to check syntax errors.  */

extern int rtl_dump_and_exit;

/* Nonzero means we should save auxiliary info into a .X file.  */

extern int flag_gen_aux_info;

/* Nonzero means suppress output of instruction numbers and line number
   notes in debugging dumps.  */

extern int flag_dump_unnumbered;

/* Nonzero means change certain warnings into errors.
   Usually these are warnings about failure to conform to some standard.  */

extern int flag_pedantic_errors;

/* Nonzero means make permerror produce warnings instead of errors.  */

extern int flag_permissive;

/* Nonzero if we are compiling code for a shared library, zero for
   executable.  */

extern int flag_shlib;

/* -dA causes debug information to be produced in
   the generated assembly code (to make it more readable).  This option
   is generally only of use to those who actually need to read the
   generated assembly code (perhaps while debugging the compiler itself).
   Currently, this switch is only used by dwarfout.c; however, it is intended
   to be a catchall for printing debug information in the assembler file.  */

extern int flag_debug_asm;

/* Generate code for GNU or NeXT Objective-C runtime environment.  */

extern int flag_next_runtime;

extern int flag_dump_rtl_in_asm;

/* The algorithm used for the integrated register allocator (IRA).  */
enum ira_algorithm
{
  IRA_ALGORITHM_CB,
  IRA_ALGORITHM_PRIORITY
};

extern enum ira_algorithm flag_ira_algorithm;

/* The regions used for the integrated register allocator (IRA).  */
enum ira_region
{
  IRA_REGION_ONE,
  IRA_REGION_ALL,
  IRA_REGION_MIXED
};

extern enum ira_region flag_ira_region;

extern unsigned int flag_ira_verbose;


/* Other basic status info about current function.  */

/* Nonzero if subexpressions must be evaluated from left-to-right.  */
extern int flag_evaluation_order;

/* Value of the -G xx switch, and whether it was passed or not.  */
extern unsigned HOST_WIDE_INT g_switch_value;
extern bool g_switch_set;

/* Same for selective scheduling.  */
extern bool sel_sched_switch_set;

/* Values of the -falign-* flags: how much to align labels in code. 
   0 means `use default', 1 means `don't align'.  
   For each variable, there is an _log variant which is the power
   of two not less than the variable, for .align output.  */

extern int align_loops_log;
extern int align_loops_max_skip;
extern int align_jumps_log;
extern int align_jumps_max_skip;
extern int align_labels_log;
extern int align_labels_max_skip;
extern int align_functions_log;

/* Nonzero if we dump in VCG format, not plain text.  */
extern int dump_for_graph;

/* Selection of the graph form.  */
enum graph_dump_types
{
  no_graph = 0,
  vcg
};
extern enum graph_dump_types graph_dump_format;

/* Nonzero means to collect statistics which might be expensive
   and to print them when we are done.  */
extern int flag_detailed_statistics;

/* Nonzero means that we defer emitting functions until they are actually
   used.  */
extern int flag_remove_unreachable_functions;

/* Nonzero if we should track variables.  */
extern int flag_var_tracking;

/* True if flag_speculative_prefetching was set by user.  Used to suppress
   warning message in case flag was set by -fprofile-{generate,use}.  */
extern bool flag_speculative_prefetching_set;

/* Type of stack check.  */
enum stack_check_type
{
  /* Do not check the stack.  */
  NO_STACK_CHECK = 0,

  /* Check the stack generically, i.e. assume no specific support
     from the target configuration files.  */
  GENERIC_STACK_CHECK,

  /* Check the stack and rely on the target configuration files to
     check the static frame of functions, i.e. use the generic
     mechanism only for dynamic stack allocations.  */
  STATIC_BUILTIN_STACK_CHECK,

  /* Check the stack and entirely rely on the target configuration
     files, i.e. do not use the generic mechanism at all.  */
  FULL_BUILTIN_STACK_CHECK
};
extern enum stack_check_type flag_stack_check;

/* Returns TRUE if generated code should match ABI version N or
   greater is in use.  */

#define abi_version_at_least(N) \
  (flag_abi_version == 0 || flag_abi_version >= (N))

/* Return whether the function should be excluded from
   instrumentation.  */
extern bool flag_instrument_functions_exclude_p (tree fndecl);

/* True if the given mode has a NaN representation and the treatment of
   NaN operands is important.  Certain optimizations, such as folding
   x * 0 into 0, are not correct for NaN operands, and are normally
   disabled for modes with NaNs.  The user can ask for them to be
   done anyway using the -funsafe-math-optimizations switch.  */
#define HONOR_NANS(MODE) \
  (MODE_HAS_NANS (MODE) && !flag_finite_math_only)

/* Like HONOR_NANs, but true if we honor signaling NaNs (or sNaNs).  */
#define HONOR_SNANS(MODE) (flag_signaling_nans && HONOR_NANS (MODE))

/* As for HONOR_NANS, but true if the mode can represent infinity and
   the treatment of infinite values is important.  */
#define HONOR_INFINITIES(MODE) \
  (MODE_HAS_INFINITIES (MODE) && !flag_finite_math_only)

/* Like HONOR_NANS, but true if the given mode distinguishes between
   positive and negative zero, and the sign of zero is important.  */
#define HONOR_SIGNED_ZEROS(MODE) \
  (MODE_HAS_SIGNED_ZEROS (MODE) && flag_signed_zeros)

/* Like HONOR_NANS, but true if given mode supports sign-dependent rounding,
   and the rounding mode is important.  */
#define HONOR_SIGN_DEPENDENT_ROUNDING(MODE) \
  (MODE_HAS_SIGN_DEPENDENT_ROUNDING (MODE) && flag_rounding_math)

/* True if overflow wraps around for the given integral type.  That
   is, TYPE_MAX + 1 == TYPE_MIN.  */
#define TYPE_OVERFLOW_WRAPS(TYPE) \
  (TYPE_UNSIGNED (TYPE) || flag_wrapv)

/* True if overflow is undefined for the given integral type.  We may
   optimize on the assumption that values in the type never overflow.

   IMPORTANT NOTE: Any optimization based on TYPE_OVERFLOW_UNDEFINED
   must issue a warning based on warn_strict_overflow.  In some cases
   it will be appropriate to issue the warning immediately, and in
   other cases it will be appropriate to simply set a flag and let the
   caller decide whether a warning is appropriate or not.  */
#define TYPE_OVERFLOW_UNDEFINED(TYPE) \
  (!TYPE_UNSIGNED (TYPE) && !flag_wrapv && !flag_trapv && flag_strict_overflow)

/* True if overflow for the given integral type should issue a
   trap.  */
#define TYPE_OVERFLOW_TRAPS(TYPE) \
  (!TYPE_UNSIGNED (TYPE) && flag_trapv)

/* True if pointer types have undefined overflow.  */
#define POINTER_TYPE_OVERFLOW_UNDEFINED (flag_strict_overflow)

/* Names for the different levels of -Wstrict-overflow=N.  The numeric
   values here correspond to N.  */

enum warn_strict_overflow_code
{
  /* Overflow warning that should be issued with -Wall: a questionable
     construct that is easy to avoid even when using macros.  Example:
     folding (x + CONSTANT > x) to 1.  */
  WARN_STRICT_OVERFLOW_ALL = 1,
  /* Overflow warning about folding a comparison to a constant because
     of undefined signed overflow, other than cases covered by
     WARN_STRICT_OVERFLOW_ALL.  Example: folding (abs (x) >= 0) to 1
     (this is false when x == INT_MIN).  */
  WARN_STRICT_OVERFLOW_CONDITIONAL = 2,
  /* Overflow warning about changes to comparisons other than folding
     them to a constant.  Example: folding (x + 1 > 1) to (x > 0).  */
  WARN_STRICT_OVERFLOW_COMPARISON = 3,
  /* Overflow warnings not covered by the above cases.  Example:
     folding ((x * 10) / 5) to (x * 2).  */
  WARN_STRICT_OVERFLOW_MISC = 4,
  /* Overflow warnings about reducing magnitude of constants in
     comparison.  Example: folding (x + 2 > y) to (x + 1 >= y).  */
  WARN_STRICT_OVERFLOW_MAGNITUDE = 5
};

/* Whether to emit an overflow warning whose code is C.  */
#define issue_strict_overflow_warning(c) (warn_strict_overflow >= (int) (c))

#endif /* ! GCC_FLAGS_H */
