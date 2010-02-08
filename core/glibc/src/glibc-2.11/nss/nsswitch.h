/* Copyright (C) 1996-1999,2001,2002,2003,2004,2007
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _NSSWITCH_H
#define _NSSWITCH_H	1

/* This is an *internal* header.  */

#include <arpa/nameser.h>
#include <netinet/in.h>
#include <nss.h>
#include <resolv.h>
#include <search.h>
#include <dlfcn.h>

/* Actions performed after lookup finished.  */
typedef enum
{
  NSS_ACTION_CONTINUE,
  NSS_ACTION_RETURN
} lookup_actions;


typedef struct service_library
{
  /* Name of service (`files', `dns', `nis', ...).  */
  const char *name;
  /* Pointer to the loaded shared library.  */
  void *lib_handle;
  /* And the link to the next entry.  */
  struct service_library *next;
} service_library;


/* For mapping a function name to a function pointer.  It is known in
   nsswitch.c:nss_lookup_function that a string pointer for the lookup key
   is the first member.  */
typedef struct
{
  const char *fct_name;
  void *fct_ptr;
} known_function;


typedef struct service_user
{
  /* And the link to the next entry.  */
  struct service_user *next;
  /* Action according to result.  */
  lookup_actions actions[5];
  /* Link to the underlying library object.  */
  service_library *library;
  /* Collection of known functions.  */
  void *known;
  /* Name of the service (`files', `dns', `nis', ...).  */
  char name[0];
} service_user;

/* To access the action based on the status value use this macro.  */
#define nss_next_action(ni, status) ((ni)->actions[2 + status])


typedef struct name_database_entry
{
  /* And the link to the next entry.  */
  struct name_database_entry *next;
  /* List of service to be used.  */
  service_user *service;
  /* Name of the database.  */
  char name[0];
} name_database_entry;


typedef struct name_database
{
  /* List of all known databases.  */
  name_database_entry *entry;
  /* List of libraries with service implementation.  */
  service_library *library;
} name_database;


/* Interface functions for NSS.  */

/* Get the data structure representing the specified database.
   If there is no configuration for this database in the file,
   parse a service list from DEFCONFIG and use that.  More
   than one function can use the database.  */
extern int __nss_database_lookup (const char *database,
				  const char *alternative_name,
				  const char *defconfig, service_user **ni);
libc_hidden_proto (__nss_database_lookup)

/* Put first function with name FCT_NAME for SERVICE in FCTP.  The
   position is remembered in NI.  The function returns a value < 0 if
   an error occurred or no such function exists.  */
extern int __nss_lookup (service_user **ni, const char *fct_name,
			 const char *fct2_name, void **fctp) attribute_hidden;

/* Determine the next step in the lookup process according to the
   result STATUS of the call to the last function returned by
   `__nss_lookup' or `__nss_next'.  NI specifies the last function
   examined.  The function return a value > 0 if the process should
   stop with the last result of the last function call to be the
   result of the entire lookup.  The returned value is 0 if there is
   another function to use and < 0 if an error occurred.

   If ALL_VALUES is nonzero, the return value will not be > 0 as long as
   there is a possibility the lookup process can ever use following
   services.  In other words, only if all four lookup results have
   the action RETURN associated the lookup process stops before the
   natural end.  */
extern int __nss_next2 (service_user **ni, const char *fct_name,
			const char *fct2_name, void **fctp, int status,
			int all_values) attribute_hidden;
libc_hidden_proto (__nss_next2)
extern int __nss_next (service_user **ni, const char *fct_name, void **fctp,
		       int status, int all_values);

/* Search for the service described in NI for a function named FCT_NAME
   and return a pointer to this function if successful.  */
extern void *__nss_lookup_function (service_user *ni, const char *fct_name);
libc_hidden_proto (__nss_lookup_function)


/* Called by NSCD to disable recursive calls.  */
extern void __nss_disable_nscd (void);


typedef int (*db_lookup_function) (service_user **, const char *, const char *,
				   void **)
     internal_function;
typedef enum nss_status (*setent_function) (int);
typedef enum nss_status (*endent_function) (void);
typedef enum nss_status (*getent_function) (void *, char *, size_t,
					    int *, int *);
typedef int (*getent_r_function) (void *, char *, size_t,
				  void **result, int *);

extern void __nss_setent (const char *func_name,
			  db_lookup_function lookup_fct,
			  service_user **nip, service_user **startp,
			  service_user **last_nip, int stayon,
			  int *stayon_tmp, int res);
extern void __nss_endent (const char *func_name,
			  db_lookup_function lookup_fct,
			  service_user **nip, service_user **startp,
			  service_user **last_nip, int res);
extern int __nss_getent_r (const char *getent_func_name,
			   const char *setent_func_name,
			   db_lookup_function lookup_fct,
			   service_user **nip, service_user **startp,
			   service_user **last_nip, int *stayon_tmp,
			   int res,
			   void *resbuf, char *buffer, size_t buflen,
			   void **result, int *h_errnop);
extern void *__nss_getent (getent_r_function func,
			   void **resbuf, char **buffer, size_t buflen,
			   size_t *buffer_size, int *h_errnop);
struct hostent;
extern int __nss_hostname_digits_dots (const char *name,
				       struct hostent *resbuf, char **buffer,
				       size_t *buffer_size, size_t buflen,
				       struct hostent **result,
				       enum nss_status *status, int af,
				       int *h_errnop);
libc_hidden_proto (__nss_hostname_digits_dots)

#endif	/* nsswitch.h */
