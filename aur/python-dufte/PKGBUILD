# Maintainer: graysky <graysky AT archlinux DOT us>
# Contributor: loqs

_pkgname=dufte
pkgname=python-dufte
pkgver=0.2.22
pkgrel=2
pkgdesc='Creates clean and beautiful plots that work on light and dark backgrounds.'
arch=(any)
url='https://github.com/nschloe/dufte'
license=(GPL3)
depends=(python-matplotlib)
makedepends=(python-pytest python-setuptools python-build python-install python-wheel)
source=("$pkgname-$pkgver.tar.gz::https://github.com/nschloe/$_pkgname/archive/v$pkgver.tar.gz")
b2sums=('8982616d6665f11fe14a0f06841c66cd8c334a369b99279e9ea7065793891c7ecd1799154f2cfceedd9b4d04102c3bb72b2a24cd04fae9711419928c2f453415')

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
