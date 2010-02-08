#ifndef _SYS_STATFS_H
#include <io/sys/statfs.h>

/* Now define the internal interfaces.  */
extern int __statfs (__const char *__file, struct statfs *__buf);
libc_hidden_proto (__statfs)
extern int __fstatfs (int __fildes, struct statfs *__buf);
extern int __statfs64 (__const char *__file, struct statfs64 *__buf);
extern int __fstatfs64 (int __fildes, struct statfs64 *__buf);
#endif
