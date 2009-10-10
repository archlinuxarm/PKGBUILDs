# printoxx makefile 
# use $PREFIX if defined, else assume /usr/local
# replace CFLAGS and LFLAGS -O (oh) with -g for GDB debugging

PROGRAM = printoxx
VERSION = 2.0.2

ifeq "$(PREFIX)" ""
	PREFIX = /usr/local
endif

SOURCE = $(PROGRAM)-$(VERSION).cpp
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share/$(PROGRAM)
DOCDIR = $(PREFIX)/share/doc/$(PROGRAM)
DESKTOP = $(PREFIX)/share/applications/$(PROGRAM).desktop

CFLAGS = -O -Wall -c `pkg-config --cflags gtk+-2.0`
LFLAGS = -O -Wall -rdynamic `pkg-config --libs gtk+-2.0 gthread-2.0`

all:	$(PROGRAM)
$(PROGRAM): $(PROGRAM).o zfuncs.o
	@ g++ $(LFLAGS) -o $(PROGRAM) $(PROGRAM).o zfuncs.o

$(PROGRAM).o: $(SOURCE) zfuncs.h
	@ g++ $(CFLAGS) -o $(PROGRAM).o $(SOURCE)		\
	  -D "DATADIR=\"$(DATADIR)\"" -D "DOCDIR=\"$(DOCDIR)\""

zfuncs.o: zfuncs.cpp zfuncs.h
	@ g++ $(CFLAGS) zfuncs.cpp	\
	  -D "DATADIR=\"$(DATADIR)\"" -D "DOCDIR=\"$(DOCDIR)\"" -D "BINDIR=\"$(BINDIR)\""

install: $(PROGRAM)
	mkdir -p $(DESTDIR)$(DOCDIR) 
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(DATADIR)
	mkdir -p $(DESTDIR)$(DATADIR)/locales
	cp -R doc/* $(DESTDIR)$(DOCDIR)
	cp -R data/* $(DESTDIR)$(DATADIR)
	cp $(PROGRAM) $(DESTDIR)$(BINDIR)
	cp -R locales/* $(DESTDIR)$(DATADIR)/locales
#  install .desktop file (menu entry)
	@rm -f $(DESKTOP)
	@echo "[Desktop Entry]" > $(DESKTOP)
	@echo "Name=$(PROGRAM)" >> $(DESKTOP)
	@echo "Categories=Graphics" >> $(DESKTOP)
	@echo "Type=Application" >> $(DESKTOP)
	@echo "Terminal=false" >> $(DESKTOP)
	@echo "Exec=$(BINDIR)/$(PROGRAM)" >> $(DESKTOP)
	@echo "Icon=$(DATADIR)/icons/$(PROGRAM).png" >> $(DESKTOP)
	@echo "GenericName=Image Layout and Print" >> $(DESKTOP)
	@chown $(USER): $(DESKTOP)
	#@xdg-desktop-menu install $(DESKTOP)

uninstall:
	xdg-desktop-menu uninstall $(DESTDIR)$(DATADIR)/kornelix-$(PROGRAM).desktop
	rm -f $(DESTDIR)$(BINDIR)/$(PROGRAM)
	rm -R -f $(DESTDIR)$(DOCDIR)
	rm -R -f $(DESTDIR)$(DATADIR)

clean: uninstall


