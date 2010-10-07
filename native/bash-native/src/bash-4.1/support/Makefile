#
# Simple Makefile for the support programs.
#
# documentation support: man2html
# testing support: printenv recho zecho xcase
#
# bashbug lives here but is created by the top-level makefile
#
# Currently only man2html is built
#
# Copyright (C) 1998-2009 Free Software Foundation, Inc.     

#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#
# Boilerplate
#
topdir = ..
srcdir = .
VPATH = .
BUILD_DIR = /build/plugapps/native/bash-native/src/bash-4.1

RM	= rm -f
SHELL	= /bin/sh
CC = arm-none-linux-gnueabi-gcc
CC_FOR_BUILD = $(CC)

EXEEXT = 

#
# Compiler options:
#
PROFILE_FLAGS = 

CFLAGS = -march=armv5te -O2 -pipe
CFLAGS_FOR_BUILD = -g
CPPFLAGS = 
CPPFLAGS_FOR_BUILD = 
LOCAL_CFLAGS = 
DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = -DSHELL

LIBS = -ldl 
LIBS_FOR_BUILD = ${LIBS}		# XXX

LOCAL_LDFLAGS = -rdynamic
LDFLAGS = -Wl,--hash-style=gnu -Wl,--as-needed $(LOCAL_LDFLAGS) $(CFLAGS)
LDFLAGS_FOR_BUILD = $(LDFLAGS) $(LOCAL_LDFLAGS) $(CFLAGS_FOR_BUILD)

INCLUDES = -I${BUILD_DIR} -I${topdir}

BASE_CCFLAGS = ${PROFILE_FLAGS} $(DEFS) $(LOCAL_DEFS) $(SYSTEM_FLAGS) \
	  ${INCLUDES} $(LOCAL_CFLAGS)

CCFLAGS = $(BASE_CCFLAGS) $(CPPFLAGS) $(CFLAGS)
CCFLAGS_FOR_BUILD = $(BASE_CCFLAGS) $(CPPFLAGS_FOR_BUILD) $(CFLAGS_FOR_BUILD)

SRC1 =	man2html.c
OBJ1 =	man2html.o

.c.o:
	$(RM) $@
	$(CC_FOR_BUILD) -c $(CCFLAGS_FOR_BUILD) $<

all: man2html$(EXEEXT)

man2html$(EXEEXT): $(OBJ1)
	$(CC_FOR_BUILD) $(CCFLAGS_FOR_BUILD) $(OBJ1) -o $@ ${LIBS_FOR_BUILD}

clean:
	$(RM) man2html$(EXEEXT)

distclean maintainer-clean mostlyclean: clean
	$(RM) $(OBJ1)

man2html.o: man2html.c
