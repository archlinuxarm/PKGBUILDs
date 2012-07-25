# Maintainer: Mike Swanson <mikeonthecomputer@gmail.com>
# OldMaintainer: Jason Reardon <aetherfly87@gmail.com>
# Contributor: Christoph Zeiler <archNOSPAM_at_moonblade.dot.org>

pkgname=chocolate-doom
pkgver=1.7.0
pkgrel=1
pkgdesc="A Doom port reproducing the behavior of the original DOS version"
arch=('i686' 'x86_64')
url="http://www.chocolate-doom.org/"
license=('GPL')
depends=('sdl' 'sdl_mixer' 'sdl_net')
makedepends=('autoconf' 'automake' 'python2')
install=$pkgname.install
source=(http://sourceforge.net/projects/$pkgname/files/$pkgname/$pkgver/$pkgname-$pkgver.tar.gz \
        chocolate-doom.desktop \
        chocolate-setup.desktop)
sha256sums=('c1cffb602e3cc3a82941b4675b9dc816a6e419076faf8f54db46df397f86bfc0'
            '67b077b4a57971678236fb3bfa0968d091febc81bde5516503f9a2084df560e6'
            '9db67a88ece98c92e551857ee19b2b3ba69f476d4869661667f5f1ed325bf69a')

build() {
  cd $pkgname-$pkgver

  sed 's|/games|/bin|g' -i {src,setup}/Makefile.am
  sed 's|share/games|share|g' -i src/d_iwad.c
  sed	-e '/samplerate/d' \
	-e 's| CFLAGS=|: # CFLAGS=|g' \
	-i configure.in

  autoreconf -i

  ./configure --prefix=/usr
  make
}

package() {
  cd "$srcdir"/$pkgname-$pkgver

  make DESTDIR="$pkgdir" install

  mkdir -p "$pkgdir"/usr/share/{applications,doom}
  install -m644 ../chocolate-{doom,setup}.desktop "$pkgdir"/usr/share/applications/
  install -m644 -D src/doom-screensaver.desktop \
	"$pkgdir"/usr/share/applications/chocolate-doom-screensaver.desktop
}
