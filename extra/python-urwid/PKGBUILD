# $Id: PKGBUILD,v 1.10 2009/03/13 21:14:09 sergej Exp $
# Maintainer: Daniel Isenmann <daniel@archlinux.org>
# Maintainer: Sergej Pupykin <sergej@aur.archlinux.org>
# Maintainer: Douglas Soares de Andrade <dsandrade@gmail.com>

pkgname=python-urwid
pkgver=0.9.8.4
pkgrel=1
pkgdesc="Urwid is a curses-based user interface library."
license=('GPL')
arch=('i686' 'x86_64')
depends=('python')
url="http://excess.org/urwid/"
source=(http://excess.org/urwid/urwid-$pkgver.tar.gz)
md5sums=('28f918c66887d4e470ae0c3535579ad7')

build() {
  cd $startdir/src/urwid-$pkgver

  python setup.py install --prefix=/usr --root=$startdir/pkg || return 1

  rm -rf $startdir/src/$_hgname-build
}
