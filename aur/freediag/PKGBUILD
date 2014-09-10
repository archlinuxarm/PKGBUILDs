pkgname=freediag
pkgver=1.04
pkgrel=1
pkgdesc="Vehicle diagnostic program including OBDII Scantool and support for certain manufacturer-specific controllers."
arch=('armv6h')
url="http://freediag.sourceforge.net/"
license=('GPL2')
source=("http://sourceforge.net/projects/freediag/files/freediag/1.0/${pkgname}-${pkgver}-src.tar.gz/download"
        configure.ac.patch)

sha1sums=('a41030f3ca4de8c94859e7df61a7bbc9def4bdff'
          '2d707bfeda25123639e339dce37cea2c8357f91c')
          
build() {
  cd "${srcdir}/${pkgname}-${pkgver}-src"
  patch -p1 < ${srcdir}/configure.ac.patch

  echo "Starting automated reconfigure..."
  autoconf
  autoheader
  aclocal

  echo "Fixing missing..."
  automake --add-missing

  echo "Starting configure..."
  chmod o+x ./configure
  ./configure

  echo "Making..."
  make -j1
}

package() {
  cd "${srcdir}/${pkgname}-${pkgver}-src"

  echo "Installing..."
  make -j1 install
}
