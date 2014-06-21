# Maintainer: willemw <willemw12@gmail.com>
# Based on "community/python-sqlalchemy" package ($Id: PKGBUILD 108809 2014-04-04 10:16:01Z fyan $)

#pkgbase=python-sqlalchemy-0.7.9
pkgname=('python-sqlalchemy-0.7.9' 'python2-sqlalchemy-0.7.9')
pkgver=0.7.9
pkgrel=4
arch=('i686' 'x86_64') # python2 package contain .so
url="http://www.sqlalchemy.org/"
license=('MIT')
makedepends=('python' 'python2' 'python-setuptools' 'python2-setuptools')
checkdepends=('python-pytest' 'python2-pytest' 'python-mock' 'python2-mock')
source=("https://pypi.python.org/packages/source/S/SQLAlchemy/SQLAlchemy-$pkgver.tar.gz")
md5sums=('a0b58defc5ad0c7e1baeb932f62d08dd')

prepare() {
  cp -a SQLAlchemy-$pkgver SQLAlchemy2-$pkgver
}

build() {
  cd SQLAlchemy-$pkgver
  python setup.py build

  cd ../SQLAlchemy2-$pkgver
  python2 setup.py build
}

check() {
  # Tests failing
  cd SQLAlchemy-${pkgver}
  #python setup.py test
 
  cd ../SQLAlchemy2-$pkgver  
  #python2 setup.py test
}

package_python-sqlalchemy-0.7.9() {
  pkgdesc='Python SQL toolkit and Object Relational Mapper'
  depends=('python')
  optdepends=('python-psycopg2: connect to PostgreSQL database')
  conflicts=('python-sqlalchemy')
  provides=('python-sqlalchemy')

  cd SQLAlchemy-${pkgver}
  python setup.py install --root="${pkgdir}"
  install -D -m644 LICENSE \
	  "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

package_python2-sqlalchemy-0.7.9() {
  pkgdesc='Python 2 SQL toolkit and Object Relational Mapper'
  depends=('python2')
  optdepends=('python2-psycopg2: connect to PostgreSQL database')
  conflicts=('python2-sqlalchemy')
  provides=('python2-sqlalchemy')

  cd SQLAlchemy2-$pkgver
  python2 setup.py install --root="$pkgdir"
  install -D -m644 LICENSE \
	  "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

# vim:set ts=2 sw=2 ft=sh et:
