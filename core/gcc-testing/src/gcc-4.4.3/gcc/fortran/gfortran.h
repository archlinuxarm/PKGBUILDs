/* gfortran header file
   Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
   Free Software Foundation, Inc.
   Contributed by Andy Vaught

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

#ifndef GCC_GFORTRAN_H
#define GCC_GFORTRAN_H

/* It's probably insane to have this large of a header file, but it
   seemed like everything had to be recompiled anyway when a change
   was made to a header file, and there were ordering issues with
   multiple header files.  Besides, Microsoft's winnt.h was 250k last
   time I looked, so by comparison this is perfectly reasonable.  */

/* Declarations common to the front-end and library are put in
   libgfortran/libgfortran_frontend.h  */
#include "libgfortran.h"


#include "system.h"
#include "intl.h"
#include "coretypes.h"
#include "input.h"
#include "splay-tree.h"
/* The following ifdefs are recommended by the autoconf documentation
   for any code using alloca.  */

/* AIX requires this to be the first thing in the file.  */
#ifdef __GNUC__
#else /* not __GNUC__ */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else /* do not HAVE_ALLOCA_H */
#ifdef _AIX
#pragma alloca
#else
#ifndef alloca			/* predefined by HP cc +Olibcalls */
char *alloca ();
#endif /* not predefined */
#endif /* not _AIX */
#endif /* do not HAVE_ALLOCA_H */
#endif /* not __GNUC__ */

/* Major control parameters.  */

#define GFC_MAX_SYMBOL_LEN 63   /* Must be at least 63 for F2003.  */
#define GFC_MAX_BINDING_LABEL_LEN 126 /* (2 * GFC_MAX_SYMBOL_LEN) */
#define GFC_MAX_LINE 132	/* Characters beyond this are not seen.  */
#define GFC_LETTERS 26		/* Number of letters in the alphabet.  */

#define MAX_SUBRECORD_LENGTH 2147483639   /* 2**31-9 */


#define free(x) Use_gfc_free_instead_of_free()
#define gfc_is_whitespace(c) ((c==' ') || (c=='\t'))

#ifndef NULL
#define NULL ((void *) 0)
#endif

/* Stringization.  */
#define stringize(x) expand_macro(x)
#define expand_macro(x) # x

/* For the runtime library, a standard prefix is a requirement to
   avoid cluttering the namespace with things nobody asked for.  It's
   ugly to look at and a pain to type when you add the prefix by hand,
   so we hide it behind a macro.  */
#define PREFIX(x) "_gfortran_" x
#define PREFIX_LEN 10

#define BLANK_COMMON_NAME "__BLNK__"

/* Macro to initialize an mstring structure.  */
#define minit(s, t) { s, NULL, t }

/* Structure for storing strings to be matched by gfc_match_string.  */
typedef struct
{
  const char *string;
  const char *mp;
  int tag;
}
mstring;



/*************************** Enums *****************************/

/* Used when matching and resolving data I/O transfer statements.  */

typedef enum
{ M_READ, M_WRITE, M_PRINT, M_INQUIRE }
io_kind;

/* The author remains confused to this day about the convention of
   returning '0' for 'SUCCESS'... or was it the other way around?  The
   following enum makes things much more readable.  We also start
   values off at one instead of zero.  */

typedef enum
{ SUCCESS = 1, FAILURE }
gfc_try;

/* This is returned by gfc_notification_std to know if, given the flags
   that were given (-std=, -pedantic) we should issue an error, a warning
   or nothing.  */

typedef enum
{ SILENT, WARNING, ERROR }
notification;

/* Matchers return one of these three values.  The difference between
   MATCH_NO and MATCH_ERROR is that MATCH_ERROR means that a match was
   successful, but that something non-syntactic is wrong and an error
   has already been issued.  */

typedef enum
{ MATCH_NO = 1, MATCH_YES, MATCH_ERROR }
match;

typedef enum
{ FORM_FREE, FORM_FIXED, FORM_UNKNOWN }
gfc_source_form;

/* Basic types.  BT_VOID is used by ISO C Binding so funcs like c_f_pointer
   can take any arg with the pointer attribute as a param.  */
typedef enum
{ BT_UNKNOWN = 1, BT_INTEGER, BT_REAL, BT_COMPLEX,
  BT_LOGICAL, BT_CHARACTER, BT_DERIVED, BT_PROCEDURE, BT_HOLLERITH,
  BT_VOID
}
bt;

/* Expression node types.  */
typedef enum
{ EXPR_OP = 1, EXPR_FUNCTION, EXPR_CONSTANT, EXPR_VARIABLE,
  EXPR_SUBSTRING, EXPR_STRUCTURE, EXPR_ARRAY, EXPR_NULL, EXPR_COMPCALL
}
expr_t;

/* Array types.  */
typedef enum
{ AS_EXPLICIT = 1, AS_ASSUMED_SHAPE, AS_DEFERRED,
  AS_ASSUMED_SIZE, AS_UNKNOWN
}
array_type;

typedef enum
{ AR_FULL = 1, AR_ELEMENT, AR_SECTION, AR_UNKNOWN }
ar_type;

/* Statement label types.  */
typedef enum
{ ST_LABEL_UNKNOWN = 1, ST_LABEL_TARGET,
  ST_LABEL_BAD_TARGET, ST_LABEL_FORMAT
}
gfc_sl_type;

/* Intrinsic operators.  */
typedef enum
{ GFC_INTRINSIC_BEGIN = 0,
  INTRINSIC_NONE = -1, INTRINSIC_UPLUS = GFC_INTRINSIC_BEGIN,
  INTRINSIC_UMINUS, INTRINSIC_PLUS, INTRINSIC_MINUS, INTRINSIC_TIMES,
  INTRINSIC_DIVIDE, INTRINSIC_POWER, INTRINSIC_CONCAT,
  INTRINSIC_AND, INTRINSIC_OR, INTRINSIC_EQV, INTRINSIC_NEQV,
  /* ==, /=, >, >=, <, <=  */
  INTRINSIC_EQ, INTRINSIC_NE, INTRINSIC_GT, INTRINSIC_GE,
  INTRINSIC_LT, INTRINSIC_LE, 
  /* .EQ., .NE., .GT., .GE., .LT., .LE. (OS = Old-Style)  */
  INTRINSIC_EQ_OS, INTRINSIC_NE_OS, INTRINSIC_GT_OS, INTRINSIC_GE_OS,
  INTRINSIC_LT_OS, INTRINSIC_LE_OS, 
  INTRINSIC_NOT, INTRINSIC_USER, INTRINSIC_ASSIGN, 
  INTRINSIC_PARENTHESES, GFC_INTRINSIC_END /* Sentinel */
}
gfc_intrinsic_op;


/* This macro is the number of intrinsic operators that exist.
   Assumptions are made about the numbering of the interface_op enums.  */
#define GFC_INTRINSIC_OPS GFC_INTRINSIC_END

/* Arithmetic results.  */
typedef enum
{ ARITH_OK = 1, ARITH_OVERFLOW, ARITH_UNDERFLOW, ARITH_NAN,
  ARITH_DIV0, ARITH_INCOMMENSURATE, ARITH_ASYMMETRIC
}
arith;

/* Statements.  */
typedef enum
{
  ST_ARITHMETIC_IF, ST_ALLOCATE, ST_ATTR_DECL, ST_BACKSPACE, ST_BLOCK_DATA,
  ST_CALL, ST_CASE, ST_CLOSE, ST_COMMON, ST_CONTINUE, ST_CONTAINS, ST_CYCLE,
  ST_DATA, ST_DATA_DECL, ST_DEALLOCATE, ST_DO, ST_ELSE, ST_ELSEIF,
  ST_ELSEWHERE, ST_END_BLOCK_DATA, ST_ENDDO, ST_IMPLIED_ENDDO,
  ST_END_FILE, ST_FINAL, ST_FLUSH, ST_END_FORALL, ST_END_FUNCTION, ST_ENDIF,
  ST_END_INTERFACE, ST_END_MODULE, ST_END_PROGRAM, ST_END_SELECT,
  ST_END_SUBROUTINE, ST_END_WHERE, ST_END_TYPE, ST_ENTRY, ST_EQUIVALENCE,
  ST_EXIT, ST_FORALL, ST_FORALL_BLOCK, ST_FORMAT, ST_FUNCTION, ST_GOTO,
  ST_IF_BLOCK, ST_IMPLICIT, ST_IMPLICIT_NONE, ST_IMPORT, ST_INQUIRE, ST_INTERFACE,
  ST_PARAMETER, ST_MODULE, ST_MODULE_PROC, ST_NAMELIST, ST_NULLIFY, ST_OPEN,
  ST_PAUSE, ST_PRIVATE, ST_PROGRAM, ST_PUBLIC, ST_READ, ST_RETURN, ST_REWIND,
  ST_STOP, ST_SUBROUTINE, ST_TYPE, ST_USE, ST_WHERE_BLOCK, ST_WHERE, ST_WAIT, 
  ST_WRITE, ST_ASSIGNMENT, ST_POINTER_ASSIGNMENT, ST_SELECT_CASE, ST_SEQUENCE,
  ST_SIMPLE_IF, ST_STATEMENT_FUNCTION, ST_DERIVED_DECL, ST_LABEL_ASSIGNMENT,
  ST_ENUM, ST_ENUMERATOR, ST_END_ENUM,
  ST_OMP_ATOMIC, ST_OMP_BARRIER, ST_OMP_CRITICAL, ST_OMP_END_CRITICAL,
  ST_OMP_END_DO, ST_OMP_END_MASTER, ST_OMP_END_ORDERED, ST_OMP_END_PARALLEL,
  ST_OMP_END_PARALLEL_DO, ST_OMP_END_PARALLEL_SECTIONS,
  ST_OMP_END_PARALLEL_WORKSHARE, ST_OMP_END_SECTIONS, ST_OMP_END_SINGLE,
  ST_OMP_END_WORKSHARE, ST_OMP_DO, ST_OMP_FLUSH, ST_OMP_MASTER, ST_OMP_ORDERED,
  ST_OMP_PARALLEL, ST_OMP_PARALLEL_DO, ST_OMP_PARALLEL_SECTIONS,
  ST_OMP_PARALLEL_WORKSHARE, ST_OMP_SECTIONS, ST_OMP_SECTION, ST_OMP_SINGLE,
  ST_OMP_THREADPRIVATE, ST_OMP_WORKSHARE, ST_OMP_TASK, ST_OMP_END_TASK,
  ST_OMP_TASKWAIT, ST_PROCEDURE, ST_GENERIC,
  ST_GET_FCN_CHARACTERISTICS, ST_NONE
}
gfc_statement;


/* Types of interfaces that we can have.  Assignment interfaces are
   considered to be intrinsic operators.  */
typedef enum
{
  INTERFACE_NAMELESS = 1, INTERFACE_GENERIC,
  INTERFACE_INTRINSIC_OP, INTERFACE_USER_OP, INTERFACE_ABSTRACT
}
interface_type;

/* Symbol flavors: these are all mutually exclusive.
   10 elements = 4 bits.  */
typedef enum sym_flavor
{
  FL_UNKNOWN = 0, FL_PROGRAM, FL_BLOCK_DATA, FL_MODULE, FL_VARIABLE,
  FL_PARAMETER, FL_LABEL, FL_PROCEDURE, FL_DERIVED, FL_NAMELIST,
  FL_VOID
}
sym_flavor;

/* Procedure types.  7 elements = 3 bits.  */
typedef enum procedure_type
{ PROC_UNKNOWN, PROC_MODULE, PROC_INTERNAL, PROC_DUMMY,
  PROC_INTRINSIC, PROC_ST_FUNCTION, PROC_EXTERNAL
}
procedure_type;

/* Intent types.  */
typedef enum sym_intent
{ INTENT_UNKNOWN = 0, INTENT_IN, INTENT_OUT, INTENT_INOUT
}
sym_intent;

/* Access types.  */
typedef enum gfc_access
{ ACCESS_UNKNOWN = 0, ACCESS_PUBLIC, ACCESS_PRIVATE
}
gfc_access;

/* Flags to keep track of where an interface came from.
   4 elements = 2 bits.  */
typedef enum ifsrc
{ IFSRC_UNKNOWN = 0, IFSRC_DECL, IFSRC_IFBODY, IFSRC_USAGE
}
ifsrc;

/* Whether a SAVE attribute was set explicitly or implicitly.  */
typedef enum save_state
{ SAVE_NONE = 0, SAVE_EXPLICIT, SAVE_IMPLICIT
}
save_state;

/* Strings for all symbol attributes.  We use these for dumping the
   parse tree, in error messages, and also when reading and writing
   modules.  In symbol.c.  */
extern const mstring flavors[];
extern const mstring procedures[];
extern const mstring intents[];
extern const mstring access_types[];
extern const mstring ifsrc_types[];
extern const mstring save_status[];

/* Enumeration of all the generic intrinsic functions.  Used by the
   backend for identification of a function.  */

enum gfc_isym_id
{
  /* GFC_ISYM_NONE is used for intrinsics which will never be seen by
     the backend (e.g. KIND).  */
  GFC_ISYM_NONE = 0,
  GFC_ISYM_ABORT,
  GFC_ISYM_ABS,
  GFC_ISYM_ACCESS,
  GFC_ISYM_ACHAR,
  GFC_ISYM_ACOS,
  GFC_ISYM_ACOSH,
  GFC_ISYM_ADJUSTL,
  GFC_ISYM_ADJUSTR,
  GFC_ISYM_AIMAG,
  GFC_ISYM_AINT,
  GFC_ISYM_ALARM,
  GFC_ISYM_ALL,
  GFC_ISYM_ALLOCATED,
  GFC_ISYM_AND,
  GFC_ISYM_ANINT,
  GFC_ISYM_ANY,
  GFC_ISYM_ASIN,
  GFC_ISYM_ASINH,
  GFC_ISYM_ASSOCIATED,
  GFC_ISYM_ATAN,
  GFC_ISYM_ATAN2,
  GFC_ISYM_ATANH,
  GFC_ISYM_BIT_SIZE,
  GFC_ISYM_BTEST,
  GFC_ISYM_CEILING,
  GFC_ISYM_CHAR,
  GFC_ISYM_CHDIR,
  GFC_ISYM_CHMOD,
  GFC_ISYM_CMPLX,
  GFC_ISYM_COMMAND_ARGUMENT_COUNT,
  GFC_ISYM_COMPLEX,
  GFC_ISYM_CONJG,
  GFC_ISYM_CONVERSION,
  GFC_ISYM_COS,
  GFC_ISYM_COSH,
  GFC_ISYM_COUNT,
  GFC_ISYM_CPU_TIME,
  GFC_ISYM_CSHIFT,
  GFC_ISYM_CTIME,
  GFC_ISYM_DATE_AND_TIME,
  GFC_ISYM_DBLE,
  GFC_ISYM_DIGITS,
  GFC_ISYM_DIM,
  GFC_ISYM_DOT_PRODUCT,
  GFC_ISYM_DPROD,
  GFC_ISYM_DTIME,
  GFC_ISYM_EOSHIFT,
  GFC_ISYM_EPSILON,
  GFC_ISYM_ERF,
  GFC_ISYM_ERFC,
  GFC_ISYM_ERFC_SCALED,
  GFC_ISYM_ETIME,
  GFC_ISYM_EXIT,
  GFC_ISYM_EXP,
  GFC_ISYM_EXPONENT,
  GFC_ISYM_FDATE,
  GFC_ISYM_FGET,
  GFC_ISYM_FGETC,
  GFC_ISYM_FLOOR,
  GFC_ISYM_FLUSH,
  GFC_ISYM_FNUM,
  GFC_ISYM_FPUT,
  GFC_ISYM_FPUTC,
  GFC_ISYM_FRACTION,
  GFC_ISYM_FREE,
  GFC_ISYM_FSEEK,
  GFC_ISYM_FSTAT,
  GFC_ISYM_FTELL,
  GFC_ISYM_GAMMA,
  GFC_ISYM_GERROR,
  GFC_ISYM_GETARG,
  GFC_ISYM_GET_COMMAND,
  GFC_ISYM_GET_COMMAND_ARGUMENT,
  GFC_ISYM_GETCWD,
  GFC_ISYM_GETENV,
  GFC_ISYM_GET_ENVIRONMENT_VARIABLE,
  GFC_ISYM_GETGID,
  GFC_ISYM_GETLOG,
  GFC_ISYM_GETPID,
  GFC_ISYM_GETUID,
  GFC_ISYM_GMTIME,
  GFC_ISYM_HOSTNM,
  GFC_ISYM_HUGE,
  GFC_ISYM_HYPOT,
  GFC_ISYM_IACHAR,
  GFC_ISYM_IAND,
  GFC_ISYM_IARGC,
  GFC_ISYM_IBCLR,
  GFC_ISYM_IBITS,
  GFC_ISYM_IBSET,
  GFC_ISYM_ICHAR,
  GFC_ISYM_IDATE,
  GFC_ISYM_IEOR,
  GFC_ISYM_IERRNO,
  GFC_ISYM_INDEX,
  GFC_ISYM_INT,
  GFC_ISYM_INT2,
  GFC_ISYM_INT8,
  GFC_ISYM_IOR,
  GFC_ISYM_IRAND,
  GFC_ISYM_ISATTY,
  GFC_ISYM_IS_IOSTAT_END,
  GFC_ISYM_IS_IOSTAT_EOR,
  GFC_ISYM_ISNAN,
  GFC_ISYM_ISHFT,
  GFC_ISYM_ISHFTC,
  GFC_ISYM_ITIME,
  GFC_ISYM_J0,
  GFC_ISYM_J1,
  GFC_ISYM_JN,
  GFC_ISYM_KILL,
  GFC_ISYM_KIND,
  GFC_ISYM_LBOUND,
  GFC_ISYM_LEADZ,
  GFC_ISYM_LEN,
  GFC_ISYM_LEN_TRIM,
  GFC_ISYM_LGAMMA,
  GFC_ISYM_LGE,
  GFC_ISYM_LGT,
  GFC_ISYM_LINK,
  GFC_ISYM_LLE,
  GFC_ISYM_LLT,
  GFC_ISYM_LOC,
  GFC_ISYM_LOG,
  GFC_ISYM_LOG10,
  GFC_ISYM_LOGICAL,
  GFC_ISYM_LONG,
  GFC_ISYM_LSHIFT,
  GFC_ISYM_LSTAT,
  GFC_ISYM_LTIME,
  GFC_ISYM_MALLOC,
  GFC_ISYM_MATMUL,
  GFC_ISYM_MAX,
  GFC_ISYM_MAXEXPONENT,
  GFC_ISYM_MAXLOC,
  GFC_ISYM_MAXVAL,
  GFC_ISYM_MCLOCK,
  GFC_ISYM_MCLOCK8,
  GFC_ISYM_MERGE,
  GFC_ISYM_MIN,
  GFC_ISYM_MINEXPONENT,
  GFC_ISYM_MINLOC,
  GFC_ISYM_MINVAL,
  GFC_ISYM_MOD,
  GFC_ISYM_MODULO,
  GFC_ISYM_MOVE_ALLOC,
  GFC_ISYM_MVBITS,
  GFC_ISYM_NEAREST,
  GFC_ISYM_NEW_LINE,
  GFC_ISYM_NINT,
  GFC_ISYM_NOT,
  GFC_ISYM_NULL,
  GFC_ISYM_OR,
  GFC_ISYM_PACK,
  GFC_ISYM_PERROR,
  GFC_ISYM_PRECISION,
  GFC_ISYM_PRESENT,
  GFC_ISYM_PRODUCT,
  GFC_ISYM_RADIX,
  GFC_ISYM_RAND,
  GFC_ISYM_RANDOM_NUMBER,
  GFC_ISYM_RANDOM_SEED,
  GFC_ISYM_RANGE,
  GFC_ISYM_REAL,
  GFC_ISYM_RENAME,
  GFC_ISYM_REPEAT,
  GFC_ISYM_RESHAPE,
  GFC_ISYM_RRSPACING,
  GFC_ISYM_RSHIFT,
  GFC_ISYM_SC_KIND,
  GFC_ISYM_SCALE,
  GFC_ISYM_SCAN,
  GFC_ISYM_SECNDS,
  GFC_ISYM_SECOND,
  GFC_ISYM_SET_EXPONENT,
  GFC_ISYM_SHAPE,
  GFC_ISYM_SIGN,
  GFC_ISYM_SIGNAL,
  GFC_ISYM_SI_KIND,
  GFC_ISYM_SIN,
  GFC_ISYM_SINH,
  GFC_ISYM_SIZE,
  GFC_ISYM_SLEEP,
  GFC_ISYM_SIZEOF,
  GFC_ISYM_SPACING,
  GFC_ISYM_SPREAD,
  GFC_ISYM_SQRT,
  GFC_ISYM_SRAND,
  GFC_ISYM_SR_KIND,
  GFC_ISYM_STAT,
  GFC_ISYM_SUM,
  GFC_ISYM_SYMLINK,
  GFC_ISYM_SYMLNK,
  GFC_ISYM_SYSTEM,
  GFC_ISYM_SYSTEM_CLOCK,
  GFC_ISYM_TAN,
  GFC_ISYM_TANH,
  GFC_ISYM_TIME,
  GFC_ISYM_TIME8,
  GFC_ISYM_TINY,
  GFC_ISYM_TRAILZ,
  GFC_ISYM_TRANSFER,
  GFC_ISYM_TRANSPOSE,
  GFC_ISYM_TRIM,
  GFC_ISYM_TTYNAM,
  GFC_ISYM_UBOUND,
  GFC_ISYM_UMASK,
  GFC_ISYM_UNLINK,
  GFC_ISYM_UNPACK,
  GFC_ISYM_VERIFY,
  GFC_ISYM_XOR,
  GFC_ISYM_Y0,
  GFC_ISYM_Y1,
  GFC_ISYM_YN
};
typedef enum gfc_isym_id gfc_isym_id;


typedef enum
{
  GFC_INIT_REAL_OFF = 0,
  GFC_INIT_REAL_ZERO,
  GFC_INIT_REAL_NAN,
  GFC_INIT_REAL_INF,
  GFC_INIT_REAL_NEG_INF
}
init_local_real;

typedef enum
{
  GFC_INIT_LOGICAL_OFF = 0,
  GFC_INIT_LOGICAL_FALSE,
  GFC_INIT_LOGICAL_TRUE
}
init_local_logical;

typedef enum
{
  GFC_INIT_CHARACTER_OFF = 0,
  GFC_INIT_CHARACTER_ON
}
init_local_character;

typedef enum
{
  GFC_INIT_INTEGER_OFF = 0,
  GFC_INIT_INTEGER_ON
}
init_local_integer;

/************************* Structures *****************************/

/* Used for keeping things in balanced binary trees.  */
#define BBT_HEADER(self) int priority; struct self *left, *right

#define NAMED_INTCST(a,b,c,d) a,
typedef enum
{
  ISOFORTRANENV_INVALID = -1,
#include "iso-fortran-env.def"
  ISOFORTRANENV_LAST, ISOFORTRANENV_NUMBER = ISOFORTRANENV_LAST
}
iso_fortran_env_symbol;
#undef NAMED_INTCST

#define NAMED_INTCST(a,b,c,d) a,
#define NAMED_REALCST(a,b,c) a,
#define NAMED_CMPXCST(a,b,c) a,
#define NAMED_LOGCST(a,b,c) a,
#define NAMED_CHARKNDCST(a,b,c) a,
#define NAMED_CHARCST(a,b,c) a,
#define DERIVED_TYPE(a,b,c) a,
#define PROCEDURE(a,b) a,
typedef enum
{
  ISOCBINDING_INVALID = -1, 
#include "iso-c-binding.def"
  ISOCBINDING_LAST,
  ISOCBINDING_NUMBER = ISOCBINDING_LAST
}
iso_c_binding_symbol;
#undef NAMED_INTCST
#undef NAMED_REALCST
#undef NAMED_CMPXCST
#undef NAMED_LOGCST
#undef NAMED_CHARKNDCST
#undef NAMED_CHARCST
#undef DERIVED_TYPE
#undef PROCEDURE

typedef enum
{
  INTMOD_NONE = 0, INTMOD_ISO_FORTRAN_ENV, INTMOD_ISO_C_BINDING
}
intmod_id;

typedef struct
{
  char name[GFC_MAX_SYMBOL_LEN + 1];
  int value;  /* Used for both integer and character values.  */
  bt f90_type;
}
CInteropKind_t;

/* Array of structs, where the structs represent the C interop kinds.
   The list will be implemented based on a hash of the kind name since
   these could be accessed multiple times.
   Declared in trans-types.c as a global, since it's in that file
   that the list is initialized.  */
extern CInteropKind_t c_interop_kinds_table[];

/* Symbol attribute structure.  */
typedef struct
{
  /* Variable attributes.  */
  unsigned allocatable:1, dimension:1, external:1, intrinsic:1,
    optional:1, pointer:1, target:1, value:1, volatile_:1,
    dummy:1, result:1, assign:1, threadprivate:1, not_always_present:1,
    implied_index:1, subref_array_pointer:1, proc_pointer:1;

  ENUM_BITFIELD (save_state) save:2;

  unsigned data:1,		/* Symbol is named in a DATA statement.  */
    is_protected:1,		/* Symbol has been marked as protected.  */
    use_assoc:1,		/* Symbol has been use-associated.  */
    use_only:1,			/* Symbol has been use-associated, with ONLY.  */
    use_rename:1,		/* Symbol has been use-associated and renamed.  */
    imported:1;			/* Symbol has been associated by IMPORT.  */

  unsigned in_namelist:1, in_common:1, in_equivalence:1;
  unsigned function:1, subroutine:1, procedure:1;
  unsigned generic:1, generic_copy:1;
  unsigned implicit_type:1;	/* Type defined via implicit rules.  */
  unsigned untyped:1;		/* No implicit type could be found.  */

  unsigned is_bind_c:1;		/* say if is bound to C.  */
  unsigned extension:1;		/* extends a derived type.  */

  /* These flags are both in the typespec and attribute.  The attribute
     list is what gets read from/written to a module file.  The typespec
     is created from a decl being processed.  */
  unsigned is_c_interop:1;	/* It's c interoperable.  */
  unsigned is_iso_c:1;		/* Symbol is from iso_c_binding.  */

  /* Function/subroutine attributes */
  unsigned sequence:1, elemental:1, pure:1, recursive:1;
  unsigned unmaskable:1, masked:1, contained:1, mod_proc:1, abstract:1;

  /* This is set if the subroutine doesn't return.  Currently, this
     is only possible for intrinsic subroutines.  */
  unsigned noreturn:1;

  /* Set if this procedure is an alternate entry point.  These procedures
     don't have any code associated, and the backend will turn them into
     thunks to the master function.  */
  unsigned entry:1;

  /* Set if this is the master function for a procedure with multiple
     entry points.  */
  unsigned entry_master:1;

  /* Set if this is the master function for a function with multiple
     entry points where characteristics of the entry points differ.  */
  unsigned mixed_entry_master:1;

  /* Set if a function must always be referenced by an explicit interface.  */
  unsigned always_explicit:1;

  /* Set if the symbol has been referenced in an expression.  No further
     modification of type or type parameters is permitted.  */
  unsigned referenced:1;

  /* Set if the symbol has ambiguous interfaces.  */
  unsigned ambiguous_interfaces:1;

  /* Set if this is the symbol for the main program.  */
  unsigned is_main_program:1;

  /* Mutually exclusive multibit attributes.  */
  ENUM_BITFIELD (gfc_access) access:2;
  ENUM_BITFIELD (sym_intent) intent:2;
  ENUM_BITFIELD (sym_flavor) flavor:4;
  ENUM_BITFIELD (ifsrc) if_source:2;

  ENUM_BITFIELD (procedure_type) proc:3;

  /* Special attributes for Cray pointers, pointees.  */
  unsigned cray_pointer:1, cray_pointee:1;

  /* The symbol is a derived type with allocatable components, pointer 
     components or private components, possibly nested.  zero_comp
     is true if the derived type has no component at all.  */
  unsigned alloc_comp:1, pointer_comp:1, private_comp:1, zero_comp:1;

  /* The namespace where the VOLATILE attribute has been set.  */
  struct gfc_namespace *volatile_ns;
}
symbol_attribute;


/* We need to store source lines as sequences of multibyte source
   characters. We define here a type wide enough to hold any multibyte
   source character, just like libcpp does.  A 32-bit type is enough.  */

#if HOST_BITS_PER_INT >= 32
typedef unsigned int gfc_char_t;
#elif HOST_BITS_PER_LONG >= 32
typedef unsigned long gfc_char_t;
#elif defined(HAVE_LONG_LONG) && (HOST_BITS_PER_LONGLONG >= 32)
typedef unsigned long long gfc_char_t;
#else
# error "Cannot find an integer type with at least 32 bits"
#endif


/* The following three structures are used to identify a location in
   the sources.

   gfc_file is used to maintain a tree of the source files and how
   they include each other

   gfc_linebuf holds a single line of source code and information
   which file it resides in

   locus point to the sourceline and the character in the source
   line.
*/

typedef struct gfc_file
{
  struct gfc_file *next, *up;
  int inclusion_line, line;
  char *filename;
} gfc_file;

typedef struct gfc_linebuf
{
  source_location location;
  struct gfc_file *file;
  struct gfc_linebuf *next;

  int truncated;
  bool dbg_emitted;

  gfc_char_t line[1];
} gfc_linebuf;

#define gfc_linebuf_header_size (offsetof (gfc_linebuf, line))

#define gfc_linebuf_linenum(LBUF) (LOCATION_LINE ((LBUF)->location))

typedef struct
{
  gfc_char_t *nextc;
  gfc_linebuf *lb;
} locus;

/* In order for the "gfc" format checking to work correctly, you must
   have declared a typedef locus first.  */
#if GCC_VERSION >= 4001
#define ATTRIBUTE_GCC_GFC(m, n) __attribute__ ((__format__ (__gcc_gfc__, m, n))) ATTRIBUTE_NONNULL(m)
#else
#define ATTRIBUTE_GCC_GFC(m, n) ATTRIBUTE_NONNULL(m)
#endif


/* Suppress error messages or re-enable them.  */

void gfc_push_suppress_errors (void);
void gfc_pop_suppress_errors (void);


/* Character length structures hold the expression that gives the
   length of a character variable.  We avoid putting these into
   gfc_typespec because doing so prevents us from doing structure
   copies and forces us to deallocate any typespecs we create, as well
   as structures that contain typespecs.  They also can have multiple
   character typespecs pointing to them.

   These structures form a singly linked list within the current
   namespace and are deallocated with the namespace.  It is possible to
   end up with gfc_charlen structures that have nothing pointing to them.  */

typedef struct gfc_charlen
{
  struct gfc_expr *length;
  struct gfc_charlen *next;
  bool length_from_typespec; /* Length from explicit array ctor typespec?  */
  tree backend_decl;

  int resolved;
}
gfc_charlen;

#define gfc_get_charlen() XCNEW (gfc_charlen)

/* Type specification structure.  FIXME: derived and cl could be union???  */
typedef struct
{
  bt type;
  int kind;
  struct gfc_symbol *derived;
  gfc_charlen *cl;	/* For character types only.  */
  struct gfc_symbol *interface;	/* For PROCEDURE declarations.  */
  int is_c_interop;
  int is_iso_c;
  bt f90_type; 
}
gfc_typespec;

/* Array specification.  */
typedef struct
{
  int rank;	/* A rank of zero means that a variable is a scalar.  */
  array_type type;
  struct gfc_expr *lower[GFC_MAX_DIMENSIONS], *upper[GFC_MAX_DIMENSIONS];

  /* These two fields are used with the Cray Pointer extension.  */
  bool cray_pointee; /* True iff this spec belongs to a cray pointee.  */
  bool cp_was_assumed; /* AS_ASSUMED_SIZE cp arrays are converted to
			AS_EXPLICIT, but we want to remember that we
			did this.  */

}
gfc_array_spec;

#define gfc_get_array_spec() XCNEW (gfc_array_spec)


/* Components of derived types.  */
typedef struct gfc_component
{
  const char *name;
  gfc_typespec ts;

  symbol_attribute attr;
  gfc_array_spec *as;

  tree backend_decl;
  locus loc;
  struct gfc_expr *initializer;
  struct gfc_component *next;
}
gfc_component;

#define gfc_get_component() XCNEW (gfc_component)

/* Formal argument lists are lists of symbols.  */
typedef struct gfc_formal_arglist
{
  /* Symbol representing the argument at this position in the arglist.  */
  struct gfc_symbol *sym;
  /* Points to the next formal argument.  */
  struct gfc_formal_arglist *next;
}
gfc_formal_arglist;

#define gfc_get_formal_arglist() XCNEW (gfc_formal_arglist)


/* The gfc_actual_arglist structure is for actual arguments.  */
typedef struct gfc_actual_arglist
{
  const char *name;
  /* Alternate return label when the expr member is null.  */
  struct gfc_st_label *label;

  /* This is set to the type of an eventual omitted optional
     argument. This is used to determine if a hidden string length
     argument has to be added to a function call.  */
  bt missing_arg_type;

  struct gfc_expr *expr;
  struct gfc_actual_arglist *next;
}
gfc_actual_arglist;

#define gfc_get_actual_arglist() XCNEW (gfc_actual_arglist)


/* Because a symbol can belong to multiple namelists, they must be
   linked externally to the symbol itself.  */
typedef struct gfc_namelist
{
  struct gfc_symbol *sym;
  struct gfc_namelist *next;
}
gfc_namelist;

#define gfc_get_namelist() XCNEW (gfc_namelist)

enum
{
  OMP_LIST_PRIVATE,
  OMP_LIST_FIRSTPRIVATE,
  OMP_LIST_LASTPRIVATE,
  OMP_LIST_COPYPRIVATE,
  OMP_LIST_SHARED,
  OMP_LIST_COPYIN,
  OMP_LIST_PLUS,
  OMP_LIST_REDUCTION_FIRST = OMP_LIST_PLUS,
  OMP_LIST_MULT,
  OMP_LIST_SUB,
  OMP_LIST_AND,
  OMP_LIST_OR,
  OMP_LIST_EQV,
  OMP_LIST_NEQV,
  OMP_LIST_MAX,
  OMP_LIST_MIN,
  OMP_LIST_IAND,
  OMP_LIST_IOR,
  OMP_LIST_IEOR,
  OMP_LIST_REDUCTION_LAST = OMP_LIST_IEOR,
  OMP_LIST_NUM
};

/* Because a symbol can belong to multiple namelists, they must be
   linked externally to the symbol itself.  */
typedef struct gfc_omp_clauses
{
  struct gfc_expr *if_expr;
  struct gfc_expr *num_threads;
  gfc_namelist *lists[OMP_LIST_NUM];
  enum
    {
      OMP_SCHED_NONE,
      OMP_SCHED_STATIC,
      OMP_SCHED_DYNAMIC,
      OMP_SCHED_GUIDED,
      OMP_SCHED_RUNTIME,
      OMP_SCHED_AUTO
    } sched_kind;
  struct gfc_expr *chunk_size;
  enum
    {
      OMP_DEFAULT_UNKNOWN,
      OMP_DEFAULT_NONE,
      OMP_DEFAULT_PRIVATE,
      OMP_DEFAULT_SHARED,
      OMP_DEFAULT_FIRSTPRIVATE
    } default_sharing;
  int collapse;
  bool nowait, ordered, untied;
}
gfc_omp_clauses;

#define gfc_get_omp_clauses() XCNEW (gfc_omp_clauses)


/* The gfc_st_label structure is a doubly linked list attached to a
   namespace that records the usage of statement labels within that
   space.  */
/* TODO: Make format/statement specifics a union.  */
typedef struct gfc_st_label
{
  BBT_HEADER(gfc_st_label);

  int value;

  gfc_sl_type defined, referenced;

  struct gfc_expr *format;

  tree backend_decl;

  locus where;
}
gfc_st_label;


/* gfc_interface()-- Interfaces are lists of symbols strung together.  */
typedef struct gfc_interface
{
  struct gfc_symbol *sym;
  locus where;
  struct gfc_interface *next;
}
gfc_interface;

#define gfc_get_interface() XCNEW (gfc_interface)

/* User operator nodes.  These are like stripped down symbols.  */
typedef struct
{
  const char *name;

  gfc_interface *op;
  struct gfc_namespace *ns;
  gfc_access access;
}
gfc_user_op;


/* A list of specific bindings that are associated with a generic spec.  */
typedef struct gfc_tbp_generic
{
  /* The parser sets specific_st, upon resolution we look for the corresponding
     gfc_typebound_proc and set specific for further use.  */
  struct gfc_symtree* specific_st;
  struct gfc_typebound_proc* specific;

  struct gfc_tbp_generic* next;
}
gfc_tbp_generic;

#define gfc_get_tbp_generic() XCNEW (gfc_tbp_generic)


/* Data needed for type-bound procedures.  */
typedef struct gfc_typebound_proc
{
  locus where; /* Where the PROCEDURE/GENERIC definition was.  */

  union
  {
    struct gfc_symtree* specific;
    gfc_tbp_generic* generic;
  }
  u;

  gfc_access access;
  char* pass_arg; /* Argument-name for PASS.  NULL if not specified.  */

  /* The overridden type-bound proc (or GENERIC with this name in the
     parent-type) or NULL if non.  */
  struct gfc_typebound_proc* overridden;

  /* Once resolved, we use the position of pass_arg in the formal arglist of
     the binding-target procedure to identify it.  The first argument has
     number 1 here, the second 2, and so on.  */
  unsigned pass_arg_num;

  unsigned nopass:1; /* Whether we have NOPASS (PASS otherwise).  */
  unsigned non_overridable:1;
  unsigned is_generic:1;
  unsigned function:1, subroutine:1;
  unsigned error:1; /* Ignore it, when an error occurred during resolution.  */
}
gfc_typebound_proc;

#define gfc_get_typebound_proc() XCNEW (gfc_typebound_proc)


/* Symbol nodes.  These are important things.  They are what the
   standard refers to as "entities".  The possibly multiple names that
   refer to the same entity are accomplished by a binary tree of
   symtree structures that is balanced by the red-black method-- more
   than one symtree node can point to any given symbol.  */

typedef struct gfc_symbol
{
  const char *name;	/* Primary name, before renaming */
  const char *module;	/* Module this symbol came from */
  locus declared_at;

  gfc_typespec ts;
  symbol_attribute attr;

  /* The formal member points to the formal argument list if the
     symbol is a function or subroutine name.  If the symbol is a
     generic name, the generic member points to the list of
     interfaces.  */

  gfc_interface *generic;
  gfc_access component_access;

  gfc_formal_arglist *formal;
  struct gfc_namespace *formal_ns;
  struct gfc_namespace *f2k_derived;

  struct gfc_expr *value;	/* Parameter/Initializer value */
  gfc_array_spec *as;
  struct gfc_symbol *result;	/* function result symbol */
  gfc_component *components;	/* Derived type components */

  /* Defined only for Cray pointees; points to their pointer.  */
  struct gfc_symbol *cp_pointer;

  struct gfc_symbol *common_next;	/* Links for COMMON syms */

  /* This is in fact a gfc_common_head but it is only used for pointer
     comparisons to check if symbols are in the same common block.  */
  struct gfc_common_head* common_head;

  /* Make sure setup code for dummy arguments is generated in the correct
     order.  */
  int dummy_order;

  int entry_id;

  gfc_namelist *namelist, *namelist_tail;

  /* Change management fields.  Symbols that might be modified by the
     current statement have the mark member nonzero and are kept in a
     singly linked list through the tlink field.  Of these symbols,
     symbols with old_symbol equal to NULL are symbols created within
     the current statement.  Otherwise, old_symbol points to a copy of
     the old symbol.  */

  struct gfc_symbol *old_symbol, *tlink;
  unsigned mark:1, gfc_new:1;
  /* Nonzero if all equivalences associated with this symbol have been
     processed.  */
  unsigned equiv_built:1;
  /* Set if this variable is used as an index name in a FORALL.  */
  unsigned forall_index:1;
  int refs;
  struct gfc_namespace *ns;	/* namespace containing this symbol */

  tree backend_decl;
   
  /* Identity of the intrinsic module the symbol comes from, or
     INTMOD_NONE if it's not imported from a intrinsic module.  */
  intmod_id from_intmod;
  /* Identity of the symbol from intrinsic modules, from enums maintained
     separately by each intrinsic module.  Used together with from_intmod,
     it uniquely identifies a symbol from an intrinsic module.  */
  int intmod_sym_id;

  /* This may be repetitive, since the typespec now has a binding
     label field.  */
  char binding_label[GFC_MAX_BINDING_LABEL_LEN + 1];
  /* Store a reference to the common_block, if this symbol is in one.  */
  struct gfc_common_head *common_block;
}
gfc_symbol;


/* This structure is used to keep track of symbols in common blocks.  */
typedef struct gfc_common_head
{
  locus where;
  char use_assoc, saved, threadprivate;
  char name[GFC_MAX_SYMBOL_LEN + 1];
  struct gfc_symbol *head;
  char binding_label[GFC_MAX_BINDING_LABEL_LEN + 1];
  int is_bind_c;
}
gfc_common_head;

#define gfc_get_common_head() XCNEW (gfc_common_head)


/* A list of all the alternate entry points for a procedure.  */

typedef struct gfc_entry_list
{
  /* The symbol for this entry point.  */
  gfc_symbol *sym;
  /* The zero-based id of this entry point.  */
  int id;
  /* The LABEL_EXPR marking this entry point.  */
  tree label;
  /* The next item in the list.  */
  struct gfc_entry_list *next;
}
gfc_entry_list;

#define gfc_get_entry_list() \
  (gfc_entry_list *) gfc_getmem(sizeof(gfc_entry_list))

/* Lists of rename info for the USE statement.  */

typedef struct gfc_use_rename
{
  char local_name[GFC_MAX_SYMBOL_LEN + 1], use_name[GFC_MAX_SYMBOL_LEN + 1];
  struct gfc_use_rename *next;
  int found;
  gfc_intrinsic_op op;
  locus where;
}
gfc_use_rename;

#define gfc_get_use_rename() XCNEW (gfc_use_rename);

/* A list of all USE statements in a namespace.  */

typedef struct gfc_use_list
{
  const char *module_name;
  int only_flag;
  struct gfc_use_rename *rename;
  locus where;
  /* Next USE statement.  */
  struct gfc_use_list *next;
}
gfc_use_list;

#define gfc_get_use_list() \
  (gfc_use_list *) gfc_getmem(sizeof(gfc_use_list))

/* Within a namespace, symbols are pointed to by symtree nodes that
   are linked together in a balanced binary tree.  There can be
   several symtrees pointing to the same symbol node via USE
   statements.  */

typedef struct gfc_symtree
{
  BBT_HEADER (gfc_symtree);
  const char *name;
  int ambiguous;
  union
  {
    gfc_symbol *sym;		/* Symbol associated with this node */
    gfc_user_op *uop;
    gfc_common_head *common;
  }
  n;

  /* Data for type-bound procedures; NULL if no type-bound procedure.  */
  gfc_typebound_proc* typebound;
}
gfc_symtree;

/* A linked list of derived types in the namespace.  */
typedef struct gfc_dt_list
{
  struct gfc_symbol *derived;
  struct gfc_dt_list *next;
}
gfc_dt_list;

#define gfc_get_dt_list() XCNEW (gfc_dt_list)

  /* A list of all derived types.  */
  extern gfc_dt_list *gfc_derived_types;

/* A namespace describes the contents of procedure, module or
   interface block.  */
/* ??? Anything else use these?  */

typedef struct gfc_namespace
{
  /* Tree containing all the symbols in this namespace.  */
  gfc_symtree *sym_root;
  /* Tree containing all the user-defined operators in the namespace.  */
  gfc_symtree *uop_root;
  /* Tree containing all the common blocks.  */
  gfc_symtree *common_root;
  /* Linked list of finalizer procedures.  */
  struct gfc_finalizer *finalizers;

  /* If set_flag[letter] is set, an implicit type has been set for letter.  */
  int set_flag[GFC_LETTERS];
  /* Keeps track of the implicit types associated with the letters.  */
  gfc_typespec default_type[GFC_LETTERS];
  /* Store the positions of IMPLICIT statements.  */
  locus implicit_loc[GFC_LETTERS];

  /* If this is a namespace of a procedure, this points to the procedure.  */
  struct gfc_symbol *proc_name;
  /* If this is the namespace of a unit which contains executable
     code, this points to it.  */
  struct gfc_code *code;

  /* Points to the equivalences set up in this namespace.  */
  struct gfc_equiv *equiv;

  /* Points to the equivalence groups produced by trans_common.  */
  struct gfc_equiv_list *equiv_lists;

  gfc_interface *op[GFC_INTRINSIC_OPS];

  /* Points to the parent namespace, i.e. the namespace of a module or
     procedure in which the procedure belonging to this namespace is
     contained. The parent namespace points to this namespace either
     directly via CONTAINED, or indirectly via the chain built by
     SIBLING.  */
  struct gfc_namespace *parent;
  /* CONTAINED points to the first contained namespace. Sibling
     namespaces are chained via SIBLING.  */
  struct gfc_namespace  *contained, *sibling;

  gfc_common_head blank_common;
  gfc_access default_access, operator_access[GFC_INTRINSIC_OPS];

  gfc_st_label *st_labels;
  /* This list holds information about all the data initializers in
     this namespace.  */
  struct gfc_data *data;

  gfc_charlen *cl_list, *old_cl_list;

  int save_all, seen_save, seen_implicit_none;

  /* Normally we don't need to refcount namespaces.  However when we read
     a module containing a function with multiple entry points, this
     will appear as several functions with the same formal namespace.  */
  int refs;

  /* A list of all alternate entry points to this procedure (or NULL).  */
  gfc_entry_list *entries;

  /* A list of USE statements in this namespace.  */
  gfc_use_list *use_stmts;

  /* Set to 1 if namespace is a BLOCK DATA program unit.  */
  int is_block_data;

  /* Set to 1 if namespace is an interface body with "IMPORT" used.  */
  int has_import_set;
}
gfc_namespace;

extern gfc_namespace *gfc_current_ns;

/* Global symbols are symbols of global scope. Currently we only use
   this to detect collisions already when parsing.
   TODO: Extend to verify procedure calls.  */

typedef struct gfc_gsymbol
{
  BBT_HEADER(gfc_gsymbol);

  const char *name;
  const char *sym_name;
  const char *mod_name;
  const char *binding_label;
  enum { GSYM_UNKNOWN=1, GSYM_PROGRAM, GSYM_FUNCTION, GSYM_SUBROUTINE,
        GSYM_MODULE, GSYM_COMMON, GSYM_BLOCK_DATA } type;

  int defined, used;
  locus where;
}
gfc_gsymbol;

extern gfc_gsymbol *gfc_gsym_root;

/* Information on interfaces being built.  */
typedef struct
{
  interface_type type;
  gfc_symbol *sym;
  gfc_namespace *ns;
  gfc_user_op *uop;
  gfc_intrinsic_op op;
}
gfc_interface_info;

extern gfc_interface_info current_interface;


/* Array reference.  */
typedef struct gfc_array_ref
{
  ar_type type;
  int dimen;			/* # of components in the reference */
  locus where;
  gfc_array_spec *as;

  locus c_where[GFC_MAX_DIMENSIONS];	/* All expressions can be NULL */
  struct gfc_expr *start[GFC_MAX_DIMENSIONS], *end[GFC_MAX_DIMENSIONS],
    *stride[GFC_MAX_DIMENSIONS];

  enum
  { DIMEN_ELEMENT = 1, DIMEN_RANGE, DIMEN_VECTOR, DIMEN_UNKNOWN }
  dimen_type[GFC_MAX_DIMENSIONS];

  struct gfc_expr *offset;
}
gfc_array_ref;

#define gfc_get_array_ref() XCNEW (gfc_array_ref)


/* Component reference nodes.  A variable is stored as an expression
   node that points to the base symbol.  After that, a singly linked
   list of component reference nodes gives the variable's complete
   resolution.  The array_ref component may be present and comes
   before the component component.  */

typedef enum
  { REF_ARRAY, REF_COMPONENT, REF_SUBSTRING }
ref_type;

typedef struct gfc_ref
{
  ref_type type;

  union
  {
    struct gfc_array_ref ar;

    struct
    {
      gfc_component *component;
      gfc_symbol *sym;
    }
    c;

    struct
    {
      struct gfc_expr *start, *end;	/* Substring */
      gfc_charlen *length;
    }
    ss;

  }
  u;

  struct gfc_ref *next;
}
gfc_ref;

#define gfc_get_ref() XCNEW (gfc_ref)


/* Structures representing intrinsic symbols and their arguments lists.  */
typedef struct gfc_intrinsic_arg
{
  char name[GFC_MAX_SYMBOL_LEN + 1];

  gfc_typespec ts;
  int optional;
  gfc_actual_arglist *actual;

  struct gfc_intrinsic_arg *next;

}
gfc_intrinsic_arg;


/* Specifies the various kinds of check functions used to verify the
   argument lists of intrinsic functions. fX with X an integer refer
   to check functions of intrinsics with X arguments. f1m is used for
   the MAX and MIN intrinsics which can have an arbitrary number of
   arguments, f3ml is used for the MINLOC and MAXLOC intrinsics as
   these have special semantics.  */

typedef union
{
  gfc_try (*f0)(void);
  gfc_try (*f1)(struct gfc_expr *);
  gfc_try (*f1m)(gfc_actual_arglist *);
  gfc_try (*f2)(struct gfc_expr *, struct gfc_expr *);
  gfc_try (*f3)(struct gfc_expr *, struct gfc_expr *, struct gfc_expr *);
  gfc_try (*f3ml)(gfc_actual_arglist *);
  gfc_try (*f3red)(gfc_actual_arglist *);
  gfc_try (*f4)(struct gfc_expr *, struct gfc_expr *, struct gfc_expr *,
	    struct gfc_expr *);
  gfc_try (*f5)(struct gfc_expr *, struct gfc_expr *, struct gfc_expr *,
	    struct gfc_expr *, struct gfc_expr *);
}
gfc_check_f;

/* Like gfc_check_f, these specify the type of the simplification
   function associated with an intrinsic. The fX are just like in
   gfc_check_f. cc is used for type conversion functions.  */

typedef union
{
  struct gfc_expr *(*f0)(void);
  struct gfc_expr *(*f1)(struct gfc_expr *);
  struct gfc_expr *(*f2)(struct gfc_expr *, struct gfc_expr *);
  struct gfc_expr *(*f3)(struct gfc_expr *, struct gfc_expr *,
			 struct gfc_expr *);
  struct gfc_expr *(*f4)(struct gfc_expr *, struct gfc_expr *,
			 struct gfc_expr *, struct gfc_expr *);
  struct gfc_expr *(*f5)(struct gfc_expr *, struct gfc_expr *,
			 struct gfc_expr *, struct gfc_expr *,
			 struct gfc_expr *);
  struct gfc_expr *(*cc)(struct gfc_expr *, bt, int);
}
gfc_simplify_f;

/* Again like gfc_check_f, these specify the type of the resolution
   function associated with an intrinsic. The fX are just like in
   gfc_check_f. f1m is used for MIN and MAX, s1 is used for abort().  */

typedef union
{
  void (*f0)(struct gfc_expr *);
  void (*f1)(struct gfc_expr *, struct gfc_expr *);
  void (*f1m)(struct gfc_expr *, struct gfc_actual_arglist *);
  void (*f2)(struct gfc_expr *, struct gfc_expr *, struct gfc_expr *);
  void (*f3)(struct gfc_expr *, struct gfc_expr *, struct gfc_expr *,
	     struct gfc_expr *);
  void (*f4)(struct gfc_expr *, struct gfc_expr *, struct gfc_expr *,
	     struct gfc_expr *, struct gfc_expr *);
  void (*f5)(struct gfc_expr *, struct gfc_expr *, struct gfc_expr *,
	     struct gfc_expr *, struct gfc_expr *, struct gfc_expr *);
  void (*s1)(struct gfc_code *);
}
gfc_resolve_f;


typedef struct gfc_intrinsic_sym
{
  const char *name, *lib_name;
  gfc_intrinsic_arg *formal;
  gfc_typespec ts;
  unsigned elemental:1, inquiry:1, transformational:1, pure:1, 
    generic:1, specific:1, actual_ok:1, noreturn:1, conversion:1;

  int standard;

  gfc_simplify_f simplify;
  gfc_check_f check;
  gfc_resolve_f resolve;
  struct gfc_intrinsic_sym *specific_head, *next;
  gfc_isym_id id;

}
gfc_intrinsic_sym;


/* Expression nodes.  The expression node types deserve explanations,
   since the last couple can be easily misconstrued:

   EXPR_OP         Operator node pointing to one or two other nodes
   EXPR_FUNCTION   Function call, symbol points to function's name
   EXPR_CONSTANT   A scalar constant: Logical, String, Real, Int or Complex
   EXPR_VARIABLE   An Lvalue with a root symbol and possible reference list
		   which expresses structure, array and substring refs.
   EXPR_NULL       The NULL pointer value (which also has a basic type).
   EXPR_SUBSTRING  A substring of a constant string
   EXPR_STRUCTURE  A structure constructor
   EXPR_ARRAY      An array constructor.
   EXPR_COMPCALL   Function (or subroutine) call of a procedure pointer
		   component or type-bound procedure.  */

#include <gmp.h>
#include <mpfr.h>
#define GFC_RND_MODE GMP_RNDN

typedef struct gfc_expr
{
  expr_t expr_type;

  gfc_typespec ts;	/* These two refer to the overall expression */

  int rank;
  mpz_t *shape;		/* Can be NULL if shape is unknown at compile time */

  /* Nonnull for functions and structure constructors, the base object for
     component-calls.  */
  gfc_symtree *symtree;

  gfc_ref *ref;

  locus where;

  /* True if the expression is a call to a function that returns an array,
     and if we have decided not to allocate temporary data for that array.  */
  unsigned int inline_noncopying_intrinsic : 1, is_boz : 1;

  /* Sometimes, when an error has been emitted, it is necessary to prevent
      it from recurring.  */
  unsigned int error : 1;
  
  /* Mark and expression where a user operator has been substituted by
     a function call in interface.c(gfc_extend_expr).  */
  unsigned int user_operator : 1;

  /* Used to quickly find a given constructor by its offset.  */
  splay_tree con_by_offset;

  /* If an expression comes from a Hollerith constant or compile-time
     evaluation of a transfer statement, it may have a prescribed target-
     memory representation, and these cannot always be backformed from
     the value.  */
  struct
  {
    int length;
    char *string;
  }
  representation;

  union
  {
    int logical;

    io_kind iokind;

    mpz_t integer;

    mpfr_t real;

    struct
    {
      mpfr_t r, i;
    }
    complex;

    struct
    {
      gfc_intrinsic_op op;
      gfc_user_op *uop;
      struct gfc_expr *op1, *op2;
    }
    op;

    struct
    {
      gfc_actual_arglist *actual;
      const char *name;	/* Points to the ultimate name of the function */
      gfc_intrinsic_sym *isym;
      gfc_symbol *esym;
    }
    function;

    struct
    {
      gfc_actual_arglist* actual;
      gfc_typebound_proc* tbp;
      const char* name;
    }
    compcall;

    struct
    {
      int length;
      gfc_char_t *string;
    }
    character;

    struct gfc_constructor *constructor;
  }
  value;

}
gfc_expr;


#define gfc_get_shape(rank) ((mpz_t *) gfc_getmem((rank)*sizeof(mpz_t)))

/* Structures for information associated with different kinds of
   numbers.  The first set of integer parameters define all there is
   to know about a particular kind.  The rest of the elements are
   computed from the first elements.  */

typedef struct
{
  /* Values really representable by the target.  */
  mpz_t huge, pedantic_min_int, min_int;

  int kind, radix, digits, bit_size, range;

  /* True if the C type of the given name maps to this precision.
     Note that more than one bit can be set.  */
  unsigned int c_char : 1;
  unsigned int c_short : 1;
  unsigned int c_int : 1;
  unsigned int c_long : 1;
  unsigned int c_long_long : 1;
}
gfc_integer_info;

extern gfc_integer_info gfc_integer_kinds[];


typedef struct
{
  int kind, bit_size;

  /* True if the C++ type bool, C99 type _Bool, maps to this precision.  */
  unsigned int c_bool : 1;
}
gfc_logical_info;

extern gfc_logical_info gfc_logical_kinds[];


typedef struct
{
  mpfr_t epsilon, huge, tiny, subnormal;
  int kind, radix, digits, min_exponent, max_exponent;
  int range, precision;

  /* The precision of the type as reported by GET_MODE_PRECISION.  */
  int mode_precision;

  /* True if the C type of the given name maps to this precision.
     Note that more than one bit can be set.  */
  unsigned int c_float : 1;
  unsigned int c_double : 1;
  unsigned int c_long_double : 1;
}
gfc_real_info;

extern gfc_real_info gfc_real_kinds[];

typedef struct
{
  int kind, bit_size;
  const char *name;
}
gfc_character_info;

extern gfc_character_info gfc_character_kinds[];


/* Equivalence structures.  Equivalent lvalues are linked along the
   *eq pointer, equivalence sets are strung along the *next node.  */
typedef struct gfc_equiv
{
  struct gfc_equiv *next, *eq;
  gfc_expr *expr;
  const char *module;
  int used;
}
gfc_equiv;

#define gfc_get_equiv() XCNEW (gfc_equiv)

/* Holds a single equivalence member after processing.  */
typedef struct gfc_equiv_info
{
  gfc_symbol *sym;
  HOST_WIDE_INT offset;
  HOST_WIDE_INT length;
  struct gfc_equiv_info *next;
} gfc_equiv_info;

/* Holds equivalence groups, after they have been processed.  */
typedef struct gfc_equiv_list
{
  gfc_equiv_info *equiv;
  struct gfc_equiv_list *next;
} gfc_equiv_list;

/* gfc_case stores the selector list of a case statement.  The *low
   and *high pointers can point to the same expression in the case of
   a single value.  If *high is NULL, the selection is from *low
   upwards, if *low is NULL the selection is *high downwards.

   This structure has separate fields to allow single and double linked
   lists of CASEs at the same time.  The singe linked list along the NEXT
   field is a list of cases for a single CASE label.  The double linked
   list along the LEFT/RIGHT fields is used to detect overlap and to
   build a table of the cases for SELECT constructs with a CHARACTER
   case expression.  */

typedef struct gfc_case
{
  /* Where we saw this case.  */
  locus where;
  int n;

  /* Case range values.  If (low == high), it's a single value.  If one of
     the labels is NULL, it's an unbounded case.  If both are NULL, this
     represents the default case.  */
  gfc_expr *low, *high;

  /* Next case label in the list of cases for a single CASE label.  */
  struct gfc_case *next;

  /* Used for detecting overlap, and for code generation.  */
  struct gfc_case *left, *right;

  /* True if this case label can never be matched.  */
  int unreachable;
}
gfc_case;

#define gfc_get_case() XCNEW (gfc_case)


typedef struct
{
  gfc_expr *var, *start, *end, *step;
}
gfc_iterator;

#define gfc_get_iterator() XCNEW (gfc_iterator)


/* Allocation structure for ALLOCATE, DEALLOCATE and NULLIFY statements.  */

typedef struct gfc_alloc
{
  gfc_expr *expr;
  struct gfc_alloc *next;
}
gfc_alloc;

#define gfc_get_alloc() XCNEW (gfc_alloc)


typedef struct
{
  gfc_expr *unit, *file, *status, *access, *form, *recl,
    *blank, *position, *action, *delim, *pad, *iostat, *iomsg, *convert,
    *decimal, *encoding, *round, *sign, *asynchronous, *id;
  gfc_st_label *err;
}
gfc_open;


typedef struct
{
  gfc_expr *unit, *status, *iostat, *iomsg;
  gfc_st_label *err;
}
gfc_close;


typedef struct
{
  gfc_expr *unit, *iostat, *iomsg;
  gfc_st_label *err;
}
gfc_filepos;


typedef struct
{
  gfc_expr *unit, *file, *iostat, *exist, *opened, *number, *named,
    *name, *access, *sequential, *direct, *form, *formatted,
    *unformatted, *recl, *nextrec, *blank, *position, *action, *read,
    *write, *readwrite, *delim, *pad, *iolength, *iomsg, *convert, *strm_pos,
    *asynchronous, *decimal, *encoding, *pending, *round, *sign, *size, *id;

  gfc_st_label *err;

}
gfc_inquire;


typedef struct
{
  gfc_expr *unit, *iostat, *iomsg, *id;
  gfc_st_label *err, *end, *eor;
}
gfc_wait;


typedef struct
{
  gfc_expr *io_unit, *format_expr, *rec, *advance, *iostat, *size, *iomsg,
	   *id, *pos, *asynchronous, *blank, *decimal, *delim, *pad, *round,
	   *sign, *extra_comma;

  gfc_symbol *namelist;
  /* A format_label of `format_asterisk' indicates the "*" format */
  gfc_st_label *format_label;
  gfc_st_label *err, *end, *eor;

  locus eor_where, end_where, err_where;
}
gfc_dt;


typedef struct gfc_forall_iterator
{
  gfc_expr *var, *start, *end, *stride;
  struct gfc_forall_iterator *next;
}
gfc_forall_iterator;


/* Executable statements that fill gfc_code structures.  */
typedef enum
{
  EXEC_NOP = 1, EXEC_ASSIGN, EXEC_LABEL_ASSIGN, EXEC_POINTER_ASSIGN,
  EXEC_GOTO, EXEC_CALL, EXEC_COMPCALL, EXEC_ASSIGN_CALL, EXEC_RETURN,
  EXEC_ENTRY, EXEC_PAUSE, EXEC_STOP, EXEC_CONTINUE, EXEC_INIT_ASSIGN,
  EXEC_IF, EXEC_ARITHMETIC_IF, EXEC_DO, EXEC_DO_WHILE, EXEC_SELECT,
  EXEC_FORALL, EXEC_WHERE, EXEC_CYCLE, EXEC_EXIT,
  EXEC_ALLOCATE, EXEC_DEALLOCATE,
  EXEC_OPEN, EXEC_CLOSE, EXEC_WAIT,
  EXEC_READ, EXEC_WRITE, EXEC_IOLENGTH, EXEC_TRANSFER, EXEC_DT_END,
  EXEC_BACKSPACE, EXEC_ENDFILE, EXEC_INQUIRE, EXEC_REWIND, EXEC_FLUSH,
  EXEC_OMP_CRITICAL, EXEC_OMP_DO, EXEC_OMP_FLUSH, EXEC_OMP_MASTER,
  EXEC_OMP_ORDERED, EXEC_OMP_PARALLEL, EXEC_OMP_PARALLEL_DO,
  EXEC_OMP_PARALLEL_SECTIONS, EXEC_OMP_PARALLEL_WORKSHARE,
  EXEC_OMP_SECTIONS, EXEC_OMP_SINGLE, EXEC_OMP_WORKSHARE,
  EXEC_OMP_ATOMIC, EXEC_OMP_BARRIER, EXEC_OMP_END_NOWAIT,
  EXEC_OMP_END_SINGLE, EXEC_OMP_TASK, EXEC_OMP_TASKWAIT
}
gfc_exec_op;

typedef struct gfc_code
{
  gfc_exec_op op;

  struct gfc_code *block, *next;
  locus loc;

  gfc_st_label *here, *label, *label2, *label3;
  gfc_symtree *symtree;
  gfc_expr *expr, *expr2;
  /* A name isn't sufficient to identify a subroutine, we need the actual
     symbol for the interface definition.
  const char *sub_name;  */
  gfc_symbol *resolved_sym;
  gfc_intrinsic_sym *resolved_isym;

  union
  {
    gfc_actual_arglist *actual;
    gfc_case *case_list;
    gfc_iterator *iterator;
    gfc_alloc *alloc_list;
    gfc_open *open;
    gfc_close *close;
    gfc_filepos *filepos;
    gfc_inquire *inquire;
    gfc_wait *wait;
    gfc_dt *dt;
    gfc_forall_iterator *forall_iterator;
    struct gfc_code *whichloop;
    int stop_code;
    gfc_entry_list *entry;
    gfc_omp_clauses *omp_clauses;
    const char *omp_name;
    gfc_namelist *omp_namelist;
    bool omp_bool;
  }
  ext;		/* Points to additional structures required by statement */

  /* Backend_decl is used for cycle and break labels in do loops, and
     probably for other constructs as well, once we translate them.  */
  tree backend_decl;
}
gfc_code;


/* Storage for DATA statements.  */
typedef struct gfc_data_variable
{
  gfc_expr *expr;
  gfc_iterator iter;
  struct gfc_data_variable *list, *next;
}
gfc_data_variable;


typedef struct gfc_data_value
{
  mpz_t repeat;
  gfc_expr *expr;
  struct gfc_data_value *next;
}
gfc_data_value;


typedef struct gfc_data
{
  gfc_data_variable *var;
  gfc_data_value *value;
  locus where;

  struct gfc_data *next;
}
gfc_data;


/* Structure for holding compile options */
typedef struct
{
  char *module_dir;
  gfc_source_form source_form;
  /* Maximum line lengths in fixed- and free-form source, respectively.
     When fixed_line_length or free_line_length are 0, the whole line is used,
     regardless of length.

     If the user requests a fixed_line_length <7 then gfc_init_options()
     emits a fatal error.  */
  int fixed_line_length;
  int free_line_length;
  /* Maximum number of continuation lines in fixed- and free-form source,
     respectively.  */
  int max_continue_fixed;
  int max_continue_free;
  int max_identifier_length;
  int dump_parse_tree;

  int warn_aliasing;
  int warn_ampersand;
  int warn_conversion;
  int warn_implicit_interface;
  int warn_line_truncation;
  int warn_surprising;
  int warn_tabs;
  int warn_underflow;
  int warn_intrinsic_shadow;
  int warn_intrinsics_std;
  int warn_character_truncation;
  int warn_array_temp;
  int warn_align_commons;
  int max_errors;

  int flag_all_intrinsics;
  int flag_default_double;
  int flag_default_integer;
  int flag_default_real;
  int flag_dollar_ok;
  int flag_underscoring;
  int flag_second_underscore;
  int flag_implicit_none;
  int flag_max_stack_var_size;
  int flag_max_array_constructor;
  int flag_range_check;
  int flag_pack_derived;
  int flag_repack_arrays;
  int flag_preprocessed;
  int flag_f2c;
  int flag_automatic;
  int flag_backslash;
  int flag_backtrace;
  int flag_check_array_temporaries;
  int flag_allow_leading_underscore;
  int flag_dump_core;
  int flag_external_blas;
  int blas_matmul_limit;
  int flag_cray_pointer;
  int flag_d_lines;
  int flag_openmp;
  int flag_sign_zero;
  int flag_module_private;
  int flag_recursive;
  int flag_init_local_zero;
  int flag_init_integer;
  int flag_init_integer_value;
  int flag_init_real;
  int flag_init_logical;
  int flag_init_character;
  char flag_init_character_value;
  int flag_align_commons;

  int fpe;

  int warn_std;
  int allow_std;
  int fshort_enums;
  int convert;
  int record_marker;
  int max_subrecord_length;
}
gfc_option_t;

extern gfc_option_t gfc_option;

/* Constructor nodes for array and structure constructors.  */
typedef struct gfc_constructor
{
  gfc_expr *expr;
  gfc_iterator *iterator;
  locus where;
  struct gfc_constructor *next;
  struct
  {
    mpz_t offset; /* Record the offset of array element which appears in
                     data statement like "data a(5)/4/".  */
    gfc_component *component; /* Record the component being initialized.  */
  }
  n;
  mpz_t repeat; /* Record the repeat number of initial values in data
                 statement like "data a/5*10/".  */
}
gfc_constructor;


typedef struct iterator_stack
{
  gfc_symtree *variable;
  mpz_t value;
  struct iterator_stack *prev;
}
iterator_stack;
extern iterator_stack *iter_stack;


/* Node in the linked list used for storing finalizer procedures.  */

typedef struct gfc_finalizer
{
  struct gfc_finalizer* next;
  locus where; /* Where the FINAL declaration occurred.  */

  /* Up to resolution, we want the gfc_symbol, there we lookup the corresponding
     symtree and later need only that.  This way, we can access and call the
     finalizers from every context as they should be "always accessible".  I
     don't make this a union because we need the information whether proc_sym is
     still referenced or not for dereferencing it on deleting a gfc_finalizer
     structure.  */
  gfc_symbol*  proc_sym;
  gfc_symtree* proc_tree; 
}
gfc_finalizer;
#define gfc_get_finalizer() XCNEW (gfc_finalizer)


/************************ Function prototypes *************************/

/* decl.c */
bool gfc_in_match_data (void);

/* scanner.c */
void gfc_scanner_done_1 (void);
void gfc_scanner_init_1 (void);

void gfc_add_include_path (const char *, bool, bool);
void gfc_add_intrinsic_modules_path (const char *);
void gfc_release_include_path (void);
FILE *gfc_open_included_file (const char *, bool, bool);
FILE *gfc_open_intrinsic_module (const char *);

int gfc_at_end (void);
int gfc_at_eof (void);
int gfc_at_bol (void);
int gfc_at_eol (void);
void gfc_advance_line (void);
int gfc_check_include (void);
int gfc_define_undef_line (void);

int gfc_wide_is_printable (gfc_char_t);
int gfc_wide_is_digit (gfc_char_t);
int gfc_wide_fits_in_byte (gfc_char_t);
gfc_char_t gfc_wide_tolower (gfc_char_t);
gfc_char_t gfc_wide_toupper (gfc_char_t);
size_t gfc_wide_strlen (const gfc_char_t *);
int gfc_wide_strncasecmp (const gfc_char_t *, const char *, size_t);
gfc_char_t *gfc_wide_memset (gfc_char_t *, gfc_char_t, size_t);
char *gfc_widechar_to_char (const gfc_char_t *, int);
gfc_char_t *gfc_char_to_widechar (const char *);

#define gfc_get_wide_string(n) XCNEWVEC (gfc_char_t, n)

void gfc_skip_comments (void);
gfc_char_t gfc_next_char_literal (int);
gfc_char_t gfc_next_char (void);
char gfc_next_ascii_char (void);
gfc_char_t gfc_peek_char (void);
char gfc_peek_ascii_char (void);
void gfc_error_recovery (void);
void gfc_gobble_whitespace (void);
gfc_try gfc_new_file (void);
const char * gfc_read_orig_filename (const char *, const char **);

extern gfc_source_form gfc_current_form;
extern const char *gfc_source_file;
extern locus gfc_current_locus;

void gfc_start_source_files (void);
void gfc_end_source_files (void);

/* misc.c */
void *gfc_getmem (size_t) ATTRIBUTE_MALLOC;
void gfc_free (void *);
int gfc_terminal_width (void);
void gfc_clear_ts (gfc_typespec *);
FILE *gfc_open_file (const char *);
const char *gfc_basic_typename (bt);
const char *gfc_typename (gfc_typespec *);
const char *gfc_op2string (gfc_intrinsic_op);
const char *gfc_code2string (const mstring *, int);
int gfc_string2code (const mstring *, const char *);
const char *gfc_intent_string (sym_intent);

void gfc_init_1 (void);
void gfc_init_2 (void);
void gfc_done_1 (void);
void gfc_done_2 (void);

int get_c_kind (const char *, CInteropKind_t *);

/* options.c */
unsigned int gfc_init_options (unsigned int, const char **);
int gfc_handle_option (size_t, const char *, int);
bool gfc_post_options (const char **);

/* iresolve.c */
const char * gfc_get_string (const char *, ...) ATTRIBUTE_PRINTF_1;
bool gfc_find_sym_in_expr (gfc_symbol *, gfc_expr *);

/* error.c */

typedef struct gfc_error_buf
{
  int flag;
  size_t allocated, index;
  char *message;
} gfc_error_buf;

void gfc_error_init_1 (void);
void gfc_buffer_error (int);

const char *gfc_print_wide_char (gfc_char_t);

void gfc_warning (const char *, ...) ATTRIBUTE_GCC_GFC(1,2);
void gfc_warning_now (const char *, ...) ATTRIBUTE_GCC_GFC(1,2);
void gfc_clear_warning (void);
void gfc_warning_check (void);

void gfc_error (const char *, ...) ATTRIBUTE_GCC_GFC(1,2);
void gfc_error_now (const char *, ...) ATTRIBUTE_GCC_GFC(1,2);
void gfc_fatal_error (const char *, ...) ATTRIBUTE_NORETURN ATTRIBUTE_GCC_GFC(1,2);
void gfc_internal_error (const char *, ...) ATTRIBUTE_NORETURN ATTRIBUTE_GCC_GFC(1,2);
void gfc_clear_error (void);
int gfc_error_check (void);
int gfc_error_flag_test (void);

notification gfc_notification_std (int);
gfc_try gfc_notify_std (int, const char *, ...) ATTRIBUTE_GCC_GFC(2,3);

/* A general purpose syntax error.  */
#define gfc_syntax_error(ST)	\
  gfc_error ("Syntax error in %s statement at %C", gfc_ascii_statement (ST));

void gfc_push_error (gfc_error_buf *);
void gfc_pop_error (gfc_error_buf *);
void gfc_free_error (gfc_error_buf *);

void gfc_get_errors (int *, int *);

/* arith.c */
void gfc_arith_init_1 (void);
void gfc_arith_done_1 (void);
gfc_expr *gfc_enum_initializer (gfc_expr *, locus);
arith gfc_check_integer_range (mpz_t p, int kind);
bool gfc_check_character_range (gfc_char_t, int);

/* trans-types.c */
gfc_try gfc_check_any_c_kind (gfc_typespec *);
int gfc_validate_kind (bt, int, bool);
extern int gfc_index_integer_kind;
extern int gfc_default_integer_kind;
extern int gfc_max_integer_kind;
extern int gfc_default_real_kind;
extern int gfc_default_double_kind;
extern int gfc_default_character_kind;
extern int gfc_default_logical_kind;
extern int gfc_default_complex_kind;
extern int gfc_c_int_kind;
extern int gfc_intio_kind;
extern int gfc_charlen_int_kind;
extern int gfc_numeric_storage_size;
extern int gfc_character_storage_size;

/* symbol.c */
void gfc_clear_new_implicit (void);
gfc_try gfc_add_new_implicit_range (int, int);
gfc_try gfc_merge_new_implicit (gfc_typespec *);
void gfc_set_implicit_none (void);
void gfc_check_function_type (gfc_namespace *);
bool gfc_is_intrinsic_typename (const char *);

gfc_typespec *gfc_get_default_type (gfc_symbol *, gfc_namespace *);
gfc_try gfc_set_default_type (gfc_symbol *, int, gfc_namespace *);

void gfc_set_sym_referenced (gfc_symbol *);

gfc_try gfc_add_attribute (symbol_attribute *, locus *);
gfc_try gfc_add_allocatable (symbol_attribute *, locus *);
gfc_try gfc_add_dimension (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_external (symbol_attribute *, locus *);
gfc_try gfc_add_intrinsic (symbol_attribute *, locus *);
gfc_try gfc_add_optional (symbol_attribute *, locus *);
gfc_try gfc_add_pointer (symbol_attribute *, locus *);
gfc_try gfc_add_cray_pointer (symbol_attribute *, locus *);
gfc_try gfc_add_cray_pointee (symbol_attribute *, locus *);
gfc_try gfc_mod_pointee_as (gfc_array_spec *);
gfc_try gfc_add_protected (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_result (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_save (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_threadprivate (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_saved_common (symbol_attribute *, locus *);
gfc_try gfc_add_target (symbol_attribute *, locus *);
gfc_try gfc_add_dummy (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_generic (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_common (symbol_attribute *, locus *);
gfc_try gfc_add_in_common (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_in_equivalence (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_data (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_in_namelist (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_sequence (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_elemental (symbol_attribute *, locus *);
gfc_try gfc_add_pure (symbol_attribute *, locus *);
gfc_try gfc_add_recursive (symbol_attribute *, locus *);
gfc_try gfc_add_function (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_subroutine (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_volatile (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_proc (symbol_attribute *attr, const char *name, locus *where);
gfc_try gfc_add_abstract (symbol_attribute* attr, locus* where);

gfc_try gfc_add_access (symbol_attribute *, gfc_access, const char *, locus *);
gfc_try gfc_add_is_bind_c (symbol_attribute *, const char *, locus *, int);
gfc_try gfc_add_extension (symbol_attribute *, locus *);
gfc_try gfc_add_value (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_flavor (symbol_attribute *, sym_flavor, const char *, locus *);
gfc_try gfc_add_entry (symbol_attribute *, const char *, locus *);
gfc_try gfc_add_procedure (symbol_attribute *, procedure_type,
		       const char *, locus *);
gfc_try gfc_add_intent (symbol_attribute *, sym_intent, locus *);
gfc_try gfc_add_explicit_interface (gfc_symbol *, ifsrc,
				gfc_formal_arglist *, locus *);
gfc_try gfc_add_type (gfc_symbol *, gfc_typespec *, locus *);

void gfc_clear_attr (symbol_attribute *);
gfc_try gfc_missing_attr (symbol_attribute *, locus *);
gfc_try gfc_copy_attr (symbol_attribute *, symbol_attribute *, locus *);

gfc_try gfc_add_component (gfc_symbol *, const char *, gfc_component **);
gfc_symbol *gfc_use_derived (gfc_symbol *);
gfc_symtree *gfc_use_derived_tree (gfc_symtree *);
gfc_component *gfc_find_component (gfc_symbol *, const char *, bool, bool);

gfc_st_label *gfc_get_st_label (int);
void gfc_free_st_label (gfc_st_label *);
void gfc_define_st_label (gfc_st_label *, gfc_sl_type, locus *);
gfc_try gfc_reference_st_label (gfc_st_label *, gfc_sl_type);

gfc_expr * gfc_lval_expr_from_sym (gfc_symbol *);

gfc_namespace *gfc_get_namespace (gfc_namespace *, int);
gfc_symtree *gfc_new_symtree (gfc_symtree **, const char *);
gfc_symtree *gfc_find_symtree (gfc_symtree *, const char *);
void gfc_delete_symtree (gfc_symtree **, const char *);
gfc_symtree *gfc_get_unique_symtree (gfc_namespace *);
gfc_user_op *gfc_get_uop (const char *);
gfc_user_op *gfc_find_uop (const char *, gfc_namespace *);
void gfc_free_symbol (gfc_symbol *);
gfc_symbol *gfc_new_symbol (const char *, gfc_namespace *);
int gfc_find_symbol (const char *, gfc_namespace *, int, gfc_symbol **);
int gfc_find_sym_tree (const char *, gfc_namespace *, int, gfc_symtree **);
int gfc_get_symbol (const char *, gfc_namespace *, gfc_symbol **);
gfc_try verify_c_interop (gfc_typespec *);
gfc_try verify_c_interop_param (gfc_symbol *);
gfc_try verify_bind_c_sym (gfc_symbol *, gfc_typespec *, int, gfc_common_head *);
gfc_try verify_bind_c_derived_type (gfc_symbol *);
gfc_try verify_com_block_vars_c_interop (gfc_common_head *);
void generate_isocbinding_symbol (const char *, iso_c_binding_symbol, const char *);
gfc_symbol *get_iso_c_sym (gfc_symbol *, char *, char *, int);
int gfc_get_sym_tree (const char *, gfc_namespace *, gfc_symtree **);
int gfc_get_ha_symbol (const char *, gfc_symbol **);
int gfc_get_ha_sym_tree (const char *, gfc_symtree **);

int gfc_symbols_could_alias (gfc_symbol *, gfc_symbol *);

void gfc_undo_symbols (void);
void gfc_commit_symbols (void);
void gfc_commit_symbol (gfc_symbol *);
void gfc_free_charlen (gfc_charlen *, gfc_charlen *);
void gfc_free_namespace (gfc_namespace *);

void gfc_symbol_init_2 (void);
void gfc_symbol_done_2 (void);

void gfc_traverse_symtree (gfc_symtree *, void (*)(gfc_symtree *));
void gfc_traverse_ns (gfc_namespace *, void (*)(gfc_symbol *));
void gfc_traverse_user_op (gfc_namespace *, void (*)(gfc_user_op *));
void gfc_save_all (gfc_namespace *);

void gfc_symbol_state (void);

gfc_gsymbol *gfc_get_gsymbol (const char *);
gfc_gsymbol *gfc_find_gsymbol (gfc_gsymbol *, const char *);

gfc_symbol* gfc_get_derived_super_type (gfc_symbol*);
gfc_symtree* gfc_find_typebound_proc (gfc_symbol*, gfc_try*, const char*, bool);

void copy_formal_args (gfc_symbol *dest, gfc_symbol *src);

void gfc_free_finalizer (gfc_finalizer *el); /* Needed in resolve.c, too  */

gfc_try gfc_check_symbol_typed (gfc_symbol*, gfc_namespace*, bool, locus);

/* intrinsic.c */
extern int gfc_init_expr;

/* Given a symbol that we have decided is intrinsic, mark it as such
   by placing it into a special module that is otherwise impossible to
   read or write.  */

#define gfc_intrinsic_symbol(SYM) SYM->module = gfc_get_string ("(intrinsic)")

void gfc_intrinsic_init_1 (void);
void gfc_intrinsic_done_1 (void);

char gfc_type_letter (bt);
gfc_symbol * gfc_get_intrinsic_sub_symbol (const char *);
gfc_try gfc_convert_type (gfc_expr *, gfc_typespec *, int);
gfc_try gfc_convert_type_warn (gfc_expr *, gfc_typespec *, int, int);
gfc_try gfc_convert_chartype (gfc_expr *, gfc_typespec *);
int gfc_generic_intrinsic (const char *);
int gfc_specific_intrinsic (const char *);
bool gfc_is_intrinsic (gfc_symbol*, int, locus);
int gfc_intrinsic_actual_ok (const char *, const bool);
gfc_intrinsic_sym *gfc_find_function (const char *);
gfc_intrinsic_sym *gfc_find_subroutine (const char *);

match gfc_intrinsic_func_interface (gfc_expr *, int);
match gfc_intrinsic_sub_interface (gfc_code *, int);

void gfc_warn_intrinsic_shadow (const gfc_symbol*, bool, bool);
gfc_try gfc_check_intrinsic_standard (const gfc_intrinsic_sym*, const char**,
				      bool, locus);

/* match.c -- FIXME */
void gfc_free_iterator (gfc_iterator *, int);
void gfc_free_forall_iterator (gfc_forall_iterator *);
void gfc_free_alloc_list (gfc_alloc *);
void gfc_free_namelist (gfc_namelist *);
void gfc_free_equiv (gfc_equiv *);
void gfc_free_data (gfc_data *);
void gfc_free_case_list (gfc_case *);

/* matchexp.c -- FIXME too?  */
gfc_expr *gfc_get_parentheses (gfc_expr *);

/* openmp.c */
void gfc_free_omp_clauses (gfc_omp_clauses *);
void gfc_resolve_omp_directive (gfc_code *, gfc_namespace *);
void gfc_resolve_do_iterator (gfc_code *, gfc_symbol *);
void gfc_resolve_omp_parallel_blocks (gfc_code *, gfc_namespace *);
void gfc_resolve_omp_do_blocks (gfc_code *, gfc_namespace *);

/* expr.c */
void gfc_free_actual_arglist (gfc_actual_arglist *);
gfc_actual_arglist *gfc_copy_actual_arglist (gfc_actual_arglist *);
const char *gfc_extract_int (gfc_expr *, int *);
gfc_expr *gfc_expr_to_initialize (gfc_expr *);
bool is_subref_array (gfc_expr *);

gfc_expr *gfc_build_conversion (gfc_expr *);
void gfc_free_ref_list (gfc_ref *);
void gfc_type_convert_binary (gfc_expr *);
int gfc_is_constant_expr (gfc_expr *);
gfc_try gfc_simplify_expr (gfc_expr *, int);
int gfc_has_vector_index (gfc_expr *);

gfc_expr *gfc_get_expr (void);
void gfc_free_expr (gfc_expr *);
void gfc_replace_expr (gfc_expr *, gfc_expr *);
gfc_expr *gfc_int_expr (int);
gfc_expr *gfc_logical_expr (int, locus *);
mpz_t *gfc_copy_shape (mpz_t *, int);
mpz_t *gfc_copy_shape_excluding (mpz_t *, int, gfc_expr *);
gfc_expr *gfc_copy_expr (gfc_expr *);
gfc_ref* gfc_copy_ref (gfc_ref*);

gfc_try gfc_specification_expr (gfc_expr *);

int gfc_numeric_ts (gfc_typespec *);
int gfc_kind_max (gfc_expr *, gfc_expr *);

gfc_try gfc_check_conformance (const char *, gfc_expr *, gfc_expr *);
gfc_try gfc_check_assign (gfc_expr *, gfc_expr *, int);
gfc_try gfc_check_pointer_assign (gfc_expr *, gfc_expr *);
gfc_try gfc_check_assign_symbol (gfc_symbol *, gfc_expr *);

gfc_expr *gfc_default_initializer (gfc_typespec *);
gfc_expr *gfc_get_variable_expr (gfc_symtree *);

bool gfc_traverse_expr (gfc_expr *, gfc_symbol *,
			bool (*)(gfc_expr *, gfc_symbol *, int*),
			int);
void gfc_expr_set_symbols_referenced (gfc_expr *);
gfc_try gfc_expr_check_typed (gfc_expr*, gfc_namespace*, bool);
void gfc_expr_replace_symbols (gfc_expr *, gfc_symbol *);

/* st.c */
extern gfc_code new_st;

void gfc_clear_new_st (void);
gfc_code *gfc_get_code (void);
gfc_code *gfc_append_code (gfc_code *, gfc_code *);
void gfc_free_statement (gfc_code *);
void gfc_free_statements (gfc_code *);

/* resolve.c */
gfc_try gfc_resolve_expr (gfc_expr *);
void gfc_resolve (gfc_namespace *);
void gfc_resolve_blocks (gfc_code *, gfc_namespace *);
int gfc_impure_variable (gfc_symbol *);
int gfc_pure (gfc_symbol *);
int gfc_elemental (gfc_symbol *);
gfc_try gfc_resolve_iterator (gfc_iterator *, bool);
gfc_try find_forall_index (gfc_expr *, gfc_symbol *, int);
gfc_try gfc_resolve_index (gfc_expr *, int);
gfc_try gfc_resolve_dim_arg (gfc_expr *);
int gfc_is_formal_arg (void);
void gfc_resolve_substring_charlen (gfc_expr *);
match gfc_iso_c_sub_interface(gfc_code *, gfc_symbol *);


/* array.c */
void gfc_free_array_spec (gfc_array_spec *);
gfc_array_ref *gfc_copy_array_ref (gfc_array_ref *);

gfc_try gfc_set_array_spec (gfc_symbol *, gfc_array_spec *, locus *);
gfc_array_spec *gfc_copy_array_spec (gfc_array_spec *);
gfc_try gfc_resolve_array_spec (gfc_array_spec *, int);

int gfc_compare_array_spec (gfc_array_spec *, gfc_array_spec *);

gfc_expr *gfc_start_constructor (bt, int, locus *);
void gfc_append_constructor (gfc_expr *, gfc_expr *);
void gfc_free_constructor (gfc_constructor *);
void gfc_simplify_iterator_var (gfc_expr *);
gfc_try gfc_expand_constructor (gfc_expr *);
int gfc_constant_ac (gfc_expr *);
int gfc_expanded_ac (gfc_expr *);
gfc_try gfc_resolve_character_array_constructor (gfc_expr *);
gfc_try gfc_resolve_array_constructor (gfc_expr *);
gfc_try gfc_check_constructor_type (gfc_expr *);
gfc_try gfc_check_iter_variable (gfc_expr *);
gfc_try gfc_check_constructor (gfc_expr *, gfc_try (*)(gfc_expr *));
gfc_constructor *gfc_copy_constructor (gfc_constructor *);
gfc_expr *gfc_get_array_element (gfc_expr *, int);
gfc_try gfc_array_size (gfc_expr *, mpz_t *);
gfc_try gfc_array_dimen_size (gfc_expr *, int, mpz_t *);
gfc_try gfc_array_ref_shape (gfc_array_ref *, mpz_t *);
gfc_array_ref *gfc_find_array_ref (gfc_expr *);
void gfc_insert_constructor (gfc_expr *, gfc_constructor *);
gfc_constructor *gfc_get_constructor (void);
tree gfc_conv_array_initializer (tree type, gfc_expr *);
gfc_try spec_size (gfc_array_spec *, mpz_t *);
gfc_try spec_dimen_size (gfc_array_spec *, int, mpz_t *);
int gfc_is_compile_time_shape (gfc_array_spec *);

gfc_try gfc_ref_dimen_size (gfc_array_ref *, int dimen, mpz_t *);


/* interface.c -- FIXME: some of these should be in symbol.c */
void gfc_free_interface (gfc_interface *);
int gfc_compare_derived_types (gfc_symbol *, gfc_symbol *);
int gfc_compare_types (gfc_typespec *, gfc_typespec *);
int gfc_compare_interfaces (gfc_symbol*, gfc_symbol*, int);
void gfc_check_interfaces (gfc_namespace *);
void gfc_procedure_use (gfc_symbol *, gfc_actual_arglist **, locus *);
gfc_symbol *gfc_search_interface (gfc_interface *, int,
				  gfc_actual_arglist **);
gfc_try gfc_extend_expr (gfc_expr *);
void gfc_free_formal_arglist (gfc_formal_arglist *);
gfc_try gfc_extend_assign (gfc_code *, gfc_namespace *);
gfc_try gfc_add_interface (gfc_symbol *);
gfc_interface *gfc_current_interface_head (void);
void gfc_set_current_interface_head (gfc_interface *);
gfc_symtree* gfc_find_sym_in_symtree (gfc_symbol*);
bool gfc_arglist_matches_symbol (gfc_actual_arglist**, gfc_symbol*);

/* io.c */
extern gfc_st_label format_asterisk;

void gfc_free_open (gfc_open *);
gfc_try gfc_resolve_open (gfc_open *);
void gfc_free_close (gfc_close *);
gfc_try gfc_resolve_close (gfc_close *);
void gfc_free_filepos (gfc_filepos *);
gfc_try gfc_resolve_filepos (gfc_filepos *);
void gfc_free_inquire (gfc_inquire *);
gfc_try gfc_resolve_inquire (gfc_inquire *);
void gfc_free_dt (gfc_dt *);
gfc_try gfc_resolve_dt (gfc_dt *);
void gfc_free_wait (gfc_wait *);
gfc_try gfc_resolve_wait (gfc_wait *);

/* module.c */
void gfc_module_init_2 (void);
void gfc_module_done_2 (void);
void gfc_dump_module (const char *, int);
bool gfc_check_access (gfc_access, gfc_access);
void gfc_free_use_stmts (gfc_use_list *);

/* primary.c */
symbol_attribute gfc_variable_attr (gfc_expr *, gfc_typespec *);
symbol_attribute gfc_expr_attr (gfc_expr *);
match gfc_match_rvalue (gfc_expr **);
match gfc_match_varspec (gfc_expr*, int, bool);
int gfc_check_digit (char, int);

/* trans.c */
void gfc_generate_code (gfc_namespace *);
void gfc_generate_module_code (gfc_namespace *);

/* bbt.c */
typedef int (*compare_fn) (void *, void *);
void gfc_insert_bbt (void *, void *, compare_fn);
void gfc_delete_bbt (void *, void *, compare_fn);

/* dump-parse-tree.c */
void gfc_dump_parse_tree (gfc_namespace *, FILE *);

/* parse.c */
gfc_try gfc_parse_file (void);
void gfc_global_used (gfc_gsymbol *, locus *);

/* dependency.c */
int gfc_dep_compare_expr (gfc_expr *, gfc_expr *);
int gfc_is_data_pointer (gfc_expr *);

/* check.c */
gfc_try gfc_check_same_strlen (const gfc_expr*, const gfc_expr*, const char*);

#endif /* GCC_GFORTRAN_H  */
