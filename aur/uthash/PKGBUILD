# Maintainer: Michael Frey <mail@mfrey.net>
pkgname=uthash
pkgver=1.9.7
pkgrel=1
pkgdesc="uthash provides C preprocessor implementations of a hash table and a linked list"
arch=('any')
url="http://uthash.sourceforge.net/"
license=('BSD')
source=(http://downloads.sourceforge.net/uthash/$pkgname-$pkgver.tar.bz2)
sha512sums=('8c02f9ac846a6bfd7292724a3683ae360ff37bba8e3ca98fca106da84976cac9c61766069989ee2c633b20bf82b64658400687a7cfbccab5e98e5fb6cb2e5caa')

package() {
  cd "${srcdir}/${pkgname}-${pkgver}/src"
 
  # create directory for header files 
  install -dm755 "${pkgdir}"/usr/include/

  # install header files in /usr/include
  for h in $(ls -1 *.h); do
      install -m 644 ${h} "${pkgdir}"/usr/include/
  done

  # install licence file
  install -D -m644 ../LICENSE "${pkgdir}"/usr/share/licenses/${pkgname}/LICENSE
}

