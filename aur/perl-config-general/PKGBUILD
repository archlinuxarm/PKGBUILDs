# Maintainer: Jason St. John <jstjohn .. purdue . edu>

_perlmod=Config-General
_modnamespace=Config
pkgname=perl-config-general
pkgver=2.56
pkgrel=2
pkgdesc="Config::General - Generic Config Module"
arch=('any')
url="http://search.cpan.org/dist/${_perlmod}"
license=('GPL' 'PerlArtistic')
options=('!emptydirs')
source=("http://cpan.org/modules/by-module/${_modnamespace}/${_perlmod}-${pkgver}.tar.gz")
sha512sums=('0439d690e58fb30cafd18e3c51fb1c6226cb20017fcd260959ad0210006d0e98a32a939d314b384e5c62871c0a59400a678099e3d703d9e2ed859de20292de9a')

build() {
	cd "${_perlmod}-${pkgver}"

	# Install module in vendor directories.
	PERL_MM_USE_DEFAULT=1 perl Makefile.PL INSTALLDIRS=vendor
	make
}

check() {
	cd "${_perlmod}-${pkgver}"
	make test
}

package() {
	cd "${_perlmod}-${pkgver}"
	make install DESTDIR="${pkgdir}"
}
