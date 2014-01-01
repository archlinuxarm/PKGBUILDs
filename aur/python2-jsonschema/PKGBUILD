# Maintainer: Bogdan Szczurek <thebodzio@gmail.com>
# This package is almost identical to the package
# python-jsonschema by Ismo Toijala, so all praises
# go to him :): Ismo Toijala <ismo.toijala@gmail.com>

pkgname=python2-jsonschema
pkgver=2.3.0
pkgrel=1
pkgdesc="An implementation of JSON Schema validation for Python"
arch=(any)
url="http://pypi.python.org/pypi/jsonschema"
license=('MIT')
depends=('python2')
source=("http://pypi.python.org/packages/source/j/jsonschema/${pkgname:8}-$pkgver.tar.gz")

package() {
  cd "$srcdir/${pkgname:8}-$pkgver"
  python2 setup.py install --root="$pkgdir/" --optimize=1

  install -D -m644 json/LICENSE $pkgdir/usr/share/licenses/$pkgname/LICENSE
}

md5sums=('410075e1969a9ec1838b5a6e1313c32b')
