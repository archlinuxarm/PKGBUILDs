#ifndef _MALLOC_H
#include <malloc/malloc.h>


/* In the GNU libc we rename the global variable
   `__malloc_initialized' to `__libc_malloc_initialized'.  */
#define __malloc_initialized __libc_malloc_initialized
/* Nonzero if the malloc is already initialized.  */
extern int __malloc_initialized attribute_hidden;

struct malloc_state;
typedef struct malloc_state *mstate;

#endif
