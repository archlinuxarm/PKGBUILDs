# Maintainer: graysky <graysky AT archlinux DOT us>
# Contributor: loqs

_pkgname=dufte
pkgname=python-dufte
pkgver=0.2.28
pkgrel=1
pkgdesc='Creates clean and beautiful plots that work on light and dark backgrounds.'
arch=(any)
url='https://github.com/nschloe/dufte'
license=(MIT)
depends=(python-matplotlib)
makedepends=(python-pytest python-setuptools python-build python-install python-wheel)
source=("$pkgname-$pkgver.tar.gz::https://github.com/nschloe/$_pkgname/archive/v$pkgver.tar.gz")
b2sums=('8fc6726677c2379743a647fe0e389d6aaf5ece5e03b4df5507f0c5e7783dbf8d3f495ac57e5138f3d937f6d1db043e6a2190598bb4a2ba66f0345306706cb81c')

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
