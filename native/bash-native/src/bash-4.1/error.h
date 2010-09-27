/* error.h -- External declarations of functions appearing in error.c. */

/* Copyright (C) 1993-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined (_ERROR_H_)
#define _ERROR_H_

#include "stdc.h"

/* Get the name of the shell or shell script for an error message. */
extern char *get_name_for_error __P((void));

/* Report an error having to do with FILENAME. */
extern void file_error __P((const char *));

/* Report a programmer's error, and abort.  Pass REASON, and ARG1 ... ARG5. */
extern void programming_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* General error reporting.  Pass FORMAT and ARG1 ... ARG5. */
extern void report_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Error messages for parts of the parser that don't call report_syntax_error */
extern void parser_error __P((int, const char *, ...))  __attribute__((__format__ (printf, 2, 3)));

/* Report an unrecoverable error and exit.  Pass FORMAT and ARG1 ... ARG5. */
extern void fatal_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Report a system error, like BSD warn(3). */
extern void sys_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Report an internal error. */
extern void internal_error __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Report an internal warning. */
extern void internal_warning __P((const char *, ...))  __attribute__((__format__ (printf, 1, 2)));

/* Debugging functions, not enabled in released version. */
extern void itrace __P((const char *, ...)) __attribute__ ((__format__ (printf, 1, 2)));
extern void trace __P((const char *, ...)) __attribute__ ((__format__ (printf, 1, 2)));

/* Report an error having to do with command parsing or execution. */
extern void command_error __P((const char *, int, int, int));

extern char *command_errstr __P((int));

/* Specific errror message functions that eventually call report_error or
   internal_error. */

extern void err_badarraysub __P((const char *));
extern void err_unboundvar __P((const char *));
extern void err_readonly __P((const char *));

#endif /* !_ERROR_H_ */
