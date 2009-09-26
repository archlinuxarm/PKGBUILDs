# Makefile based on BSD make.
# Our mk stubs also work with GNU make.
# Copyright (c) 2008 Roy Marples <roy@marples.name>

PROG=		dhcpcd
SRCS=		arp.c bind.c common.c control.c dhcp.c dhcpcd.c duid.c eloop.c
SRCS+=		if-options.c if-pref.c ipv4ll.c net.c signals.c
SRCS+=		configure.c
SRCS+=		${SRC_IF} ${SRC_PF}

LIBEXECDIR?=	${PREFIX}/opt/lib/dhcpcd/
SCRIPT=		${LIBEXECDIR}/dhcpcd-run-hooks
HOOKDIR=	${LIBEXECDIR}/dhcpcd-hooks

BINDIR=		${PREFIX}/sbin
DBDIR?=		/opt/var/db
SYSCONFDIR?=	${PREFIX}/opt/etc

MAN=		dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8
CLEANFILES=	dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8

SCRIPTS=	dhcpcd-run-hooks
SCRIPTSDIR=	${LIBEXECDIR}
CLEANFILES+=	dhcpcd-run-hooks

FILES=		dhcpcd.conf
FILESDIR=	${SYSCONFDIR}

CPPFLAGS+=	-DDBDIR=\"${DBDIR}\"
CPPFLAGS+=	-DSCRIPT=\"${SCRIPT}\"
CPPFLAGS+=	-DSYSCONFDIR=\"${SYSCONFDIR}\"
LDADD+=		${LIBRT}

SUBDIRS=	dhcpcd-hooks

.SUFFIXES:	.in

SED_DBDIR=	-e 's:@DBDIR@:${DBDIR}:g'
SED_HOOKDIR=	-e 's:@HOOKDIR@:${HOOKDIR}:g'
SED_SCRIPT=	-e 's:@SCRIPT@:${SCRIPT}:g'
SED_SYS=	-e 's:@SYSCONFDIR@:${SYSCONFDIR}:g'

.in:
	${SED} ${SED_DBDIR} ${SED_HOOKDIR} ${SED_SCRIPT} ${SED_SYS} $< > $@

MK=		mk
include ${MK}/sys.mk
include ${MK}/os.mk
include ${MK}/prog.mk
