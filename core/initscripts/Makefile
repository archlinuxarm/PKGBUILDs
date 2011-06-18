VER  := $(shell git describe)
DIRS := /etc/rc.d /etc/conf.d /etc/rc.d/functions.d /etc/cron.hourly /sbin /etc/bash_completion.d /usr/share/zsh/site-functions

minilogd: minilogd.o

installdirs:
	install -dm755 $(foreach DIR, $(DIRS), $(DESTDIR)$(DIR))

install: minilogd installdirs
	install -m644 -t $(DESTDIR)/etc inittab rc.conf
	install -m755 -t $(DESTDIR)/etc rc.local rc.local.shutdown rc.multi rc.shutdown rc.single rc.sysinit
	install -m755 -t $(DESTDIR)/etc/cron.hourly adjtime
	install -m644 -t $(DESTDIR)/etc/rc.d functions
	install -m755 -t $(DESTDIR)/etc/rc.d hwclock network netfs
	install -m755 -t $(DESTDIR)/sbin minilogd rc.d
	install -m644 -T bash-completion $(DESTDIR)/etc/bash_completion.d/rc.d
	install -m644 -T zsh-completion $(DESTDIR)/usr/share/zsh/site-functions/_rc.d

clean:
	rm -f minilogd minilogd.o

release:
	git archive HEAD --prefix=initscripts-$(VER)/ | xz > initscripts-$(VER).tar.xz
