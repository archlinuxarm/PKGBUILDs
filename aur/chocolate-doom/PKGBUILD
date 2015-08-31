# Maintainer: Mike Swanson <mikeonthecomputer@gmail.com>

pkgname=(chocolate-{doom,heretic,hexen,strife,common})
pkgbase=${pkgname[0]}
pkgdesc="Historically-accurate Doom, Heretic, Hexen, and Strife ports."
pkgver=2.2.0
pkgrel=3
arch=('i686' 'x86_64')
url="http://www.chocolate-doom.org/"
license=('GPL2')
depends=('libsamplerate' 'sdl_mixer' 'sdl_net')
makedepends=('python')
source=(http://chocolate-doom.org/downloads/${pkgver}/${pkgbase}-${pkgver}.tar.gz
        0001-setup-Fix-help-URL-for-level-warp-menu.patch
        0002-Update-README.Strife.patch
        0003-setup.desktop-Add-a-missing-semicolon-to-Categories.patch
        0004-Add-Keywords-to-the-setup.desktop-file.patch
        0005-doom-Avoid-overflow-for-spawn-angle-calculation.patch
        0006-osx-Add-CFBundleIdentifier-to-Info.plist.patch
        0007-Added-deh_bexstr.c-to-MSVC-project.patch
        0008-Fixed-Heretic-Hexen-vcproj-files.patch
        0009-Remove-duplicate-variable-definitions.patch
        0010-setup-manifest-Add-compatibility-GUID-for-Windows-10.patch
        0011-Apply-gcc_struct-tag-to-packed-structs.patch
        0012-Make-IWAD-search-paths-compliant-with-XDG-spec.patch
        0013-man-Add-documentation-for-IWAD-search-paths.patch)
sha256sums=('9fa9c56e72f8a04adf8f0800141192e9bafd38c9e424f70942ece6e515570173'
            '172b7df49de7c233cf2720d74341e606dd896bad396a3490e99b13de48184347'
            'dac8bb2e341e33b83f05c5475144b482f724070df80f6b83dc3feb7d2224b8d0'
            '8d2bd8b1db28c7ec2c0ec05e4dfadfbf13f26e02a772d59523e76b2cb957228a'
            'fa3a2b6aa1c9ac0a38a1969ac7ada9e506654929ab0f976ebb87798943561916'
            'dbb1b3bb84befafbc4be305515c3d73943ccd93d13a65f5fa33dfc5811187d2b'
            'c3bee8e2b7c7edeb1b305b4d892aa75ac44e661d21b91644b0c356d388976c8c'
            '46a7327c4fd5e60f3969fa2677e815cb306f9d443033d52a6e9d007bcfab1aeb'
            'ccb421f20e33b328b413601e9dd56553481e96810347fb523531dfcd4a5ca01a'
            '859b29c78932f97cad9c8c77fbd333664b9f01e1d3c3881ebb1e16769a60d04f'
            '7886a2df4967b3db0032bede909cccfe14a302cb51b1a4005b2a877231e4ab46'
            '33f2b99d847060e13ca3e8996a178731f8652660e450121aad0f34b6af7225f6'
            '99c57dc6264cc0ff6e0964b34f5457b1208d369eef601756313e886fcb92f85c'
            '8588637c1e09ae2eedf097bc4350052e80eed50bbfbbb814412005ee2b208287')

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
