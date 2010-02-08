#ifndef _PWD_H
#include <pwd/pwd.h>

/* Now define the internal interfaces.  */
extern int __getpwent_r (struct passwd *__resultbuf, char *__buffer,
			 size_t __buflen, struct passwd **__result)
     attribute_hidden;
extern int __old_getpwent_r (struct passwd *__resultbuf, char *__buffer,
			     size_t __buflen, struct passwd **__result);
extern int __getpwuid_r (__uid_t __uid, struct passwd *__resultbuf,
			 char *__buffer, size_t __buflen,
			 struct passwd **__result);
extern int __old_getpwuid_r (__uid_t __uid, struct passwd *__resultbuf,
			     char *__buffer, size_t __buflen,
			     struct passwd **__result);
extern int __getpwnam_r (__const char *__name, struct passwd *__resultbuf,
			 char *__buffer, size_t __buflen,
			 struct passwd **__result);
extern int __old_getpwnam_r (__const char *__name, struct passwd *__resultbuf,
			     char *__buffer, size_t __buflen,
			     struct passwd **__result);
extern int __fgetpwent_r (FILE * __stream, struct passwd *__resultbuf,
			  char *__buffer, size_t __buflen,
			  struct passwd **__result);

#include <nss/nss.h>

struct parser_data;
extern int _nss_files_parse_pwent (char *line, struct passwd *result,
				   struct parser_data *data,
				   size_t datalen, int *errnop);
libc_hidden_proto (_nss_files_parse_pwent)

#define DECLARE_NSS_PROTOTYPES(service)					\
extern enum nss_status _nss_ ## service ## _setpwent (int);		\
extern enum nss_status _nss_ ## service ## _endpwent (void);		\
extern enum nss_status _nss_ ## service ## _getpwnam_r			\
                       (const char *name, struct passwd *pwd,		\
			char *buffer, size_t buflen, int *errnop);	\
extern enum nss_status _nss_ ## service ## _getpwuid_r			\
                       (uid_t uid, struct passwd *pwd,			\
			char *buffer, size_t buflen, int *errnop);	\
extern enum nss_status _nss_ ## service ##_getpwent_r			\
                       (struct passwd *result, char *buffer,		\
			size_t buflen, int *errnop);

DECLARE_NSS_PROTOTYPES (compat)
DECLARE_NSS_PROTOTYPES (files)
DECLARE_NSS_PROTOTYPES (hesiod)
DECLARE_NSS_PROTOTYPES (nis)
DECLARE_NSS_PROTOTYPES (nisplus)

#undef DECLARE_NSS_PROTOTYPES

#endif
