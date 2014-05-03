# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=python-rtslib-fb
_pkgname=rtslib-fb
pkgver=2.1.fb47
pkgrel=1
epoch=
pkgdesc="free branch version of the LIO target API"
arch=('any')
url="https://github.com/agrover/rtslib-fb"
license=('Apache')
depends=()
makedepends=('python-setuptools')
# this is incompatible to python2-rtslib
provides=()
conflicts=('python2-rtslib' 'targetcli-fb<=2.1.fb31')
backup=()
options=()
install=
source=(https://fedorahosted.org/releases/t/a/targetcli-fb/$_pkgname-$pkgver.tar.gz target.service)
md5sums=('1c2a9befa2275ad79d84e9c417c2b807'
         '0bd602821a24cd598488807d130b55d7')

build() {
  cd "$srcdir/$_pkgname-$pkgver"
  python setup.py build
}

package() {
  cd "$srcdir/$_pkgname-$pkgver"
  python setup.py install --skip-build --root="$pkgdir/" --optimize=1

  install -Dm 644 doc/targetctl.8 "$pkgdir/usr/share/man/man8/targetctl.8"
  install -Dm 644 doc/saveconfig.json.5 "$pkgdir/usr/share/man/man5/saveconfig.json.5"

  # arch specific
  cd "$srcdir"
  install -d "$pkgdir/etc/target"
  install -d "$pkgdir/etc/target/backup"
  # systemd
  mkdir -p "$pkgdir/usr/lib/systemd/system"
  cp target.service "$pkgdir/usr/lib/systemd/system/"
}

# vim:set ts=2 sw=2 et:
