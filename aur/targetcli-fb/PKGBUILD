# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=targetcli-fb
pkgver=2.1.fb41
pkgrel=1
pkgdesc="free branch of the targetcli LIO administration shell (iSCSI + Co)"
arch=('any')
url="https://github.com/agrover/targetcli-fb"
license=('Apache')
groups=()
depends=('python-rtslib-fb>=2.1.fb52' 'python-configshell-fb>=1.1.fb17')
makedepends=('python-setuptools')
# TODO: optdepend on python-ethtool when available for python3
optdepends=()
provides=('targetcli')
backup=()
options=()
install=
source=(https://fedorahosted.org/releases/t/a/targetcli-fb/$pkgname-$pkgver.tar.gz)
sha256sums=('0f32ab6f3ecd234280d55be5e89ce2b3a02f9c82c8dccaae66e26464cff8901a')


build() {
  cd "$srcdir/$pkgname-$pkgver"
  python setup.py build
  gzip --stdout targetcli.8 > "targetcli.8.gz"
}

package() {
  cd "$srcdir/$pkgname-$pkgver"
  python setup.py install --skip-build --root="$pkgdir/" --optimize=1

  install -D -m 644 targetcli.8.gz "$pkgdir/usr/share/man/man8/targetcli.8.gz"
}

# vim:set ts=2 sw=2 et:
