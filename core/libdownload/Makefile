prefix = /opt
DESTDIR =
DEBUG = false
ENABLE_HTTPS = true

CFLAGS   = -O2 -pipe -I. -DINET6 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
WARNINGS = -Wall -Wstrict-prototypes -Wsign-compare -Wchar-subscripts \
           -Wpointer-arith -Wcast-align -Wsign-compare
CFLAGS  += $(WARNINGS)

ifeq ($(strip $(DEBUG)), true)
CFLAGS += -g -DDEBUG
else
CFLAGS += -UDEBUG
endif

ifeq ($(strip $(ENALE_HTTPS)),true)
CFLAGS  += -DWITH_SSL
LDFLAGS += -lssl -lcrypto
endif

CC = gcc
LD = gcc
AR = ar
RANLIB = ranlib
INSTALL = install -c -D

OBJS= download.o common.o ftp.o http.o file.o
INCS= download.h common.h
GEN = ftperr.h httperr.h
MAN = libdownload.3

#pretty print!
E = @/bin/echo
Q = @

all: libdownload.so libdownload.a
	$(E) "  built with:  " $(CFLAGS)
.PHONY: all

%.o: %.c $(INCS) $(GEN)
	$(E) "  compile   " $@
	$(Q) $(CC) $(CFLAGS) -c $<

ftperr.h: ftp.errors
	$(E) "  generate  " $@
	@/bin/echo "static struct downloaderr _ftp_errlist[] = {" > $@
	@cat $< \
	  | grep -v ^# \
	  | sort \
	  | while read NUM CAT STRING; do \
	    /bin/echo "    { $${NUM}, DLERR_$${CAT}, \"$${STRING}\" },"; \
	  done >> $@
	@/bin/echo -e "    { -1, DLERR_UNKNOWN, \"Unknown FTP error\" }\n};" >> $@

httperr.h: http.errors
	$(E) "  generate  " $@
	@/bin/echo "static struct downloaderr _http_errlist[] = {" > $@
	@cat $< \
	  | grep -v ^# \
	  | sort \
	  | while read NUM CAT STRING; do \
	    /bin/echo "    { $${NUM}, DLERR_$${CAT}, \"$${STRING}\" },"; \
	  done >> $@
	@/bin/echo -e "    { -1, DLERR_UNKNOWN, \"Unknown HTTP error\" }\n};" >> $@

libdownload.so: $(GEN) $(INCS) $(OBJS)
	$(E) "  build     " $@
	$(Q) rm -f $@
	$(Q) $(LD) $(LDFLAGS) *.o -shared -o $@

libdownload.a: $(GEN) $(INCS) $(OBJS)
	$(E) "  build     " $@
	$(Q) rm -f $@
	$(Q) $(AR) rcs $@ *.o
	$(Q) $(RANLIB) $@

clean:
	$(E) "  clean     "
	$(Q) rm -f libdownload.so libdownload.a *.o $(GEN)
.PHONY: clean

install: all
	$(Q) $(INSTALL) -m 755 libdownload.so $(DESTDIR)$(prefix)/lib/libdownload.so
	$(Q) $(INSTALL) -m 644 libdownload.a $(DESTDIR)$(prefix)/lib/libdownload.a
	$(Q) $(INSTALL) -m 644 download.h $(DESTDIR)$(prefix)/include/download.h
	$(Q) $(INSTALL) -m 644 libdownload.3 $(DESTDIR)$(prefix)/man/man3/libdownload.3
.PHONY: install

uninstall:
	$(Q) rm -f $(DESTDIR)$(prefix)/lib/libdownload.so
	$(Q) rm -f $(DESTDIR)$(prefix)/lib/libdownload.a
	$(Q) rm -f $(DESTDIR)$(prefix)/include/download.h
	$(Q) rm -f $(DESTDIR)$(prefix)/man/man3/libdownload.3
.PHONY: uninstall
