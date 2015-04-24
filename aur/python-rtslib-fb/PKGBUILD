# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=('python-rtslib-fb' 'python2-rtslib-fb')
_pkgname=rtslib-fb
pkgver=2.1.fb53
pkgrel=1
pkgdesc="free branch version of the LIO target API"
arch=('any')
url="https://github.com/agrover/rtslib-fb"
license=('Apache')
makedepends=('python-setuptools' 'python2-setuptools')
backup=()
options=()
install=
source=(https://fedorahosted.org/releases/t/a/targetcli-fb/$_pkgname-$pkgver.tar.gz target.service)
sha256sums=('0578a8c32227cda5129e41dd4142ad43325d1aafe56d4e60a31430b15a269744'
            '74b9e5c11eab1781aa8b43680b429080ae800fbcdafd29626791b5426a4cdea8')


package_python-rtslib-fb() {
  depends=('python')
  conflicts=('python2-rtslib' 'targetcli-fb<=2.1.fb31')

  cd "$srcdir/$_pkgname-$pkgver"
  python setup.py install --root="$pkgdir/" --optimize=1

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

package_python2-rtslib-fb() {
  depends=('python2')
  conflicts=('python2-rtslib')

  cd "$srcdir/$_pkgname-$pkgver"
  python2 setup.py install --root="$pkgdir/" --optimize=1
  # the service file and targetctl script is in python-rtslib-fb
  rm -r "$pkgdir/usr/bin"
}

# vim:set ts=2 sw=2 et:
