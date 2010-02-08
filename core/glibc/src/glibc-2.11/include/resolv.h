#ifndef _RESOLV_H_

#define RES_SET_H_ERRNO(r,x)			\
  do						\
    {						\
      (r)->res_h_errno = x;			\
      __set_h_errno(x);				\
    }						\
  while (0)

#include <stdbool.h>
#include <resolv/resolv.h>

#ifdef _RESOLV_H_

# ifdef _LIBC_REENTRANT
#  include <tls.h>
#  if USE___THREAD
#   undef _res
#   ifndef NOT_IN_libc
#    define __resp __libc_resp
#   endif
#   define _res (*__resp)
extern __thread struct __res_state *__resp attribute_tls_model_ie;
#  endif
# else
#  ifndef __BIND_NOSTATIC
#   undef _res
extern struct __res_state _res;
#  endif
# endif

/* Now define the internal interfaces.  */
extern int __res_vinit (res_state, int);
extern int __res_maybe_init (res_state, int);
extern void _sethtent (int);
extern void _endhtent (void);
extern struct hostent *_gethtent (void);
extern struct hostent *_gethtbyname (const char *__name);
extern struct hostent *_gethtbyname2 (const char *__name, int __af);
struct hostent *_gethtbyaddr (const char *addr, size_t __len, int __af);
extern u_int32_t _getlong (const u_char *__src);
extern u_int16_t _getshort (const u_char *__src);
extern void res_pquery (const res_state __statp, const u_char *__msg,
			int __len, FILE *__file);
extern void res_send_setqhook (res_send_qhook __hook);
extern void res_send_setrhook (res_send_rhook __hook);
extern int res_ourserver_p (const res_state __statp,
			    const struct sockaddr_in6 *__inp);
extern void __res_iclose (res_state statp, bool free_addr);
extern int __res_nopt(res_state statp, int n0, u_char *buf, int buflen,
		      int anslen);
libc_hidden_proto (__res_ninit)
libc_hidden_proto (__res_maybe_init)
libc_hidden_proto (__res_nclose)
libc_hidden_proto (__res_iclose)
libc_hidden_proto (__res_randomid)
libc_hidden_proto (__res_state)

int __libc_res_nquery (res_state, const char *, int, int, u_char *, int,
		       u_char **, u_char **, int *, int *);
int __libc_res_nsearch (res_state, const char *, int, int, u_char *, int,
			u_char **, u_char **, int *, int *);
int __libc_res_nsend (res_state, const u_char *, int, const u_char *, int,
		      u_char *, int, u_char **, u_char **, int *, int *)
  attribute_hidden;

libresolv_hidden_proto (_sethtent)
libresolv_hidden_proto (_gethtent)
libresolv_hidden_proto (_gethtbyaddr)
libresolv_hidden_proto (_gethtbyname2)
libresolv_hidden_proto (__dn_expand)
libresolv_hidden_proto (__dn_comp)
libresolv_hidden_proto (__dn_skipname)
libresolv_hidden_proto (__res_hnok)
libresolv_hidden_proto (__res_dnok)
libresolv_hidden_proto (__putlong)
libresolv_hidden_proto (__putshort)
libresolv_hidden_proto (__p_cdnname)
libresolv_hidden_proto (__p_fqnname)
libresolv_hidden_proto (__p_option)
libresolv_hidden_proto (__sym_ntos)
libresolv_hidden_proto (__p_rcode)
libresolv_hidden_proto (__p_class)
libresolv_hidden_proto (__p_type)
libresolv_hidden_proto (__loc_ntoa)
libresolv_hidden_proto (__fp_nquery)
libresolv_hidden_proto (__fp_query)
libresolv_hidden_proto (__hostalias)
libresolv_hidden_proto (__res_nmkquery)
libresolv_hidden_proto (__libc_res_nquery)
libresolv_hidden_proto (__res_nquery)
libresolv_hidden_proto (__res_nquerydomain)
libresolv_hidden_proto (__res_hostalias)
libresolv_hidden_proto (__libc_res_nsearch)
libresolv_hidden_proto (__res_nsearch)
libresolv_hidden_proto (__res_nameinquery)
libresolv_hidden_proto (__res_queriesmatch)
libresolv_hidden_proto (__res_nsend)
libresolv_hidden_proto (__b64_ntop)
libresolv_hidden_proto (__res_nopt)
libresolv_hidden_proto (__dn_count_labels)
libresolv_hidden_proto (__p_secstodate)

extern const char *_res_opcodes[];
libresolv_hidden_proto (_res_opcodes)

#endif

#endif
