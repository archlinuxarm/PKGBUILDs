# Maintainer: graysky <graysky AT archlinux DOT us>
# Contributor: loqs

pkgname=stressberry
pkgver=0.3.3
pkgrel=4
pkgdesc='Stress tests and temperature plots for the Raspberry Pi'
arch=(any)
url='https://github.com/nschloe/stressberry'
license=(GPL3)
depends=(python-matplotlib python-yaml python-matplotx stress)
makedepends=(python-pytest python-setuptools python-build python-install python-wheel)
source=("$pkgname-$pkgver.tar.gz::https://github.com/nschloe/$pkgname/archive/v$pkgver.tar.gz"
0001-convert-to-matplotx-from-dufte.patch::https://github.com/graysky2/stressberry/commit/55501513eaa2497c955daaa2179ca5c2e48eaa6b.patch)
b2sums=('3ebdcc0f0a951b9a0c8a5a35d96cda2edc535b58f08abeb3eefb281a7bb6c582e66e9eb601acce6c79ad92320c0b97ef481d2e76ffa97ae7e7033a5a7e9c38a3'
        '6498fa0c57241ec9c228ecbbd25f4df6fe39b60fcbded91f90e85ee16c98cf64b0cb97e9a962bd1bea65d81d85c534eca6af5c456ee77883fcc793d83d13a876')

prepare() {
  cd $pkgname-$pkgver
  patch -Np1 -i ../0001-convert-to-matplotx-from-dufte.patch
}

build() {
  cd $pkgname-$pkgver
  python -m build --wheel --skip-dependency-check --no-isolation
}

check() {
  cd $pkgname-$pkgver
  mkdir -p temp
  local site_packages=$(python -c "import site; print(site.getsitepackages()[0])")
  python -m install --optimize=1 --destdir=temp dist/*.whl
  PATH="$PWD/temp/usr/bin:$PATH" PYTHONPATH="$PWD/temp/$site_packages" python -m pytest
}

package() {
  cd $pkgname-$pkgver
  export PYTHONHASHSEED=0
  python -m install --optimize=1 --destdir="$pkgdir" dist/*.whl

  # Symlink license file
  local site_packages=$(python -c "import site; print(site.getsitepackages()[0])")
  install -d "$pkgdir"/usr/share/licenses/$pkgname
  ln -s $site_packages/$pkgname-$pkgver.dist-info/LICENSE \
    "$pkgdir"/usr/share/licenses/$pkgname/LICENSE

  chmod +x "$pkgdir"/usr/bin/stressberry-plot
  chmod +x "$pkgdir"/usr/bin/stressberry-run
}
