# Maintainer: Stefan Husmann <stefan-husmann@t-online.de
# Contributor: Andrea Scarpino <bash.lnx@gmail.com>
# Contributor: Bjorn Lindeijer <bjorn@lindeijer.nl>
# Contributor: kritoke <kritoke@nospam.gmail.com>

pkgname=ruby-pango
pkgver=0.19.1
pkgrel=1
pkgdesc="Ruby language bindings for Pango"
arch=('i686' 'x86_64')
url="http://ruby-gnome2.sourceforge.jp"
license=('LGPL')
depends=('ruby-glib2' 'pango')
source=(http://downloads.sourceforge.net/ruby-gnome2/ruby-gtk2-$pkgver.tar.gz)
md5sums=('bdce4ae02d0edf2d25e5fc66c49fe15e')

build() {
  cd "$srcdir/ruby-gtk2-$pkgver"
  ruby extconf.rb pango || return 1
  make || return 1
  make DESTDIR="$pkgdir" install || return 1  
  mv $pkgdir/usr/lib/pkgconfig $pkgdir/usr/lib/pkgconfig.orig
  install -d $pkgdir/usr/lib/pkgconfig
  mv $pkgdir/usr/lib/pkgconfig.orig  $pkgdir/usr/lib/pkgconfig/$pkgname.pc
}
