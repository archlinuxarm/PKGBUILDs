;;  Machine Description for MIPS based processor synchronization
;;  instructions.
;;  Copyright (C) 2007, 2008, 2009
;;  Free Software Foundation, Inc.

;; This file is part of GCC.

;; GCC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.

;; GCC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

;; Atomic fetch bitwise operations.
(define_code_iterator fetchop_bit [ior xor and])

;; Atomic HI and QI operations
(define_code_iterator atomic_hiqi_op [plus minus ior xor and])

;; Atomic memory operations.

(define_expand "memory_barrier"
  [(set (match_dup 0)
	(unspec:BLK [(match_dup 0)] UNSPEC_MEMORY_BARRIER))]
  "GENERATE_SYNC"
{
  operands[0] = gen_rtx_MEM (BLKmode, gen_rtx_SCRATCH (Pmode));
  MEM_VOLATILE_P (operands[0]) = 1;
})

(define_insn "*memory_barrier"
  [(set (match_operand:BLK 0 "" "")
	(unspec:BLK [(match_dup 0)] UNSPEC_MEMORY_BARRIER))]
  "GENERATE_SYNC"
  "%|sync%-")

(define_insn "sync_compare_and_swap<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d,&d")
	(match_operand:GPR 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
	(unspec_volatile:GPR [(match_operand:GPR 2 "reg_or_0_operand" "dJ,dJ")
			      (match_operand:GPR 3 "arith_operand" "I,d")]
	 UNSPEC_COMPARE_AND_SWAP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_COMPARE_AND_SWAP ("<d>", "li"));
  else
    return mips_output_sync_loop (MIPS_COMPARE_AND_SWAP ("<d>", "move"));
}
  [(set_attr "length" "32")])

(define_expand "sync_compare_and_swap<mode>"
  [(match_operand:SHORT 0 "register_operand")
   (match_operand:SHORT 1 "memory_operand")
   (match_operand:SHORT 2 "general_operand")
   (match_operand:SHORT 3 "general_operand")]
  "GENERATE_LL_SC"
{
  union mips_gen_fn_ptrs generator;
  generator.fn_6 = gen_compare_and_swap_12;
  mips_expand_atomic_qihi (generator,
			   operands[0], operands[1], operands[2], operands[3]);
  DONE;
})

;; Helper insn for mips_expand_atomic_qihi.
(define_insn "compare_and_swap_12"
  [(set (match_operand:SI 0 "register_operand" "=&d,&d")
	(match_operand:SI 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
	(unspec_volatile:SI [(match_operand:SI 2 "register_operand" "d,d")
			     (match_operand:SI 3 "register_operand" "d,d")
			     (match_operand:SI 4 "reg_or_0_operand" "dJ,dJ")
			     (match_operand:SI 5 "reg_or_0_operand" "d,J")]
			    UNSPEC_COMPARE_AND_SWAP_12))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return (mips_output_sync_loop
	    (MIPS_COMPARE_AND_SWAP_12 (MIPS_COMPARE_AND_SWAP_12_NONZERO_OP)));
  else
    return (mips_output_sync_loop
	    (MIPS_COMPARE_AND_SWAP_12 (MIPS_COMPARE_AND_SWAP_12_ZERO_OP)));
}
  [(set_attr "length" "40,36")])

(define_insn "sync_add<mode>"
  [(set (match_operand:GPR 0 "memory_operand" "+R,R")
	(unspec_volatile:GPR
          [(plus:GPR (match_dup 0)
		     (match_operand:GPR 1 "arith_operand" "I,d"))]
	  UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_SYNC_OP ("<d>", "<d>addiu"));
  else
    return mips_output_sync_loop (MIPS_SYNC_OP ("<d>", "<d>addu"));
}
  [(set_attr "length" "28")])

(define_expand "sync_<optab><mode>"
  [(set (match_operand:SHORT 0 "memory_operand")
	(unspec_volatile:SHORT
	  [(atomic_hiqi_op:SHORT (match_dup 0)
				 (match_operand:SHORT 1 "general_operand"))]
	  UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  union mips_gen_fn_ptrs generator;
  generator.fn_4 = gen_sync_<optab>_12;
  mips_expand_atomic_qihi (generator,
			   NULL, operands[0], operands[1], NULL);
  DONE;
})

;; Helper insn for sync_<optab><mode>
(define_insn "sync_<optab>_12"
  [(set (match_operand:SI 0 "memory_operand" "+R")
	(unspec_volatile:SI
          [(match_operand:SI 1 "register_operand" "d")
	   (match_operand:SI 2 "register_operand" "d")
	   (atomic_hiqi_op:SI (match_dup 0)
			      (match_operand:SI 3 "register_operand" "dJ"))]
	  UNSPEC_SYNC_OLD_OP_12))
   (clobber (match_scratch:SI 4 "=&d"))]
  "GENERATE_LL_SC"
{
    return (mips_output_sync_loop
	    (MIPS_SYNC_OP_12 ("<insn>", MIPS_SYNC_OP_12_AND)));
}
  [(set_attr "length" "40")])

(define_expand "sync_old_<optab><mode>"
  [(parallel [
     (set (match_operand:SHORT 0 "register_operand")
	  (match_operand:SHORT 1 "memory_operand"))
     (set (match_dup 1)
	  (unspec_volatile:SHORT [(atomic_hiqi_op:SHORT
				    (match_dup 1)
				    (match_operand:SHORT 2 "general_operand"))]
	    UNSPEC_SYNC_OLD_OP))])]
  "GENERATE_LL_SC"
{
  union mips_gen_fn_ptrs generator;
  generator.fn_5 = gen_sync_old_<optab>_12;
  mips_expand_atomic_qihi (generator,
			   operands[0], operands[1], operands[2], NULL);
  DONE;
})

;; Helper insn for sync_old_<optab><mode>
(define_insn "sync_old_<optab>_12"
  [(set (match_operand:SI 0 "register_operand" "=&d")
	(match_operand:SI 1 "memory_operand" "+R"))
   (set (match_dup 1)
	(unspec_volatile:SI
          [(match_operand:SI 2 "register_operand" "d")
	   (match_operand:SI 3 "register_operand" "d")
	   (atomic_hiqi_op:SI (match_dup 0)
			      (match_operand:SI 4 "register_operand" "dJ"))]
	  UNSPEC_SYNC_OLD_OP_12))
   (clobber (match_scratch:SI 5 "=&d"))]
  "GENERATE_LL_SC"
{
    return (mips_output_sync_loop
	    (MIPS_SYNC_OLD_OP_12 ("<insn>", MIPS_SYNC_OLD_OP_12_AND)));
}
  [(set_attr "length" "40")])

(define_expand "sync_new_<optab><mode>"
  [(parallel [
     (set (match_operand:SHORT 0 "register_operand")
	  (unspec_volatile:SHORT [(atomic_hiqi_op:SHORT
				    (match_operand:SHORT 1 "memory_operand")
				    (match_operand:SHORT 2 "general_operand"))]
	    UNSPEC_SYNC_NEW_OP))
     (set (match_dup 1)
	  (unspec_volatile:SHORT [(match_dup 1) (match_dup 2)]
	    UNSPEC_SYNC_NEW_OP))])]
  "GENERATE_LL_SC"
{
  union mips_gen_fn_ptrs generator;
  generator.fn_5 = gen_sync_new_<optab>_12;
  mips_expand_atomic_qihi (generator,
			   operands[0], operands[1], operands[2], NULL);
  DONE;
})

;; Helper insn for sync_new_<optab><mode>
(define_insn "sync_new_<optab>_12"
  [(set (match_operand:SI 0 "register_operand" "=&d")
	(unspec_volatile:SI
          [(match_operand:SI 1 "memory_operand" "+R")
	   (match_operand:SI 2 "register_operand" "d")
	   (match_operand:SI 3 "register_operand" "d")
	   (atomic_hiqi_op:SI (match_dup 0)
			      (match_operand:SI 4 "register_operand" "dJ"))]
	  UNSPEC_SYNC_NEW_OP_12))
   (set (match_dup 1)
	(unspec_volatile:SI
	  [(match_dup 1)
	   (match_dup 2)
	   (match_dup 3)
	   (match_dup 4)] UNSPEC_SYNC_NEW_OP_12))]
  "GENERATE_LL_SC"
{
    return (mips_output_sync_loop
	    (MIPS_SYNC_NEW_OP_12 ("<insn>", MIPS_SYNC_NEW_OP_12_AND)));
}
  [(set_attr "length" "40")])

(define_expand "sync_nand<mode>"
  [(set (match_operand:SHORT 0 "memory_operand")
	(unspec_volatile:SHORT
	  [(match_dup 0)
	   (match_operand:SHORT 1 "general_operand")]
	  UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  union mips_gen_fn_ptrs generator;
  generator.fn_4 = gen_sync_nand_12;
  mips_expand_atomic_qihi (generator,
			   NULL, operands[0], operands[1], NULL);
  DONE;
})

;; Helper insn for sync_nand<mode>
(define_insn "sync_nand_12"
  [(set (match_operand:SI 0 "memory_operand" "+R")
	(unspec_volatile:SI
          [(match_operand:SI 1 "register_operand" "d")
	   (match_operand:SI 2 "register_operand" "d")
	   (match_dup 0)
	   (match_operand:SI 3 "register_operand" "dJ")]
	  UNSPEC_SYNC_OLD_OP_12))
   (clobber (match_scratch:SI 4 "=&d"))]
  "GENERATE_LL_SC"
{
    return (mips_output_sync_loop
	    (MIPS_SYNC_OP_12 ("and", MIPS_SYNC_OP_12_XOR)));
}
  [(set_attr "length" "40")])

(define_expand "sync_old_nand<mode>"
  [(parallel [
     (set (match_operand:SHORT 0 "register_operand")
	  (match_operand:SHORT 1 "memory_operand"))
     (set (match_dup 1)
	  (unspec_volatile:SHORT [(match_dup 1)
				  (match_operand:SHORT 2 "general_operand")]
	    UNSPEC_SYNC_OLD_OP))])]
  "GENERATE_LL_SC"
{
  union mips_gen_fn_ptrs generator;
  generator.fn_5 = gen_sync_old_nand_12;
  mips_expand_atomic_qihi (generator,
			   operands[0], operands[1], operands[2], NULL);
  DONE;
})

;; Helper insn for sync_old_nand<mode>
(define_insn "sync_old_nand_12"
  [(set (match_operand:SI 0 "register_operand" "=&d")
	(match_operand:SI 1 "memory_operand" "+R"))
   (set (match_dup 1)
	(unspec_volatile:SI
          [(match_operand:SI 2 "register_operand" "d")
	   (match_operand:SI 3 "register_operand" "d")
	   (match_operand:SI 4 "register_operand" "dJ")]
	  UNSPEC_SYNC_OLD_OP_12))
   (clobber (match_scratch:SI 5 "=&d"))]
  "GENERATE_LL_SC"
{
    return (mips_output_sync_loop
	    (MIPS_SYNC_OLD_OP_12 ("and", MIPS_SYNC_OLD_OP_12_XOR)));
}
  [(set_attr "length" "40")])

(define_expand "sync_new_nand<mode>"
  [(parallel [
     (set (match_operand:SHORT 0 "register_operand")
	  (unspec_volatile:SHORT [(match_operand:SHORT 1 "memory_operand")
				  (match_operand:SHORT 2 "general_operand")]
	    UNSPEC_SYNC_NEW_OP))
     (set (match_dup 1)
	  (unspec_volatile:SHORT [(match_dup 1) (match_dup 2)]
	    UNSPEC_SYNC_NEW_OP))])]
  "GENERATE_LL_SC"
{
  union mips_gen_fn_ptrs generator;
  generator.fn_5 = gen_sync_new_nand_12;
  mips_expand_atomic_qihi (generator,
			   operands[0], operands[1], operands[2], NULL);
  DONE;
})

;; Helper insn for sync_new_nand<mode>
(define_insn "sync_new_nand_12"
  [(set (match_operand:SI 0 "register_operand" "=&d")
	(unspec_volatile:SI
          [(match_operand:SI 1 "memory_operand" "+R")
	   (match_operand:SI 2 "register_operand" "d")
	   (match_operand:SI 3 "register_operand" "d")
	   (match_operand:SI 4 "register_operand" "dJ")]
	  UNSPEC_SYNC_NEW_OP_12))
   (set (match_dup 1)
	(unspec_volatile:SI
	  [(match_dup 1)
	   (match_dup 2)
	   (match_dup 3)
	   (match_dup 4)] UNSPEC_SYNC_NEW_OP_12))]
  "GENERATE_LL_SC"
{
    return (mips_output_sync_loop
	    (MIPS_SYNC_NEW_OP_12 ("and", MIPS_SYNC_NEW_OP_12_XOR)));
}
  [(set_attr "length" "40")])

(define_insn "sync_sub<mode>"
  [(set (match_operand:GPR 0 "memory_operand" "+R")
	(unspec_volatile:GPR
          [(minus:GPR (match_dup 0)
			      (match_operand:GPR 1 "register_operand" "d"))]
	 UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  return mips_output_sync_loop (MIPS_SYNC_OP ("<d>", "<d>subu"));
}
  [(set_attr "length" "28")])

(define_insn "sync_old_add<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d,&d")
	(match_operand:GPR 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
	(unspec_volatile:GPR
          [(plus:GPR (match_dup 1)
		     (match_operand:GPR 2 "arith_operand" "I,d"))]
	 UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_SYNC_OLD_OP ("<d>", "<d>addiu"));
  else
    return mips_output_sync_loop (MIPS_SYNC_OLD_OP ("<d>", "<d>addu"));
}
  [(set_attr "length" "28")])

(define_insn "sync_old_sub<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d")
	(match_operand:GPR 1 "memory_operand" "+R"))
   (set (match_dup 1)
	(unspec_volatile:GPR
          [(minus:GPR (match_dup 1)
		      (match_operand:GPR 2 "register_operand" "d"))]
	 UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  return mips_output_sync_loop (MIPS_SYNC_OLD_OP ("<d>", "<d>subu"));
}
  [(set_attr "length" "28")])

(define_insn "sync_new_add<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d,&d")
        (plus:GPR (match_operand:GPR 1 "memory_operand" "+R,R")
		  (match_operand:GPR 2 "arith_operand" "I,d")))
   (set (match_dup 1)
	(unspec_volatile:GPR
	  [(plus:GPR (match_dup 1) (match_dup 2))]
	 UNSPEC_SYNC_NEW_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_SYNC_NEW_OP ("<d>", "<d>addiu"));
  else
    return mips_output_sync_loop (MIPS_SYNC_NEW_OP ("<d>", "<d>addu"));
}
  [(set_attr "length" "28")])

(define_insn "sync_new_sub<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d")
        (minus:GPR (match_operand:GPR 1 "memory_operand" "+R")
		   (match_operand:GPR 2 "register_operand" "d")))
   (set (match_dup 1)
	(unspec_volatile:GPR
	  [(minus:GPR (match_dup 1) (match_dup 2))]
	 UNSPEC_SYNC_NEW_OP))]
  "GENERATE_LL_SC"
{
  return mips_output_sync_loop (MIPS_SYNC_NEW_OP ("<d>", "<d>subu"));
}
  [(set_attr "length" "28")])

(define_insn "sync_<optab><mode>"
  [(set (match_operand:GPR 0 "memory_operand" "+R,R")
	(unspec_volatile:GPR
          [(fetchop_bit:GPR (match_operand:GPR 1 "uns_arith_operand" "K,d")
			      (match_dup 0))]
	 UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_SYNC_OP ("<d>", "<immediate_insn>"));
  else
    return mips_output_sync_loop (MIPS_SYNC_OP ("<d>", "<insn>"));
}
  [(set_attr "length" "28")])

(define_insn "sync_old_<optab><mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d,&d")
	(match_operand:GPR 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
	(unspec_volatile:GPR
          [(fetchop_bit:GPR (match_operand:GPR 2 "uns_arith_operand" "K,d")
			    (match_dup 1))]
	 UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return (mips_output_sync_loop
	    (MIPS_SYNC_OLD_OP ("<d>", "<immediate_insn>")));
  else
    return mips_output_sync_loop (MIPS_SYNC_OLD_OP ("<d>", "<insn>"));
}
  [(set_attr "length" "28")])

(define_insn "sync_new_<optab><mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d,&d")
	(match_operand:GPR 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
	(unspec_volatile:GPR
          [(fetchop_bit:GPR (match_operand:GPR 2 "uns_arith_operand" "K,d")
			    (match_dup 1))]
	 UNSPEC_SYNC_NEW_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return (mips_output_sync_loop
	    (MIPS_SYNC_NEW_OP ("<d>", "<immediate_insn>")));
  else
    return mips_output_sync_loop (MIPS_SYNC_NEW_OP ("<d>", "<insn>"));
}
  [(set_attr "length" "28")])

(define_insn "sync_nand<mode>"
  [(set (match_operand:GPR 0 "memory_operand" "+R,R")
	(unspec_volatile:GPR [(match_operand:GPR 1 "uns_arith_operand" "K,d")]
	 UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_SYNC_NAND ("<d>", "andi"));
  else
    return mips_output_sync_loop (MIPS_SYNC_NAND ("<d>", "and"));
}
  [(set_attr "length" "32")])

(define_insn "sync_old_nand<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d,&d")
	(match_operand:GPR 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
        (unspec_volatile:GPR [(match_operand:GPR 2 "uns_arith_operand" "K,d")]
	 UNSPEC_SYNC_OLD_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_SYNC_OLD_NAND ("<d>", "andi"));
  else
    return mips_output_sync_loop (MIPS_SYNC_OLD_NAND ("<d>", "and"));
}
  [(set_attr "length" "32")])

(define_insn "sync_new_nand<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d,&d")
	(match_operand:GPR 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
	(unspec_volatile:GPR [(match_operand:GPR 2 "uns_arith_operand" "K,d")]
	 UNSPEC_SYNC_NEW_OP))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_SYNC_NEW_NAND ("<d>", "andi"));
  else
    return mips_output_sync_loop (MIPS_SYNC_NEW_NAND ("<d>", "and"));
}
  [(set_attr "length" "32")])

(define_insn "sync_lock_test_and_set<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&d,&d")
	(match_operand:GPR 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
	(unspec_volatile:GPR [(match_operand:GPR 2 "arith_operand" "I,d")]
	 UNSPEC_SYNC_EXCHANGE))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return mips_output_sync_loop (MIPS_SYNC_EXCHANGE ("<d>", "li"));
  else
    return mips_output_sync_loop (MIPS_SYNC_EXCHANGE ("<d>", "move"));
}
  [(set_attr "length" "24")])

(define_expand "sync_lock_test_and_set<mode>"
  [(match_operand:SHORT 0 "register_operand")
   (match_operand:SHORT 1 "memory_operand")
   (match_operand:SHORT 2 "general_operand")]
  "GENERATE_LL_SC"
{
  union mips_gen_fn_ptrs generator;
  generator.fn_5 = gen_test_and_set_12;
  mips_expand_atomic_qihi (generator,
			   operands[0], operands[1], operands[2], NULL);
  DONE;
})

(define_insn "test_and_set_12"
  [(set (match_operand:SI 0 "register_operand" "=&d,&d")
	(match_operand:SI 1 "memory_operand" "+R,R"))
   (set (match_dup 1)
	(unspec_volatile:SI [(match_operand:SI 2 "register_operand" "d,d")
			     (match_operand:SI 3 "register_operand" "d,d")
			     (match_operand:SI 4 "arith_operand" "d,J")]
	  UNSPEC_SYNC_EXCHANGE_12))]
  "GENERATE_LL_SC"
{
  if (which_alternative == 0)
    return (mips_output_sync_loop
	    (MIPS_SYNC_EXCHANGE_12 (MIPS_SYNC_EXCHANGE_12_NONZERO_OP)));
  else
    return (mips_output_sync_loop
	    (MIPS_SYNC_EXCHANGE_12 (MIPS_SYNC_EXCHANGE_12_ZERO_OP)));
}
  [(set_attr "length" "28,24")])
