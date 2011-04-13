#!/bin/sh
AUFS2VERSION="-38"
KERNELVERSION=2.6.38
GITSNAPSHOT=20110314
# aufs2 (no -xx) for the latest -rc version.
git clone http://git.c3sl.ufpr.br/pub/scm/aufs/aufs2-standalone.git aufs2-standalone.git
cd aufs2-standalone.git
#git checkout origin/aufs2.1${AUFS2VERSION}
git checkout origin/aufs2.1
#*** apply "aufs2-base.patch" and "aufs2-standalone.patch" to your kernel source files.
cd ..
rm -rf aufs2-${KERNELVERSION}_${GITSNAPSHOT}
cp -a aufs2-standalone.git aufs2-${KERNELVERSION}_${GITSNAPSHOT}
tar -czf aufs2-${KERNELVERSION}_${GITSNAPSHOT}.tar.gz --exclude=.git aufs2-${KERNELVERSION}_${GITSNAPSHOT}
