# Original Author: Henner Zeller <h.zeller at acm dot org>
# Maintainer: Florian Will <florian dot will at gmail dot com>
# Contributor: Drew DeVore <w.drew.devore at gmail dot com>

pkgname=gmrender-resurrect-git
_gitname=gmrender-resurrect
pkgver=275.61f5a8f
pkgrel=2
pkgdesc="Application to stream music from a UPnP server using gstreamer."
arch=('i686' 'x86_64')
url="https://github.com/hzeller/gmrender-resurrect"
license=('GPL2')
conflicts=('gmediarender')
depends=('libupnp' 'gst-plugins-good' 'gst-plugins-base')
optdepends=(
    'gst-libav: Extra media codecs'
    'gst-plugins-bad: Extra media codecs'
    'gst-plugins-ugly: Extra media codecs'
)
makedepends=('git')
backup=('etc/conf.d/gmediarender')
install='gmrender-resurrect.install'
source=(
	'git+https://github.com/hzeller/gmrender-resurrect.git'
	'gmediarender.service'
	'gmediarender')
md5sums=('SKIP'
         'a3fa4bedc6e0853cf40b48c80269736f'
         '979798ff9cac610930f13fb922ca95d4')


pkgver() {
	cd $_gitname
	echo $(git rev-list --count HEAD).$(git rev-parse --short HEAD)
}

build() {
	cd $_gitname
	./autogen.sh
	./configure --prefix=/usr/
	make
}

package() {
	cd $_gitname
	make DESTDIR=$pkgdir install
	install -D -m 644 $srcdir/gmediarender.service "$pkgdir/usr/lib/systemd/system/gmediarender.service"
	install -D -m 644 $srcdir/gmediarender "$pkgdir/etc/conf.d/gmediarender"
}
