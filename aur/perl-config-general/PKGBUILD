# Maintainer: Brian Bidulock <bidulock@openss7.org>
# Contributor: Jason St. John <jstjohn .. purdue . edu>
_perlmod=Config-General
_modnamespace=Config
pkgname=perl-config-general
pkgver=2.58
pkgrel=1
pkgdesc="Config::General - Generic Config Module"
arch=('any')
url="http://search.cpan.org/dist/${_perlmod}"
license=('GPL' 'PerlArtistic')
options=('!emptydirs')
source=("http://cpan.org/modules/by-module/${_modnamespace}/${_perlmod}-${pkgver}.tar.gz")
sha512sums=('7a2720ca4ceb7b1cbb4556e08b1cdd16a5a721609afa7ec474803355d29aa292ad09af1ba097a57e6560413486236ac94c615268d22c0fe2923c92e7e0ddf4b7')

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
