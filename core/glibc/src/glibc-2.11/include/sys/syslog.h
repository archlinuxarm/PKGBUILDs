#include <misc/sys/syslog.h>

libc_hidden_proto (syslog)
libc_hidden_proto (vsyslog)

extern void __vsyslog_chk (int __pri, int __flag, __const char *__fmt,
			   __gnuc_va_list __ap)
     __attribute__ ((__format__ (__printf__, 3, 0)));
libc_hidden_proto (__vsyslog_chk)
