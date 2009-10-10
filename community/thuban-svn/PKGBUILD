# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Contributor: dibblethewrecker dibblethewrecker.at.jiwe.dot.org

pkgname=thuban-svn
pkgver=2878
pkgrel=1
pkgdesc="Interactive Geographic Data Viewer with support for vector & raster data."
arch=('i686' 'x86_64')
url="http://thuban.intevation.org/"
license=('GPL')
depends=('gdal' 'python-pysqlite-legacy' 'wxpython' 'wxgtk')
makedepends=('proj' 'subversion')
conflicts=('thuban')
provides=('thuban')
source=(thuban.desktop)
md5sums=('25b3d0a5a997c4f102bc9bed3853cdda')

_svntrunk=https://scm.wald.intevation.org/svn/thuban/trunk
_svnmod=thuban

build() {
  cd $startdir/src/
  svn co $_svntrunk --config-dir ./ -r $pkgver $_svnmod

  cp -r $_svnmod $_svnmod-build
  cd $_svnmod-build/$_svnmod
  
  python setup.py build || return 1
  python setup.py install --root=$startdir/pkg --prefix=/usr

  # install some freedesktop.org compatibility
  install -D -m644 $startdir/src/thuban.desktop \
      $startdir/pkg/usr/share/applications/thuban.desktop

  rm -rf $startdir/src/$_svnmod-build
}
# vim:syntax=sh
