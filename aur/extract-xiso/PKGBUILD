# Maintainer: Joe Carta <cartakid at gmail dot com>
# Contributor: sidious/SiD <miste78 web de>
# Contributor: Wes Brewer <brewerw@gmail.com>

pkgname=extract-xiso
pkgver=2.7.1
pkgrel=1
pkgdesc="xdvdfs (xbox iso) file creation and extraction utility"
arch=('i686' 'x86_64')
url="http://sourceforge.net/projects/extract-xiso"
license=('custom')
conflicts=('extract-xiso-somski')
source=(http://downloads.sourceforge.net/$pkgname/${pkgname}-${pkgver}.tar.gz)
md5sums=('464aeb312aca6f4a1ffee42384b3c738')

build() {
  cd ${srcdir}/${pkgname}
  # build
  make 
}
package() {
  cd ${srcdir}/${pkgname}
  # install binary
  install -Dm755 extract-xiso ${pkgdir}/usr/bin/extract-xiso
  # install custom license
  install -Dm644 LICENSE.TXT ${pkgdir}/usr/share/licenses/${pkgname}/LICENSE.TXT
}
