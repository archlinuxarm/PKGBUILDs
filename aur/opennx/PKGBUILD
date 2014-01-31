# Maintainer: Miguel Rasero <skuda21@gmail.com>
#Original opennx PKGBUILD: Tomas Groth tomasgroth.at.yahoo.dk

pkgname=opennx
pkgver=0.16.0.729
pkgrel=2
pkgdesc="A GPL replacement for the NoMachine client"
url="http://opennx.sf.net/"
arch=('i686' 'x86_64')
license=('GPL')
depends=('wxgtk2.8' 'libcups' 'libxext' 'libxft' 'xorg-xauth' 'curl' 'nx-common' 'hicolor-icon-theme')
makedepends=('zip' 'opensc' 'libpulse' 'smbclient' 'libusb-compat')
source=(http://downloads.sourceforge.net/project/opennx/opennx/CI-source/opennx-$pkgver.tar.gz)
md5sums=('5271a2430693858803f2e1ca860e5a6c')

build() {
  cd "$srcdir"/opennx*
  ./configure --prefix=/usr \
    --enable-usbip \
    --with-wx-config=wx-config-2.8
  make
}

package() {
  cd "$srcdir"/opennx*
  make DESTDIR="${pkgdir}" install
  make DESTDIR="${pkgdir}" install-man
  
  # fix some file locations
  install -dm755 "${pkgdir}"/usr/share/applications
  cp -aR "${pkgdir}"/usr/share/applnk/xdg/* "${pkgdir}"/usr/share/applications
  rm -rf "${pkgdir}"/usr/share/applnk
  
  sed -i -e "s:Exec=/usr/NX/bin/opennx:Exec=/usr/bin/opennx:" "${pkgdir}"/usr/share/applications/*.desktop

  mkdir "${pkgdir}"/usr/share/icons/hicolor
  cd "${pkgdir}"/usr/share/icons
  find ./ -maxdepth 1 -mindepth 1 -type d -not -name hicolor -exec mv {} hicolor/ \;
}

post_install() {
  gtk-update-icon-cache -q -t -f usr/share/icons/hicolor
}

post_upgrade() {
  gtk-update-icon-cache -q -t -f usr/share/icons/hicolor
}

post_remove() {
  gtk-update-icon-cache -q -t -f usr/share/icons/hicolor
}
