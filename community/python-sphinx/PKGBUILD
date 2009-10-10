# Maintainer: Douglas Soares de Andrade <douglas@archlinux.org>
# Contributor: Fabio Volpe <volpefabio@gmail.com>

pkgname=python-sphinx
pkgver=0.6.2
pkgrel=1
pkgdesc="Python documentation generator"
arch=('i686' 'x86_64')
url="http://sphinx.pocoo.org/"
license=('GPL')
depends=('python>=2.4' 'pygments' 'docutils' 'python-jinja')
makedepends=('setuptools')
source=(http://pypi.python.org/packages/source/S/Sphinx/Sphinx-$pkgver.tar.gz)

build() {
    cd "$startdir/src/Sphinx-$pkgver"

    python setup.py build
    python setup.py install --root=$pkgdir
}
md5sums=('c8f79e171addc73342cc2e775dc78880')
