/* Language-dependent hooks for Objective-C++.
   Copyright 2005, 2007, 2008 Free Software Foundation, Inc.
   Contributed by Ziemowit Laski  <zlaski@apple.com>

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


#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "cp-tree.h"
#include "c-common.h"
#include "toplev.h"
#include "objc-act.h"
#include "langhooks.h"
#include "langhooks-def.h"
#include "diagnostic.h"
#include "debug.h"
#include "cp-objcp-common.h"

enum c_language_kind c_language = clk_objcxx;
static void objcxx_init_ts (void);

/* Lang hooks common to C++ and ObjC++ are declared in cp/cp-objcp-common.h;
   consequently, there should be very few hooks below.  */

#undef LANG_HOOKS_NAME
#define LANG_HOOKS_NAME "GNU Objective-C++"
#undef LANG_HOOKS_INIT
#define LANG_HOOKS_INIT objc_init
#undef LANG_HOOKS_DECL_PRINTABLE_NAME
#define LANG_HOOKS_DECL_PRINTABLE_NAME	objc_printable_name
#undef LANG_HOOKS_GIMPLIFY_EXPR 
#define LANG_HOOKS_GIMPLIFY_EXPR objc_gimplify_expr
#undef LANG_HOOKS_INIT_TS
#define LANG_HOOKS_INIT_TS objcxx_init_ts

/* Each front end provides its own lang hook initializer.  */
const struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;

/* Lang hook routines common to C++ and ObjC++ appear in cp/cp-objcp-common.c;
   there should be very few (if any) routines below.  */

tree
objcp_tsubst_copy_and_build (tree t, tree args, tsubst_flags_t complain, 
			     tree in_decl, bool function_p ATTRIBUTE_UNUSED)
{
#define RECURSE(NODE)							\
  tsubst_copy_and_build (NODE, args, complain, in_decl, 		\
			 /*function_p=*/false,				\
			 /*integral_constant_expression_p=*/false)

  /* The following two can only occur in Objective-C++.  */

  switch ((int) TREE_CODE (t))
    {
    case MESSAGE_SEND_EXPR:
      return objc_finish_message_expr
	(RECURSE (TREE_OPERAND (t, 0)),
	 TREE_OPERAND (t, 1),  /* No need to expand the selector.  */
	 RECURSE (TREE_OPERAND (t, 2)));

    case CLASS_REFERENCE_EXPR:
      return objc_get_class_reference
	(RECURSE (TREE_OPERAND (t, 0)));

    default:
      break;
    }

  /* Fall back to C++ processing.  */
  return NULL_TREE;

#undef RECURSE
}

static void
objcxx_init_ts (void)
{
  /* objc decls */
  tree_contains_struct[CLASS_METHOD_DECL][TS_DECL_NON_COMMON] = 1;
  tree_contains_struct[INSTANCE_METHOD_DECL][TS_DECL_NON_COMMON] = 1;
  tree_contains_struct[KEYWORD_DECL][TS_DECL_NON_COMMON] = 1;
  
  tree_contains_struct[CLASS_METHOD_DECL][TS_DECL_WITH_VIS] = 1;
  tree_contains_struct[INSTANCE_METHOD_DECL][TS_DECL_WITH_VIS] = 1;
  tree_contains_struct[KEYWORD_DECL][TS_DECL_WITH_VIS] = 1;

  tree_contains_struct[CLASS_METHOD_DECL][TS_DECL_WRTL] = 1;
  tree_contains_struct[INSTANCE_METHOD_DECL][TS_DECL_WRTL] = 1;
  tree_contains_struct[KEYWORD_DECL][TS_DECL_WRTL] = 1;
  
  tree_contains_struct[CLASS_METHOD_DECL][TS_DECL_MINIMAL] = 1;
  tree_contains_struct[INSTANCE_METHOD_DECL][TS_DECL_MINIMAL] = 1;
  tree_contains_struct[KEYWORD_DECL][TS_DECL_MINIMAL] = 1;
  
  tree_contains_struct[CLASS_METHOD_DECL][TS_DECL_COMMON] = 1;
  tree_contains_struct[INSTANCE_METHOD_DECL][TS_DECL_COMMON] = 1;
  tree_contains_struct[KEYWORD_DECL][TS_DECL_COMMON] = 1;
  
  /* C++ decls */
  tree_contains_struct[NAMESPACE_DECL][TS_DECL_NON_COMMON] = 1;
  tree_contains_struct[USING_DECL][TS_DECL_NON_COMMON] = 1;
  tree_contains_struct[TEMPLATE_DECL][TS_DECL_NON_COMMON] = 1;

  tree_contains_struct[NAMESPACE_DECL][TS_DECL_WITH_VIS] = 1;
  tree_contains_struct[USING_DECL][TS_DECL_WITH_VIS] = 1;
  tree_contains_struct[TEMPLATE_DECL][TS_DECL_WITH_VIS] = 1;

  tree_contains_struct[NAMESPACE_DECL][TS_DECL_WRTL] = 1;
  tree_contains_struct[USING_DECL][TS_DECL_WRTL] = 1;
  tree_contains_struct[TEMPLATE_DECL][TS_DECL_WRTL] = 1;
  
  tree_contains_struct[NAMESPACE_DECL][TS_DECL_COMMON] = 1;
  tree_contains_struct[USING_DECL][TS_DECL_COMMON] = 1;
  tree_contains_struct[TEMPLATE_DECL][TS_DECL_COMMON] = 1;
 
  tree_contains_struct[NAMESPACE_DECL][TS_DECL_MINIMAL] = 1;
  tree_contains_struct[USING_DECL][TS_DECL_MINIMAL] = 1;
  tree_contains_struct[TEMPLATE_DECL][TS_DECL_MINIMAL] = 1;

  init_shadowed_var_for_decl ();
}


void
finish_file (void)
{
  objc_finish_file ();
}

#include "gtype-objcp.h"
