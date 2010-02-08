#ifndef	_NETDB_H
#include <resolv/netdb.h>

/* Macros for accessing h_errno from inside libc.  */
# ifdef _LIBC_REENTRANT
#  include <tls.h>
#  if USE___THREAD
#   undef  h_errno
#   ifndef NOT_IN_libc
#    define h_errno __libc_h_errno
#   else
#    define h_errno h_errno	/* For #ifndef h_errno tests.  */
#   endif
extern __thread int h_errno attribute_tls_model_ie;
#   define __set_h_errno(x)	(h_errno = (x))
#  else
static inline int
__set_h_errno (int __err)
{
  return *__h_errno_location () = __err;
}
#  endif
# else
#  undef  h_errno
#  define __set_h_errno(x) (h_errno = (x))
extern int h_errno;
# endif	/* _LIBC_REENTRANT */

libc_hidden_proto (hstrerror)
libc_hidden_proto (innetgr)
libc_hidden_proto (rcmd_af)
libc_hidden_proto (rexec_af)
libc_hidden_proto (rresvport_af)
libc_hidden_proto (ruserok_af)

libc_hidden_proto (getaddrinfo)
libc_hidden_proto (getnameinfo)
libc_hidden_proto (freeaddrinfo)
libc_hidden_proto (gai_strerror)
libc_hidden_proto (__h_errno_location)

/* Document internal interfaces.  */
extern int __gethostent_r (struct hostent *__restrict __result_buf,
			   char *__restrict __buf, size_t __buflen,
			   struct hostent **__restrict __result,
			   int *__restrict __h_errnop) attribute_hidden;
extern int __old_gethostent_r (struct hostent *__restrict __result_buf,
			       char *__restrict __buf, size_t __buflen,
			       struct hostent **__restrict __result,
			       int *__restrict __h_errnop);

extern int __gethostbyaddr_r (__const void *__restrict __addr,
			      socklen_t __len, int __type,
			      struct hostent *__restrict __result_buf,
			      char *__restrict __buf, size_t __buflen,
			      struct hostent **__restrict __result,
			      int *__restrict __h_errnop);
extern int __old_gethostbyaddr_r (__const void *__restrict __addr,
				  socklen_t __len, int __type,
				  struct hostent *__restrict __result_buf,
				  char *__restrict __buf, size_t __buflen,
				  struct hostent **__restrict __result,
				  int *__restrict __h_errnop);

extern int __gethostbyaddr2_r (__const void *__restrict __addr,
			       socklen_t __len, int __type,
			       struct hostent *__restrict __result_buf,
			       char *__restrict __buf, size_t __buflen,
			       struct hostent **__restrict __result,
			       int *__restrict __h_errnop,
			       int32_t *ttlp);

extern int __gethostbyname_r (__const char *__restrict __name,
			      struct hostent *__restrict __result_buf,
			      char *__restrict __buf, size_t __buflen,
			      struct hostent **__restrict __result,
			      int *__restrict __h_errnop);
extern int __old_gethostbyname_r (__const char *__restrict __name,
				  struct hostent *__restrict __result_buf,
				  char *__restrict __buf, size_t __buflen,
				  struct hostent **__restrict __result,
				  int *__restrict __h_errnop);

extern int __gethostbyname2_r (__const char *__restrict __name, int __af,
			       struct hostent *__restrict __result_buf,
			       char *__restrict __buf, size_t __buflen,
			       struct hostent **__restrict __result,
			       int *__restrict __h_errnop);
extern int __old_gethostbyname2_r (__const char *__restrict __name, int __af,
				   struct hostent *__restrict __result_buf,
				   char *__restrict __buf, size_t __buflen,
				   struct hostent **__restrict __result,
				   int *__restrict __h_errnop);

extern int __gethostbyname3_r (__const char *__restrict __name, int __af,
			       struct hostent *__restrict __result_buf,
			       char *__restrict __buf, size_t __buflen,
			       struct hostent **__restrict __result,
			       int *__restrict __h_errnop,
			       int32_t *ttlp, char **canonp);

extern int __getnetent_r (struct netent *__restrict __result_buf,
			  char *__restrict __buf, size_t __buflen,
			  struct netent **__restrict __result,
			  int *__restrict __h_errnop) attribute_hidden;
extern int __old_getnetent_r (struct netent *__restrict __result_buf,
			      char *__restrict __buf, size_t __buflen,
			      struct netent **__restrict __result,
			      int *__restrict __h_errnop);

extern int __getnetbyaddr_r (uint32_t __net, int __type,
			     struct netent *__restrict __result_buf,
			     char *__restrict __buf, size_t __buflen,
			     struct netent **__restrict __result,
			     int *__restrict __h_errnop);
extern int __old_getnetbyaddr_r (uint32_t __net, int __type,
				 struct netent *__restrict __result_buf,
				 char *__restrict __buf, size_t __buflen,
				 struct netent **__restrict __result,
				 int *__restrict __h_errnop);

extern int __getnetbyname_r (__const char *__restrict __name,
			     struct netent *__restrict __result_buf,
			     char *__restrict __buf, size_t __buflen,
			     struct netent **__restrict __result,
			     int *__restrict __h_errnop);
extern int __old_getnetbyname_r (__const char *__restrict __name,
				 struct netent *__restrict __result_buf,
				 char *__restrict __buf, size_t __buflen,
				 struct netent **__restrict __result,
				 int *__restrict __h_errnop);

extern int __getservent_r (struct servent *__restrict __result_buf,
			   char *__restrict __buf, size_t __buflen,
			   struct servent **__restrict __result)
     attribute_hidden;
extern int __old_getservent_r (struct servent *__restrict __result_buf,
			       char *__restrict __buf, size_t __buflen,
			       struct servent **__restrict __result);

extern int __getservbyname_r (__const char *__restrict __name,
			      __const char *__restrict __proto,
			      struct servent *__restrict __result_buf,
			      char *__restrict __buf, size_t __buflen,
			      struct servent **__restrict __result);
extern int __old_getservbyname_r (__const char *__restrict __name,
				  __const char *__restrict __proto,
				  struct servent *__restrict __result_buf,
				  char *__restrict __buf, size_t __buflen,
				  struct servent **__restrict __result);

extern int __getservbyport_r (int __port,
			      __const char *__restrict __proto,
			      struct servent *__restrict __result_buf,
			      char *__restrict __buf, size_t __buflen,
			      struct servent **__restrict __result);
extern int __old_getservbyport_r (int __port,
				  __const char *__restrict __proto,
				  struct servent *__restrict __result_buf,
				  char *__restrict __buf, size_t __buflen,
				  struct servent **__restrict __result);

extern int __getprotoent_r (struct protoent *__restrict __result_buf,
			    char *__restrict __buf, size_t __buflen,
			    struct protoent **__restrict __result)
     attribute_hidden;
extern int __old_getprotoent_r (struct protoent *__restrict __result_buf,
				char *__restrict __buf, size_t __buflen,
				struct protoent **__restrict __result);

extern int __getprotobyname_r (__const char *__restrict __name,
			       struct protoent *__restrict __result_buf,
			       char *__restrict __buf, size_t __buflen,
			       struct protoent **__restrict __result);
extern int __old_getprotobyname_r (__const char *__restrict __name,
				   struct protoent *__restrict __result_buf,
				   char *__restrict __buf, size_t __buflen,
				   struct protoent **__restrict __result);

extern int __getprotobynumber_r (int __proto,
				 struct protoent *__restrict __res_buf,
				 char *__restrict __buf, size_t __buflen,
				 struct protoent **__restrict __result);
extern int __old_getprotobynumber_r (int __proto,
				     struct protoent *__restrict __res_buf,
				     char *__restrict __buf, size_t __buflen,
				     struct protoent **__restrict __result);

extern int __getnetgrent_r (char **__restrict __hostp,
			    char **__restrict __userp,
			    char **__restrict __domainp,
			    char *__restrict __buffer, size_t __buflen);

extern int ruserpass (const char *host, const char **aname,
		      const char **apass);
libc_hidden_proto (ruserpass)


/* The following definition has been removed from the public header
   since we don't want people to use them.  */

#define AI_DEFAULT    (AI_V4MAPPED | AI_ADDRCONFIG)

#include <inet/netgroup.h>

struct parser_data;
extern int _nss_files_parse_protoent (char *line, struct protoent *result,
				      struct parser_data *data,
				      size_t datalen, int *errnop);
extern int _nss_files_parse_servent (char *line, struct servent *result,
				     struct parser_data *data,
				     size_t datalen, int *errnop);
extern int _nss_files_parse_netent (char *line, struct netent *result,
				    struct parser_data *data,
				    size_t datalen, int *errnop);
extern enum nss_status _nss_netgroup_parseline (char **cursor,
						struct __netgrent *result,
						char *buffer, size_t buflen,
						int *errnop);
libnss_files_hidden_proto (_nss_files_parse_protoent)
libnss_files_hidden_proto (_nss_files_parse_servent)
libnss_files_hidden_proto (_nss_files_parse_netent)
libnss_files_hidden_proto (_nss_netgroup_parseline)

#define DECLARE_NSS_PROTOTYPES(service)					      \
extern enum nss_status _nss_ ## service ## _setprotoent (int);		      \
extern enum nss_status _nss_ ## service ## _endprotoent (void);		      \
extern enum nss_status _nss_ ## service ## _getprotoent_r		      \
		       (struct protoent *proto, char *buffer, size_t buflen,  \
			int *errnop);					      \
extern enum nss_status _nss_ ## service ## _getprotobyname_r		      \
		       (const char *name, struct protoent *proto,	      \
			char *buffer, size_t buflen, int *errnop);	      \
extern enum nss_status _nss_ ## service ## _getprotobynumber_r		      \
		       (int number, struct protoent *proto,		      \
			char *buffer, size_t buflen, int *errnop);	      \
extern enum nss_status _nss_ ## service ## _sethostent (int);		      \
extern enum nss_status _nss_ ## service ## _endhostent (void);		      \
extern enum nss_status _nss_ ## service ## _gethostent_r		      \
		       (struct hostent *host, char *buffer, size_t buflen,    \
			int *errnop, int *h_errnop);			      \
extern enum nss_status _nss_ ## service ## _gethostbyname2_r		      \
		       (const char *name, int af, struct hostent *host,	      \
			char *buffer, size_t buflen, int *errnop,	      \
			int *h_errnop);					      \
extern enum nss_status _nss_ ## service ## _gethostbyname_r		      \
		       (const char *name, struct hostent *host, char *buffer, \
			size_t buflen, int *errnop, int *h_errnop);	      \
extern enum nss_status _nss_ ## service ## _gethostbyaddr_r		      \
		       (const void *addr, socklen_t addrlen, int af,	      \
			struct hostent *host, char *buffer, size_t buflen,    \
			int *errnop, int *h_errnop);			      \
extern enum nss_status _nss_ ## service ## _setservent (int);		      \
extern enum nss_status _nss_ ## service ## _endservent (void);		      \
extern enum nss_status _nss_ ## service ## _getservent_r		      \
		       (struct servent *serv, char *buffer, size_t buflen,    \
			int *errnop);					      \
extern enum nss_status _nss_ ## service ## _getservbyname_r		      \
		       (const char *name, const char *protocol,		      \
			struct servent *serv, char *buffer, size_t buflen,    \
			int *errnop);					      \
extern enum nss_status _nss_ ## service ## _getservbyport_r		      \
		       (int port, const char *protocol, struct servent *serv, \
			char *buffer, size_t buflen, int *errnop);	      \
extern enum nss_status _nss_ ## service ## _setnetgrent			      \
		       (const char *group, struct __netgrent *result);	      \
extern enum nss_status _nss_ ## service ## _endnetgrent			      \
		       (struct __netgrent *result);			      \
extern enum nss_status _nss_ ## service ## _getnetgrent_r		      \
		       (struct __netgrent *result, char *buffer,	      \
			size_t buflen, int *errnop);			      \
extern enum nss_status _nss_ ## service ## _setnetent (int stayopen);	      \
extern enum nss_status _nss_ ## service ## _endnetent (void);		      \
extern enum nss_status _nss_ ## service ## _getnetent_r			      \
			(struct netent *net, char *buffer, size_t buflen,     \
			 int *errnop, int *herrnop);			      \
extern enum nss_status _nss_ ## service ## _getnetbyname_r		      \
			(const char *name, struct netent *net, char *buffer,  \
			 size_t buflen, int *errnop, int *herrnop);	      \
extern enum nss_status _nss_ ## service ## _getnetbyaddr_r		      \
		       (uint32_t addr, int type, struct netent *net,	      \
			char *buffer, size_t buflen, int *errnop,	      \
			int *herrnop);

DECLARE_NSS_PROTOTYPES (compat)
DECLARE_NSS_PROTOTYPES (dns)
DECLARE_NSS_PROTOTYPES (files)
DECLARE_NSS_PROTOTYPES (hesiod)
DECLARE_NSS_PROTOTYPES (nis)
DECLARE_NSS_PROTOTYPES (nisplus)

#undef DECLARE_NSS_PROTOTYPES

#endif /* !_NETDB_H */
