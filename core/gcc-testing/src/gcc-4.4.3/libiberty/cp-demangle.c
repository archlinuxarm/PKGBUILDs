/* Demangler for g++ V3 ABI.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008
   Free Software Foundation, Inc.
   Written by Ian Lance Taylor <ian@wasabisystems.com>.

   This file is part of the libiberty library, which is part of GCC.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   In addition to the permissions in the GNU General Public License, the
   Free Software Foundation gives you unlimited permission to link the
   compiled version of this file into combinations with other programs,
   and to distribute those combinations without any restriction coming
   from the use of this file.  (The General Public License restrictions
   do apply in other respects; for example, they cover modification of
   the file, and distribution when not linked into a combined
   executable.)

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA. 
*/

/* This code implements a demangler for the g++ V3 ABI.  The ABI is
   described on this web page:
       http://www.codesourcery.com/cxx-abi/abi.html#mangling

   This code was written while looking at the demangler written by
   Alex Samuel <samuel@codesourcery.com>.

   This code first pulls the mangled name apart into a list of
   components, and then walks the list generating the demangled
   name.

   This file will normally define the following functions, q.v.:
      char *cplus_demangle_v3(const char *mangled, int options)
      char *java_demangle_v3(const char *mangled)
      int cplus_demangle_v3_callback(const char *mangled, int options,
                                     demangle_callbackref callback)
      int java_demangle_v3_callback(const char *mangled,
                                    demangle_callbackref callback)
      enum gnu_v3_ctor_kinds is_gnu_v3_mangled_ctor (const char *name)
      enum gnu_v3_dtor_kinds is_gnu_v3_mangled_dtor (const char *name)

   Also, the interface to the component list is public, and defined in
   demangle.h.  The interface consists of these types, which are
   defined in demangle.h:
      enum demangle_component_type
      struct demangle_component
      demangle_callbackref
   and these functions defined in this file:
      cplus_demangle_fill_name
      cplus_demangle_fill_extended_operator
      cplus_demangle_fill_ctor
      cplus_demangle_fill_dtor
      cplus_demangle_print
      cplus_demangle_print_callback
   and other functions defined in the file cp-demint.c.

   This file also defines some other functions and variables which are
   only to be used by the file cp-demint.c.

   Preprocessor macros you can define while compiling this file:

   IN_LIBGCC2
      If defined, this file defines the following functions, q.v.:
         char *__cxa_demangle (const char *mangled, char *buf, size_t *len,
                               int *status)
         int __gcclibcxx_demangle_callback (const char *,
                                            void (*)
                                              (const char *, size_t, void *),
                                            void *)
      instead of cplus_demangle_v3[_callback]() and
      java_demangle_v3[_callback]().

   IN_GLIBCPP_V3
      If defined, this file defines only __cxa_demangle() and
      __gcclibcxx_demangle_callback(), and no other publically visible
      functions or variables.

   STANDALONE_DEMANGLER
      If defined, this file defines a main() function which demangles
      any arguments, or, if none, demangles stdin.

   CP_DEMANGLE_DEBUG
      If defined, turns on debugging mode, which prints information on
      stdout about the mangled string.  This is not generally useful.
*/

#if defined (_AIX) && !defined (__GNUC__)
 #pragma alloca
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#else
# ifndef alloca
#  ifdef __GNUC__
#   define alloca __builtin_alloca
#  else
extern char *alloca ();
#  endif /* __GNUC__ */
# endif /* alloca */
#endif /* HAVE_ALLOCA_H */

#include "ansidecl.h"
#include "libiberty.h"
#include "demangle.h"
#include "cp-demangle.h"

/* If IN_GLIBCPP_V3 is defined, some functions are made static.  We
   also rename them via #define to avoid compiler errors when the
   static definition conflicts with the extern declaration in a header
   file.  */
#ifdef IN_GLIBCPP_V3

#define CP_STATIC_IF_GLIBCPP_V3 static

#define cplus_demangle_fill_name d_fill_name
static int d_fill_name (struct demangle_component *, const char *, int);

#define cplus_demangle_fill_extended_operator d_fill_extended_operator
static int
d_fill_extended_operator (struct demangle_component *, int,
                          struct demangle_component *);

#define cplus_demangle_fill_ctor d_fill_ctor
static int
d_fill_ctor (struct demangle_component *, enum gnu_v3_ctor_kinds,
             struct demangle_component *);

#define cplus_demangle_fill_dtor d_fill_dtor
static int
d_fill_dtor (struct demangle_component *, enum gnu_v3_dtor_kinds,
             struct demangle_component *);

#define cplus_demangle_mangled_name d_mangled_name
static struct demangle_component *d_mangled_name (struct d_info *, int);

#define cplus_demangle_type d_type
static struct demangle_component *d_type (struct d_info *);

#define cplus_demangle_print d_print
static char *d_print (int, const struct demangle_component *, int, size_t *);

#define cplus_demangle_print_callback d_print_callback
static int d_print_callback (int, const struct demangle_component *,
                             demangle_callbackref, void *);

#define cplus_demangle_init_info d_init_info
static void d_init_info (const char *, int, size_t, struct d_info *);

#else /* ! defined(IN_GLIBCPP_V3) */
#define CP_STATIC_IF_GLIBCPP_V3
#endif /* ! defined(IN_GLIBCPP_V3) */

/* See if the compiler supports dynamic arrays.  */

#ifdef __GNUC__
#define CP_DYNAMIC_ARRAYS
#else
#ifdef __STDC__
#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 199901L
#define CP_DYNAMIC_ARRAYS
#endif /* __STDC__VERSION >= 199901L */
#endif /* defined (__STDC_VERSION__) */
#endif /* defined (__STDC__) */
#endif /* ! defined (__GNUC__) */

/* We avoid pulling in the ctype tables, to prevent pulling in
   additional unresolved symbols when this code is used in a library.
   FIXME: Is this really a valid reason?  This comes from the original
   V3 demangler code.

   As of this writing this file has the following undefined references
   when compiled with -DIN_GLIBCPP_V3: realloc, free, memcpy, strcpy,
   strcat, strlen.  */

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_UPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define IS_LOWER(c) ((c) >= 'a' && (c) <= 'z')

/* The prefix prepended by GCC to an identifier represnting the
   anonymous namespace.  */
#define ANONYMOUS_NAMESPACE_PREFIX "_GLOBAL_"
#define ANONYMOUS_NAMESPACE_PREFIX_LEN \
  (sizeof (ANONYMOUS_NAMESPACE_PREFIX) - 1)

/* Information we keep for the standard substitutions.  */

struct d_standard_sub_info
{
  /* The code for this substitution.  */
  char code;
  /* The simple string it expands to.  */
  const char *simple_expansion;
  /* The length of the simple expansion.  */
  int simple_len;
  /* The results of a full, verbose, expansion.  This is used when
     qualifying a constructor/destructor, or when in verbose mode.  */
  const char *full_expansion;
  /* The length of the full expansion.  */
  int full_len;
  /* What to set the last_name field of d_info to; NULL if we should
     not set it.  This is only relevant when qualifying a
     constructor/destructor.  */
  const char *set_last_name;
  /* The length of set_last_name.  */
  int set_last_name_len;
};

/* Accessors for subtrees of struct demangle_component.  */

#define d_left(dc) ((dc)->u.s_binary.left)
#define d_right(dc) ((dc)->u.s_binary.right)

/* A list of templates.  This is used while printing.  */

struct d_print_template
{
  /* Next template on the list.  */
  struct d_print_template *next;
  /* This template.  */
  const struct demangle_component *template_decl;
};

/* A list of type modifiers.  This is used while printing.  */

struct d_print_mod
{
  /* Next modifier on the list.  These are in the reverse of the order
     in which they appeared in the mangled string.  */
  struct d_print_mod *next;
  /* The modifier.  */
  const struct demangle_component *mod;
  /* Whether this modifier was printed.  */
  int printed;
  /* The list of templates which applies to this modifier.  */
  struct d_print_template *templates;
};

/* We use these structures to hold information during printing.  */

struct d_growable_string
{
  /* Buffer holding the result.  */
  char *buf;
  /* Current length of data in buffer.  */
  size_t len;
  /* Allocated size of buffer.  */
  size_t alc;
  /* Set to 1 if we had a memory allocation failure.  */
  int allocation_failure;
};

enum { D_PRINT_BUFFER_LENGTH = 256 };
struct d_print_info
{
  /* The options passed to the demangler.  */
  int options;
  /* Fixed-length allocated buffer for demangled data, flushed to the
     callback with a NUL termination once full.  */
  char buf[D_PRINT_BUFFER_LENGTH];
  /* Current length of data in buffer.  */
  size_t len;
  /* The last character printed, saved individually so that it survives
     any buffer flush.  */
  char last_char;
  /* Callback function to handle demangled buffer flush.  */
  demangle_callbackref callback;
  /* Opaque callback argument.  */
  void *opaque;
  /* The current list of templates, if any.  */
  struct d_print_template *templates;
  /* The current list of modifiers (e.g., pointer, reference, etc.),
     if any.  */
  struct d_print_mod *modifiers;
  /* Set to 1 if we saw a demangling error.  */
  int demangle_failure;
  /* The current index into any template argument packs we are using
     for printing.  */
  int pack_index;
};

#ifdef CP_DEMANGLE_DEBUG
static void d_dump (struct demangle_component *, int);
#endif

static struct demangle_component *
d_make_empty (struct d_info *);

static struct demangle_component *
d_make_comp (struct d_info *, enum demangle_component_type,
             struct demangle_component *,
             struct demangle_component *);

static struct demangle_component *
d_make_name (struct d_info *, const char *, int);

static struct demangle_component *
d_make_builtin_type (struct d_info *,
                     const struct demangle_builtin_type_info *);

static struct demangle_component *
d_make_operator (struct d_info *,
                 const struct demangle_operator_info *);

static struct demangle_component *
d_make_extended_operator (struct d_info *, int,
                          struct demangle_component *);

static struct demangle_component *
d_make_ctor (struct d_info *, enum gnu_v3_ctor_kinds,
             struct demangle_component *);

static struct demangle_component *
d_make_dtor (struct d_info *, enum gnu_v3_dtor_kinds,
             struct demangle_component *);

static struct demangle_component *
d_make_template_param (struct d_info *, long);

static struct demangle_component *
d_make_sub (struct d_info *, const char *, int);

static int
has_return_type (struct demangle_component *);

static int
is_ctor_dtor_or_conversion (struct demangle_component *);

static struct demangle_component *d_encoding (struct d_info *, int);

static struct demangle_component *d_name (struct d_info *);

static struct demangle_component *d_nested_name (struct d_info *);

static struct demangle_component *d_prefix (struct d_info *);

static struct demangle_component *d_unqualified_name (struct d_info *);

static struct demangle_component *d_source_name (struct d_info *);

static long d_number (struct d_info *);

static struct demangle_component *d_identifier (struct d_info *, int);

static struct demangle_component *d_operator_name (struct d_info *);

static struct demangle_component *d_special_name (struct d_info *);

static int d_call_offset (struct d_info *, int);

static struct demangle_component *d_ctor_dtor_name (struct d_info *);

static struct demangle_component **
d_cv_qualifiers (struct d_info *, struct demangle_component **, int);

static struct demangle_component *
d_function_type (struct d_info *);

static struct demangle_component *
d_bare_function_type (struct d_info *, int);

static struct demangle_component *
d_class_enum_type (struct d_info *);

static struct demangle_component *d_array_type (struct d_info *);

static struct demangle_component *
d_pointer_to_member_type (struct d_info *);

static struct demangle_component *
d_template_param (struct d_info *);

static struct demangle_component *d_template_args (struct d_info *);

static struct demangle_component *
d_template_arg (struct d_info *);

static struct demangle_component *d_expression (struct d_info *);

static struct demangle_component *d_expr_primary (struct d_info *);

static struct demangle_component *d_local_name (struct d_info *);

static int d_discriminator (struct d_info *);

static int
d_add_substitution (struct d_info *, struct demangle_component *);

static struct demangle_component *d_substitution (struct d_info *, int);

static void d_growable_string_init (struct d_growable_string *, size_t);

static inline void
d_growable_string_resize (struct d_growable_string *, size_t);

static inline void
d_growable_string_append_buffer (struct d_growable_string *,
                                 const char *, size_t);
static void
d_growable_string_callback_adapter (const char *, size_t, void *);

static void
d_print_init (struct d_print_info *, int, demangle_callbackref, void *);

static inline void d_print_error (struct d_print_info *);

static inline int d_print_saw_error (struct d_print_info *);

static inline void d_print_flush (struct d_print_info *);

static inline void d_append_char (struct d_print_info *, char);

static inline void d_append_buffer (struct d_print_info *,
                                    const char *, size_t);

static inline void d_append_string (struct d_print_info *, const char *);

static inline char d_last_char (struct d_print_info *);

static void
d_print_comp (struct d_print_info *, const struct demangle_component *);

static void
d_print_java_identifier (struct d_print_info *, const char *, int);

static void
d_print_mod_list (struct d_print_info *, struct d_print_mod *, int);

static void
d_print_mod (struct d_print_info *, const struct demangle_component *);

static void
d_print_function_type (struct d_print_info *,
                       const struct demangle_component *,
                       struct d_print_mod *);

static void
d_print_array_type (struct d_print_info *,
                    const struct demangle_component *,
                    struct d_print_mod *);

static void
d_print_expr_op (struct d_print_info *, const struct demangle_component *);

static void
d_print_cast (struct d_print_info *, const struct demangle_component *);

static int d_demangle_callback (const char *, int,
                                demangle_callbackref, void *);
static char *d_demangle (const char *, int, size_t *);

#ifdef CP_DEMANGLE_DEBUG

static void
d_dump (struct demangle_component *dc, int indent)
{
  int i;

  if (dc == NULL)
    {
      if (indent == 0)
        printf ("failed demangling\n");
      return;
    }

  for (i = 0; i < indent; ++i)
    putchar (' ');

  switch (dc->type)
    {
    case DEMANGLE_COMPONENT_NAME:
      printf ("name '%.*s'\n", dc->u.s_name.len, dc->u.s_name.s);
      return;
    case DEMANGLE_COMPONENT_TEMPLATE_PARAM:
      printf ("template parameter %ld\n", dc->u.s_number.number);
      return;
    case DEMANGLE_COMPONENT_CTOR:
      printf ("constructor %d\n", (int) dc->u.s_ctor.kind);
      d_dump (dc->u.s_ctor.name, indent + 2);
      return;
    case DEMANGLE_COMPONENT_DTOR:
      printf ("destructor %d\n", (int) dc->u.s_dtor.kind);
      d_dump (dc->u.s_dtor.name, indent + 2);
      return;
    case DEMANGLE_COMPONENT_SUB_STD:
      printf ("standard substitution %s\n", dc->u.s_string.string);
      return;
    case DEMANGLE_COMPONENT_BUILTIN_TYPE:
      printf ("builtin type %s\n", dc->u.s_builtin.type->name);
      return;
    case DEMANGLE_COMPONENT_OPERATOR:
      printf ("operator %s\n", dc->u.s_operator.op->name);
      return;
    case DEMANGLE_COMPONENT_EXTENDED_OPERATOR:
      printf ("extended operator with %d args\n",
	      dc->u.s_extended_operator.args);
      d_dump (dc->u.s_extended_operator.name, indent + 2);
      return;

    case DEMANGLE_COMPONENT_QUAL_NAME:
      printf ("qualified name\n");
      break;
    case DEMANGLE_COMPONENT_LOCAL_NAME:
      printf ("local name\n");
      break;
    case DEMANGLE_COMPONENT_TYPED_NAME:
      printf ("typed name\n");
      break;
    case DEMANGLE_COMPONENT_TEMPLATE:
      printf ("template\n");
      break;
    case DEMANGLE_COMPONENT_VTABLE:
      printf ("vtable\n");
      break;
    case DEMANGLE_COMPONENT_VTT:
      printf ("VTT\n");
      break;
    case DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE:
      printf ("construction vtable\n");
      break;
    case DEMANGLE_COMPONENT_TYPEINFO:
      printf ("typeinfo\n");
      break;
    case DEMANGLE_COMPONENT_TYPEINFO_NAME:
      printf ("typeinfo name\n");
      break;
    case DEMANGLE_COMPONENT_TYPEINFO_FN:
      printf ("typeinfo function\n");
      break;
    case DEMANGLE_COMPONENT_THUNK:
      printf ("thunk\n");
      break;
    case DEMANGLE_COMPONENT_VIRTUAL_THUNK:
      printf ("virtual thunk\n");
      break;
    case DEMANGLE_COMPONENT_COVARIANT_THUNK:
      printf ("covariant thunk\n");
      break;
    case DEMANGLE_COMPONENT_JAVA_CLASS:
      printf ("java class\n");
      break;
    case DEMANGLE_COMPONENT_GUARD:
      printf ("guard\n");
      break;
    case DEMANGLE_COMPONENT_REFTEMP:
      printf ("reference temporary\n");
      break;
    case DEMANGLE_COMPONENT_HIDDEN_ALIAS:
      printf ("hidden alias\n");
      break;
    case DEMANGLE_COMPONENT_RESTRICT:
      printf ("restrict\n");
      break;
    case DEMANGLE_COMPONENT_VOLATILE:
      printf ("volatile\n");
      break;
    case DEMANGLE_COMPONENT_CONST:
      printf ("const\n");
      break;
    case DEMANGLE_COMPONENT_RESTRICT_THIS:
      printf ("restrict this\n");
      break;
    case DEMANGLE_COMPONENT_VOLATILE_THIS:
      printf ("volatile this\n");
      break;
    case DEMANGLE_COMPONENT_CONST_THIS:
      printf ("const this\n");
      break;
    case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
      printf ("vendor type qualifier\n");
      break;
    case DEMANGLE_COMPONENT_POINTER:
      printf ("pointer\n");
      break;
    case DEMANGLE_COMPONENT_REFERENCE:
      printf ("reference\n");
      break;
    case DEMANGLE_COMPONENT_RVALUE_REFERENCE:
      printf ("rvalue reference\n");
      break;
    case DEMANGLE_COMPONENT_COMPLEX:
      printf ("complex\n");
      break;
    case DEMANGLE_COMPONENT_IMAGINARY:
      printf ("imaginary\n");
      break;
    case DEMANGLE_COMPONENT_VENDOR_TYPE:
      printf ("vendor type\n");
      break;
    case DEMANGLE_COMPONENT_FUNCTION_TYPE:
      printf ("function type\n");
      break;
    case DEMANGLE_COMPONENT_ARRAY_TYPE:
      printf ("array type\n");
      break;
    case DEMANGLE_COMPONENT_PTRMEM_TYPE:
      printf ("pointer to member type\n");
      break;
    case DEMANGLE_COMPONENT_FIXED_TYPE:
      printf ("fixed-point type\n");
      break;
    case DEMANGLE_COMPONENT_ARGLIST:
      printf ("argument list\n");
      break;
    case DEMANGLE_COMPONENT_TEMPLATE_ARGLIST:
      printf ("template argument list\n");
      break;
    case DEMANGLE_COMPONENT_CAST:
      printf ("cast\n");
      break;
    case DEMANGLE_COMPONENT_UNARY:
      printf ("unary operator\n");
      break;
    case DEMANGLE_COMPONENT_BINARY:
      printf ("binary operator\n");
      break;
    case DEMANGLE_COMPONENT_BINARY_ARGS:
      printf ("binary operator arguments\n");
      break;
    case DEMANGLE_COMPONENT_TRINARY:
      printf ("trinary operator\n");
      break;
    case DEMANGLE_COMPONENT_TRINARY_ARG1:
      printf ("trinary operator arguments 1\n");
      break;
    case DEMANGLE_COMPONENT_TRINARY_ARG2:
      printf ("trinary operator arguments 1\n");
      break;
    case DEMANGLE_COMPONENT_LITERAL:
      printf ("literal\n");
      break;
    case DEMANGLE_COMPONENT_LITERAL_NEG:
      printf ("negative literal\n");
      break;
    case DEMANGLE_COMPONENT_JAVA_RESOURCE:
      printf ("java resource\n");
      break;
    case DEMANGLE_COMPONENT_COMPOUND_NAME:
      printf ("compound name\n");
      break;
    case DEMANGLE_COMPONENT_CHARACTER:
      printf ("character '%c'\n",  dc->u.s_character.character);
      return;
    case DEMANGLE_COMPONENT_DECLTYPE:
      printf ("decltype\n");
      break;
    case DEMANGLE_COMPONENT_PACK_EXPANSION:
      printf ("pack expansion\n");
      break;
    }

  d_dump (d_left (dc), indent + 2);
  d_dump (d_right (dc), indent + 2);
}

#endif /* CP_DEMANGLE_DEBUG */

/* Fill in a DEMANGLE_COMPONENT_NAME.  */

CP_STATIC_IF_GLIBCPP_V3
int
cplus_demangle_fill_name (struct demangle_component *p, const char *s, int len)
{
  if (p == NULL || s == NULL || len == 0)
    return 0;
  p->type = DEMANGLE_COMPONENT_NAME;
  p->u.s_name.s = s;
  p->u.s_name.len = len;
  return 1;
}

/* Fill in a DEMANGLE_COMPONENT_EXTENDED_OPERATOR.  */

CP_STATIC_IF_GLIBCPP_V3
int
cplus_demangle_fill_extended_operator (struct demangle_component *p, int args,
                                       struct demangle_component *name)
{
  if (p == NULL || args < 0 || name == NULL)
    return 0;
  p->type = DEMANGLE_COMPONENT_EXTENDED_OPERATOR;
  p->u.s_extended_operator.args = args;
  p->u.s_extended_operator.name = name;
  return 1;
}

/* Fill in a DEMANGLE_COMPONENT_CTOR.  */

CP_STATIC_IF_GLIBCPP_V3
int
cplus_demangle_fill_ctor (struct demangle_component *p,
                          enum gnu_v3_ctor_kinds kind,
                          struct demangle_component *name)
{
  if (p == NULL
      || name == NULL
      || (kind < gnu_v3_complete_object_ctor
	  && kind > gnu_v3_complete_object_allocating_ctor))
    return 0;
  p->type = DEMANGLE_COMPONENT_CTOR;
  p->u.s_ctor.kind = kind;
  p->u.s_ctor.name = name;
  return 1;
}

/* Fill in a DEMANGLE_COMPONENT_DTOR.  */

CP_STATIC_IF_GLIBCPP_V3
int
cplus_demangle_fill_dtor (struct demangle_component *p,
                          enum gnu_v3_dtor_kinds kind,
                          struct demangle_component *name)
{
  if (p == NULL
      || name == NULL
      || (kind < gnu_v3_deleting_dtor
	  && kind > gnu_v3_base_object_dtor))
    return 0;
  p->type = DEMANGLE_COMPONENT_DTOR;
  p->u.s_dtor.kind = kind;
  p->u.s_dtor.name = name;
  return 1;
}

/* Add a new component.  */

static struct demangle_component *
d_make_empty (struct d_info *di)
{
  struct demangle_component *p;

  if (di->next_comp >= di->num_comps)
    return NULL;
  p = &di->comps[di->next_comp];
  ++di->next_comp;
  return p;
}

/* Add a new generic component.  */

static struct demangle_component *
d_make_comp (struct d_info *di, enum demangle_component_type type,
             struct demangle_component *left,
             struct demangle_component *right)
{
  struct demangle_component *p;

  /* We check for errors here.  A typical error would be a NULL return
     from a subroutine.  We catch those here, and return NULL
     upward.  */
  switch (type)
    {
      /* These types require two parameters.  */
    case DEMANGLE_COMPONENT_QUAL_NAME:
    case DEMANGLE_COMPONENT_LOCAL_NAME:
    case DEMANGLE_COMPONENT_TYPED_NAME:
    case DEMANGLE_COMPONENT_TEMPLATE:
    case DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE:
    case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
    case DEMANGLE_COMPONENT_PTRMEM_TYPE:
    case DEMANGLE_COMPONENT_UNARY:
    case DEMANGLE_COMPONENT_BINARY:
    case DEMANGLE_COMPONENT_BINARY_ARGS:
    case DEMANGLE_COMPONENT_TRINARY:
    case DEMANGLE_COMPONENT_TRINARY_ARG1:
    case DEMANGLE_COMPONENT_TRINARY_ARG2:
    case DEMANGLE_COMPONENT_LITERAL:
    case DEMANGLE_COMPONENT_LITERAL_NEG:
    case DEMANGLE_COMPONENT_COMPOUND_NAME:
      if (left == NULL || right == NULL)
	return NULL;
      break;

      /* These types only require one parameter.  */
    case DEMANGLE_COMPONENT_VTABLE:
    case DEMANGLE_COMPONENT_VTT:
    case DEMANGLE_COMPONENT_TYPEINFO:
    case DEMANGLE_COMPONENT_TYPEINFO_NAME:
    case DEMANGLE_COMPONENT_TYPEINFO_FN:
    case DEMANGLE_COMPONENT_THUNK:
    case DEMANGLE_COMPONENT_VIRTUAL_THUNK:
    case DEMANGLE_COMPONENT_COVARIANT_THUNK:
    case DEMANGLE_COMPONENT_JAVA_CLASS:
    case DEMANGLE_COMPONENT_GUARD:
    case DEMANGLE_COMPONENT_REFTEMP:
    case DEMANGLE_COMPONENT_HIDDEN_ALIAS:
    case DEMANGLE_COMPONENT_POINTER:
    case DEMANGLE_COMPONENT_REFERENCE:
    case DEMANGLE_COMPONENT_RVALUE_REFERENCE:
    case DEMANGLE_COMPONENT_COMPLEX:
    case DEMANGLE_COMPONENT_IMAGINARY:
    case DEMANGLE_COMPONENT_VENDOR_TYPE:
    case DEMANGLE_COMPONENT_CAST:
    case DEMANGLE_COMPONENT_JAVA_RESOURCE:
    case DEMANGLE_COMPONENT_DECLTYPE:
    case DEMANGLE_COMPONENT_PACK_EXPANSION:
      if (left == NULL)
	return NULL;
      break;

      /* This needs a right parameter, but the left parameter can be
	 empty.  */
    case DEMANGLE_COMPONENT_ARRAY_TYPE:
      if (right == NULL)
	return NULL;
      break;

      /* These are allowed to have no parameters--in some cases they
	 will be filled in later.  */
    case DEMANGLE_COMPONENT_FUNCTION_TYPE:
    case DEMANGLE_COMPONENT_RESTRICT:
    case DEMANGLE_COMPONENT_VOLATILE:
    case DEMANGLE_COMPONENT_CONST:
    case DEMANGLE_COMPONENT_RESTRICT_THIS:
    case DEMANGLE_COMPONENT_VOLATILE_THIS:
    case DEMANGLE_COMPONENT_CONST_THIS:
    case DEMANGLE_COMPONENT_ARGLIST:
    case DEMANGLE_COMPONENT_TEMPLATE_ARGLIST:
      break;

      /* Other types should not be seen here.  */
    default:
      return NULL;
    }

  p = d_make_empty (di);
  if (p != NULL)
    {
      p->type = type;
      p->u.s_binary.left = left;
      p->u.s_binary.right = right;
    }
  return p;
}

/* Add a new name component.  */

static struct demangle_component *
d_make_name (struct d_info *di, const char *s, int len)
{
  struct demangle_component *p;

  p = d_make_empty (di);
  if (! cplus_demangle_fill_name (p, s, len))
    return NULL;
  return p;
}

/* Add a new builtin type component.  */

static struct demangle_component *
d_make_builtin_type (struct d_info *di,
                     const struct demangle_builtin_type_info *type)
{
  struct demangle_component *p;

  if (type == NULL)
    return NULL;
  p = d_make_empty (di);
  if (p != NULL)
    {
      p->type = DEMANGLE_COMPONENT_BUILTIN_TYPE;
      p->u.s_builtin.type = type;
    }
  return p;
}

/* Add a new operator component.  */

static struct demangle_component *
d_make_operator (struct d_info *di, const struct demangle_operator_info *op)
{
  struct demangle_component *p;

  p = d_make_empty (di);
  if (p != NULL)
    {
      p->type = DEMANGLE_COMPONENT_OPERATOR;
      p->u.s_operator.op = op;
    }
  return p;
}

/* Add a new extended operator component.  */

static struct demangle_component *
d_make_extended_operator (struct d_info *di, int args,
                          struct demangle_component *name)
{
  struct demangle_component *p;

  p = d_make_empty (di);
  if (! cplus_demangle_fill_extended_operator (p, args, name))
    return NULL;
  return p;
}

/* Add a new constructor component.  */

static struct demangle_component *
d_make_ctor (struct d_info *di, enum gnu_v3_ctor_kinds kind,
             struct demangle_component *name)
{
  struct demangle_component *p;

  p = d_make_empty (di);
  if (! cplus_demangle_fill_ctor (p, kind, name))
    return NULL;
  return p;
}

/* Add a new destructor component.  */

static struct demangle_component *
d_make_dtor (struct d_info *di, enum gnu_v3_dtor_kinds kind,
             struct demangle_component *name)
{
  struct demangle_component *p;

  p = d_make_empty (di);
  if (! cplus_demangle_fill_dtor (p, kind, name))
    return NULL;
  return p;
}

/* Add a new template parameter.  */

static struct demangle_component *
d_make_template_param (struct d_info *di, long i)
{
  struct demangle_component *p;

  p = d_make_empty (di);
  if (p != NULL)
    {
      p->type = DEMANGLE_COMPONENT_TEMPLATE_PARAM;
      p->u.s_number.number = i;
    }
  return p;
}

/* Add a new function parameter.  */

static struct demangle_component *
d_make_function_param (struct d_info *di, long i)
{
  struct demangle_component *p;

  p = d_make_empty (di);
  if (p != NULL)
    {
      p->type = DEMANGLE_COMPONENT_FUNCTION_PARAM;
      p->u.s_number.number = i;
    }
  return p;
}

/* Add a new standard substitution component.  */

static struct demangle_component *
d_make_sub (struct d_info *di, const char *name, int len)
{
  struct demangle_component *p;

  p = d_make_empty (di);
  if (p != NULL)
    {
      p->type = DEMANGLE_COMPONENT_SUB_STD;
      p->u.s_string.string = name;
      p->u.s_string.len = len;
    }
  return p;
}

/* <mangled-name> ::= _Z <encoding>

   TOP_LEVEL is non-zero when called at the top level.  */

CP_STATIC_IF_GLIBCPP_V3
struct demangle_component *
cplus_demangle_mangled_name (struct d_info *di, int top_level)
{
  if (! d_check_char (di, '_')
      /* Allow missing _ if not at toplevel to work around a
	 bug in G++ abi-version=2 mangling; see the comment in
	 write_template_arg.  */
      && top_level)
    return NULL;
  if (! d_check_char (di, 'Z'))
    return NULL;
  return d_encoding (di, top_level);
}

/* Return whether a function should have a return type.  The argument
   is the function name, which may be qualified in various ways.  The
   rules are that template functions have return types with some
   exceptions, function types which are not part of a function name
   mangling have return types with some exceptions, and non-template
   function names do not have return types.  The exceptions are that
   constructors, destructors, and conversion operators do not have
   return types.  */

static int
has_return_type (struct demangle_component *dc)
{
  if (dc == NULL)
    return 0;
  switch (dc->type)
    {
    default:
      return 0;
    case DEMANGLE_COMPONENT_TEMPLATE:
      return ! is_ctor_dtor_or_conversion (d_left (dc));
    case DEMANGLE_COMPONENT_RESTRICT_THIS:
    case DEMANGLE_COMPONENT_VOLATILE_THIS:
    case DEMANGLE_COMPONENT_CONST_THIS:
      return has_return_type (d_left (dc));
    }
}

/* Return whether a name is a constructor, a destructor, or a
   conversion operator.  */

static int
is_ctor_dtor_or_conversion (struct demangle_component *dc)
{
  if (dc == NULL)
    return 0;
  switch (dc->type)
    {
    default:
      return 0;
    case DEMANGLE_COMPONENT_QUAL_NAME:
    case DEMANGLE_COMPONENT_LOCAL_NAME:
      return is_ctor_dtor_or_conversion (d_right (dc));
    case DEMANGLE_COMPONENT_CTOR:
    case DEMANGLE_COMPONENT_DTOR:
    case DEMANGLE_COMPONENT_CAST:
      return 1;
    }
}

/* <encoding> ::= <(function) name> <bare-function-type>
              ::= <(data) name>
              ::= <special-name>

   TOP_LEVEL is non-zero when called at the top level, in which case
   if DMGL_PARAMS is not set we do not demangle the function
   parameters.  We only set this at the top level, because otherwise
   we would not correctly demangle names in local scopes.  */

static struct demangle_component *
d_encoding (struct d_info *di, int top_level)
{
  char peek = d_peek_char (di);

  if (peek == 'G' || peek == 'T')
    return d_special_name (di);
  else
    {
      struct demangle_component *dc;

      dc = d_name (di);

      if (dc != NULL && top_level && (di->options & DMGL_PARAMS) == 0)
	{
	  /* Strip off any initial CV-qualifiers, as they really apply
	     to the `this' parameter, and they were not output by the
	     v2 demangler without DMGL_PARAMS.  */
	  while (dc->type == DEMANGLE_COMPONENT_RESTRICT_THIS
		 || dc->type == DEMANGLE_COMPONENT_VOLATILE_THIS
		 || dc->type == DEMANGLE_COMPONENT_CONST_THIS)
	    dc = d_left (dc);

	  /* If the top level is a DEMANGLE_COMPONENT_LOCAL_NAME, then
	     there may be CV-qualifiers on its right argument which
	     really apply here; this happens when parsing a class
	     which is local to a function.  */
	  if (dc->type == DEMANGLE_COMPONENT_LOCAL_NAME)
	    {
	      struct demangle_component *dcr;

	      dcr = d_right (dc);
	      while (dcr->type == DEMANGLE_COMPONENT_RESTRICT_THIS
		     || dcr->type == DEMANGLE_COMPONENT_VOLATILE_THIS
		     || dcr->type == DEMANGLE_COMPONENT_CONST_THIS)
		dcr = d_left (dcr);
	      dc->u.s_binary.right = dcr;
	    }

	  return dc;
	}

      peek = d_peek_char (di);
      if (dc == NULL || peek == '\0' || peek == 'E')
	return dc;
      return d_make_comp (di, DEMANGLE_COMPONENT_TYPED_NAME, dc,
			  d_bare_function_type (di, has_return_type (dc)));
    }
}

/* <name> ::= <nested-name>
          ::= <unscoped-name>
          ::= <unscoped-template-name> <template-args>
          ::= <local-name>

   <unscoped-name> ::= <unqualified-name>
                   ::= St <unqualified-name>

   <unscoped-template-name> ::= <unscoped-name>
                            ::= <substitution>
*/

static struct demangle_component *
d_name (struct d_info *di)
{
  char peek = d_peek_char (di);
  struct demangle_component *dc;

  switch (peek)
    {
    case 'N':
      return d_nested_name (di);

    case 'Z':
      return d_local_name (di);

    case 'L':
      return d_unqualified_name (di);
	
    case 'S':
      {
	int subst;

	if (d_peek_next_char (di) != 't')
	  {
	    dc = d_substitution (di, 0);
	    subst = 1;
	  }
	else
	  {
	    d_advance (di, 2);
	    dc = d_make_comp (di, DEMANGLE_COMPONENT_QUAL_NAME,
			      d_make_name (di, "std", 3),
			      d_unqualified_name (di));
	    di->expansion += 3;
	    subst = 0;
	  }

	if (d_peek_char (di) != 'I')
	  {
	    /* The grammar does not permit this case to occur if we
	       called d_substitution() above (i.e., subst == 1).  We
	       don't bother to check.  */
	  }
	else
	  {
	    /* This is <template-args>, which means that we just saw
	       <unscoped-template-name>, which is a substitution
	       candidate if we didn't just get it from a
	       substitution.  */
	    if (! subst)
	      {
		if (! d_add_substitution (di, dc))
		  return NULL;
	      }
	    dc = d_make_comp (di, DEMANGLE_COMPONENT_TEMPLATE, dc,
			      d_template_args (di));
	  }

	return dc;
      }

    default:
      dc = d_unqualified_name (di);
      if (d_peek_char (di) == 'I')
	{
	  /* This is <template-args>, which means that we just saw
	     <unscoped-template-name>, which is a substitution
	     candidate.  */
	  if (! d_add_substitution (di, dc))
	    return NULL;
	  dc = d_make_comp (di, DEMANGLE_COMPONENT_TEMPLATE, dc,
			    d_template_args (di));
	}
      return dc;
    }
}

/* <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name> E
                 ::= N [<CV-qualifiers>] <template-prefix> <template-args> E
*/

static struct demangle_component *
d_nested_name (struct d_info *di)
{
  struct demangle_component *ret;
  struct demangle_component **pret;

  if (! d_check_char (di, 'N'))
    return NULL;

  pret = d_cv_qualifiers (di, &ret, 1);
  if (pret == NULL)
    return NULL;

  *pret = d_prefix (di);
  if (*pret == NULL)
    return NULL;

  if (! d_check_char (di, 'E'))
    return NULL;

  return ret;
}

/* <prefix> ::= <prefix> <unqualified-name>
            ::= <template-prefix> <template-args>
            ::= <template-param>
            ::=
            ::= <substitution>

   <template-prefix> ::= <prefix> <(template) unqualified-name>
                     ::= <template-param>
                     ::= <substitution>
*/

static struct demangle_component *
d_prefix (struct d_info *di)
{
  struct demangle_component *ret = NULL;

  while (1)
    {
      char peek;
      enum demangle_component_type comb_type;
      struct demangle_component *dc;

      peek = d_peek_char (di);
      if (peek == '\0')
	return NULL;

      /* The older code accepts a <local-name> here, but I don't see
	 that in the grammar.  The older code does not accept a
	 <template-param> here.  */

      comb_type = DEMANGLE_COMPONENT_QUAL_NAME;
      if (IS_DIGIT (peek)
	  || IS_LOWER (peek)
	  || peek == 'C'
	  || peek == 'D'
	  || peek == 'L')
	dc = d_unqualified_name (di);
      else if (peek == 'S')
	dc = d_substitution (di, 1);
      else if (peek == 'I')
	{
	  if (ret == NULL)
	    return NULL;
	  comb_type = DEMANGLE_COMPONENT_TEMPLATE;
	  dc = d_template_args (di);
	}
      else if (peek == 'T')
	dc = d_template_param (di);
      else if (peek == 'E')
	return ret;
      else
	return NULL;

      if (ret == NULL)
	ret = dc;
      else
	ret = d_make_comp (di, comb_type, ret, dc);

      if (peek != 'S' && d_peek_char (di) != 'E')
	{
	  if (! d_add_substitution (di, ret))
	    return NULL;
	}
    }
}

/* <unqualified-name> ::= <operator-name>
                      ::= <ctor-dtor-name>
                      ::= <source-name>
		      ::= <local-source-name> 

    <local-source-name>	::= L <source-name> <discriminator>
*/

static struct demangle_component *
d_unqualified_name (struct d_info *di)
{
  char peek;

  peek = d_peek_char (di);
  if (IS_DIGIT (peek))
    return d_source_name (di);
  else if (IS_LOWER (peek))
    {
      struct demangle_component *ret;

      ret = d_operator_name (di);
      if (ret != NULL && ret->type == DEMANGLE_COMPONENT_OPERATOR)
	di->expansion += sizeof "operator" + ret->u.s_operator.op->len - 2;
      return ret;
    }
  else if (peek == 'C' || peek == 'D')
    return d_ctor_dtor_name (di);
  else if (peek == 'L')
    {
      struct demangle_component * ret;

      d_advance (di, 1);

      ret = d_source_name (di);
      if (ret == NULL)
	return NULL;
      if (! d_discriminator (di))
	return NULL;
      return ret;
    }
  else
    return NULL;
}

/* <source-name> ::= <(positive length) number> <identifier>  */

static struct demangle_component *
d_source_name (struct d_info *di)
{
  long len;
  struct demangle_component *ret;

  len = d_number (di);
  if (len <= 0)
    return NULL;
  ret = d_identifier (di, len);
  di->last_name = ret;
  return ret;
}

/* number ::= [n] <(non-negative decimal integer)>  */

static long
d_number (struct d_info *di)
{
  int negative;
  char peek;
  long ret;

  negative = 0;
  peek = d_peek_char (di);
  if (peek == 'n')
    {
      negative = 1;
      d_advance (di, 1);
      peek = d_peek_char (di);
    }

  ret = 0;
  while (1)
    {
      if (! IS_DIGIT (peek))
	{
	  if (negative)
	    ret = - ret;
	  return ret;
	}
      ret = ret * 10 + peek - '0';
      d_advance (di, 1);
      peek = d_peek_char (di);
    }
}

/* identifier ::= <(unqualified source code identifier)>  */

static struct demangle_component *
d_identifier (struct d_info *di, int len)
{
  const char *name;

  name = d_str (di);

  if (di->send - name < len)
    return NULL;

  d_advance (di, len);

  /* A Java mangled name may have a trailing '$' if it is a C++
     keyword.  This '$' is not included in the length count.  We just
     ignore the '$'.  */
  if ((di->options & DMGL_JAVA) != 0
      && d_peek_char (di) == '$')
    d_advance (di, 1);

  /* Look for something which looks like a gcc encoding of an
     anonymous namespace, and replace it with a more user friendly
     name.  */
  if (len >= (int) ANONYMOUS_NAMESPACE_PREFIX_LEN + 2
      && memcmp (name, ANONYMOUS_NAMESPACE_PREFIX,
		 ANONYMOUS_NAMESPACE_PREFIX_LEN) == 0)
    {
      const char *s;

      s = name + ANONYMOUS_NAMESPACE_PREFIX_LEN;
      if ((*s == '.' || *s == '_' || *s == '$')
	  && s[1] == 'N')
	{
	  di->expansion -= len - sizeof "(anonymous namespace)";
	  return d_make_name (di, "(anonymous namespace)",
			      sizeof "(anonymous namespace)" - 1);
	}
    }

  return d_make_name (di, name, len);
}

/* operator_name ::= many different two character encodings.
                 ::= cv <type>
                 ::= v <digit> <source-name>
*/

#define NL(s) s, (sizeof s) - 1

CP_STATIC_IF_GLIBCPP_V3
const struct demangle_operator_info cplus_demangle_operators[] =
{
  { "aN", NL ("&="),        2 },
  { "aS", NL ("="),         2 },
  { "aa", NL ("&&"),        2 },
  { "ad", NL ("&"),         1 },
  { "an", NL ("&"),         2 },
  { "cl", NL ("()"),        2 },
  { "cm", NL (","),         2 },
  { "co", NL ("~"),         1 },
  { "dV", NL ("/="),        2 },
  { "da", NL ("delete[]"),  1 },
  { "de", NL ("*"),         1 },
  { "dl", NL ("delete"),    1 },
  { "dt", NL ("."),         2 },
  { "dv", NL ("/"),         2 },
  { "eO", NL ("^="),        2 },
  { "eo", NL ("^"),         2 },
  { "eq", NL ("=="),        2 },
  { "ge", NL (">="),        2 },
  { "gt", NL (">"),         2 },
  { "ix", NL ("[]"),        2 },
  { "lS", NL ("<<="),       2 },
  { "le", NL ("<="),        2 },
  { "ls", NL ("<<"),        2 },
  { "lt", NL ("<"),         2 },
  { "mI", NL ("-="),        2 },
  { "mL", NL ("*="),        2 },
  { "mi", NL ("-"),         2 },
  { "ml", NL ("*"),         2 },
  { "mm", NL ("--"),        1 },
  { "na", NL ("new[]"),     1 },
  { "ne", NL ("!="),        2 },
  { "ng", NL ("-"),         1 },
  { "nt", NL ("!"),         1 },
  { "nw", NL ("new"),       1 },
  { "oR", NL ("|="),        2 },
  { "oo", NL ("||"),        2 },
  { "or", NL ("|"),         2 },
  { "pL", NL ("+="),        2 },
  { "pl", NL ("+"),         2 },
  { "pm", NL ("->*"),       2 },
  { "pp", NL ("++"),        1 },
  { "ps", NL ("+"),         1 },
  { "pt", NL ("->"),        2 },
  { "qu", NL ("?"),         3 },
  { "rM", NL ("%="),        2 },
  { "rS", NL (">>="),       2 },
  { "rm", NL ("%"),         2 },
  { "rs", NL (">>"),        2 },
  { "st", NL ("sizeof "),   1 },
  { "sz", NL ("sizeof "),   1 },
  { "at", NL ("alignof "),   1 },
  { "az", NL ("alignof "),   1 },
  { NULL, NULL, 0,          0 }
};

static struct demangle_component *
d_operator_name (struct d_info *di)
{
  char c1;
  char c2;

  c1 = d_next_char (di);
  c2 = d_next_char (di);
  if (c1 == 'v' && IS_DIGIT (c2))
    return d_make_extended_operator (di, c2 - '0', d_source_name (di));
  else if (c1 == 'c' && c2 == 'v')
    return d_make_comp (di, DEMANGLE_COMPONENT_CAST,
			cplus_demangle_type (di), NULL);
  else
    {
      /* LOW is the inclusive lower bound.  */
      int low = 0;
      /* HIGH is the exclusive upper bound.  We subtract one to ignore
	 the sentinel at the end of the array.  */
      int high = ((sizeof (cplus_demangle_operators)
		   / sizeof (cplus_demangle_operators[0]))
		  - 1);

      while (1)
	{
	  int i;
	  const struct demangle_operator_info *p;

	  i = low + (high - low) / 2;
	  p = cplus_demangle_operators + i;

	  if (c1 == p->code[0] && c2 == p->code[1])
	    return d_make_operator (di, p);

	  if (c1 < p->code[0] || (c1 == p->code[0] && c2 < p->code[1]))
	    high = i;
	  else
	    low = i + 1;
	  if (low == high)
	    return NULL;
	}
    }
}

static struct demangle_component *
d_make_character (struct d_info *di, int c)
{
  struct demangle_component *p;
  p = d_make_empty (di);
  if (p != NULL)
    {
      p->type = DEMANGLE_COMPONENT_CHARACTER;
      p->u.s_character.character = c;
    }
  return p;
}

static struct demangle_component *
d_java_resource (struct d_info *di)
{
  struct demangle_component *p = NULL;
  struct demangle_component *next = NULL;
  long len, i;
  char c;
  const char *str;

  len = d_number (di);
  if (len <= 1)
    return NULL;

  /* Eat the leading '_'.  */
  if (d_next_char (di) != '_')
    return NULL;
  len--;

  str = d_str (di);
  i = 0;

  while (len > 0)
    {
      c = str[i];
      if (!c)
	return NULL;

      /* Each chunk is either a '$' escape...  */
      if (c == '$')
	{
	  i++;
	  switch (str[i++])
	    {
	    case 'S':
	      c = '/';
	      break;
	    case '_':
	      c = '.';
	      break;
	    case '$':
	      c = '$';
	      break;
	    default:
	      return NULL;
	    }
	  next = d_make_character (di, c);
	  d_advance (di, i);
	  str = d_str (di);
	  len -= i;
	  i = 0;
	  if (next == NULL)
	    return NULL;
	}
      /* ... or a sequence of characters.  */
      else
	{
	  while (i < len && str[i] && str[i] != '$')
	    i++;

	  next = d_make_name (di, str, i);
	  d_advance (di, i);
	  str = d_str (di);
	  len -= i;
	  i = 0;
	  if (next == NULL)
	    return NULL;
	}

      if (p == NULL)
	p = next;
      else
	{
	  p = d_make_comp (di, DEMANGLE_COMPONENT_COMPOUND_NAME, p, next);
	  if (p == NULL)
	    return NULL;
	}
    }

  p = d_make_comp (di, DEMANGLE_COMPONENT_JAVA_RESOURCE, p, NULL);

  return p;
}

/* <special-name> ::= TV <type>
                  ::= TT <type>
                  ::= TI <type>
                  ::= TS <type>
                  ::= GV <(object) name>
                  ::= T <call-offset> <(base) encoding>
                  ::= Tc <call-offset> <call-offset> <(base) encoding>
   Also g++ extensions:
                  ::= TC <type> <(offset) number> _ <(base) type>
                  ::= TF <type>
                  ::= TJ <type>
                  ::= GR <name>
		  ::= GA <encoding>
		  ::= Gr <resource name>
*/

static struct demangle_component *
d_special_name (struct d_info *di)
{
  di->expansion += 20;
  if (d_check_char (di, 'T'))
    {
      switch (d_next_char (di))
	{
	case 'V':
	  di->expansion -= 5;
	  return d_make_comp (di, DEMANGLE_COMPONENT_VTABLE,
			      cplus_demangle_type (di), NULL);
	case 'T':
	  di->expansion -= 10;
	  return d_make_comp (di, DEMANGLE_COMPONENT_VTT,
			      cplus_demangle_type (di), NULL);
	case 'I':
	  return d_make_comp (di, DEMANGLE_COMPONENT_TYPEINFO,
			      cplus_demangle_type (di), NULL);
	case 'S':
	  return d_make_comp (di, DEMANGLE_COMPONENT_TYPEINFO_NAME,
			      cplus_demangle_type (di), NULL);

	case 'h':
	  if (! d_call_offset (di, 'h'))
	    return NULL;
	  return d_make_comp (di, DEMANGLE_COMPONENT_THUNK,
			      d_encoding (di, 0), NULL);

	case 'v':
	  if (! d_call_offset (di, 'v'))
	    return NULL;
	  return d_make_comp (di, DEMANGLE_COMPONENT_VIRTUAL_THUNK,
			      d_encoding (di, 0), NULL);

	case 'c':
	  if (! d_call_offset (di, '\0'))
	    return NULL;
	  if (! d_call_offset (di, '\0'))
	    return NULL;
	  return d_make_comp (di, DEMANGLE_COMPONENT_COVARIANT_THUNK,
			      d_encoding (di, 0), NULL);

	case 'C':
	  {
	    struct demangle_component *derived_type;
	    long offset;
	    struct demangle_component *base_type;

	    derived_type = cplus_demangle_type (di);
	    offset = d_number (di);
	    if (offset < 0)
	      return NULL;
	    if (! d_check_char (di, '_'))
	      return NULL;
	    base_type = cplus_demangle_type (di);
	    /* We don't display the offset.  FIXME: We should display
	       it in verbose mode.  */
	    di->expansion += 5;
	    return d_make_comp (di, DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE,
				base_type, derived_type);
	  }

	case 'F':
	  return d_make_comp (di, DEMANGLE_COMPONENT_TYPEINFO_FN,
			      cplus_demangle_type (di), NULL);
	case 'J':
	  return d_make_comp (di, DEMANGLE_COMPONENT_JAVA_CLASS,
			      cplus_demangle_type (di), NULL);

	default:
	  return NULL;
	}
    }
  else if (d_check_char (di, 'G'))
    {
      switch (d_next_char (di))
	{
	case 'V':
	  return d_make_comp (di, DEMANGLE_COMPONENT_GUARD, d_name (di), NULL);

	case 'R':
	  return d_make_comp (di, DEMANGLE_COMPONENT_REFTEMP, d_name (di),
			      NULL);

	case 'A':
	  return d_make_comp (di, DEMANGLE_COMPONENT_HIDDEN_ALIAS,
			      d_encoding (di, 0), NULL);

	case 'r':
	  return d_java_resource (di);

	default:
	  return NULL;
	}
    }
  else
    return NULL;
}

/* <call-offset> ::= h <nv-offset> _
                 ::= v <v-offset> _

   <nv-offset> ::= <(offset) number>

   <v-offset> ::= <(offset) number> _ <(virtual offset) number>

   The C parameter, if not '\0', is a character we just read which is
   the start of the <call-offset>.

   We don't display the offset information anywhere.  FIXME: We should
   display it in verbose mode.  */

static int
d_call_offset (struct d_info *di, int c)
{
  if (c == '\0')
    c = d_next_char (di);

  if (c == 'h')
    d_number (di);
  else if (c == 'v')
    {
      d_number (di);
      if (! d_check_char (di, '_'))
	return 0;
      d_number (di);
    }
  else
    return 0;

  if (! d_check_char (di, '_'))
    return 0;

  return 1;
}

/* <ctor-dtor-name> ::= C1
                    ::= C2
                    ::= C3
                    ::= D0
                    ::= D1
                    ::= D2
*/

static struct demangle_component *
d_ctor_dtor_name (struct d_info *di)
{
  if (di->last_name != NULL)
    {
      if (di->last_name->type == DEMANGLE_COMPONENT_NAME)
	di->expansion += di->last_name->u.s_name.len;
      else if (di->last_name->type == DEMANGLE_COMPONENT_SUB_STD)
	di->expansion += di->last_name->u.s_string.len;
    }
  switch (d_peek_char (di))
    {
    case 'C':
      {
	enum gnu_v3_ctor_kinds kind;

	switch (d_peek_next_char (di))
	  {
	  case '1':
	    kind = gnu_v3_complete_object_ctor;
	    break;
	  case '2':
	    kind = gnu_v3_base_object_ctor;
	    break;
	  case '3':
	    kind = gnu_v3_complete_object_allocating_ctor;
	    break;
	  default:
	    return NULL;
	  }
	d_advance (di, 2);
	return d_make_ctor (di, kind, di->last_name);
      }

    case 'D':
      {
	enum gnu_v3_dtor_kinds kind;

	switch (d_peek_next_char (di))
	  {
	  case '0':
	    kind = gnu_v3_deleting_dtor;
	    break;
	  case '1':
	    kind = gnu_v3_complete_object_dtor;
	    break;
	  case '2':
	    kind = gnu_v3_base_object_dtor;
	    break;
	  default:
	    return NULL;
	  }
	d_advance (di, 2);
	return d_make_dtor (di, kind, di->last_name);
      }

    default:
      return NULL;
    }
}

/* <type> ::= <builtin-type>
          ::= <function-type>
          ::= <class-enum-type>
          ::= <array-type>
          ::= <pointer-to-member-type>
          ::= <template-param>
          ::= <template-template-param> <template-args>
          ::= <substitution>
          ::= <CV-qualifiers> <type>
          ::= P <type>
          ::= R <type>
          ::= O <type> (C++0x)
          ::= C <type>
          ::= G <type>
          ::= U <source-name> <type>

   <builtin-type> ::= various one letter codes
                  ::= u <source-name>
*/

CP_STATIC_IF_GLIBCPP_V3
const struct demangle_builtin_type_info
cplus_demangle_builtin_types[D_BUILTIN_TYPE_COUNT] =
{
  /* a */ { NL ("signed char"),	NL ("signed char"),	D_PRINT_DEFAULT },
  /* b */ { NL ("bool"),	NL ("boolean"),		D_PRINT_BOOL },
  /* c */ { NL ("char"),	NL ("byte"),		D_PRINT_DEFAULT },
  /* d */ { NL ("double"),	NL ("double"),		D_PRINT_FLOAT },
  /* e */ { NL ("long double"),	NL ("long double"),	D_PRINT_FLOAT },
  /* f */ { NL ("float"),	NL ("float"),		D_PRINT_FLOAT },
  /* g */ { NL ("__float128"),	NL ("__float128"),	D_PRINT_FLOAT },
  /* h */ { NL ("unsigned char"), NL ("unsigned char"),	D_PRINT_DEFAULT },
  /* i */ { NL ("int"),		NL ("int"),		D_PRINT_INT },
  /* j */ { NL ("unsigned int"), NL ("unsigned"),	D_PRINT_UNSIGNED },
  /* k */ { NULL, 0,		NULL, 0,		D_PRINT_DEFAULT },
  /* l */ { NL ("long"),	NL ("long"),		D_PRINT_LONG },
  /* m */ { NL ("unsigned long"), NL ("unsigned long"),	D_PRINT_UNSIGNED_LONG },
  /* n */ { NL ("__int128"),	NL ("__int128"),	D_PRINT_DEFAULT },
  /* o */ { NL ("unsigned __int128"), NL ("unsigned __int128"),
	    D_PRINT_DEFAULT },
  /* p */ { NULL, 0,		NULL, 0,		D_PRINT_DEFAULT },
  /* q */ { NULL, 0,		NULL, 0,		D_PRINT_DEFAULT },
  /* r */ { NULL, 0,		NULL, 0,		D_PRINT_DEFAULT },
  /* s */ { NL ("short"),	NL ("short"),		D_PRINT_DEFAULT },
  /* t */ { NL ("unsigned short"), NL ("unsigned short"), D_PRINT_DEFAULT },
  /* u */ { NULL, 0,		NULL, 0,		D_PRINT_DEFAULT },
  /* v */ { NL ("void"),	NL ("void"),		D_PRINT_VOID },
  /* w */ { NL ("wchar_t"),	NL ("char"),		D_PRINT_DEFAULT },
  /* x */ { NL ("long long"),	NL ("long"),		D_PRINT_LONG_LONG },
  /* y */ { NL ("unsigned long long"), NL ("unsigned long long"),
	    D_PRINT_UNSIGNED_LONG_LONG },
  /* z */ { NL ("..."),		NL ("..."),		D_PRINT_DEFAULT },
  /* 26 */ { NL ("decimal32"),	NL ("decimal32"),	D_PRINT_DEFAULT },
  /* 27 */ { NL ("decimal64"),	NL ("decimal64"),	D_PRINT_DEFAULT },
  /* 28 */ { NL ("decimal128"),	NL ("decimal128"),	D_PRINT_DEFAULT },
  /* 29 */ { NL ("half"),	NL ("half"),		D_PRINT_FLOAT },
  /* 30 */ { NL ("char16_t"),	NL ("char16_t"),	D_PRINT_DEFAULT },
  /* 31 */ { NL ("char32_t"),	NL ("char32_t"),	D_PRINT_DEFAULT },
};

CP_STATIC_IF_GLIBCPP_V3
struct demangle_component *
cplus_demangle_type (struct d_info *di)
{
  char peek;
  struct demangle_component *ret;
  int can_subst;

  /* The ABI specifies that when CV-qualifiers are used, the base type
     is substitutable, and the fully qualified type is substitutable,
     but the base type with a strict subset of the CV-qualifiers is
     not substitutable.  The natural recursive implementation of the
     CV-qualifiers would cause subsets to be substitutable, so instead
     we pull them all off now.

     FIXME: The ABI says that order-insensitive vendor qualifiers
     should be handled in the same way, but we have no way to tell
     which vendor qualifiers are order-insensitive and which are
     order-sensitive.  So we just assume that they are all
     order-sensitive.  g++ 3.4 supports only one vendor qualifier,
     __vector, and it treats it as order-sensitive when mangling
     names.  */

  peek = d_peek_char (di);
  if (peek == 'r' || peek == 'V' || peek == 'K')
    {
      struct demangle_component **pret;

      pret = d_cv_qualifiers (di, &ret, 0);
      if (pret == NULL)
	return NULL;
      *pret = cplus_demangle_type (di);
      if (! *pret || ! d_add_substitution (di, ret))
	return NULL;
      return ret;
    }

  can_subst = 1;

  switch (peek)
    {
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j':           case 'l': case 'm': case 'n':
    case 'o':                               case 's': case 't':
    case 'v': case 'w': case 'x': case 'y': case 'z':
      ret = d_make_builtin_type (di,
				 &cplus_demangle_builtin_types[peek - 'a']);
      di->expansion += ret->u.s_builtin.type->len;
      can_subst = 0;
      d_advance (di, 1);
      break;

    case 'u':
      d_advance (di, 1);
      ret = d_make_comp (di, DEMANGLE_COMPONENT_VENDOR_TYPE,
			 d_source_name (di), NULL);
      break;

    case 'F':
      ret = d_function_type (di);
      break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case 'N':
    case 'Z':
      ret = d_class_enum_type (di);
      break;

    case 'A':
      ret = d_array_type (di);
      break;

    case 'M':
      ret = d_pointer_to_member_type (di);
      break;

    case 'T':
      ret = d_template_param (di);
      if (d_peek_char (di) == 'I')
	{
	  /* This is <template-template-param> <template-args>.  The
	     <template-template-param> part is a substitution
	     candidate.  */
	  if (! d_add_substitution (di, ret))
	    return NULL;
	  ret = d_make_comp (di, DEMANGLE_COMPONENT_TEMPLATE, ret,
			     d_template_args (di));
	}
      break;

    case 'S':
      /* If this is a special substitution, then it is the start of
	 <class-enum-type>.  */
      {
	char peek_next;

	peek_next = d_peek_next_char (di);
	if (IS_DIGIT (peek_next)
	    || peek_next == '_'
	    || IS_UPPER (peek_next))
	  {
	    ret = d_substitution (di, 0);
	    /* The substituted name may have been a template name and
	       may be followed by tepmlate args.  */
	    if (d_peek_char (di) == 'I')
	      ret = d_make_comp (di, DEMANGLE_COMPONENT_TEMPLATE, ret,
				 d_template_args (di));
	    else
	      can_subst = 0;
	  }
	else
	  {
	    ret = d_class_enum_type (di);
	    /* If the substitution was a complete type, then it is not
	       a new substitution candidate.  However, if the
	       substitution was followed by template arguments, then
	       the whole thing is a substitution candidate.  */
	    if (ret != NULL && ret->type == DEMANGLE_COMPONENT_SUB_STD)
	      can_subst = 0;
	  }
      }
      break;

    case 'O':
      d_advance (di, 1);
      ret = d_make_comp (di, DEMANGLE_COMPONENT_RVALUE_REFERENCE,
                         cplus_demangle_type (di), NULL);
      break;

    case 'P':
      d_advance (di, 1);
      ret = d_make_comp (di, DEMANGLE_COMPONENT_POINTER,
			 cplus_demangle_type (di), NULL);
      break;

    case 'R':
      d_advance (di, 1);
      ret = d_make_comp (di, DEMANGLE_COMPONENT_REFERENCE,
                         cplus_demangle_type (di), NULL);
      break;

    case 'C':
      d_advance (di, 1);
      ret = d_make_comp (di, DEMANGLE_COMPONENT_COMPLEX,
			 cplus_demangle_type (di), NULL);
      break;

    case 'G':
      d_advance (di, 1);
      ret = d_make_comp (di, DEMANGLE_COMPONENT_IMAGINARY,
			 cplus_demangle_type (di), NULL);
      break;

    case 'U':
      d_advance (di, 1);
      ret = d_source_name (di);
      ret = d_make_comp (di, DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL,
			 cplus_demangle_type (di), ret);
      break;

    case 'D':
      can_subst = 0;
      d_advance (di, 1);
      peek = d_next_char (di);
      switch (peek)
	{
	case 'T':
	case 't':
	  /* decltype (expression) */
	  ret = d_make_comp (di, DEMANGLE_COMPONENT_DECLTYPE,
			     d_expression (di), NULL);
	  if (ret && d_next_char (di) != 'E')
	    ret = NULL;
	  break;
	  
	case 'p':
	  /* Pack expansion.  */
	  ret = d_make_comp (di, DEMANGLE_COMPONENT_PACK_EXPANSION,
			     cplus_demangle_type (di), NULL);
	  break;
	  
	case 'f':
	  /* 32-bit decimal floating point */
	  ret = d_make_builtin_type (di, &cplus_demangle_builtin_types[26]);
	  di->expansion += ret->u.s_builtin.type->len;
	  break;
	case 'd':
	  /* 64-bit DFP */
	  ret = d_make_builtin_type (di, &cplus_demangle_builtin_types[27]);
	  di->expansion += ret->u.s_builtin.type->len;
	  break;
	case 'e':
	  /* 128-bit DFP */
	  ret = d_make_builtin_type (di, &cplus_demangle_builtin_types[28]);
	  di->expansion += ret->u.s_builtin.type->len;
	  break;
	case 'h':
	  /* 16-bit half-precision FP */
	  ret = d_make_builtin_type (di, &cplus_demangle_builtin_types[29]);
	  di->expansion += ret->u.s_builtin.type->len;
	  break;
	case 's':
	  /* char16_t */
	  ret = d_make_builtin_type (di, &cplus_demangle_builtin_types[30]);
	  di->expansion += ret->u.s_builtin.type->len;
	  break;
	case 'i':
	  /* char32_t */
	  ret = d_make_builtin_type (di, &cplus_demangle_builtin_types[31]);
	  di->expansion += ret->u.s_builtin.type->len;
	  break;

	case 'F':
	  /* Fixed point types. DF<int bits><length><fract bits><sat>  */
	  ret = d_make_empty (di);
	  ret->type = DEMANGLE_COMPONENT_FIXED_TYPE;
	  if ((ret->u.s_fixed.accum = IS_DIGIT (d_peek_char (di))))
	    /* For demangling we don't care about the bits.  */
	    d_number (di);
	  ret->u.s_fixed.length = cplus_demangle_type (di);
	  d_number (di);
	  peek = d_next_char (di);
	  ret->u.s_fixed.sat = (peek == 's');
	  break;

	default:
	  return NULL;
	}
      break;

    default:
      return NULL;
    }

  if (can_subst)
    {
      if (! d_add_substitution (di, ret))
	return NULL;
    }

  return ret;
}

/* <CV-qualifiers> ::= [r] [V] [K]  */

static struct demangle_component **
d_cv_qualifiers (struct d_info *di,
                 struct demangle_component **pret, int member_fn)
{
  char peek;

  peek = d_peek_char (di);
  while (peek == 'r' || peek == 'V' || peek == 'K')
    {
      enum demangle_component_type t;

      d_advance (di, 1);
      if (peek == 'r')
	{
	  t = (member_fn
	       ? DEMANGLE_COMPONENT_RESTRICT_THIS
	       : DEMANGLE_COMPONENT_RESTRICT);
	  di->expansion += sizeof "restrict";
	}
      else if (peek == 'V')
	{
	  t = (member_fn
	       ? DEMANGLE_COMPONENT_VOLATILE_THIS
	       : DEMANGLE_COMPONENT_VOLATILE);
	  di->expansion += sizeof "volatile";
	}
      else
	{
	  t = (member_fn
	       ? DEMANGLE_COMPONENT_CONST_THIS
	       : DEMANGLE_COMPONENT_CONST);
	  di->expansion += sizeof "const";
	}

      *pret = d_make_comp (di, t, NULL, NULL);
      if (*pret == NULL)
	return NULL;
      pret = &d_left (*pret);

      peek = d_peek_char (di);
    }

  return pret;
}

/* <function-type> ::= F [Y] <bare-function-type> E  */

static struct demangle_component *
d_function_type (struct d_info *di)
{
  struct demangle_component *ret;

  if (! d_check_char (di, 'F'))
    return NULL;
  if (d_peek_char (di) == 'Y')
    {
      /* Function has C linkage.  We don't print this information.
	 FIXME: We should print it in verbose mode.  */
      d_advance (di, 1);
    }
  ret = d_bare_function_type (di, 1);
  if (! d_check_char (di, 'E'))
    return NULL;
  return ret;
}

/* <bare-function-type> ::= [J]<type>+  */

static struct demangle_component *
d_bare_function_type (struct d_info *di, int has_return_type)
{
  struct demangle_component *return_type;
  struct demangle_component *tl;
  struct demangle_component **ptl;
  char peek;

  /* Detect special qualifier indicating that the first argument
     is the return type.  */
  peek = d_peek_char (di);
  if (peek == 'J')
    {
      d_advance (di, 1);
      has_return_type = 1;
    }

  return_type = NULL;
  tl = NULL;
  ptl = &tl;
  while (1)
    {
      struct demangle_component *type;

      peek = d_peek_char (di);
      if (peek == '\0' || peek == 'E')
	break;
      type = cplus_demangle_type (di);
      if (type == NULL)
	return NULL;
      if (has_return_type)
	{
	  return_type = type;
	  has_return_type = 0;
	}
      else
	{
	  *ptl = d_make_comp (di, DEMANGLE_COMPONENT_ARGLIST, type, NULL);
	  if (*ptl == NULL)
	    return NULL;
	  ptl = &d_right (*ptl);
	}
    }

  /* There should be at least one parameter type besides the optional
     return type.  A function which takes no arguments will have a
     single parameter type void.  */
  if (tl == NULL)
    return NULL;

  /* If we have a single parameter type void, omit it.  */
  if (d_right (tl) == NULL
      && d_left (tl)->type == DEMANGLE_COMPONENT_BUILTIN_TYPE
      && d_left (tl)->u.s_builtin.type->print == D_PRINT_VOID)
    {
      di->expansion -= d_left (tl)->u.s_builtin.type->len;
      tl = NULL;
    }

  return d_make_comp (di, DEMANGLE_COMPONENT_FUNCTION_TYPE, return_type, tl);
}

/* <class-enum-type> ::= <name>  */

static struct demangle_component *
d_class_enum_type (struct d_info *di)
{
  return d_name (di);
}

/* <array-type> ::= A <(positive dimension) number> _ <(element) type>
                ::= A [<(dimension) expression>] _ <(element) type>
*/

static struct demangle_component *
d_array_type (struct d_info *di)
{
  char peek;
  struct demangle_component *dim;

  if (! d_check_char (di, 'A'))
    return NULL;

  peek = d_peek_char (di);
  if (peek == '_')
    dim = NULL;
  else if (IS_DIGIT (peek))
    {
      const char *s;

      s = d_str (di);
      do
	{
	  d_advance (di, 1);
	  peek = d_peek_char (di);
	}
      while (IS_DIGIT (peek));
      dim = d_make_name (di, s, d_str (di) - s);
      if (dim == NULL)
	return NULL;
    }
  else
    {
      dim = d_expression (di);
      if (dim == NULL)
	return NULL;
    }

  if (! d_check_char (di, '_'))
    return NULL;

  return d_make_comp (di, DEMANGLE_COMPONENT_ARRAY_TYPE, dim,
		      cplus_demangle_type (di));
}

/* <pointer-to-member-type> ::= M <(class) type> <(member) type>  */

static struct demangle_component *
d_pointer_to_member_type (struct d_info *di)
{
  struct demangle_component *cl;
  struct demangle_component *mem;
  struct demangle_component **pmem;

  if (! d_check_char (di, 'M'))
    return NULL;

  cl = cplus_demangle_type (di);

  /* The ABI specifies that any type can be a substitution source, and
     that M is followed by two types, and that when a CV-qualified
     type is seen both the base type and the CV-qualified types are
     substitution sources.  The ABI also specifies that for a pointer
     to a CV-qualified member function, the qualifiers are attached to
     the second type.  Given the grammar, a plain reading of the ABI
     suggests that both the CV-qualified member function and the
     non-qualified member function are substitution sources.  However,
     g++ does not work that way.  g++ treats only the CV-qualified
     member function as a substitution source.  FIXME.  So to work
     with g++, we need to pull off the CV-qualifiers here, in order to
     avoid calling add_substitution() in cplus_demangle_type().  But
     for a CV-qualified member which is not a function, g++ does
     follow the ABI, so we need to handle that case here by calling
     d_add_substitution ourselves.  */

  pmem = d_cv_qualifiers (di, &mem, 1);
  if (pmem == NULL)
    return NULL;
  *pmem = cplus_demangle_type (di);
  if (*pmem == NULL)
    return NULL;

  if (pmem != &mem && (*pmem)->type != DEMANGLE_COMPONENT_FUNCTION_TYPE)
    {
      if (! d_add_substitution (di, mem))
	return NULL;
    }

  return d_make_comp (di, DEMANGLE_COMPONENT_PTRMEM_TYPE, cl, mem);
}

/* <template-param> ::= T_
                    ::= T <(parameter-2 non-negative) number> _
*/

static struct demangle_component *
d_template_param (struct d_info *di)
{
  long param;

  if (! d_check_char (di, 'T'))
    return NULL;

  if (d_peek_char (di) == '_')
    param = 0;
  else
    {
      param = d_number (di);
      if (param < 0)
	return NULL;
      param += 1;
    }

  if (! d_check_char (di, '_'))
    return NULL;

  ++di->did_subs;

  return d_make_template_param (di, param);
}

/* <template-args> ::= I <template-arg>+ E  */

static struct demangle_component *
d_template_args (struct d_info *di)
{
  struct demangle_component *hold_last_name;
  struct demangle_component *al;
  struct demangle_component **pal;

  /* Preserve the last name we saw--don't let the template arguments
     clobber it, as that would give us the wrong name for a subsequent
     constructor or destructor.  */
  hold_last_name = di->last_name;

  if (! d_check_char (di, 'I'))
    return NULL;

  if (d_peek_char (di) == 'E')
    {
      /* An argument pack can be empty.  */
      d_advance (di, 1);
      return d_make_comp (di, DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, NULL, NULL);
    }

  al = NULL;
  pal = &al;
  while (1)
    {
      struct demangle_component *a;

      a = d_template_arg (di);
      if (a == NULL)
	return NULL;

      *pal = d_make_comp (di, DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, a, NULL);
      if (*pal == NULL)
	return NULL;
      pal = &d_right (*pal);

      if (d_peek_char (di) == 'E')
	{
	  d_advance (di, 1);
	  break;
	}
    }

  di->last_name = hold_last_name;

  return al;
}

/* <template-arg> ::= <type>
                  ::= X <expression> E
                  ::= <expr-primary>
*/

static struct demangle_component *
d_template_arg (struct d_info *di)
{
  struct demangle_component *ret;

  switch (d_peek_char (di))
    {
    case 'X':
      d_advance (di, 1);
      ret = d_expression (di);
      if (! d_check_char (di, 'E'))
	return NULL;
      return ret;

    case 'L':
      return d_expr_primary (di);

    case 'I':
      /* An argument pack.  */
      return d_template_args (di);

    default:
      return cplus_demangle_type (di);
    }
}

/* Subroutine of <expression> ::= cl <expression>+ E */

static struct demangle_component *
d_exprlist (struct d_info *di)
{
  struct demangle_component *list = NULL;
  struct demangle_component **p = &list;

  if (d_peek_char (di) == 'E')
    {
      d_advance (di, 1);
      return d_make_comp (di, DEMANGLE_COMPONENT_ARGLIST, NULL, NULL);
    }

  while (1)
    {
      struct demangle_component *arg = d_expression (di);
      if (arg == NULL)
	return NULL;

      *p = d_make_comp (di, DEMANGLE_COMPONENT_ARGLIST, arg, NULL);
      if (*p == NULL)
	return NULL;
      p = &d_right (*p);

      if (d_peek_char (di) == 'E')
	{
	  d_advance (di, 1);
	  break;
	}
    }

  return list;
}

/* <expression> ::= <(unary) operator-name> <expression>
                ::= <(binary) operator-name> <expression> <expression>
                ::= <(trinary) operator-name> <expression> <expression> <expression>
		::= cl <expression>+ E
                ::= st <type>
                ::= <template-param>
                ::= sr <type> <unqualified-name>
                ::= sr <type> <unqualified-name> <template-args>
                ::= <expr-primary>
*/

static struct demangle_component *
d_expression (struct d_info *di)
{
  char peek;

  peek = d_peek_char (di);
  if (peek == 'L')
    return d_expr_primary (di);
  else if (peek == 'T')
    return d_template_param (di);
  else if (peek == 's' && d_peek_next_char (di) == 'r')
    {
      struct demangle_component *type;
      struct demangle_component *name;

      d_advance (di, 2);
      type = cplus_demangle_type (di);
      name = d_unqualified_name (di);
      if (d_peek_char (di) != 'I')
	return d_make_comp (di, DEMANGLE_COMPONENT_QUAL_NAME, type, name);
      else
	return d_make_comp (di, DEMANGLE_COMPONENT_QUAL_NAME, type,
			    d_make_comp (di, DEMANGLE_COMPONENT_TEMPLATE, name,
					 d_template_args (di)));
    }
  else if (peek == 's' && d_peek_next_char (di) == 'p')
    {
      d_advance (di, 2);
      return d_make_comp (di, DEMANGLE_COMPONENT_PACK_EXPANSION,
			  d_expression (di), NULL);
    }
  else if (peek == 'f' && d_peek_next_char (di) == 'p')
    {
      /* Function parameter used in a late-specified return type.  */
      int index;
      d_advance (di, 2);
      if (d_peek_char (di) == '_')
	index = 1;
      else
	{
	  index = d_number (di);
	  if (index < 0)
	    return NULL;
	  index += 2;
	}

      if (! d_check_char (di, '_'))
	return NULL;

      return d_make_function_param (di, index);
    }
  else if (IS_DIGIT (peek))
    {
      /* We can get an unqualified name as an expression in the case of
         a dependent member access, i.e. decltype(T().i).  */
      struct demangle_component *name = d_unqualified_name (di);
      if (name == NULL)
	return NULL;
      if (d_peek_char (di) == 'I')
	return d_make_comp (di, DEMANGLE_COMPONENT_TEMPLATE, name,
			    d_template_args (di));
      else
	return name;
    }
  else
    {
      struct demangle_component *op;
      int args;

      op = d_operator_name (di);
      if (op == NULL)
	return NULL;

      if (op->type == DEMANGLE_COMPONENT_OPERATOR)
	di->expansion += op->u.s_operator.op->len - 2;

      if (op->type == DEMANGLE_COMPONENT_OPERATOR
	  && strcmp (op->u.s_operator.op->code, "st") == 0)
	return d_make_comp (di, DEMANGLE_COMPONENT_UNARY, op,
			    cplus_demangle_type (di));

      switch (op->type)
	{
	default:
	  return NULL;
	case DEMANGLE_COMPONENT_OPERATOR:
	  args = op->u.s_operator.op->args;
	  break;
	case DEMANGLE_COMPONENT_EXTENDED_OPERATOR:
	  args = op->u.s_extended_operator.args;
	  break;
	case DEMANGLE_COMPONENT_CAST:
	  args = 1;
	  break;
	}

      switch (args)
	{
	case 1:
	  {
	    struct demangle_component *operand;
	    if (op->type == DEMANGLE_COMPONENT_CAST
		&& d_check_char (di, '_'))
	      operand = d_exprlist (di);
	    else
	      operand = d_expression (di);
	    return d_make_comp (di, DEMANGLE_COMPONENT_UNARY, op,
				operand);
	  }
	case 2:
	  {
	    struct demangle_component *left;
	    struct demangle_component *right;

	    left = d_expression (di);
	    if (!strcmp (op->u.s_operator.op->code, "cl"))
	      right = d_exprlist (di);
	    else
	      right = d_expression (di);

	    return d_make_comp (di, DEMANGLE_COMPONENT_BINARY, op,
				d_make_comp (di,
					     DEMANGLE_COMPONENT_BINARY_ARGS,
					     left, right));
	  }
	case 3:
	  {
	    struct demangle_component *first;
	    struct demangle_component *second;

	    first = d_expression (di);
	    second = d_expression (di);
	    return d_make_comp (di, DEMANGLE_COMPONENT_TRINARY, op,
				d_make_comp (di,
					     DEMANGLE_COMPONENT_TRINARY_ARG1,
					     first,
					     d_make_comp (di,
							  DEMANGLE_COMPONENT_TRINARY_ARG2,
							  second,
							  d_expression (di))));
	  }
	default:
	  return NULL;
	}
    }
}

/* <expr-primary> ::= L <type> <(value) number> E
                  ::= L <type> <(value) float> E
                  ::= L <mangled-name> E
*/

static struct demangle_component *
d_expr_primary (struct d_info *di)
{
  struct demangle_component *ret;

  if (! d_check_char (di, 'L'))
    return NULL;
  if (d_peek_char (di) == '_'
      /* Workaround for G++ bug; see comment in write_template_arg.  */
      || d_peek_char (di) == 'Z')
    ret = cplus_demangle_mangled_name (di, 0);
  else
    {
      struct demangle_component *type;
      enum demangle_component_type t;
      const char *s;

      type = cplus_demangle_type (di);
      if (type == NULL)
	return NULL;

      /* If we have a type we know how to print, we aren't going to
	 print the type name itself.  */
      if (type->type == DEMANGLE_COMPONENT_BUILTIN_TYPE
	  && type->u.s_builtin.type->print != D_PRINT_DEFAULT)
	di->expansion -= type->u.s_builtin.type->len;

      /* Rather than try to interpret the literal value, we just
	 collect it as a string.  Note that it's possible to have a
	 floating point literal here.  The ABI specifies that the
	 format of such literals is machine independent.  That's fine,
	 but what's not fine is that versions of g++ up to 3.2 with
	 -fabi-version=1 used upper case letters in the hex constant,
	 and dumped out gcc's internal representation.  That makes it
	 hard to tell where the constant ends, and hard to dump the
	 constant in any readable form anyhow.  We don't attempt to
	 handle these cases.  */

      t = DEMANGLE_COMPONENT_LITERAL;
      if (d_peek_char (di) == 'n')
	{
	  t = DEMANGLE_COMPONENT_LITERAL_NEG;
	  d_advance (di, 1);
	}
      s = d_str (di);
      while (d_peek_char (di) != 'E')
	{
	  if (d_peek_char (di) == '\0')
	    return NULL;
	  d_advance (di, 1);
	}
      ret = d_make_comp (di, t, type, d_make_name (di, s, d_str (di) - s));
    }
  if (! d_check_char (di, 'E'))
    return NULL;
  return ret;
}

/* <local-name> ::= Z <(function) encoding> E <(entity) name> [<discriminator>]
                ::= Z <(function) encoding> E s [<discriminator>]
*/

static struct demangle_component *
d_local_name (struct d_info *di)
{
  struct demangle_component *function;

  if (! d_check_char (di, 'Z'))
    return NULL;

  function = d_encoding (di, 0);

  if (! d_check_char (di, 'E'))
    return NULL;

  if (d_peek_char (di) == 's')
    {
      d_advance (di, 1);
      if (! d_discriminator (di))
	return NULL;
      return d_make_comp (di, DEMANGLE_COMPONENT_LOCAL_NAME, function,
			  d_make_name (di, "string literal",
				       sizeof "string literal" - 1));
    }
  else
    {
      struct demangle_component *name;

      name = d_name (di);
      if (! d_discriminator (di))
	return NULL;
      return d_make_comp (di, DEMANGLE_COMPONENT_LOCAL_NAME, function, name);
    }
}

/* <discriminator> ::= _ <(non-negative) number>

   We demangle the discriminator, but we don't print it out.  FIXME:
   We should print it out in verbose mode.  */

static int
d_discriminator (struct d_info *di)
{
  long discrim;

  if (d_peek_char (di) != '_')
    return 1;
  d_advance (di, 1);
  discrim = d_number (di);
  if (discrim < 0)
    return 0;
  return 1;
}

/* Add a new substitution.  */

static int
d_add_substitution (struct d_info *di, struct demangle_component *dc)
{
  if (dc == NULL)
    return 0;
  if (di->next_sub >= di->num_subs)
    return 0;
  di->subs[di->next_sub] = dc;
  ++di->next_sub;
  return 1;
}

/* <substitution> ::= S <seq-id> _
                  ::= S_
                  ::= St
                  ::= Sa
                  ::= Sb
                  ::= Ss
                  ::= Si
                  ::= So
                  ::= Sd

   If PREFIX is non-zero, then this type is being used as a prefix in
   a qualified name.  In this case, for the standard substitutions, we
   need to check whether we are being used as a prefix for a
   constructor or destructor, and return a full template name.
   Otherwise we will get something like std::iostream::~iostream()
   which does not correspond particularly well to any function which
   actually appears in the source.
*/

static const struct d_standard_sub_info standard_subs[] =
{
  { 't', NL ("std"),
    NL ("std"),
    NULL, 0 },
  { 'a', NL ("std::allocator"),
    NL ("std::allocator"),
    NL ("allocator") },
  { 'b', NL ("std::basic_string"),
    NL ("std::basic_string"),
    NL ("basic_string") },
  { 's', NL ("std::string"),
    NL ("std::basic_string<char, std::char_traits<char>, std::allocator<char> >"),
    NL ("basic_string") },
  { 'i', NL ("std::istream"),
    NL ("std::basic_istream<char, std::char_traits<char> >"),
    NL ("basic_istream") },
  { 'o', NL ("std::ostream"),
    NL ("std::basic_ostream<char, std::char_traits<char> >"),
    NL ("basic_ostream") },
  { 'd', NL ("std::iostream"),
    NL ("std::basic_iostream<char, std::char_traits<char> >"),
    NL ("basic_iostream") }
};

static struct demangle_component *
d_substitution (struct d_info *di, int prefix)
{
  char c;

  if (! d_check_char (di, 'S'))
    return NULL;

  c = d_next_char (di);
  if (c == '_' || IS_DIGIT (c) || IS_UPPER (c))
    {
      unsigned int id;

      id = 0;
      if (c != '_')
	{
	  do
	    {
	      unsigned int new_id;

	      if (IS_DIGIT (c))
		new_id = id * 36 + c - '0';
	      else if (IS_UPPER (c))
		new_id = id * 36 + c - 'A' + 10;
	      else
		return NULL;
	      if (new_id < id)
		return NULL;
	      id = new_id;
	      c = d_next_char (di);
	    }
	  while (c != '_');

	  ++id;
	}

      if (id >= (unsigned int) di->next_sub)
	return NULL;

      ++di->did_subs;

      return di->subs[id];
    }
  else
    {
      int verbose;
      const struct d_standard_sub_info *p;
      const struct d_standard_sub_info *pend;

      verbose = (di->options & DMGL_VERBOSE) != 0;
      if (! verbose && prefix)
	{
	  char peek;

	  peek = d_peek_char (di);
	  if (peek == 'C' || peek == 'D')
	    verbose = 1;
	}

      pend = (&standard_subs[0]
	      + sizeof standard_subs / sizeof standard_subs[0]);
      for (p = &standard_subs[0]; p < pend; ++p)
	{
	  if (c == p->code)
	    {
	      const char *s;
	      int len;

	      if (p->set_last_name != NULL)
		di->last_name = d_make_sub (di, p->set_last_name,
					    p->set_last_name_len);
	      if (verbose)
		{
		  s = p->full_expansion;
		  len = p->full_len;
		}
	      else
		{
		  s = p->simple_expansion;
		  len = p->simple_len;
		}
	      di->expansion += len;
	      return d_make_sub (di, s, len);
	    }
	}

      return NULL;
    }
}

/* Initialize a growable string.  */

static void
d_growable_string_init (struct d_growable_string *dgs, size_t estimate)
{
  dgs->buf = NULL;
  dgs->len = 0;
  dgs->alc = 0;
  dgs->allocation_failure = 0;

  if (estimate > 0)
    d_growable_string_resize (dgs, estimate);
}

/* Grow a growable string to a given size.  */

static inline void
d_growable_string_resize (struct d_growable_string *dgs, size_t need)
{
  size_t newalc;
  char *newbuf;

  if (dgs->allocation_failure)
    return;

  /* Start allocation at two bytes to avoid any possibility of confusion
     with the special value of 1 used as a return in *palc to indicate
     allocation failures.  */
  newalc = dgs->alc > 0 ? dgs->alc : 2;
  while (newalc < need)
    newalc <<= 1;

  newbuf = (char *) realloc (dgs->buf, newalc);
  if (newbuf == NULL)
    {
      free (dgs->buf);
      dgs->buf = NULL;
      dgs->len = 0;
      dgs->alc = 0;
      dgs->allocation_failure = 1;
      return;
    }
  dgs->buf = newbuf;
  dgs->alc = newalc;
}

/* Append a buffer to a growable string.  */

static inline void
d_growable_string_append_buffer (struct d_growable_string *dgs,
                                 const char *s, size_t l)
{
  size_t need;

  need = dgs->len + l + 1;
  if (need > dgs->alc)
    d_growable_string_resize (dgs, need);

  if (dgs->allocation_failure)
    return;

  memcpy (dgs->buf + dgs->len, s, l);
  dgs->buf[dgs->len + l] = '\0';
  dgs->len += l;
}

/* Bridge growable strings to the callback mechanism.  */

static void
d_growable_string_callback_adapter (const char *s, size_t l, void *opaque)
{
  struct d_growable_string *dgs = (struct d_growable_string*) opaque;

  d_growable_string_append_buffer (dgs, s, l);
}

/* Initialize a print information structure.  */

static void
d_print_init (struct d_print_info *dpi, int options,
              demangle_callbackref callback, void *opaque)
{
  dpi->options = options;
  dpi->len = 0;
  dpi->last_char = '\0';
  dpi->templates = NULL;
  dpi->modifiers = NULL;

  dpi->callback = callback;
  dpi->opaque = opaque;

  dpi->demangle_failure = 0;
}

/* Indicate that an error occurred during printing, and test for error.  */

static inline void
d_print_error (struct d_print_info *dpi)
{
  dpi->demangle_failure = 1;
}

static inline int
d_print_saw_error (struct d_print_info *dpi)
{
  return dpi->demangle_failure != 0;
}

/* Flush buffered characters to the callback.  */

static inline void
d_print_flush (struct d_print_info *dpi)
{
  dpi->buf[dpi->len] = '\0';
  dpi->callback (dpi->buf, dpi->len, dpi->opaque);
  dpi->len = 0;
}

/* Append characters and buffers for printing.  */

static inline void
d_append_char (struct d_print_info *dpi, char c)
{
  if (dpi->len == sizeof (dpi->buf) - 1)
    d_print_flush (dpi);

  dpi->buf[dpi->len++] = c;
  dpi->last_char = c;
}

static inline void
d_append_buffer (struct d_print_info *dpi, const char *s, size_t l)
{
  size_t i;

  for (i = 0; i < l; i++)
    d_append_char (dpi, s[i]);
}

static inline void
d_append_string (struct d_print_info *dpi, const char *s)
{
  d_append_buffer (dpi, s, strlen (s));
}

static inline char
d_last_char (struct d_print_info *dpi)
{
  return dpi->last_char;
}

/* Turn components into a human readable string.  OPTIONS is the
   options bits passed to the demangler.  DC is the tree to print.
   CALLBACK is a function to call to flush demangled string segments
   as they fill the intermediate buffer, and OPAQUE is a generalized
   callback argument.  On success, this returns 1.  On failure,
   it returns 0, indicating a bad parse.  It does not use heap
   memory to build an output string, so cannot encounter memory
   allocation failure.  */

CP_STATIC_IF_GLIBCPP_V3
int
cplus_demangle_print_callback (int options,
                               const struct demangle_component *dc,
                               demangle_callbackref callback, void *opaque)
{
  struct d_print_info dpi;

  d_print_init (&dpi, options, callback, opaque);

  d_print_comp (&dpi, dc);

  d_print_flush (&dpi);

  return ! d_print_saw_error (&dpi);
}

/* Turn components into a human readable string.  OPTIONS is the
   options bits passed to the demangler.  DC is the tree to print.
   ESTIMATE is a guess at the length of the result.  This returns a
   string allocated by malloc, or NULL on error.  On success, this
   sets *PALC to the size of the allocated buffer.  On failure, this
   sets *PALC to 0 for a bad parse, or to 1 for a memory allocation
   failure.  */

CP_STATIC_IF_GLIBCPP_V3
char *
cplus_demangle_print (int options, const struct demangle_component *dc,
                      int estimate, size_t *palc)
{
  struct d_growable_string dgs;

  d_growable_string_init (&dgs, estimate);

  if (! cplus_demangle_print_callback (options, dc,
                                       d_growable_string_callback_adapter,
                                       &dgs))
    {
      free (dgs.buf);
      *palc = 0;
      return NULL;
    }

  *palc = dgs.allocation_failure ? 1 : dgs.alc;
  return dgs.buf;
}

/* Returns the I'th element of the template arglist ARGS, or NULL on
   failure.  */

static struct demangle_component *
d_index_template_argument (struct demangle_component *args, int i)
{
  struct demangle_component *a;

  for (a = args;
       a != NULL;
       a = d_right (a))
    {
      if (a->type != DEMANGLE_COMPONENT_TEMPLATE_ARGLIST)
	return NULL;
      if (i <= 0)
	break;
      --i;
    }
  if (i != 0 || a == NULL)
    return NULL;

  return d_left (a);
}

/* Returns the template argument from the current context indicated by DC,
   which is a DEMANGLE_COMPONENT_TEMPLATE_PARAM, or NULL.  */

static struct demangle_component *
d_lookup_template_argument (struct d_print_info *dpi,
			    const struct demangle_component *dc)
{
  if (dpi->templates == NULL)
    {
      d_print_error (dpi);
      return NULL;
    }
	
  return d_index_template_argument
    (d_right (dpi->templates->template_decl),
     dc->u.s_number.number);
}

/* Returns a template argument pack used in DC (any will do), or NULL.  */

static struct demangle_component *
d_find_pack (struct d_print_info *dpi,
	     const struct demangle_component *dc)
{
  struct demangle_component *a;
  if (dc == NULL)
    return NULL;

  switch (dc->type)
    {
    case DEMANGLE_COMPONENT_TEMPLATE_PARAM:
      a = d_lookup_template_argument (dpi, dc);
      if (a && a->type == DEMANGLE_COMPONENT_TEMPLATE_ARGLIST)
	return a;
      return NULL;

    case DEMANGLE_COMPONENT_PACK_EXPANSION:
      return NULL;
      
    case DEMANGLE_COMPONENT_NAME:
    case DEMANGLE_COMPONENT_OPERATOR:
    case DEMANGLE_COMPONENT_BUILTIN_TYPE:
    case DEMANGLE_COMPONENT_SUB_STD:
    case DEMANGLE_COMPONENT_CHARACTER:
    case DEMANGLE_COMPONENT_FUNCTION_PARAM:
      return NULL;

    case DEMANGLE_COMPONENT_EXTENDED_OPERATOR:
      return d_find_pack (dpi, dc->u.s_extended_operator.name);
    case DEMANGLE_COMPONENT_CTOR:
      return d_find_pack (dpi, dc->u.s_ctor.name);
    case DEMANGLE_COMPONENT_DTOR:
      return d_find_pack (dpi, dc->u.s_dtor.name);

    default:
      a = d_find_pack (dpi, d_left (dc));
      if (a)
	return a;
      return d_find_pack (dpi, d_right (dc));
    }
}

/* Returns the length of the template argument pack DC.  */

static int
d_pack_length (const struct demangle_component *dc)
{
  int count = 0;
  while (dc && dc->type == DEMANGLE_COMPONENT_TEMPLATE_ARGLIST
	 && d_left (dc) != NULL)
    {
      ++count;
      dc = d_right (dc);
    }
  return count;
}

/* DC is a component of a mangled expression.  Print it, wrapped in parens
   if needed.  */

static void
d_print_subexpr (struct d_print_info *dpi,
		 const struct demangle_component *dc)
{
  int simple = 0;
  if (dc->type == DEMANGLE_COMPONENT_NAME
      || dc->type == DEMANGLE_COMPONENT_FUNCTION_PARAM)
    simple = 1;
  if (!simple)
    d_append_char (dpi, '(');
  d_print_comp (dpi, dc);
  if (!simple)
    d_append_char (dpi, ')');
}

/* Subroutine to handle components.  */

static void
d_print_comp (struct d_print_info *dpi,
              const struct demangle_component *dc)
{
  if (dc == NULL)
    {
      d_print_error (dpi);
      return;
    }
  if (d_print_saw_error (dpi))
    return;

  switch (dc->type)
    {
    case DEMANGLE_COMPONENT_NAME:
      if ((dpi->options & DMGL_JAVA) == 0)
	d_append_buffer (dpi, dc->u.s_name.s, dc->u.s_name.len);
      else
	d_print_java_identifier (dpi, dc->u.s_name.s, dc->u.s_name.len);
      return;

    case DEMANGLE_COMPONENT_QUAL_NAME:
    case DEMANGLE_COMPONENT_LOCAL_NAME:
      d_print_comp (dpi, d_left (dc));
      if ((dpi->options & DMGL_JAVA) == 0)
	d_append_string (dpi, "::");
      else
	d_append_char (dpi, '.');
      d_print_comp (dpi, d_right (dc));
      return;

    case DEMANGLE_COMPONENT_TYPED_NAME:
      {
	struct d_print_mod *hold_modifiers;
	struct demangle_component *typed_name;
	struct d_print_mod adpm[4];
	unsigned int i;
	struct d_print_template dpt;

	/* Pass the name down to the type so that it can be printed in
	   the right place for the type.  We also have to pass down
	   any CV-qualifiers, which apply to the this parameter.  */
	hold_modifiers = dpi->modifiers;
	dpi->modifiers = 0;
	i = 0;
	typed_name = d_left (dc);
	while (typed_name != NULL)
	  {
	    if (i >= sizeof adpm / sizeof adpm[0])
	      {
		d_print_error (dpi);
		return;
	      }

	    adpm[i].next = dpi->modifiers;
	    dpi->modifiers = &adpm[i];
	    adpm[i].mod = typed_name;
	    adpm[i].printed = 0;
	    adpm[i].templates = dpi->templates;
	    ++i;

	    if (typed_name->type != DEMANGLE_COMPONENT_RESTRICT_THIS
		&& typed_name->type != DEMANGLE_COMPONENT_VOLATILE_THIS
		&& typed_name->type != DEMANGLE_COMPONENT_CONST_THIS)
	      break;

	    typed_name = d_left (typed_name);
	  }

	if (typed_name == NULL)
	  {
	    d_print_error (dpi);
	    return;
	  }

	/* If typed_name is a template, then it applies to the
	   function type as well.  */
	if (typed_name->type == DEMANGLE_COMPONENT_TEMPLATE)
	  {
	    dpt.next = dpi->templates;
	    dpi->templates = &dpt;
	    dpt.template_decl = typed_name;
	  }

	/* If typed_name is a DEMANGLE_COMPONENT_LOCAL_NAME, then
	   there may be CV-qualifiers on its right argument which
	   really apply here; this happens when parsing a class which
	   is local to a function.  */
	if (typed_name->type == DEMANGLE_COMPONENT_LOCAL_NAME)
	  {
	    struct demangle_component *local_name;

	    local_name = d_right (typed_name);
	    while (local_name->type == DEMANGLE_COMPONENT_RESTRICT_THIS
		   || local_name->type == DEMANGLE_COMPONENT_VOLATILE_THIS
		   || local_name->type == DEMANGLE_COMPONENT_CONST_THIS)
	      {
		if (i >= sizeof adpm / sizeof adpm[0])
		  {
		    d_print_error (dpi);
		    return;
		  }

		adpm[i] = adpm[i - 1];
		adpm[i].next = &adpm[i - 1];
		dpi->modifiers = &adpm[i];

		adpm[i - 1].mod = local_name;
		adpm[i - 1].printed = 0;
		adpm[i - 1].templates = dpi->templates;
		++i;

		local_name = d_left (local_name);
	      }
	  }

	d_print_comp (dpi, d_right (dc));

	if (typed_name->type == DEMANGLE_COMPONENT_TEMPLATE)
	  dpi->templates = dpt.next;

	/* If the modifiers didn't get printed by the type, print them
	   now.  */
	while (i > 0)
	  {
	    --i;
	    if (! adpm[i].printed)
	      {
		d_append_char (dpi, ' ');
		d_print_mod (dpi, adpm[i].mod);
	      }
	  }

	dpi->modifiers = hold_modifiers;

	return;
      }

    case DEMANGLE_COMPONENT_TEMPLATE:
      {
	struct d_print_mod *hold_dpm;
	struct demangle_component *dcl;

	/* Don't push modifiers into a template definition.  Doing so
	   could give the wrong definition for a template argument.
	   Instead, treat the template essentially as a name.  */

	hold_dpm = dpi->modifiers;
	dpi->modifiers = NULL;

        dcl = d_left (dc);

        if ((dpi->options & DMGL_JAVA) != 0
            && dcl->type == DEMANGLE_COMPONENT_NAME
            && dcl->u.s_name.len == 6
            && strncmp (dcl->u.s_name.s, "JArray", 6) == 0)
          {
            /* Special-case Java arrays, so that JArray<TYPE> appears
               instead as TYPE[].  */

            d_print_comp (dpi, d_right (dc));
            d_append_string (dpi, "[]");
          }
        else
          {
	    d_print_comp (dpi, dcl);
	    if (d_last_char (dpi) == '<')
	      d_append_char (dpi, ' ');
	    d_append_char (dpi, '<');
	    d_print_comp (dpi, d_right (dc));
	    /* Avoid generating two consecutive '>' characters, to avoid
	       the C++ syntactic ambiguity.  */
	    if (d_last_char (dpi) == '>')
	      d_append_char (dpi, ' ');
	    d_append_char (dpi, '>');
          }

	dpi->modifiers = hold_dpm;

	return;
      }

    case DEMANGLE_COMPONENT_TEMPLATE_PARAM:
      {
	struct d_print_template *hold_dpt;
	struct demangle_component *a = d_lookup_template_argument (dpi, dc);

	if (a && a->type == DEMANGLE_COMPONENT_TEMPLATE_ARGLIST)
	  a = d_index_template_argument (a, dpi->pack_index);

	if (a == NULL)
	  {
	    d_print_error (dpi);
	    return;
	  }

	/* While processing this parameter, we need to pop the list of
	   templates.  This is because the template parameter may
	   itself be a reference to a parameter of an outer
	   template.  */

	hold_dpt = dpi->templates;
	dpi->templates = hold_dpt->next;

	d_print_comp (dpi, a);

	dpi->templates = hold_dpt;

	return;
      }

    case DEMANGLE_COMPONENT_CTOR:
      d_print_comp (dpi, dc->u.s_ctor.name);
      return;

    case DEMANGLE_COMPONENT_DTOR:
      d_append_char (dpi, '~');
      d_print_comp (dpi, dc->u.s_dtor.name);
      return;

    case DEMANGLE_COMPONENT_VTABLE:
      d_append_string (dpi, "vtable for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_VTT:
      d_append_string (dpi, "VTT for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE:
      d_append_string (dpi, "construction vtable for ");
      d_print_comp (dpi, d_left (dc));
      d_append_string (dpi, "-in-");
      d_print_comp (dpi, d_right (dc));
      return;

    case DEMANGLE_COMPONENT_TYPEINFO:
      d_append_string (dpi, "typeinfo for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_TYPEINFO_NAME:
      d_append_string (dpi, "typeinfo name for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_TYPEINFO_FN:
      d_append_string (dpi, "typeinfo fn for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_THUNK:
      d_append_string (dpi, "non-virtual thunk to ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_VIRTUAL_THUNK:
      d_append_string (dpi, "virtual thunk to ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_COVARIANT_THUNK:
      d_append_string (dpi, "covariant return thunk to ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_JAVA_CLASS:
      d_append_string (dpi, "java Class for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_GUARD:
      d_append_string (dpi, "guard variable for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_REFTEMP:
      d_append_string (dpi, "reference temporary for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_HIDDEN_ALIAS:
      d_append_string (dpi, "hidden alias for ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_SUB_STD:
      d_append_buffer (dpi, dc->u.s_string.string, dc->u.s_string.len);
      return;

    case DEMANGLE_COMPONENT_RESTRICT:
    case DEMANGLE_COMPONENT_VOLATILE:
    case DEMANGLE_COMPONENT_CONST:
      {
	struct d_print_mod *pdpm;

	/* When printing arrays, it's possible to have cases where the
	   same CV-qualifier gets pushed on the stack multiple times.
	   We only need to print it once.  */

	for (pdpm = dpi->modifiers; pdpm != NULL; pdpm = pdpm->next)
	  {
	    if (! pdpm->printed)
	      {
		if (pdpm->mod->type != DEMANGLE_COMPONENT_RESTRICT
		    && pdpm->mod->type != DEMANGLE_COMPONENT_VOLATILE
		    && pdpm->mod->type != DEMANGLE_COMPONENT_CONST)
		  break;
		if (pdpm->mod->type == dc->type)
		  {
		    d_print_comp (dpi, d_left (dc));
		    return;
		  }
	      }
	  }
      }
      /* Fall through.  */
    case DEMANGLE_COMPONENT_RESTRICT_THIS:
    case DEMANGLE_COMPONENT_VOLATILE_THIS:
    case DEMANGLE_COMPONENT_CONST_THIS:
    case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
    case DEMANGLE_COMPONENT_POINTER:
    case DEMANGLE_COMPONENT_REFERENCE:
    case DEMANGLE_COMPONENT_RVALUE_REFERENCE:
    case DEMANGLE_COMPONENT_COMPLEX:
    case DEMANGLE_COMPONENT_IMAGINARY:
      {
	/* We keep a list of modifiers on the stack.  */
	struct d_print_mod dpm;

	dpm.next = dpi->modifiers;
	dpi->modifiers = &dpm;
	dpm.mod = dc;
	dpm.printed = 0;
	dpm.templates = dpi->templates;

	d_print_comp (dpi, d_left (dc));

	/* If the modifier didn't get printed by the type, print it
	   now.  */
	if (! dpm.printed)
	  d_print_mod (dpi, dc);

	dpi->modifiers = dpm.next;

	return;
      }

    case DEMANGLE_COMPONENT_BUILTIN_TYPE:
      if ((dpi->options & DMGL_JAVA) == 0)
	d_append_buffer (dpi, dc->u.s_builtin.type->name,
			 dc->u.s_builtin.type->len);
      else
	d_append_buffer (dpi, dc->u.s_builtin.type->java_name,
			 dc->u.s_builtin.type->java_len);
      return;

    case DEMANGLE_COMPONENT_VENDOR_TYPE:
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_FUNCTION_TYPE:
      {
	if ((dpi->options & DMGL_RET_POSTFIX) != 0)
	  d_print_function_type (dpi, dc, dpi->modifiers);

	/* Print return type if present */
	if (d_left (dc) != NULL)
	  {
	    struct d_print_mod dpm;

	    /* We must pass this type down as a modifier in order to
	       print it in the right location.  */
	    dpm.next = dpi->modifiers;
	    dpi->modifiers = &dpm;
	    dpm.mod = dc;
	    dpm.printed = 0;
	    dpm.templates = dpi->templates;

	    d_print_comp (dpi, d_left (dc));

	    dpi->modifiers = dpm.next;

	    if (dpm.printed)
	      return;

	    /* In standard prefix notation, there is a space between the
	       return type and the function signature.  */
	    if ((dpi->options & DMGL_RET_POSTFIX) == 0)
	      d_append_char (dpi, ' ');
	  }

	if ((dpi->options & DMGL_RET_POSTFIX) == 0) 
	  d_print_function_type (dpi, dc, dpi->modifiers);

	return;
      }

    case DEMANGLE_COMPONENT_ARRAY_TYPE:
      {
	struct d_print_mod *hold_modifiers;
	struct d_print_mod adpm[4];
	unsigned int i;
	struct d_print_mod *pdpm;

	/* We must pass this type down as a modifier in order to print
	   multi-dimensional arrays correctly.  If the array itself is
	   CV-qualified, we act as though the element type were
	   CV-qualified.  We do this by copying the modifiers down
	   rather than fiddling pointers, so that we don't wind up
	   with a d_print_mod higher on the stack pointing into our
	   stack frame after we return.  */

	hold_modifiers = dpi->modifiers;

	adpm[0].next = hold_modifiers;
	dpi->modifiers = &adpm[0];
	adpm[0].mod = dc;
	adpm[0].printed = 0;
	adpm[0].templates = dpi->templates;

	i = 1;
	pdpm = hold_modifiers;
	while (pdpm != NULL
	       && (pdpm->mod->type == DEMANGLE_COMPONENT_RESTRICT
		   || pdpm->mod->type == DEMANGLE_COMPONENT_VOLATILE
		   || pdpm->mod->type == DEMANGLE_COMPONENT_CONST))
	  {
	    if (! pdpm->printed)
	      {
		if (i >= sizeof adpm / sizeof adpm[0])
		  {
		    d_print_error (dpi);
		    return;
		  }

		adpm[i] = *pdpm;
		adpm[i].next = dpi->modifiers;
		dpi->modifiers = &adpm[i];
		pdpm->printed = 1;
		++i;
	      }

	    pdpm = pdpm->next;
	  }

	d_print_comp (dpi, d_right (dc));

	dpi->modifiers = hold_modifiers;

	if (adpm[0].printed)
	  return;

	while (i > 1)
	  {
	    --i;
	    d_print_mod (dpi, adpm[i].mod);
	  }

	d_print_array_type (dpi, dc, dpi->modifiers);

	return;
      }

    case DEMANGLE_COMPONENT_PTRMEM_TYPE:
      {
	struct d_print_mod dpm;

	dpm.next = dpi->modifiers;
	dpi->modifiers = &dpm;
	dpm.mod = dc;
	dpm.printed = 0;
	dpm.templates = dpi->templates;

	d_print_comp (dpi, d_right (dc));

	/* If the modifier didn't get printed by the type, print it
	   now.  */
	if (! dpm.printed)
	  {
	    d_append_char (dpi, ' ');
	    d_print_comp (dpi, d_left (dc));
	    d_append_string (dpi, "::*");
	  }

	dpi->modifiers = dpm.next;

	return;
      }

    case DEMANGLE_COMPONENT_FIXED_TYPE:
      if (dc->u.s_fixed.sat)
	d_append_string (dpi, "_Sat ");
      /* Don't print "int _Accum".  */
      if (dc->u.s_fixed.length->u.s_builtin.type
	  != &cplus_demangle_builtin_types['i'-'a'])
	{
	  d_print_comp (dpi, dc->u.s_fixed.length);
	  d_append_char (dpi, ' ');
	}
      if (dc->u.s_fixed.accum)
	d_append_string (dpi, "_Accum");
      else
	d_append_string (dpi, "_Fract");
      return;

    case DEMANGLE_COMPONENT_ARGLIST:
    case DEMANGLE_COMPONENT_TEMPLATE_ARGLIST:
      if (d_left (dc) != NULL)
	d_print_comp (dpi, d_left (dc));
      if (d_right (dc) != NULL)
	{
	  size_t len;
	  d_append_string (dpi, ", ");
	  len = dpi->len;
	  d_print_comp (dpi, d_right (dc));
	  /* If that didn't print anything (which can happen with empty
	     template argument packs), remove the comma and space.  */
	  if (dpi->len == len)
	    dpi->len -= 2;
	}
      return;

    case DEMANGLE_COMPONENT_OPERATOR:
      {
	char c;

	d_append_string (dpi, "operator");
	c = dc->u.s_operator.op->name[0];
	if (IS_LOWER (c))
	  d_append_char (dpi, ' ');
	d_append_buffer (dpi, dc->u.s_operator.op->name,
			 dc->u.s_operator.op->len);
	return;
      }

    case DEMANGLE_COMPONENT_EXTENDED_OPERATOR:
      d_append_string (dpi, "operator ");
      d_print_comp (dpi, dc->u.s_extended_operator.name);
      return;

    case DEMANGLE_COMPONENT_CAST:
      d_append_string (dpi, "operator ");
      d_print_cast (dpi, dc);
      return;

    case DEMANGLE_COMPONENT_UNARY:
      if (d_left (dc)->type != DEMANGLE_COMPONENT_CAST)
	d_print_expr_op (dpi, d_left (dc));
      else
	{
	  d_append_char (dpi, '(');
	  d_print_cast (dpi, d_left (dc));
	  d_append_char (dpi, ')');
	}
      d_print_subexpr (dpi, d_right (dc));
      return;

    case DEMANGLE_COMPONENT_BINARY:
      if (d_right (dc)->type != DEMANGLE_COMPONENT_BINARY_ARGS)
	{
	  d_print_error (dpi);
	  return;
	}

      /* We wrap an expression which uses the greater-than operator in
	 an extra layer of parens so that it does not get confused
	 with the '>' which ends the template parameters.  */
      if (d_left (dc)->type == DEMANGLE_COMPONENT_OPERATOR
	  && d_left (dc)->u.s_operator.op->len == 1
	  && d_left (dc)->u.s_operator.op->name[0] == '>')
	d_append_char (dpi, '(');

      d_print_subexpr (dpi, d_left (d_right (dc)));
      if (strcmp (d_left (dc)->u.s_operator.op->code, "cl") != 0)
	d_print_expr_op (dpi, d_left (dc));
      d_print_subexpr (dpi, d_right (d_right (dc)));

      if (d_left (dc)->type == DEMANGLE_COMPONENT_OPERATOR
	  && d_left (dc)->u.s_operator.op->len == 1
	  && d_left (dc)->u.s_operator.op->name[0] == '>')
	d_append_char (dpi, ')');

      return;

    case DEMANGLE_COMPONENT_BINARY_ARGS:
      /* We should only see this as part of DEMANGLE_COMPONENT_BINARY.  */
      d_print_error (dpi);
      return;

    case DEMANGLE_COMPONENT_TRINARY:
      if (d_right (dc)->type != DEMANGLE_COMPONENT_TRINARY_ARG1
	  || d_right (d_right (dc))->type != DEMANGLE_COMPONENT_TRINARY_ARG2)
	{
	  d_print_error (dpi);
	  return;
	}
      d_print_subexpr (dpi, d_left (d_right (dc)));
      d_print_expr_op (dpi, d_left (dc));
      d_print_subexpr (dpi, d_left (d_right (d_right (dc))));
      d_append_string (dpi, " : ");
      d_print_subexpr (dpi, d_right (d_right (d_right (dc))));
      return;

    case DEMANGLE_COMPONENT_TRINARY_ARG1:
    case DEMANGLE_COMPONENT_TRINARY_ARG2:
      /* We should only see these are part of DEMANGLE_COMPONENT_TRINARY.  */
      d_print_error (dpi);
      return;

    case DEMANGLE_COMPONENT_LITERAL:
    case DEMANGLE_COMPONENT_LITERAL_NEG:
      {
	enum d_builtin_type_print tp;

	/* For some builtin types, produce simpler output.  */
	tp = D_PRINT_DEFAULT;
	if (d_left (dc)->type == DEMANGLE_COMPONENT_BUILTIN_TYPE)
	  {
	    tp = d_left (dc)->u.s_builtin.type->print;
	    switch (tp)
	      {
	      case D_PRINT_INT:
	      case D_PRINT_UNSIGNED:
	      case D_PRINT_LONG:
	      case D_PRINT_UNSIGNED_LONG:
	      case D_PRINT_LONG_LONG:
	      case D_PRINT_UNSIGNED_LONG_LONG:
		if (d_right (dc)->type == DEMANGLE_COMPONENT_NAME)
		  {
		    if (dc->type == DEMANGLE_COMPONENT_LITERAL_NEG)
		      d_append_char (dpi, '-');
		    d_print_comp (dpi, d_right (dc));
		    switch (tp)
		      {
		      default:
			break;
		      case D_PRINT_UNSIGNED:
			d_append_char (dpi, 'u');
			break;
		      case D_PRINT_LONG:
			d_append_char (dpi, 'l');
			break;
		      case D_PRINT_UNSIGNED_LONG:
			d_append_string (dpi, "ul");
			break;
		      case D_PRINT_LONG_LONG:
			d_append_string (dpi, "ll");
			break;
		      case D_PRINT_UNSIGNED_LONG_LONG:
			d_append_string (dpi, "ull");
			break;
		      }
		    return;
		  }
		break;

	      case D_PRINT_BOOL:
		if (d_right (dc)->type == DEMANGLE_COMPONENT_NAME
		    && d_right (dc)->u.s_name.len == 1
		    && dc->type == DEMANGLE_COMPONENT_LITERAL)
		  {
		    switch (d_right (dc)->u.s_name.s[0])
		      {
		      case '0':
			d_append_string (dpi, "false");
			return;
		      case '1':
			d_append_string (dpi, "true");
			return;
		      default:
			break;
		      }
		  }
		break;

	      default:
		break;
	      }
	  }

	d_append_char (dpi, '(');
	d_print_comp (dpi, d_left (dc));
	d_append_char (dpi, ')');
	if (dc->type == DEMANGLE_COMPONENT_LITERAL_NEG)
	  d_append_char (dpi, '-');
	if (tp == D_PRINT_FLOAT)
	  d_append_char (dpi, '[');
	d_print_comp (dpi, d_right (dc));
	if (tp == D_PRINT_FLOAT)
	  d_append_char (dpi, ']');
      }
      return;

    case DEMANGLE_COMPONENT_JAVA_RESOURCE:
      d_append_string (dpi, "java resource ");
      d_print_comp (dpi, d_left (dc));
      return;

    case DEMANGLE_COMPONENT_COMPOUND_NAME:
      d_print_comp (dpi, d_left (dc));
      d_print_comp (dpi, d_right (dc));
      return;

    case DEMANGLE_COMPONENT_CHARACTER:
      d_append_char (dpi, dc->u.s_character.character);
      return;

    case DEMANGLE_COMPONENT_DECLTYPE:
      d_append_string (dpi, "decltype (");
      d_print_comp (dpi, d_left (dc));
      d_append_char (dpi, ')');
      return;

    case DEMANGLE_COMPONENT_PACK_EXPANSION:
      {
	int len;
	int i;
	struct demangle_component *a = d_find_pack (dpi, d_left (dc));
	if (a == NULL)
	  {
	    /* d_find_pack won't find anything if the only packs involved
	       in this expansion are function parameter packs; in that
	       case, just print the pattern and "...".  */
	    d_print_subexpr (dpi, d_left (dc));
	    d_append_string (dpi, "...");
	    return;
	  }

	len = d_pack_length (a);
	dc = d_left (dc);
	for (i = 0; i < len; ++i)
	  {
	    dpi->pack_index = i;
	    d_print_comp (dpi, dc);
	    if (i < len-1)
	      d_append_string (dpi, ", ");
	  }
      }
      return;

    case DEMANGLE_COMPONENT_FUNCTION_PARAM:
      {
	char buf[25];
	d_append_string (dpi, "parm#");
	sprintf(buf,"%ld", dc->u.s_number.number);
	d_append_string (dpi, buf);
	return;
      }

    default:
      d_print_error (dpi);
      return;
    }
}

/* Print a Java dentifier.  For Java we try to handle encoded extended
   Unicode characters.  The C++ ABI doesn't mention Unicode encoding,
   so we don't it for C++.  Characters are encoded as
   __U<hex-char>+_.  */

static void
d_print_java_identifier (struct d_print_info *dpi, const char *name, int len)
{
  const char *p;
  const char *end;

  end = name + len;
  for (p = name; p < end; ++p)
    {
      if (end - p > 3
	  && p[0] == '_'
	  && p[1] == '_'
	  && p[2] == 'U')
	{
	  unsigned long c;
	  const char *q;

	  c = 0;
	  for (q = p + 3; q < end; ++q)
	    {
	      int dig;

	      if (IS_DIGIT (*q))
		dig = *q - '0';
	      else if (*q >= 'A' && *q <= 'F')
		dig = *q - 'A' + 10;
	      else if (*q >= 'a' && *q <= 'f')
		dig = *q - 'a' + 10;
	      else
		break;

	      c = c * 16 + dig;
	    }
	  /* If the Unicode character is larger than 256, we don't try
	     to deal with it here.  FIXME.  */
	  if (q < end && *q == '_' && c < 256)
	    {
	      d_append_char (dpi, c);
	      p = q;
	      continue;
	    }
	}

      d_append_char (dpi, *p);
    }
}

/* Print a list of modifiers.  SUFFIX is 1 if we are printing
   qualifiers on this after printing a function.  */

static void
d_print_mod_list (struct d_print_info *dpi,
                  struct d_print_mod *mods, int suffix)
{
  struct d_print_template *hold_dpt;

  if (mods == NULL || d_print_saw_error (dpi))
    return;

  if (mods->printed
      || (! suffix
	  && (mods->mod->type == DEMANGLE_COMPONENT_RESTRICT_THIS
	      || mods->mod->type == DEMANGLE_COMPONENT_VOLATILE_THIS
	      || mods->mod->type == DEMANGLE_COMPONENT_CONST_THIS)))
    {
      d_print_mod_list (dpi, mods->next, suffix);
      return;
    }

  mods->printed = 1;

  hold_dpt = dpi->templates;
  dpi->templates = mods->templates;

  if (mods->mod->type == DEMANGLE_COMPONENT_FUNCTION_TYPE)
    {
      d_print_function_type (dpi, mods->mod, mods->next);
      dpi->templates = hold_dpt;
      return;
    }
  else if (mods->mod->type == DEMANGLE_COMPONENT_ARRAY_TYPE)
    {
      d_print_array_type (dpi, mods->mod, mods->next);
      dpi->templates = hold_dpt;
      return;
    }
  else if (mods->mod->type == DEMANGLE_COMPONENT_LOCAL_NAME)
    {
      struct d_print_mod *hold_modifiers;
      struct demangle_component *dc;

      /* When this is on the modifier stack, we have pulled any
	 qualifiers off the right argument already.  Otherwise, we
	 print it as usual, but don't let the left argument see any
	 modifiers.  */

      hold_modifiers = dpi->modifiers;
      dpi->modifiers = NULL;
      d_print_comp (dpi, d_left (mods->mod));
      dpi->modifiers = hold_modifiers;

      if ((dpi->options & DMGL_JAVA) == 0)
	d_append_string (dpi, "::");
      else
	d_append_char (dpi, '.');

      dc = d_right (mods->mod);
      while (dc->type == DEMANGLE_COMPONENT_RESTRICT_THIS
	     || dc->type == DEMANGLE_COMPONENT_VOLATILE_THIS
	     || dc->type == DEMANGLE_COMPONENT_CONST_THIS)
	dc = d_left (dc);

      d_print_comp (dpi, dc);

      dpi->templates = hold_dpt;
      return;
    }

  d_print_mod (dpi, mods->mod);

  dpi->templates = hold_dpt;

  d_print_mod_list (dpi, mods->next, suffix);
}

/* Print a modifier.  */

static void
d_print_mod (struct d_print_info *dpi,
             const struct demangle_component *mod)
{
  switch (mod->type)
    {
    case DEMANGLE_COMPONENT_RESTRICT:
    case DEMANGLE_COMPONENT_RESTRICT_THIS:
      d_append_string (dpi, " restrict");
      return;
    case DEMANGLE_COMPONENT_VOLATILE:
    case DEMANGLE_COMPONENT_VOLATILE_THIS:
      d_append_string (dpi, " volatile");
      return;
    case DEMANGLE_COMPONENT_CONST:
    case DEMANGLE_COMPONENT_CONST_THIS:
      d_append_string (dpi, " const");
      return;
    case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
      d_append_char (dpi, ' ');
      d_print_comp (dpi, d_right (mod));
      return;
    case DEMANGLE_COMPONENT_POINTER:
      /* There is no pointer symbol in Java.  */
      if ((dpi->options & DMGL_JAVA) == 0)
	d_append_char (dpi, '*');
      return;
    case DEMANGLE_COMPONENT_REFERENCE:
      d_append_char (dpi, '&');
      return;
    case DEMANGLE_COMPONENT_RVALUE_REFERENCE:
      d_append_string (dpi, "&&");
      return;
    case DEMANGLE_COMPONENT_COMPLEX:
      d_append_string (dpi, "complex ");
      return;
    case DEMANGLE_COMPONENT_IMAGINARY:
      d_append_string (dpi, "imaginary ");
      return;
    case DEMANGLE_COMPONENT_PTRMEM_TYPE:
      if (d_last_char (dpi) != '(')
	d_append_char (dpi, ' ');
      d_print_comp (dpi, d_left (mod));
      d_append_string (dpi, "::*");
      return;
    case DEMANGLE_COMPONENT_TYPED_NAME:
      d_print_comp (dpi, d_left (mod));
      return;
    default:
      /* Otherwise, we have something that won't go back on the
	 modifier stack, so we can just print it.  */
      d_print_comp (dpi, mod);
      return;
    }
}

/* Print a function type, except for the return type.  */

static void
d_print_function_type (struct d_print_info *dpi,
                       const struct demangle_component *dc,
                       struct d_print_mod *mods)
{
  int need_paren;
  int saw_mod;
  int need_space;
  struct d_print_mod *p;
  struct d_print_mod *hold_modifiers;

  need_paren = 0;
  saw_mod = 0;
  need_space = 0;
  for (p = mods; p != NULL; p = p->next)
    {
      if (p->printed)
	break;

      saw_mod = 1;
      switch (p->mod->type)
	{
	case DEMANGLE_COMPONENT_POINTER:
	case DEMANGLE_COMPONENT_REFERENCE:
	case DEMANGLE_COMPONENT_RVALUE_REFERENCE:
	  need_paren = 1;
	  break;
	case DEMANGLE_COMPONENT_RESTRICT:
	case DEMANGLE_COMPONENT_VOLATILE:
	case DEMANGLE_COMPONENT_CONST:
	case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
	case DEMANGLE_COMPONENT_COMPLEX:
	case DEMANGLE_COMPONENT_IMAGINARY:
	case DEMANGLE_COMPONENT_PTRMEM_TYPE:
	  need_space = 1;
	  need_paren = 1;
	  break;
	case DEMANGLE_COMPONENT_RESTRICT_THIS:
	case DEMANGLE_COMPONENT_VOLATILE_THIS:
	case DEMANGLE_COMPONENT_CONST_THIS:
	  break;
	default:
	  break;
	}
      if (need_paren)
	break;
    }

  if (d_left (dc) != NULL && ! saw_mod)
    need_paren = 1;

  if (need_paren)
    {
      if (! need_space)
	{
	  if (d_last_char (dpi) != '('
	      && d_last_char (dpi) != '*')
	    need_space = 1;
	}
      if (need_space && d_last_char (dpi) != ' ')
	d_append_char (dpi, ' ');
      d_append_char (dpi, '(');
    }

  hold_modifiers = dpi->modifiers;
  dpi->modifiers = NULL;

  d_print_mod_list (dpi, mods, 0);

  if (need_paren)
    d_append_char (dpi, ')');

  d_append_char (dpi, '(');

  if (d_right (dc) != NULL)
    d_print_comp (dpi, d_right (dc));

  d_append_char (dpi, ')');

  d_print_mod_list (dpi, mods, 1);

  dpi->modifiers = hold_modifiers;
}

/* Print an array type, except for the element type.  */

static void
d_print_array_type (struct d_print_info *dpi,
                    const struct demangle_component *dc,
                    struct d_print_mod *mods)
{
  int need_space;

  need_space = 1;
  if (mods != NULL)
    {
      int need_paren;
      struct d_print_mod *p;

      need_paren = 0;
      for (p = mods; p != NULL; p = p->next)
	{
	  if (! p->printed)
	    {
	      if (p->mod->type == DEMANGLE_COMPONENT_ARRAY_TYPE)
		{
		  need_space = 0;
		  break;
		}
	      else
		{
		  need_paren = 1;
		  need_space = 1;
		  break;
		}
	    }
	}

      if (need_paren)
	d_append_string (dpi, " (");

      d_print_mod_list (dpi, mods, 0);

      if (need_paren)
	d_append_char (dpi, ')');
    }

  if (need_space)
    d_append_char (dpi, ' ');

  d_append_char (dpi, '[');

  if (d_left (dc) != NULL)
    d_print_comp (dpi, d_left (dc));

  d_append_char (dpi, ']');
}

/* Print an operator in an expression.  */

static void
d_print_expr_op (struct d_print_info *dpi,
                 const struct demangle_component *dc)
{
  if (dc->type == DEMANGLE_COMPONENT_OPERATOR)
    d_append_buffer (dpi, dc->u.s_operator.op->name,
		     dc->u.s_operator.op->len);
  else
    d_print_comp (dpi, dc);
}

/* Print a cast.  */

static void
d_print_cast (struct d_print_info *dpi,
              const struct demangle_component *dc)
{
  if (d_left (dc)->type != DEMANGLE_COMPONENT_TEMPLATE)
    d_print_comp (dpi, d_left (dc));
  else
    {
      struct d_print_mod *hold_dpm;
      struct d_print_template dpt;

      /* It appears that for a templated cast operator, we need to put
	 the template parameters in scope for the operator name, but
	 not for the parameters.  The effect is that we need to handle
	 the template printing here.  */

      hold_dpm = dpi->modifiers;
      dpi->modifiers = NULL;

      dpt.next = dpi->templates;
      dpi->templates = &dpt;
      dpt.template_decl = d_left (dc);

      d_print_comp (dpi, d_left (d_left (dc)));

      dpi->templates = dpt.next;

      if (d_last_char (dpi) == '<')
	d_append_char (dpi, ' ');
      d_append_char (dpi, '<');
      d_print_comp (dpi, d_right (d_left (dc)));
      /* Avoid generating two consecutive '>' characters, to avoid
	 the C++ syntactic ambiguity.  */
      if (d_last_char (dpi) == '>')
	d_append_char (dpi, ' ');
      d_append_char (dpi, '>');

      dpi->modifiers = hold_dpm;
    }
}

/* Initialize the information structure we use to pass around
   information.  */

CP_STATIC_IF_GLIBCPP_V3
void
cplus_demangle_init_info (const char *mangled, int options, size_t len,
                          struct d_info *di)
{
  di->s = mangled;
  di->send = mangled + len;
  di->options = options;

  di->n = mangled;

  /* We can not need more components than twice the number of chars in
     the mangled string.  Most components correspond directly to
     chars, but the ARGLIST types are exceptions.  */
  di->num_comps = 2 * len;
  di->next_comp = 0;

  /* Similarly, we can not need more substitutions than there are
     chars in the mangled string.  */
  di->num_subs = len;
  di->next_sub = 0;
  di->did_subs = 0;

  di->last_name = NULL;

  di->expansion = 0;
}

/* Internal implementation for the demangler.  If MANGLED is a g++ v3 ABI
   mangled name, return strings in repeated callback giving the demangled
   name.  OPTIONS is the usual libiberty demangler options.  On success,
   this returns 1.  On failure, returns 0.  */

static int
d_demangle_callback (const char *mangled, int options,
                     demangle_callbackref callback, void *opaque)
{
  int type;
  struct d_info di;
  struct demangle_component *dc;
  int status;

  if (mangled[0] == '_' && mangled[1] == 'Z')
    type = 0;
  else if (strncmp (mangled, "_GLOBAL_", 8) == 0
	   && (mangled[8] == '.' || mangled[8] == '_' || mangled[8] == '$')
	   && (mangled[9] == 'D' || mangled[9] == 'I')
	   && mangled[10] == '_')
    {
      const char *intro;

      intro = (mangled[9] == 'I')
              ? "global constructors keyed to "
              : "global destructors keyed to ";

      callback (intro, strlen (intro), opaque);
      callback (mangled + 11, strlen (mangled + 11), opaque);
      return 1;
    }
  else
    {
      if ((options & DMGL_TYPES) == 0)
	return 0;
      type = 1;
    }

  cplus_demangle_init_info (mangled, options, strlen (mangled), &di);

  {
#ifdef CP_DYNAMIC_ARRAYS
    __extension__ struct demangle_component comps[di.num_comps];
    __extension__ struct demangle_component *subs[di.num_subs];

    di.comps = comps;
    di.subs = subs;
#else
    di.comps = alloca (di.num_comps * sizeof (*di.comps));
    di.subs = alloca (di.num_subs * sizeof (*di.subs));
#endif

    if (type)
      dc = cplus_demangle_type (&di);
    else
      dc = cplus_demangle_mangled_name (&di, 1);

    /* If DMGL_PARAMS is set, then if we didn't consume the entire
       mangled string, then we didn't successfully demangle it.  If
       DMGL_PARAMS is not set, we didn't look at the trailing
       parameters.  */
    if (((options & DMGL_PARAMS) != 0) && d_peek_char (&di) != '\0')
      dc = NULL;

#ifdef CP_DEMANGLE_DEBUG
    d_dump (dc, 0);
#endif

    status = (dc != NULL)
             ? cplus_demangle_print_callback (options, dc, callback, opaque)
             : 0;
  }

  return status;
}

/* Entry point for the demangler.  If MANGLED is a g++ v3 ABI mangled
   name, return a buffer allocated with malloc holding the demangled
   name.  OPTIONS is the usual libiberty demangler options.  On
   success, this sets *PALC to the allocated size of the returned
   buffer.  On failure, this sets *PALC to 0 for a bad name, or 1 for
   a memory allocation failure, and returns NULL.  */

static char *
d_demangle (const char *mangled, int options, size_t *palc)
{
  struct d_growable_string dgs;
  int status;

  d_growable_string_init (&dgs, 0);

  status = d_demangle_callback (mangled, options,
                                d_growable_string_callback_adapter, &dgs);
  if (status == 0)
    {
      free (dgs.buf);
      *palc = 0;
      return NULL;
    }

  *palc = dgs.allocation_failure ? 1 : 0;
  return dgs.buf;
}

#if defined(IN_LIBGCC2) || defined(IN_GLIBCPP_V3)

extern char *__cxa_demangle (const char *, char *, size_t *, int *);

/* ia64 ABI-mandated entry point in the C++ runtime library for
   performing demangling.  MANGLED_NAME is a NUL-terminated character
   string containing the name to be demangled.

   OUTPUT_BUFFER is a region of memory, allocated with malloc, of
   *LENGTH bytes, into which the demangled name is stored.  If
   OUTPUT_BUFFER is not long enough, it is expanded using realloc.
   OUTPUT_BUFFER may instead be NULL; in that case, the demangled name
   is placed in a region of memory allocated with malloc.

   If LENGTH is non-NULL, the length of the buffer containing the
   demangled name, is placed in *LENGTH.

   The return value is a pointer to the start of the NUL-terminated
   demangled name, or NULL if the demangling fails.  The caller is
   responsible for deallocating this memory using free.

   *STATUS is set to one of the following values:
      0: The demangling operation succeeded.
     -1: A memory allocation failure occurred.
     -2: MANGLED_NAME is not a valid name under the C++ ABI mangling rules.
     -3: One of the arguments is invalid.

   The demangling is performed using the C++ ABI mangling rules, with
   GNU extensions.  */

char *
__cxa_demangle (const char *mangled_name, char *output_buffer,
                size_t *length, int *status)
{
  char *demangled;
  size_t alc;

  if (mangled_name == NULL)
    {
      if (status != NULL)
	*status = -3;
      return NULL;
    }

  if (output_buffer != NULL && length == NULL)
    {
      if (status != NULL)
	*status = -3;
      return NULL;
    }

  demangled = d_demangle (mangled_name, DMGL_PARAMS | DMGL_TYPES, &alc);

  if (demangled == NULL)
    {
      if (status != NULL)
	{
	  if (alc == 1)
	    *status = -1;
	  else
	    *status = -2;
	}
      return NULL;
    }

  if (output_buffer == NULL)
    {
      if (length != NULL)
	*length = alc;
    }
  else
    {
      if (strlen (demangled) < *length)
	{
	  strcpy (output_buffer, demangled);
	  free (demangled);
	  demangled = output_buffer;
	}
      else
	{
	  free (output_buffer);
	  *length = alc;
	}
    }

  if (status != NULL)
    *status = 0;

  return demangled;
}

extern int __gcclibcxx_demangle_callback (const char *,
                                          void (*)
                                            (const char *, size_t, void *),
                                          void *);

/* Alternative, allocationless entry point in the C++ runtime library
   for performing demangling.  MANGLED_NAME is a NUL-terminated character
   string containing the name to be demangled.

   CALLBACK is a callback function, called with demangled string
   segments as demangling progresses; it is called at least once,
   but may be called more than once.  OPAQUE is a generalized pointer
   used as a callback argument.

   The return code is one of the following values, equivalent to
   the STATUS values of __cxa_demangle() (excluding -1, since this
   function performs no memory allocations):
      0: The demangling operation succeeded.
     -2: MANGLED_NAME is not a valid name under the C++ ABI mangling rules.
     -3: One of the arguments is invalid.

   The demangling is performed using the C++ ABI mangling rules, with
   GNU extensions.  */

int
__gcclibcxx_demangle_callback (const char *mangled_name,
                               void (*callback) (const char *, size_t, void *),
                               void *opaque)
{
  int status;

  if (mangled_name == NULL || callback == NULL)
    return -3;

  status = d_demangle_callback (mangled_name, DMGL_PARAMS | DMGL_TYPES,
                                callback, opaque);
  if (status == 0)
    return -2;

  return 0;
}

#else /* ! (IN_LIBGCC2 || IN_GLIBCPP_V3) */

/* Entry point for libiberty demangler.  If MANGLED is a g++ v3 ABI
   mangled name, return a buffer allocated with malloc holding the
   demangled name.  Otherwise, return NULL.  */

char *
cplus_demangle_v3 (const char *mangled, int options)
{
  size_t alc;

  return d_demangle (mangled, options, &alc);
}

int
cplus_demangle_v3_callback (const char *mangled, int options,
                            demangle_callbackref callback, void *opaque)
{
  return d_demangle_callback (mangled, options, callback, opaque);
}

/* Demangle a Java symbol.  Java uses a subset of the V3 ABI C++ mangling 
   conventions, but the output formatting is a little different.
   This instructs the C++ demangler not to emit pointer characters ("*"), to
   use Java's namespace separator symbol ("." instead of "::"), and to output
   JArray<TYPE> as TYPE[].  */

char *
java_demangle_v3 (const char *mangled)
{
  size_t alc;

  return d_demangle (mangled, DMGL_JAVA | DMGL_PARAMS | DMGL_RET_POSTFIX, &alc);
}

int
java_demangle_v3_callback (const char *mangled,
                           demangle_callbackref callback, void *opaque)
{
  return d_demangle_callback (mangled,
                              DMGL_JAVA | DMGL_PARAMS | DMGL_RET_POSTFIX,
                              callback, opaque);
}

#endif /* IN_LIBGCC2 || IN_GLIBCPP_V3 */

#ifndef IN_GLIBCPP_V3

/* Demangle a string in order to find out whether it is a constructor
   or destructor.  Return non-zero on success.  Set *CTOR_KIND and
   *DTOR_KIND appropriately.  */

static int
is_ctor_or_dtor (const char *mangled,
                 enum gnu_v3_ctor_kinds *ctor_kind,
                 enum gnu_v3_dtor_kinds *dtor_kind)
{
  struct d_info di;
  struct demangle_component *dc;
  int ret;

  *ctor_kind = (enum gnu_v3_ctor_kinds) 0;
  *dtor_kind = (enum gnu_v3_dtor_kinds) 0;

  cplus_demangle_init_info (mangled, DMGL_GNU_V3, strlen (mangled), &di);

  {
#ifdef CP_DYNAMIC_ARRAYS
    __extension__ struct demangle_component comps[di.num_comps];
    __extension__ struct demangle_component *subs[di.num_subs];

    di.comps = comps;
    di.subs = subs;
#else
    di.comps = alloca (di.num_comps * sizeof (*di.comps));
    di.subs = alloca (di.num_subs * sizeof (*di.subs));
#endif

    dc = cplus_demangle_mangled_name (&di, 1);

    /* Note that because we did not pass DMGL_PARAMS, we don't expect
       to demangle the entire string.  */

    ret = 0;
    while (dc != NULL)
      {
	switch (dc->type)
	  {
	  default:
	    dc = NULL;
	    break;
	  case DEMANGLE_COMPONENT_TYPED_NAME:
	  case DEMANGLE_COMPONENT_TEMPLATE:
	  case DEMANGLE_COMPONENT_RESTRICT_THIS:
	  case DEMANGLE_COMPONENT_VOLATILE_THIS:
	  case DEMANGLE_COMPONENT_CONST_THIS:
	    dc = d_left (dc);
	    break;
	  case DEMANGLE_COMPONENT_QUAL_NAME:
	  case DEMANGLE_COMPONENT_LOCAL_NAME:
	    dc = d_right (dc);
	    break;
	  case DEMANGLE_COMPONENT_CTOR:
	    *ctor_kind = dc->u.s_ctor.kind;
	    ret = 1;
	    dc = NULL;
	    break;
	  case DEMANGLE_COMPONENT_DTOR:
	    *dtor_kind = dc->u.s_dtor.kind;
	    ret = 1;
	    dc = NULL;
	    break;
	  }
      }
  }

  return ret;
}

/* Return whether NAME is the mangled form of a g++ V3 ABI constructor
   name.  A non-zero return indicates the type of constructor.  */

enum gnu_v3_ctor_kinds
is_gnu_v3_mangled_ctor (const char *name)
{
  enum gnu_v3_ctor_kinds ctor_kind;
  enum gnu_v3_dtor_kinds dtor_kind;

  if (! is_ctor_or_dtor (name, &ctor_kind, &dtor_kind))
    return (enum gnu_v3_ctor_kinds) 0;
  return ctor_kind;
}


/* Return whether NAME is the mangled form of a g++ V3 ABI destructor
   name.  A non-zero return indicates the type of destructor.  */

enum gnu_v3_dtor_kinds
is_gnu_v3_mangled_dtor (const char *name)
{
  enum gnu_v3_ctor_kinds ctor_kind;
  enum gnu_v3_dtor_kinds dtor_kind;

  if (! is_ctor_or_dtor (name, &ctor_kind, &dtor_kind))
    return (enum gnu_v3_dtor_kinds) 0;
  return dtor_kind;
}

#endif /* IN_GLIBCPP_V3 */

#ifdef STANDALONE_DEMANGLER

#include "getopt.h"
#include "dyn-string.h"

static void print_usage (FILE* fp, int exit_value);

#define IS_ALPHA(CHAR)                                                  \
  (((CHAR) >= 'a' && (CHAR) <= 'z')                                     \
   || ((CHAR) >= 'A' && (CHAR) <= 'Z'))

/* Non-zero if CHAR is a character than can occur in a mangled name.  */
#define is_mangled_char(CHAR)                                           \
  (IS_ALPHA (CHAR) || IS_DIGIT (CHAR)                                   \
   || (CHAR) == '_' || (CHAR) == '.' || (CHAR) == '$')

/* The name of this program, as invoked.  */
const char* program_name;

/* Prints usage summary to FP and then exits with EXIT_VALUE.  */

static void
print_usage (FILE* fp, int exit_value)
{
  fprintf (fp, "Usage: %s [options] [names ...]\n", program_name);
  fprintf (fp, "Options:\n");
  fprintf (fp, "  -h,--help       Display this message.\n");
  fprintf (fp, "  -p,--no-params  Don't display function parameters\n");
  fprintf (fp, "  -v,--verbose    Produce verbose demanglings.\n");
  fprintf (fp, "If names are provided, they are demangled.  Otherwise filters standard input.\n");

  exit (exit_value);
}

/* Option specification for getopt_long.  */
static const struct option long_options[] = 
{
  { "help",	 no_argument, NULL, 'h' },
  { "no-params", no_argument, NULL, 'p' },
  { "verbose",   no_argument, NULL, 'v' },
  { NULL,        no_argument, NULL, 0   },
};

/* Main entry for a demangling filter executable.  It will demangle
   its command line arguments, if any.  If none are provided, it will
   filter stdin to stdout, replacing any recognized mangled C++ names
   with their demangled equivalents.  */

int
main (int argc, char *argv[])
{
  int i;
  int opt_char;
  int options = DMGL_PARAMS | DMGL_ANSI | DMGL_TYPES;

  /* Use the program name of this program, as invoked.  */
  program_name = argv[0];

  /* Parse options.  */
  do 
    {
      opt_char = getopt_long (argc, argv, "hpv", long_options, NULL);
      switch (opt_char)
	{
	case '?':  /* Unrecognized option.  */
	  print_usage (stderr, 1);
	  break;

	case 'h':
	  print_usage (stdout, 0);
	  break;

	case 'p':
	  options &= ~ DMGL_PARAMS;
	  break;

	case 'v':
	  options |= DMGL_VERBOSE;
	  break;
	}
    }
  while (opt_char != -1);

  if (optind == argc) 
    /* No command line arguments were provided.  Filter stdin.  */
    {
      dyn_string_t mangled = dyn_string_new (3);
      char *s;

      /* Read all of input.  */
      while (!feof (stdin))
	{
	  char c;

	  /* Pile characters into mangled until we hit one that can't
	     occur in a mangled name.  */
	  c = getchar ();
	  while (!feof (stdin) && is_mangled_char (c))
	    {
	      dyn_string_append_char (mangled, c);
	      if (feof (stdin))
		break;
	      c = getchar ();
	    }

	  if (dyn_string_length (mangled) > 0)
	    {
#ifdef IN_GLIBCPP_V3
	      s = __cxa_demangle (dyn_string_buf (mangled), NULL, NULL, NULL);
#else
	      s = cplus_demangle_v3 (dyn_string_buf (mangled), options);
#endif

	      if (s != NULL)
		{
		  fputs (s, stdout);
		  free (s);
		}
	      else
		{
		  /* It might not have been a mangled name.  Print the
		     original text.  */
		  fputs (dyn_string_buf (mangled), stdout);
		}

	      dyn_string_clear (mangled);
	    }

	  /* If we haven't hit EOF yet, we've read one character that
	     can't occur in a mangled name, so print it out.  */
	  if (!feof (stdin))
	    putchar (c);
	}

      dyn_string_delete (mangled);
    }
  else
    /* Demangle command line arguments.  */
    {
      /* Loop over command line arguments.  */
      for (i = optind; i < argc; ++i)
	{
	  char *s;
#ifdef IN_GLIBCPP_V3
	  int status;
#endif

	  /* Attempt to demangle.  */
#ifdef IN_GLIBCPP_V3
	  s = __cxa_demangle (argv[i], NULL, NULL, &status);
#else
	  s = cplus_demangle_v3 (argv[i], options);
#endif

	  /* If it worked, print the demangled name.  */
	  if (s != NULL)
	    {
	      printf ("%s\n", s);
	      free (s);
	    }
	  else
	    {
#ifdef IN_GLIBCPP_V3
	      fprintf (stderr, "Failed: %s (status %d)\n", argv[i], status);
#else
	      fprintf (stderr, "Failed: %s\n", argv[i]);
#endif
	    }
	}
    }

  return 0;
}

#endif /* STANDALONE_DEMANGLER */
