# Contributor: Jonathan Liu <net147@gmail.com>
pkgname=libresample
pkgver=0.1.3
pkgrel=3
pkgdesc="A real-time library for audio sampling rate conversion"
arch=('i686' 'x86_64')
url="http://ccrma.stanford.edu/~jos/resample/Free_Resampling_Software.html"
license=('LGPL2')
options=('staticlibs')
source=("http://ccrma.stanford.edu/~jos/gz/$pkgname-$pkgver.tgz"
        "Makefile.in.patch")
md5sums=('99bc5ea15ef76b83e5655a10968f674b'
         '9c18aec34f5e16baac888bd0926270e7')

build() {
  cd "$srcdir/$pkgname-$pkgver"
  patch -Np1 -i "$srcdir/Makefile.in.patch"
  ./configure --prefix=/usr
}

package() {
  cd "$srcdir/$pkgname-$pkgver"
  make libresample.a
  install -D -m644 include/libresample.h "$pkgdir/usr/include/libresample.h"
  install -D -m644 libresample.a "$pkgdir/usr/lib/libresample.a"
}

# vim:set ts=2 sw=2 et:
