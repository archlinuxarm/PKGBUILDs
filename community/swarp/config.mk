# Customized for Arch Linux <http://archlinux.org>

VERSION = %VERSION%
PREFIX = /usr
MANPREFIX = ${PREFIX}/man
X11INC = ${PREFIX}/include/X11
X11LIB = ${PREFIX}/lib/X11
INCS = -I. -I${PREFIX}/include -I${X11INC}
LIBS = -L${PREFIX}/lib -lc -L${X11LIB} -lX11
CFLAGS = %CFLAGS% ${INCS} -DVERSION=\"${VERSION}\"
LDFLAGS = ${LIBS}
CC = cc
LD = ${CC}
