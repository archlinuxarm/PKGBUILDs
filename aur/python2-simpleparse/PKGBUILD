# Contributor: Roberto Alsina <ralsina@kde.org>
# Contributor: Fabio Varesano <fvaresano@yahoo.it> (Updated to 2.1.1a2)
# Contributor: Martti Kühne <mysatyre@gmail.com>
# Maintainer : David Phillips <gmail, dbphillipsnz>

pkgname=python2-simpleparse
_pkgname=SimpleParse
pkgver=2.1.1a2
pkgrel=2
pkgdesc="A simple and fast parser generator"
arch=('i686' 'x86_64')
url="http://simpleparse.sourceforge.net"
license=('custom')
depends=('python2')
makedepends=('python2-setuptools')
source=("http://downloads.sourceforge.net/simpleparse/${_pkgname}-${pkgver}.tar.gz")
sha256sums=('9899df932c6805dbb6433c7395e696fd60723f463513933e925cc77314c6bbb8')

build () {
	cd "${srcdir}/${_pkgname}-${pkgver}"
	python2 setup.py bdist_dumb
}

package () {
	cd "${srcdir}/${_pkgname}-${pkgver}"
	install -D license.txt "${pkgdir}/usr/share/licenses/${pkgname}/COPYING"
	tar -xzv \
		-f "${srcdir}/${_pkgname}-${pkgver}/dist/${_pkgname}-${pkgver}.linux"*".tar.gz" \
		-C "${pkgdir}"
				
}
