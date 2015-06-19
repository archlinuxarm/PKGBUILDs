# Maintainer: Mike Swanson <mikeonthecomputer@gmail.com>

pkgname=(chocolate-{doom,heretic,hexen,strife,common})
pkgbase=${pkgname[0]}
pkgdesc="Historically-accurate Doom, Heretic, Hexen, and Strife ports."
pkgver=2.2.0
pkgrel=2
arch=('i686' 'x86_64')
url="http://www.chocolate-doom.org/"
license=('GPL2')
depends=('libsamplerate' 'sdl_mixer' 'sdl_net')
makedepends=('python')
source=(http://chocolate-doom.org/downloads/${pkgver}/${pkgbase}-${pkgver}.tar.gz
        0001-setup-Fix-help-URL-for-level-warp-menu.patch)
sha256sums=('9fa9c56e72f8a04adf8f0800141192e9bafd38c9e424f70942ece6e515570173'
            'f890dac486a77ace888e18419e0a3f95c2ff59fab9bea08eb92558c3d7320e50')

prepare() {
  cd "${pkgbase}-${pkgver}"

  for patch in ../*.patch; do
    if [ ! -f "$patch" ]; then
      break;
    else
      patch -p1 -i "$patch"
    fi
  done

  # Change binary dir from /usr/games to /usr/bin
  sed 's|/games|/bin|g' -i src{,/setup}/Makefile.in
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
