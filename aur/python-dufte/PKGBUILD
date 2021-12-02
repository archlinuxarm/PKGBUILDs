# Maintainer: graysky <graysky AT archlinux DOT us>
# Contributor: loqs

_pkgname=dufte
pkgname=python-dufte
pkgver=0.2.29
pkgrel=1
pkgdesc='Creates clean and beautiful plots that work on light and dark backgrounds.'
arch=(any)
url='https://github.com/nschloe/dufte'
license=(MIT)
depends=(python-matplotlib)
makedepends=(python-pytest python-setuptools python-build python-install python-wheel)
source=("$pkgname-$pkgver.tar.gz::https://github.com/nschloe/$_pkgname/archive/v$pkgver.tar.gz")
b2sums=('b7ea968048bb44acbc53662ac9e2d92606156b2a842f0e07613aa085f6a55273d3555421854f343bdf21023500048c9ad32c66fe1bced91d54ca7c324d0e63c1')

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
