/* Definitions of target machine for GNU compiler.
   MIPS SDE version.
   Copyright (C) 2003, 2004, 2007, 2008, 2009
   Free Software Foundation, Inc.

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

#undef DRIVER_SELF_SPECS
#define DRIVER_SELF_SPECS						\
  /* Make sure a -mips option is present.  This helps us to pick	\
     the right multilib, and also makes the later specs easier		\
     to write.  */							\
  MIPS_ISA_LEVEL_SPEC,							\
									\
  /* Infer the default float setting from -march.  */			\
  MIPS_ARCH_FLOAT_SPEC,							\
									\
  /* If no ABI option is specified, infer one from the ISA level	\
     or -mgp setting.  */						\
  "%{!mabi=*: %{" MIPS_32BIT_OPTION_SPEC ": -mabi=32;: -mabi=n32}}",	\
									\
  /* Remove a redundant -mfp64 for -mabi=n32; we want the !mfp64	\
     multilibs.  There's no need to check whether the architecture	\
     is 64-bit; cc1 will complain if it isn't.  */			\
  "%{mabi=n32: %<mfp64}",						\
									\
  /* Make sure that an endian option is always present.  This makes	\
     things like LINK_SPEC easier to write.  */				\
  "%{!EB:%{!EL:%(endian_spec)}}",					\
									\
  /* -mcode-xonly is a traditional alias for -mcode-readable=pcrel and	\
     -mno-data-in-code is a traditional alias for -mcode-readable=no.	\
     The latter trumps the former.  */					\
  "%{mno-data-in-code: -mcode-readable=no}",				\
  "%{!mcode-readable=no: %{mcode-xonly: -mcode-readable=pcrel}}",	\
  "%<mno-data-in-code %<mcode-xonly",					\
									\
  /* Configuration-independent MIPS rules.  */				\
  BASE_DRIVER_SELF_SPECS				

/* Use trap rather than break for all but MIPS I ISA.  Force -no-mips16,
   so that MIPS16 assembler code requires an explicit ".set mips16".
   Very little hand-written MIPS16 assembler exists, and some build
   systems expect code to be assembled as non-MIPS16 even if the
   prevailing compiler flags select -mips16.  */
#undef SUBTARGET_ASM_SPEC
#define SUBTARGET_ASM_SPEC "\
%{!mips1:--trap} \
%{mips16:-no-mips16}"

#undef LINK_SPEC
#define LINK_SPEC "\
%(endian_spec) \
%{G*} %{mips1} %{mips2} %{mips3} %{mips4} %{mips32*} %{mips64*} \
%{bestGnum} \
%{shared} %{non_shared} %{call_shared} \
%{mabi=n32:-melf32%{EB:b}%{EL:l}tsmipn32} \
%{mabi=64:-melf64%{EB:b}%{EL:l}tsmip} \
%{mabi=32:-melf32%{EB:b}%{EL:l}tsmip}"

#undef DEFAULT_SIGNED_CHAR
#define DEFAULT_SIGNED_CHAR 0

/* SDE-MIPS won't ever support SDB debugging info.  */
#undef SDB_DEBUGGING_INFO

/* Describe how we implement __builtin_eh_return.  */

/* At the moment, nothing appears to use more than 2 EH data registers.
   The chosen registers must not clash with the return register ($2),
   EH_RETURN_STACKADJ ($3), or MIPS_EPILOGUE_TEMP ($5), and they must
   be general MIPS16 registers.  Pick $6 and $7.  */
#undef EH_RETURN_DATA_REGNO
#define EH_RETURN_DATA_REGNO(N) \
  ((N) < 2 ? 7 - (N) : INVALID_REGNUM)

/* Use $5 as a temporary for both MIPS16 and non-MIPS16.  */
#undef MIPS_EPILOGUE_TEMP_REGNUM
#define MIPS_EPILOGUE_TEMP_REGNUM (GP_REG_FIRST + 5)

/* Using long will always be right for size_t and ptrdiff_t, since
   sizeof(long) must equal sizeof(void *), following from the setting
   of the -mlong64 option.  */
#undef SIZE_TYPE
#define SIZE_TYPE "long unsigned int"
#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "long int"

/* Enable parsing of #pragma pack(push,<n>) and #pragma pack(pop).  */
#define HANDLE_PRAGMA_PACK_PUSH_POP 1

/* Use standard ELF-style local labels (not '$' as on early Irix).  */
#undef LOCAL_LABEL_PREFIX
#define LOCAL_LABEL_PREFIX "."

/* Use periods rather than dollar signs in special g++ assembler names.  */
#define NO_DOLLAR_IN_LABEL

/* Attach a special .ident directive to the end of the file to identify
   the version of GCC which compiled this code.  */
#undef IDENT_ASM_OP
#define IDENT_ASM_OP "\t.ident\t"

/* Output #ident string into the ELF .comment section, so it doesn't
   form part of the load image, and so that it can be stripped.  */
#undef ASM_OUTPUT_IDENT
#define ASM_OUTPUT_IDENT(STREAM, STRING) \
  fprintf (STREAM, "%s\"%s\"\n", IDENT_ASM_OP, STRING);

/* Currently we don't support 128bit long doubles, so for now we force
   n32 to be 64bit.  */
#undef LONG_DOUBLE_TYPE_SIZE
#define LONG_DOUBLE_TYPE_SIZE 64

#ifdef IN_LIBGCC2
#undef LIBGCC2_LONG_DOUBLE_TYPE_SIZE
#define LIBGCC2_LONG_DOUBLE_TYPE_SIZE 64
#endif

/* Force all .init and .fini entries to be 32-bit, not mips16, so that
   in a mixed environment they are all the same mode. The crti.asm and
   crtn.asm files will also be compiled as 32-bit due to the
   -no-mips16 flag in SUBTARGET_ASM_SPEC above. */
#undef CRT_CALL_STATIC_FUNCTION
#define CRT_CALL_STATIC_FUNCTION(SECTION_OP, FUNC) \
   asm (SECTION_OP "\n\
	.set push\n\
	.set nomips16\n\
	jal " USER_LABEL_PREFIX #FUNC "\n\
	.set pop\n\
	" TEXT_SECTION_ASM_OP);
