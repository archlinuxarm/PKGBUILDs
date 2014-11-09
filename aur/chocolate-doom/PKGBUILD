# Maintainer: Mike Swanson <mikeonthecomputer@gmail.com>

pkgname=(chocolate-{doom,heretic,hexen,strife,common})
pkgbase=${pkgname[0]}
pkgdesc="Historically-accurate Doom, Heretic, Hexen, and Strife ports."
pkgver=2.1.0
pkgrel=2
arch=('i686' 'x86_64')
url="http://www.chocolate-doom.org/"
license=('GPL2')
depends=('libsamplerate' 'sdl_mixer' 'sdl_net')
makedepends=('python')
source=(http://chocolate-doom.org/downloads/${pkgver}/${pkgbase}-${pkgver}.tar.gz
        0001-Ignore-loop-tags-on-non-looping-substitute-tracks.patch
        0002-Ignore-metadata-loop-tags-if-both-are-zero.patch)
sha256sums=('629305e7f328659f3e93e89b93adc9da4e99b5a351e51ceb749dcf3e3da8bcd3'
            'cb611700a63a3d4cb58b030d4801c6f331ce7bd08708c281b9385fe1f82066f5'
            '6330a3b1e6cde2db2554ca709925f2eb63f91df8f14b84946a7f50cff2f52ca3')

prepare() {
  cd "${pkgbase}-${pkgver}"

  # Change binary dir from /usr/games to /usr/bin
  sed 's|/games|/bin|g' -i src{,/setup}/Makefile.in

  patch -p1 -i ../0001-Ignore-loop-tags-on-non-looping-substitute-tracks.patch
  patch -p1 -i ../0002-Ignore-metadata-loop-tags-if-both-are-zero.patch
}

build() {
  cd "${pkgbase}-${pkgver}"

  ./configure --prefix=/usr
  make
}

package_chocolate-common() {
  pkgdesc="Files shared in common with Chocolate Doom-based games."
  depends=('sdl_net')
  install=chocolate-doom.install

  cd "${pkgbase}-${pkgver}"
  make DESTDIR="${pkgdir}" install
  install -dm 755 "${pkgdir}"/usr/share/games/doom

  cd "${pkgdir}"/usr/bin
  mv chocolate-doom-setup chocolate-setup
  rm -f chocolate-{doom,heretic,hexen,strife}{,-setup}

  cd "${pkgdir}"/usr/share
  rm -rf doc man/man5
  rm -rf applications/chocolate-{doom,heretic,hexen,strife}.desktop \
    applications/screensavers
  cd man/man6
  rm -f chocolate-{doom,heretic,hexen,strife}{,-setup}.6
}

package_chocolate-doom() {
  pkgdesc="Doom port accurately reproducing the original DOS EXEs."
  depends=(${depends[@]} 'chocolate-common')
  optdepends=('freedm: Free deathmatch game'
    'freedoom1: Free Ultimate Doom-compatible game (not vanilla compatible, but useful for mods)'
    'freedoom2: Free Doom II/Final Doom-compatible game (not vanilla compatible, but useful for mods)')

  cd "${pkgbase}-${pkgver}"
  make DESTDIR="${pkgdir}" install

  cd "${pkgdir}"/usr/bin
  rm -f chocolate-{heretic,hexen,strife,server} chocolate*setup
  ln -s chocolate{,-doom}-setup

  cd "${pkgdir}"/usr/share
  rm -rf doc/chocolate-{heretic,hexen,strife}
  rm -rf applications/chocolate-{setup,heretic,hexen,strife}.desktop icons
  rm -f man/man?/chocolate-{heretic,hexen,strife,setup,server}* \
    man/man5/{heretic,hexen,strife}.cfg*
}

package_chocolate-heretic() {
  pkgdesc="Heretic port accurately reproducing the original DOS EXEs."
  depends=(${depends[@]} 'chocolate-common')
  optdepends=('blasphemer: Free Heretic-compatible game')

  cd "${pkgbase}-${pkgver}"
  make DESTDIR="${pkgdir}" install

  cd "${pkgdir}"/usr/bin
  rm -f chocolate-{doom,hexen,strife,server} chocolate*setup
  ln -s chocolate{,-heretic}-setup

  cd "${pkgdir}"/usr/share
  rm -rf doc/chocolate-{doom,hexen,strife}
  rm -rf applications/chocolate-{setup,doom,hexen,strife}.desktop \
    applications/screensavers icons
  rm -f man/man?/chocolate-{doom,hexen,strife,setup,server}* \
    man/man5/{default,hexen,strife}.cfg*
}

package_chocolate-hexen() {
  pkgdesc="Hexen port accurately reproducing the original DOS EXEs."
  depends=(${depends[@]} 'chocolate-common')

  cd "${pkgbase}-${pkgver}"
  make DESTDIR="${pkgdir}" install

  cd "${pkgdir}"/usr/bin
  rm -f chocolate-{doom,heretic,strife,server} chocolate*setup
  ln -s chocolate{,-hexen}-setup

  cd "${pkgdir}"/usr/share
  rm -rf doc/chocolate-{doom,heretic,strife}
  rm -rf applications/chocolate-{setup,doom,heretic,strife}.desktop \
    applications/screensavers icons
  rm -f man/man?/chocolate-{doom,heretic,strife,setup,server}* \
    man/man5/{default,heretic,strife}.cfg*
}

package_chocolate-strife() {
  pkgdesc="Strife port accurately reproducing the original DOS EXEs."
  depends=(${depends[@]} 'chocolate-common')

  cd "${pkgbase}-${pkgver}"
  make DESTDIR="${pkgdir}" install

  cd "${pkgdir}"/usr/bin
  rm -f chocolate-{doom,heretic,hexen,server} chocolate*setup
  ln -s chocolate{,-strife}-setup

  cd "${pkgdir}"/usr/share
  rm -rf doc/chocolate-{doom,heretic,hexen}
  rm -rf applications/chocolate-{setup,doom,heretic,hexen}.desktop \
    applications/screensavers icons
  rm -f man/man?/chocolate-{doom,heretic,hexen,setup,server}* \
    man/man5/{default,heretic,hexen}.cfg*
}
