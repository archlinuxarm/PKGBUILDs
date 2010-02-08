#ifndef _RPC_AUTH_DES_H

#include <sunrpc/rpc/auth_des.h>

libc_hidden_proto (getpublickey)
libc_hidden_proto (rtime)

extern bool_t xdr_authdes_cred (XDR *xdrs, struct authdes_cred *cred);
extern bool_t xdr_authdes_verf (register XDR *xdrs,
				register struct authdes_verf *verf);
struct svc_req;
struct rpc_msg;
extern enum auth_stat _svcauth_des (register struct svc_req *rqst,
				    register struct rpc_msg *msg);


#define DECLARE_NSS_PROTOTYPES(service)					\
extern enum nss_status _nss_ ## service ## _getpublickey		\
                       (const char *netname, char *pkey, int *errnop);	\
extern enum nss_status _nss_ ## service ## _getsecretkey		\
                       (const char *netname, char *skey, char *passwd,	\
			int *errnop);

DECLARE_NSS_PROTOTYPES (files)
DECLARE_NSS_PROTOTYPES (nis)
DECLARE_NSS_PROTOTYPES (nisplus)

#undef DECLARE_NSS_PROTOTYPES

#endif
