# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Corrado Primier <bardo@aur.archlinux.org>

pkgname=opcion
pkgver=1.1.1
pkgrel=5
pkgdesc="A java font viewer"
arch=('i686' 'x86_64')
url="http://opcion.sourceforge.net/"
license=('GPL')
depends=('java-runtime')
source=(http://downloads.sourceforge.net/sourceforge/${pkgname}/${pkgname/o/O}_v${pkgver}.jar
        opcion.sh opcion.desktop opcion.png)
noextract=(${pkgname/o/O}_v${pkgver}.jar)
md5sums=('f50a0ae418554c2f88c4d2dd0dcac312' '55d0f018a92e95d2bf06a97deb597fd4'
         '3525665c74bb5674bdf0ddcdd4f38a99' '0a6599d9e9a0d67d48a5fb973252db98')

build() {
  install -Dm644 ${srcdir}/Opcion_v${pkgver}.jar ${pkgdir}/usr/share/java/opcion/opcion.jar
  install -Dm755 ${srcdir}/opcion.sh ${pkgdir}/usr/bin/opcion
  install -Dm644 ${srcdir}/opcion.desktop ${pkgdir}/usr/share/applications/opcion.desktop
  install -Dm644 ${srcdir}/opcion.png ${pkgdir}/usr/share/pixmaps/opcion.png
}

# vim:set ts=2 sw=2 et:
