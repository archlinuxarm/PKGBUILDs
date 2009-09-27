DESTDIR=
VERSION=2.2.1
VPATH = doc

install:
	install -d $(DESTDIR)/lib/network/connections $(DESTDIR)/etc/network.d/examples \
	            $(DESTDIR)/var/run/network/{interfaces,profiles} \
	            $(DESTDIR)/bin/ $(DESTDIR)/etc/rc.d/ \
				$(DESTDIR)/share/man/{man5,man8}
	# Documentation
	install -m644 examples/* $(DESTDIR)/etc/network.d/examples/
	install -m644 src/iftab $(DESTDIR)/etc/iftab
	install -m644 man/*.8 $(DESTDIR)/share/man/man8
	# Libs
	install -m644 src/{network,wireless,8021x} $(DESTDIR)/lib/network
	install -m755 src/connections/* ${DESTDIR}/lib/network/connections
	# 'Binaries'
	install -m755 src/netcfg $(DESTDIR)/bin/netcfg2
	install -m755 src/netcfg-menu $(DESTDIR)/bin/netcfg-menu
	# Daemons
	install -m755 src/net-profiles src/net-rename $(DESTDIR)/etc/rc.d

install-wireless:
	install -d $(DESTDIR)/lib/network/connections $(DESTDIR)/bin \
				$(DESTDIR)/etc/rc.d
	install -m755 src-wireless/wireless-dbus $(DESTDIR)/lib/network/connections
	install -m755 src-wireless/netcfg-auto-wireless $(DESTDIR)/bin
	install -m755 src-wireless/net-auto $(DESTDIR)/etc/rc.d

install-docs: docs
	install -d $(DESTDIR)/share/doc/netcfg
	install -m644 doc/*html $(DESTDIR)/share/doc/netcfg/
	
docs: doc/*
	for doc in $(?); do \
		pandoc $$doc -o $$doc.html;\
	done

tarball: 
	sed -i "s/NETCFG_VER=.*/NETCFG_VER=$(VERSION)/g" src/netcfg
	mkdir -p netcfg-$(VERSION)
	cp -r src src-wireless examples man Makefile LICENSE README netcfg-$(VERSION)
	tar -zcvf netcfg-$(VERSION).tar.gz netcfg-$(VERSION)
	mv netcfg-$(VERSION).tar.gz ../
	rm -rf netcfg-$(VERSION)


upload: 
	md5sum netcfg-$(VERSION)*gz > MD5SUMS.$(VERSION)
	scp netcfg-$(VERSION)*gz MD5SUMS.$(VERSION) archlinux.org:/srv/ftp/other/netcfg/

clean:
	rm doc/*html
	rm *gz
	rm -rf netcfg-*$(VERSION)
	rm -rf pkg
	rm MD5SUMS*
	
