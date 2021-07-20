# Maintainer: graysky <graysky AT archlinux DOT us>
# Contributor: loqs

_pkgname=dufte
pkgname=python-dufte
pkgver=0.2.24
pkgrel=1
pkgdesc='Creates clean and beautiful plots that work on light and dark backgrounds.'
arch=(any)
url='https://github.com/nschloe/dufte'
license=(GPL3)
depends=(python-matplotlib)
makedepends=(python-pytest python-setuptools python-build python-install python-wheel)
source=("$pkgname-$pkgver.tar.gz::https://github.com/nschloe/$_pkgname/archive/v$pkgver.tar.gz")
b2sums=('869aef15c0921b98c7ada0f8fc4d5a8f13892c7c9cfca9bc3e564f3c93423f413892bb88c7afe019e9bafe1ba33d9fcf885991d49b90909c292cf376b972c7ea')

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
