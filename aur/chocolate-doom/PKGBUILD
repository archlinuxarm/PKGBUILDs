# Maintainer: Mike Swanson <mikeonthecomputer@gmail.com>

_pkgname=chocolate-doom # for the individual package functions
# The AUR can be annoying too.
pkgname=chocolate-doom
true && pkgname=(chocolate-{doom,heretic,hexen,strife,common})
pkgver=2.0.0
pkgrel=3
arch=('i686' 'x86_64')
url="http://www.chocolate-doom.org/"
license=('GPL2')
makedepends=('autoconf' 'python')
install=chocolate-doom.install
source=(http://chocolate-doom.org/downloads/${pkgver}/chocolate-doom-${pkgver}.tar.gz)
sha256sums=('85c58b77dad933013253b453ef01907492b4719acd56cf8cb6c76f4a361ab60c')

build() {
  cd "${pkgname[0]}-${pkgver}"

  # Change binary dir from /usr/games to /usr/bin
  sed 's|/games|/bin|g' -i src{,/setup}/Makefile.am

  autoreconf -i
  ./configure --prefix=/usr
  make
}

package_chocolate-common() {
  pkgdesc="Files shared in common with Chocolate Doom-based games."
  depends=('sdl_net')

  cd "${_pkgname}-${pkgver}"
  make DESTDIR="${pkgdir}" install
  install -dm 755 "${pkgdir}"/usr/share/games/doom

  cd "${pkgdir}"/usr/bin
  mv chocolate-doom-setup chocolate-setup
  rm -f chocolate-{doom,heretic,hexen,strife}{,-setup}

  cd "${pkgdir}"/usr/share
  rm -rf doc man/man5
  rm -rf applications/chocolate-doom.desktop applications/screensavers \
    icons/chocolate-doom.png
  cd man/man6
  rm -f chocolate-{doom,heretic,hexen,strife}.6
}

package_chocolate-doom() {
  pkgdesc="Doom port accurately reproducing the original v1.9 EXEs."
  depends=(${depends[@]} 'chocolate-common')

  cd "${_pkgname}-${pkgver}"
  make DESTDIR="${pkgdir}" install

  cd "${pkgdir}"/usr/bin
  rm -f chocolate-{heretic,hexen,strife,server} chocolate*setup
  ln -s chocolate{,-doom}-setup

  cd "${pkgdir}"/usr/share
  rm -rf doc/chocolate-{heretic,hexen,strife}
  rm -f applications/chocolate-setup.desktop icons/chocolate-setup.png
  rm -f man/man?/chocolate-{heretic,hexen,strife,setup,server}* \
    man/man5/{heretic,hexen,strife}.cfg*
}

package_chocolate-heretic() {
  pkgdesc="Heretic port accurately reproducing the original v1.3 EXE."
  depends=(${depends[@]} 'chocolate-common')

  cd "${_pkgname}-${pkgver}"
  make DESTDIR="${pkgdir}" install

  cd "${pkgdir}"/usr/bin
  rm -f chocolate-{doom,hexen,strife,server} chocolate*setup
  ln -s chocolate{,-heretic}-setup

  cd "${pkgdir}"/usr/share
  rm -rf doc/chocolate-{doom,hexen,strife}
  rm -rf applications icons
  rm -f man/man?/chocolate-{doom,hexen,strife,setup,server}* \
    man/man5/{default,hexen,strife}.cfg*
}

package_chocolate-hexen() {
  pkgdesc="Hexen port accurately reproducing the original v1.1 EXE."
  depends=(${depends[@]} 'chocolate-common')

  cd "${_pkgname}-${pkgver}"
  make DESTDIR="${pkgdir}" install

  cd "${pkgdir}"/usr/bin
  rm -f chocolate-{doom,heretic,strife,server} chocolate*setup
  ln -s chocolate{,-hexen}-setup

  cd "${pkgdir}"/usr/share
  rm -rf doc/chocolate-{doom,heretic,strife}
  rm -rf applications icons
  rm -f man/man?/chocolate-{doom,heretic,strife,setup,server}* \
    man/man5/{default,heretic,strife}.cfg*
}

package_chocolate-strife() {
  pkgdesc="Strife port accurately reproducing the original v1.31 EXE."
  depends=(${depends[@]} 'chocolate-common')

  cd "${_pkgname}-${pkgver}"
  make DESTDIR="${pkgdir}" install

  cd "${pkgdir}"/usr/bin
  rm -f chocolate-{doom,heretic,hexen,server} chocolate*setup
  ln -s chocolate{,-strife}-setup

  cd "${pkgdir}"/usr/share
  rm -rf doc/chocolate-{doom,heretic,hexen}
  rm -rf applications icons
  rm -f man/man?/chocolate-{doom,heretic,hexen,setup,server}* \
    man/man5/{default,heretic,hexen}.cfg*
}

# More AUR workaround.
pkgdesc="Doom, Heretic, Hexen, Strife port accurately reproducing the originals."
depends=('libsamplerate' 'sdl_mixer' 'sdl_net')
