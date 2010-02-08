#include <sysdep.h>
#undef ret	/* get rid of the stupid "ret" macro; it breaks br.ret */

/* Support for compatible assembler handling.  */

#ifdef __ELF__
# define ASM_SIZE_DIRECTIVE(name) .size name,.-name
# define ASM_TYPE_DIRECTIVE(name,T) .type name,T
#else
# define ASM_SIZE_DIRECTIVE(name)
# define ASM_TYPE_DIRECTIVE(name,T)
#endif

#define LOCAL_LIBM_ENTRY(name)			\
	.proc name;				\
 name:

#define LOCAL_LIBM_END(name)			\
	.endp name;				\
 ASM_SIZE_DIRECTIVE(name)


#define RODATA		.rodata
#define LOCAL_OBJECT_START(name)		\
   name:;					\
   ASM_TYPE_DIRECTIVE(name, @object)
#define LOCAL_OBJECT_END(name)			\
   ASM_SIZE_DIRECTIVE(name)

#define GLOBAL_LIBM_ENTRY(name)			\
	LOCAL_LIBM_ENTRY(name);			\
	.global name
#define GLOBAL_LIBM_END(name)		LOCAL_LIBM_END(name)

#define INTERNAL_LIBM_ENTRY(name)		\
	GLOBAL_LIBM_ENTRY(__libm_##name);	\
	.global __libm_##name
#define INTERNAL_LIBM_END(name)		GLOBAL_LIBM_END(__libm_##name)

#define WEAK_LIBM_ENTRY(name)			\
	.align 32;				\
	LOCAL_LIBM_ENTRY(__##name);		\
	.global __##name;			\
 __##name:
#define WEAK_LIBM_END(name)			\
 weak_alias (__##name, name);			\
 .hidden __##name;				\
	LOCAL_LIBM_END(__##name);		\
 ASM_SIZE_DIRECTIVE(__##name);			\
 ASM_TYPE_DIRECTIVE(__##name, @function)

#define GLOBAL_IEEE754_ENTRY(name)		\
	WEAK_LIBM_ENTRY(name);			\
	.global __ieee754_##name;		\
	.hidden __ieee754_##name;		\
 __ieee754_##name:
#define GLOBAL_IEEE754_END(name)			\
	WEAK_LIBM_END(name);				\
 ASM_SIZE_DIRECTIVE(__ieee754_##name);			\
 ASM_TYPE_DIRECTIVE(__ieee754_##name, @function)

#if defined ASSEMBLER && !defined NOT_IN_libc
# define __libm_error_support	HIDDEN_JUMPTARGET(__libm_error_support)
#endif
