# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=('python-rtslib-fb' 'python2-rtslib-fb')
_pkgname=rtslib-fb
pkgver=2.1.fb60
pkgrel=2
pkgdesc="free branch version of the LIO target API"
arch=('any')
url="https://github.com/agrover/rtslib-fb"
license=('Apache')
makedepends=('python-setuptools' 'python2-setuptools' 'python-pyudev' 'python2-pyudev')
backup=()
options=()
install=
source=(https://fedorahosted.org/releases/t/a/targetcli-fb/$_pkgname-$pkgver.tar.gz target.service)
sha256sums=('11b664f02219e5108c4a996f5bc7445500912cff5696b3f51be2f6a1b243e957'
            '74b9e5c11eab1781aa8b43680b429080ae800fbcdafd29626791b5426a4cdea8')

prepare() {
  cd "$srcdir/$_pkgname-$pkgver"
  #patch -p1 < ../setup-syntax.patch
}


package_python-rtslib-fb() {
  depends=('python' 'python-six' 'python-pyudev')
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
  depends=('python2' 'python2-six' 'python2-pyudev')
  conflicts=('python2-rtslib')

  cd "$srcdir/$_pkgname-$pkgver"
  python2 setup.py install --root="$pkgdir/" --optimize=1
  # the service file and targetctl script is in python-rtslib-fb
  rm -r "$pkgdir/usr/bin"
}

# vim:set ts=2 sw=2 et:
