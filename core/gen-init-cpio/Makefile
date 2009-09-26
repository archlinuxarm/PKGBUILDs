
DESTDIR =
PREFIX = /

MKDIR = /bin/mkdir
INSTALL = /bin/install -c -m 755

CC   = /usr/bin/gcc 
LD   = /usr/bin/gcc

CFLAGS += -Wall -Wstrict-prototypes -Wsign-compare -Wchar-subscripts \
           -Wpointer-arith -Wcast-align -Wsign-compare

#pretty print!
E = @echo
Q = @

all: gen_init_cpio
.PHONY: all
.DEFAULT: all

%.o: %.c
	$(E) "  compile " $@
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

gen_init_cpio: gen_init_cpio.o
	$(E) ">>build   " $@
	$(Q) $(LD) $(LDFLAGS) $@.o -o $@ $(LIB_OBJS)

clean:
	$(E) "  clean   "
	$(Q) rm -f gen_init_cpio *.o
.PHONY: clean

install: all
	$(MKDIR) -p $(DESTDIR)$(PREFIX)sbin/
	$(INSTALL) gen_init_cpio $(DESTDIR)$(PREFIX)sbin/
.PHONY: install

uninstall:
	rm $(DESTDIR)$(PREFIX)sbin/gen_init_cpio
.PHONY: uninstall
