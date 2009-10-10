CFLAGS+=-fPIC -Iinclude
LDFLAGS+=-lpng -lGL
SHAREDLIBFLAGS=-shared
DESTDIR=/usr/local
LIB=lib

all: libglpng.a libglpng.so.1.45

libglpng.a: glpng.o
	ar rv $@ $<

libglpng.so.1.45: glpng.o
	gcc $(CFLAGS) $(SHAREDLIBFLAGS) -Wl,-soname=libglpng.so.1 -Wl,--whole-archive $< -Wl,--no-whole-archive $(LDFLAGS) -o $@

glpng.o: src/glpng.c
	gcc $(CFLAGS) -c $<

clean:
	rm glpng.o libglpng.*

install:
	for i in include include/GL $(LIB); do \
		install -m 755 -d $(DESTDIR)/$$i; \
	done
	install -p -m 644 include/GL/glpng.h $(DESTDIR)/include/GL
	install -m 755 libglpng.* $(DESTDIR)/$(LIB)
	ln -s libglpng.so.1.45 $(DESTDIR)/$(LIB)/libglpng.so.1
	ln -s libglpng.so.1.45 $(DESTDIR)/$(LIB)/libglpng.so

.PHONY: clean install
