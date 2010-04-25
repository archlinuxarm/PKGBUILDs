/* Prototypes of target machine for GNU compiler for Xtensa.
   Copyright 2001, 2002, 2003, 2004, 2005, 2007, 2008
   Free Software Foundation, Inc.
   Contributed by Bob Wilson (bwilson@tensilica.com) at Tensilica.

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

#ifndef __XTENSA_PROTOS_H__
#define __XTENSA_PROTOS_H__

/* Functions to test whether an immediate fits in a given field.  */
extern bool xtensa_simm8 (HOST_WIDE_INT);
extern bool xtensa_simm8x256 (HOST_WIDE_INT);
extern bool xtensa_simm12b (HOST_WIDE_INT);
extern bool xtensa_b4const_or_zero (HOST_WIDE_INT);
extern bool xtensa_b4constu (HOST_WIDE_INT);
extern bool xtensa_mask_immediate (HOST_WIDE_INT);
extern bool xtensa_mem_offset (unsigned, enum machine_mode);

/* Functions within xtensa.c that we reference.  */
#ifdef RTX_CODE
extern int xt_true_regnum (rtx);
extern int xtensa_valid_move (enum machine_mode, rtx *);
extern int smalloffset_mem_p (rtx);
extern int constantpool_address_p (rtx);
extern int constantpool_mem_p (rtx);
extern void xtensa_extend_reg (rtx, rtx);
extern void xtensa_expand_conditional_branch (rtx *, enum rtx_code);
extern int xtensa_expand_conditional_move (rtx *, int);
extern int xtensa_expand_scc (rtx *);
extern int xtensa_expand_block_move (rtx *);
extern void xtensa_split_operand_pair (rtx *, enum machine_mode);
extern int xtensa_emit_move_sequence (rtx *, enum machine_mode);
extern rtx xtensa_copy_incoming_a7 (rtx);
extern void xtensa_expand_nonlocal_goto (rtx *);
extern void xtensa_expand_compare_and_swap (rtx, rtx, rtx, rtx);
extern void xtensa_expand_atomic (enum rtx_code, rtx, rtx, rtx, bool);
extern void xtensa_emit_loop_end (rtx, rtx *);
extern char *xtensa_emit_branch (bool, bool, rtx *);
extern char *xtensa_emit_bit_branch (bool, bool, rtx *);
extern char *xtensa_emit_movcc (bool, bool, bool, rtx *);
extern char *xtensa_emit_call (int, rtx *);
extern bool xtensa_legitimate_address_p (enum machine_mode, rtx, bool);
extern rtx xtensa_legitimize_address (rtx, rtx, enum machine_mode);
extern bool xtensa_tls_referenced_p (rtx);

#ifdef TREE_CODE
extern void init_cumulative_args (CUMULATIVE_ARGS *, int);
#endif /* TREE_CODE */

extern void print_operand (FILE *, rtx, int);
extern void print_operand_address (FILE *, rtx);
extern bool xtensa_output_addr_const_extra (FILE *, rtx);
extern void xtensa_output_literal (FILE *, rtx, enum machine_mode, int);
extern rtx xtensa_return_addr (int, rtx);
extern enum reg_class xtensa_preferred_reload_class (rtx, enum reg_class, int);
struct secondary_reload_info;
extern enum reg_class xtensa_secondary_reload (bool, rtx, enum reg_class,
					       enum machine_mode,
					       struct secondary_reload_info *);
extern void xtensa_initialize_trampoline (rtx, rtx, rtx);
#endif /* RTX_CODE */

#ifdef TREE_CODE
extern void function_arg_advance (CUMULATIVE_ARGS *, enum machine_mode, tree);
extern struct rtx_def *function_arg (CUMULATIVE_ARGS *, enum machine_mode,
				     tree, int);
extern int function_arg_boundary (enum machine_mode, tree);
#endif /* TREE_CODE */

extern void xtensa_setup_frame_addresses (void);
extern int xtensa_dbx_register_number (int);
extern void override_options (void);
extern long compute_frame_size (int);
extern int xtensa_frame_pointer_required (void);
extern void xtensa_expand_prologue (void);
extern void order_regs_for_local_alloc (void);
extern void xtensa_trampoline_template (FILE *);

#endif /* !__XTENSA_PROTOS_H__ */
