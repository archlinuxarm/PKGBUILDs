# Contributor: Matthew Bauer <mjbauer95@gmail.com>
# Contributor: TingPing <tingping@fedoraproject.org>
# Maintainer: Steven Allen <steven@stebalien.com>

_pkgname=pithos
pkgname=$_pkgname-git
pkgver=1.1.1.r43.gb325db4
pkgrel=1
pkgdesc='Pandora Internet Radio player for GNOME'
arch=('any')
url="https://pithos.github.io/"
license=('GPL3')
depends=('gtk3' 'python-gobject' 'libsecret' 'python-cairo'
         'gst-plugins-good' 'gst-plugins-bad' 'gst-plugins-base')
optdepends=('libkeybinder3: for media keys plugin'
            'gst-plugins-ugly: MP3 playback support'
            'libappindicator-gtk3: Unity indicator applet support'
            'python-pacparser: PAC proxy support'
            'python-pylast: Last.fm scrobbling support'
            'libnotify: Notification support'
            'python-dbus: MPRIS/Screensaver Pause/Gnome mediakeys support')
makedepends=('git' 'automake' 'autoconf' 'intltool')
provides=("$_pkgname")
conflicts=("$_pkgname-bzr" "$_pkgname")
install="$pkgname.install"
sha256sums=('5b3bb0f5a3cfe676eb716f3728ba3ba606e6eaa205b7f2050ba8ab9e6ded98d5'
            'SKIP')

source=("$pkgname.install" "git+https://github.com/pithos/pithos.git")

pkgver() {
  cd "$srcdir/$_pkgname"
  git describe --tags | sed 's/-/.r/; s/-/./'
}

build() {
  cd "$srcdir/$_pkgname"
  ./autogen.sh --prefix=/usr
  make
}

package() {
  cd "$srcdir/$_pkgname"
  DESTDIR="$pkgdir" make install
}
