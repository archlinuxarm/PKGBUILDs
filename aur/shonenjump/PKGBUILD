# Maintainer: graysky <therealgraysky AT protonmail DOT com>

pkgname=shonenjump
pkgver=0.8.0
pkgrel=1
pkgdesc="A faster way to change directory and improve command line productivity"
arch=(x86_64)
url="https://github.com/suzaku/shonenjump"
license=(custom)
makedepends=(go)
depends=(glibc)
conflicts=(autojump)
install=readme.install
source=("$pkgname-$pkgver.tar.gz::https://github.com/suzaku/shonenjump/archive/refs/tags/v$pkgver.tar.gz")
b2sums=('9b46e3a27d29b2b1d3be3a703b90e33f5b4c5e5d3377df12bdcd5eee1efca9c35858b32462b9d100541fa76b59bac3b07f07d37825c83c510edca59504fffaf0')

build() {
  cd "$pkgname-$pkgver"
  CGO_CPPFLAGS="${CPPFLAGS}" CGO_CFLAGS="${CFLAGS}" CGO_CXXFLAGS="${CXXFLAGS}" CGO_LDFLAGS="${LDFLAGS}" \
  go build -buildmode=pie -ldflags "-linkmode external -extldflags \"${LDFLAGS}\""
}

package() {
  cd "$pkgname-$pkgver"
  install -d "$pkgdir/usr/bin/"
  install -m755 "$pkgname" "$pkgdir/usr/bin/$pkgname"

  install -d "$pkgdir/usr/share/licenses/$pkgname"
  install -m644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"

  install -d "$pkgdir/etc/profile.d"

  cd scripts
  for i in bash fish zsh; do
    install -m644 "$pkgname.$i" "$pkgdir/etc/profile.d/$pkgname.$i"
  done

  install -d "$pkgdir/usr/share/zsh/site-functions"
  install -m644 _j "$pkgdir/usr/share/zsh/site-functions/_j"
}
