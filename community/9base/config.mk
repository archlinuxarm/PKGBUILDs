# Customize to fit your system

OBJTYPE     = x86_64

# paths
PREFIX      = /opt/plan9
MANPREFIX   = ${PREFIX}/man

# flags
VERSION     = 4
CFLAGS      = -c -I. -DPREFIX="\"${PREFIX}\"" %CFLAGS%
LDFLAGS     = -static %LDFLAGS%

# compiler
AR          = ar rc
CC          = cc
YACC        = ../yacc/9yacc
