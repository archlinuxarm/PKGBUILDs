# Maintainer:   AXVill
# Contributor:  dacoit <dacoit at tuta.io>
# Contributor:  xjpvictor Huang <ke [AT] xjpvictor [DOT] info>
# Contributor:  uuwe
pkgname=openswan
pkgver=2.6.47
pkgrel=1
pkgdesc='Open Source implementation of IPsec for the Linux operating system'
url='https://www.openswan.org'
arch=('i686' 'x86_64')
license=('GPL' 'custom')
depends=('gmp' 'perl' 'iproute2')
makedepends=('flex' 'bison')
optdepends=('python2')
conflicts=('ipsec-tools')
options=('!makeflags')
backup=('etc/ipsec.conf'
        'etc/ipsec.d/policies/'{block,clear,clear-or-private,private,private-or-clear})
source=("http://download.openswan.org/openswan/openswan-${pkgver}.tar.gz"
        'openswan.service')

prepare() {
  cd "$pkgname-$pkgver"

  # Change install paths to Arch defaults
  sed -i 's|/usr/local|/usr|;s|libexec/ipsec|lib/openswan|;s|)/sbin|)/bin|' Makefile.inc

  # Replace invalid init script paths with systemd script path
  sed -i 's/^INC_RCDIRS.*/INC_RCDIRS\?\=\/usr\/lib\/systemd\/scripts/' Makefile.inc
}

build() {
  cd "$pkgname-$pkgver"
  make USE_XAUTH=true USE_OBJDIR=true programs || return 1
}

package() {
  cd "$pkgname-$pkgver"

  # Pre-create init script directory
  mkdir -p "$pkgdir/usr/lib/systemd/scripts"

  make DESTDIR="$pkgdir" install

  # Change permissions in /var
  mv "$pkgdir/var/run" "$pkgdir/"
  rm -r "$pkgdir/var"
  chmod 700 "$pkgdir/run/pluto"

  # Copy License
  install -Dm644 LICENSE "$pkgdir/usr/share/licenses/openswan/LICENSE"

  # Install service unit
  install -Dm644 "$srcdir/openswan.service" "$pkgdir/usr/lib/systemd/system/openswan.service"

  # fix manpages
  mv "$pkgdir/usr/man" "$pkgdir/usr/share/"

  # fix python2
  sed -i '1s|python|python2|' "$pkgdir/usr/lib/openswan/verify"
}
md5sums=('54aa71adb46e4f1b07f3db534540058a'
         'd8b465c10838c72e31329d65011002b6')

