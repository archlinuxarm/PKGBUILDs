# Maintainer: Aaron Schaefer <aaron@elasticdog.com>
pkgname=python-pexpect
pkgver=2.3
pkgrel=2
pkgdesc='A pure Python Expect-like module'
arch=('i686' 'x86_64')
url='http://pexpect.sourceforge.net/'
license=('MIT')
depends=('python')
source=("http://downloads.sourceforge.net/sourceforge/pexpect/pexpect-$pkgver.tar.gz")
md5sums=('bf107cf54e67bc6dec5bea1f3e6a65c3')

build() {
  cd $startdir/src/pexpect-$pkgver
  python setup.py install --root=$startdir/pkg || return 1
  install -D -m644 LICENSE $startdir/pkg/usr/share/licenses/$pkgname/LICENSE
}
