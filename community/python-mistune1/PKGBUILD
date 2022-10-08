# Maintainer: David Runge <dvzrv@archlinux.org>

_name=mistune
pkgname=python-mistune1
pkgver=0.8.4
pkgrel=4
pkgdesc="A fast yet powerful Python Markdown parser with renderers and plugins"
arch=('any')
url="https://github.com/lepture/mistune"
license=('BSD')
depends=('python')
makedepends=('python-setuptools')
checkdepends=('python-pytest')
conflicts=('python-mistune')
provides=("python-mistune=${pkgver}")
source=("https://files.pythonhosted.org/packages/source/${_name::1}/${_name}/${_name}-${pkgver}.tar.gz")
sha512sums=('36c3ef5d5537f5cceaa43e4da20a84b27c378cb744a93f0380024faefde490bcb42c453f79002ca049083fc437278f4afb3e10de5462f9eeb9077ca2a2fcaea7')
b2sums=('e65d45c5c95706a49a7fd407afe5f10e82a475766ca272ec3bebba8c89e670fe6efced7b09537efb69d3fd36e75091e370170a15ad7488b5cbe9186e2ccaf4db')

prepare() {
  mv -v "${_name}-$pkgver" "$pkgname-$pkgver"
}

build() {
  cd "$pkgname-$pkgver"
  python setup.py build
}

check() {
  cd "$pkgname-$pkgver"
  export PYTHONPATH="build:${PYTHONPATH}"
  pytest -v
}

package() {
  cd "$pkgname-$pkgver"
  python setup.py install --optimize=1 --root="${pkgdir}"
  install -vDm 644 {CHANGES,README}.rst -t "${pkgdir}/usr/share/doc/${pkgname}"
  install -vDm 644 LICENSE -t "${pkgdir}/usr/share/licenses/${pkgname}"
}
