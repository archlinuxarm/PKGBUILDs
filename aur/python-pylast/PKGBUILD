# $Id$
# Maintainer: Steven Allen <steven@stebalien.com>
# Contributor: Maxime Gauduin <alucryd@gmail.com>
# Contributor: Limao Luo <luolimao+AUR@gmail.com>
# Contributor: Wieland Hoffmann <the_mineo@web.de>
# Contributor: Amr Hassan <amr.hassan@gmail.com>

pkgname=python-pylast
pkgver=1.1.0
pkgrel=2
pkgdesc='A Python interface to the last.fm API'
arch=('any')
url='https://github.com/pylast/pylast'
license=('Apache')
depends=('python' 'python-six')
makedepends=('python-setuptools')
source=("https://github.com/pylast/pylast/archive/${pkgver}.tar.gz")
sha256sums=('0c8ec43d931624d65582e38d4e0c10f8a1215a4a1caa9d7841642d2d083d1610')

build() {
  cd ${pkgname#*-}-$pkgver

  python setup.py build
}

package() {
  cd ${pkgname#*-}-$pkgver

  python setup.py install --root="${pkgdir}" --optimize=1
}

# vim: ts=2 sw=2 et:
