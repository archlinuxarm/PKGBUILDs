# Maintainer: Brian Bidulock <bidulock@openss7.org>
# Contributor: Jason St. John <jstjohn .. purdue . edu>
_perlmod=Config-General
_modnamespace=Config
pkgname=perl-config-general
pkgver=2.63
pkgrel=2
pkgdesc="Config::General - Generic Config Module"
arch=('any')
url="http://search.cpan.org/dist/${_perlmod}"
license=('GPL' 'PerlArtistic')
options=('!emptydirs')
source=("http://cpan.org/modules/by-module/${_modnamespace}/${_perlmod}-${pkgver}.tar.gz")
sha512sums=('ba9fdbf992049936ea288a90d8f8360821fc96f8d42df0298888b25543d2ac43e2958c5f7a8bbbae7cad1e2151ea00528756a3bc0cfbe408e5ee82bf309615f3')

build() {
  cd "${_perlmod}-${pkgver}"
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
