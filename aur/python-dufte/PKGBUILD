# Maintainer: graysky <graysky AT archlinux DOT us>
# Contributor: loqs

_pkgname=dufte
pkgname=python-dufte
pkgver=0.2.27
pkgrel=1
pkgdesc='Creates clean and beautiful plots that work on light and dark backgrounds.'
arch=(any)
url='https://github.com/nschloe/dufte'
license=(MIT)
depends=(python-matplotlib)
makedepends=(python-pytest python-setuptools python-build python-install python-wheel)
source=("$pkgname-$pkgver.tar.gz::https://github.com/nschloe/$_pkgname/archive/$pkgver.tar.gz")
b2sums=('b68ee18f9012322891b8559d9c2c8b39576c1a11f3902571c9944ca681c22ad198396fcf160d57cd2159a74ed3174a1333e2c883f236fb75bd4be0253f344ac6')

build() {
  cd $_pkgname-$pkgver
  python -m build --wheel --skip-dependency-check --no-isolation
}

check() {
  cd $_pkgname-$pkgver
  mkdir -p temp
  local site_packages=$(python -c "import site; print(site.getsitepackages()[0])")
  python -m install --optimize=1 --destdir=temp dist/*.whl
  PATH="$PWD/temp/usr/bin:$PATH" PYTHONPATH="$PWD/temp/$site_packages" python -m pytest
}

package() {
  cd $_pkgname-$pkgver
  export PYTHONHASHSEED=0
  python -m install --optimize=1 --destdir="$pkgdir" dist/*.whl

  # Symlink license file
  local site_packages=$(python -c "import site; print(site.getsitepackages()[0])")
  install -d "$pkgdir"/usr/share/licenses/$pkgname
  ln -s $site_packages/$_pkgname-$pkgver.dist-info/LICENSE \
    "$pkgdir"/usr/share/licenses/$pkgname/LICENSE
}
