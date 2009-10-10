# Maintainer: Aaron Schaefer <aaron@elasticdog.com>
pkgname=python-boto
pkgver=1.7a
pkgrel=1
pkgdesc='A Python interface to Amazon Web Services (AWS)'
arch=('i686' 'x86_64')
url='http://code.google.com/p/boto/'
license=('MIT')
depends=('python')
source=("http://boto.googlecode.com/files/boto-$pkgver.tar.gz" 'LICENSE')
md5sums=('e64e995ff2313452e78ef37dddeb44c1'
         '35ebd993c05c79ba78e9f4fb3e65e547')

build() {
  cd "$srcdir/boto-$pkgver"

  python setup.py install --root="$pkgdir" || return 1
  install -D -m644 ../LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
