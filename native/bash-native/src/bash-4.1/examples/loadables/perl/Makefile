#
# Makefile for builtin perl interpreter
#
#
#   Copyright (C) 1998 Free Software Foundation, Inc.     
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Include some boilerplate Gnu makefile definitions.
prefix = /usr/arm-softfloat-linux-gnueabi
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
infodir = ${datarootdir}/info
includedir = ${prefix}/include

datarootdir = ${prefix}/share

topdir = ../../..
BUILD_DIR = /build/plugapps/native/bash-native/src/bash-4.1
srcdir = .
VPATH = .


CC = arm-none-linux-gnueabi-gcc
RM = rm -f

SHELL = /bin/sh

PERL5 = perl5

CFLAGS = -march=armv5te -O2 -pipe

#
# These values are generated for configure by ${topdir}/support/shobj-conf.
# If your system is not supported by that script, but includes facilities for
# dynamic loading of shared objects, please update the script and send the
# changes to bash-maintainers@gnu.org.
#
SHOBJ_CC = arm-none-linux-gnueabi-gcc
SHOBJ_CFLAGS = -fPIC
SHOBJ_LD = ${CC}
SHOBJ_LDFLAGS = -shared -Wl,-soname,$@
SHOBJ_XLDFLAGS = 
SHOBJ_LIBS = 
SHOBJ_STATUS = supported

# Values used for compiling the perl files
PERL_LDOPTS = `${PERL5} -MExtUtils::Embed -e ldopts`
PERL_CFLAGS = ${CCFLAGS} `${PERL5} -MExtUtils::Embed -e ccopts`

SRC = bperl.c iperl.c perlxsi.c
OBJ = bperl.o iperl.o perlxsi.o

BUILTIN = bperl5

INC = -I. -I.. -I$(topdir) -I$(topdir)/lib -I$(topdir)/builtins \
      -I$(topdir)/include -I$(BUILD_DIR) -I$(BUILD_DIR)/lib \
      -I$(BUILD_DIR)/builtins


${BUILTIN}:	${OBJ}
	${RM} $@
	${SHOBJ_LD} ${SHOBJ_LDFLAGS} ${SHOBJ_XLDFLAGS} -o $@ ${OBJ} ${PERL_LDOPTS} ${SHOBJ_LIBS}

bperl.o:	bperl.c
	${RM} $@
	$(SHOBJ_CC) $(SHOBJ_CFLAGS) $(CFLAGS) $(INC) -c -o $@ ${srcdir}/bperl.c

iperl.o:	iperl.c
	${RM} $@
	$(SHOBJ_CC) ${SHOBJ_CFLAGS} $(PERL_CFLAGS) -c -o $@ ${srcdir}/iperl.c

perlxsi.c:
	${PERL5} -MExtUtils::Embed -e xsinit -- -o $@

perlxsi.o:	perlxsi.c
	${RM} $@
	${SHOBJ_CC} ${SHOBJ_CFLAGS} $(PERL_CFLAGS) -c -o $@ perlxsi.c

clean mostlyclean:
	${RM} ${OBJ}
	${RM} ${BUILTIN}

distclean maintainer-clean: clean
	${RM} perlxsi.c
