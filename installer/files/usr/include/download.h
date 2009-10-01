/*-
 * Copyright (c) 2006 Aaron Griffin <aaronmgriffin@gmail.com>
 *
 * Based on original libfetch code from:
 * Copyright (c) 1998-2004 Dag-Erling Coïdan Smørgrav
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef DOWNLOAD_H_INCLUDED
#define DOWNLOAD_H_INCLUDED

#include <stdio.h>
#include <sys/param.h> /* MAXHOSTNAMELEN */

#define USER_AGENT_STRING "libdownload/1.0"

#define URL_SCHEMELEN 16
#define URL_USERLEN 256
#define URL_PWDLEN 256

struct url {
	char    scheme[URL_SCHEMELEN+1];
	char    user[URL_USERLEN+1];
	char    pwd[URL_PWDLEN+1];
	char    host[MAXHOSTNAMELEN+1];
	int     port;
	char    *doc;
	off_t   offset;
	size_t  length;
};

struct url_stat {
	off_t   size;
	time_t  atime;
	time_t  mtime;
};

/* Recognized schemes */
#define SCHEME_FTP	    "ftp"
#define SCHEME_HTTP     "http"
#define SCHEME_HTTPS    "https"
#define SCHEME_FILE	    "file"

/* Error codes */
#define DLERR_NONE   0
#define	DLERR_ABORT     1
#define	DLERR_AUTH      2
#define	DLERR_DOWN      3
#define	DLERR_EXISTS    4
#define	DLERR_FULL      5
#define	DLERR_INFO      6
#define	DLERR_MEMORY    7
#define	DLERR_MOVED     8
#define	DLERR_NETWORK   9
#define	DLERR_OK        10
#define	DLERR_PROTO	11
#define	DLERR_RESOLV	12
#define	DLERR_SERVER	13
#define	DLERR_TEMP	14
#define	DLERR_TIMEOUT	15
#define	DLERR_UNAVAIL	16
#define	DLERR_UNKNOWN	17
#define	DLERR_URL	18
#define	DLERR_VERBOSE	19

__BEGIN_DECLS

/* FILE-specific functions */
FILE *downloadXGetFile(struct url *, struct url_stat *, const char *);
FILE *downloadGetFile(struct url *, const char *);
int downloadStatFile(struct url *, struct url_stat *, const char *);

/* HTTP-specific functions */
FILE *downloadXGetHTTP(struct url *, struct url_stat *, const char *);
FILE *downloadGetHTTP(struct url *, const char *);
int downloadStatHTTP(struct url *, struct url_stat *, const char *);

/* FTP-specific functions */
FILE *downloadXGetFTP(struct url *, struct url_stat *, const char *);
FILE *downloadGetFTP(struct url *, const char *);
int downloadStatFTP(struct url *, struct url_stat *, const char *);

/* Generic functions */
FILE *downloadXGetURL(const char *, struct url_stat *, const char *);
FILE *downloadGetURL(const char *, const char *);
int downloadStatURL(const char *, struct url_stat *, const char *);
FILE *downloadXGet(struct url *, struct url_stat *, const char *);
FILE *downloadGet(struct url *, const char *);
int downloadStat(struct url *, struct url_stat *, const char *);

/* URL parsing */
struct url *downloadMakeURL(const char *, const char *, int, const char *,
                            const char *, const char *);
struct url *downloadParseURL(const char *);
void downloadFreeURL(struct url *);

__END_DECLS

/* Authentication */
typedef int (*auth_t)(struct url *);
extern auth_t downloadAuthMethod;

/* Last error code */
extern int downloadLastErrCode;
#define MAXERRSTRING 256
extern char downloadLastErrString[MAXERRSTRING];

/* I/O timeout */
extern int downloadTimeout;

/* Restart interrupted syscalls */
extern int downloadRestartCalls;

/* Extra verbosity */
extern int downloadDebug;

#endif /*DOWNLOAD_H_INCLUDED*/
