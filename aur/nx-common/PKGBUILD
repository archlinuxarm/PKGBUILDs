# Maintainer: Miguel Rasero <skuda21@gmail.com>
# Contributor: Tobias Powalowski <tpowa@archlinux.org>
# Contributed: eliott <eliott@solarblue.net>, Andre Naumann <anaumann@SPARCed.org>

pkgname=nx-common
pkgver=3.5.0
pkgrel=6
pkgdesc="NoMachine NX common package for client and server"
arch=('i686' 'x86_64')
license=('GPL')
url="http://nomachine.com/"
depends=('libjpeg-turbo' 'libpng' 'openssl' 'gcc-libs' 'libxcomp') 
makedepends=('xorg-server-devel' 'nx-headers')
source=(ftp://ftp.uni-duisburg.de/X11/NX/sources/$pkgver/nxcompsh-$pkgver-1.tar.gz
        ftp://ftp.uni-duisburg.de/X11/NX/sources/$pkgver/nxssh-$pkgver-2.tar.gz        
        nxcompsh-gcc43.patch)
options=('!libtool')
md5sums=('84ade443b79ea079380b754aba9d392e'
         'f52fcdb38e09f8dcfb9ff0344dfbbbd6'
         '3b2436f0090cbe0fa77cde0dfa80a87a')

build() {
  # nxcompsh
  cd ${srcdir}/nxcompsh
  patch -Np1 -i ${srcdir}/nxcompsh-gcc43.patch
  ./configure --prefix=/usr/lib/nx
  make

  # nxssh
  cd ${srcdir}/nxssh
  sed -i "s:NX.h:nx/NX.h:g" clientloop.c packet.c proxy.c
  ./configure --prefix=/usr --sysconfdir=/etc --libexecdir=/usr/lib
  make
}

package() {
  install -dm755 ${pkgdir}/usr/{bin,lib/nx}

  # nxcompsh
  cd ${srcdir}/nxcompsh
  cp -a libXcompsh.so* ${pkgdir}/usr/lib/nx
  cd ${pkgdir}/usr/lib/
  ln -sv /usr/lib/nx/libXcompsh.so{,.3,.3.5.0} .
  # ^ really needed?

  # nxssh
  cd ${srcdir}/nxssh
  install -D -m755 nxssh ${pkgdir}/usr/lib/nx/bin/nxssh
  cd ${pkgdir}/usr/bin
  ln -sv /usr/lib/nx/bin/nxssh .
}
