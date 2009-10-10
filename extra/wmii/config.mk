# Customized for Arch Linux <http://archlinux.org/>

# paths
PREFIX = /usr
CONFPREFIX = /etc
MANPREFIX = ${PREFIX}/man

X11INC = /usr/include/X11
X11LIB = /usr/lib

VERSION = 3.5.1

# includes and libs
LIBS = -L${PREFIX}/lib -L/usr/include -lc -L${X11LIB} -lX11 -lixp -lm

# Linux/BSD
CFLAGS = -O2 -I. -I${PREFIX}/include -I/usr/include -I${X11INC} \
	-DVERSION=\"${VERSION}\"
LDFLAGS = ${LIBS}

AR = ar cr
CC = cc
LD = ${CC}
RANLIB = ranlib
