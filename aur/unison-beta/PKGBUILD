#Contributor: Andreas Wagner <andreas dot wagner at em dot uni-frankfurt dot de>

pkgname=unison-beta
pkgver=2.48.3
pkgrel=1
pkgdesc="Unison is a file-synchronization tool. Beta Version"
arch=(i686 x86_64)
license=('GPL2')
url="http://www.cis.upenn.edu/~bcpierce/unison"
depends=('glibc')
makedepends=('ocaml' 'lablgtk2' 'imagemagick')
optdepends=('gtk2: graphical UI')
source=(http://www.cis.upenn.edu/~bcpierce/unison/download/releases/unison-$pkgver/unison-$pkgver.tar.gz \
        unison.desktop)
conflicts=('unison')
options=(!makeflags)
install=unison.install

build() {
  cd $srcdir/unison-$pkgver
  CFLAGS=""
  make clean
  make mkProjectInfo
  make UISTYLE=text DEBUGGING=false THREADS=true
  cp unison $srcdir/unison

## clean the builddir and rebuild with gtk support
#  make clean
#  make mkProjectInfo
#  make UISTYLE=gtk DEBUGGING=false THREADS=true
#  cp unison $srcdir/unison-gtk

## clean the builddir and rebuild with gtk2 support
  make clean
  make mkProjectInfo
  make UISTYLE=gtk2 DEBUGGING=false THREADS=true
  cp unison $srcdir/unison-gtk2
}

package() {
  cd $srcdir/unison-$pkgver
  mkdir -p $pkgdir/usr/bin
  install -Dm755 ../unison      $pkgdir/usr/bin/unison
  install -Dm755 ../unison-gtk2 $pkgdir/usr/bin/unison-gtk2

# install a .desktop file; create a compliant icon from ico file and install the png
  install -Dm644 ../unison.desktop $pkgdir/usr/share/applications/unison.desktop
  convert win32rc/U.ico unison.png
  install -Dm644 unison-1.png $pkgdir/share/pixmaps/unison.png
## make symlink for .desktop file
  cd $pkgdir/usr/bin
  ln -s unison-gtk2 unison-x11
}

md5sums=('91ff2ef4141aede9af719fdd5e848bcb'
         '2daecba7705455a8e4b769e48b059872')
