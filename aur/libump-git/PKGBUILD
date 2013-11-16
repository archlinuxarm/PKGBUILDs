# Maintainer: smotocel69 <smotocel69@gmail.com>

pkgname=libump-git
pkgver=3
pkgrel=0
pkgdesc="Unified Memory Provider userspace API source code"
arch=('armv7h')
url="http://github.com/linux-sunxi/sunxi-mali"
license=('custom')
depends=('libdri2-git' 'libxfixes' 'libdrm')
makedepends=('git')
provides=('libump')
conflicts=('libump')
replaces=('libump')

build() {
	cd $srcdir
	if [[ -d $pkgname ]]; then
		cd $pkgname && git pull origin
	else
		git clone $url --depth 1
	fi
	rm -rf $srcdir/build
	cd sunxi-mali 
	git submodule init
	git submodule update
	cd $srcdir
	mv sunxi-mali $pkgname
	cp -r $srcdir/$pkgname $srcdir/build
	cd $srcdir/build
	VERSION=r3p0 ABI=armhf EGL_TYPE=x11 make config
	make
}

package() {
	
	mkdir -p $pkgdir/usr/{include,lib}
	cd $srcdir/build/lib/ump
	make install prefix=$pkgdir/usr/
	cd $srcdir/build/include/
	make install_ump prefix=$pkgdir/usr/
}