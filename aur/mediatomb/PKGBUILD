# Maintainer: S Leduc <sebastien@sleduc.fr>
# Contributor: Martin Villagra <mvillagra0@gmail.com>
# Contributor: William Rea <sillywilly@gmail.com>
# Contributor: Nikhil Bysani <nikron@gmail.com>
# Contributor: Mika HynnÃ¤ <igheax@gmail.com>
# Contributor: Jonathan Conder <jonno.conder@gmail.com>
# Contributor: Peter Richard Lewis <plewis@aur.archlinux.org>

pkgname=mediatomb
pkgver=0.12.1
pkgrel=13
pkgdesc="Free UPnP/DLNA media server"
arch=('i686' 'x86_64' 'armv6h')
url="http://mediatomb.cc/"
license=('GPL')
depends=('file' 'curl' 'ffmpegthumbnailer' 'libexif' 'libmp4v2' 'sqlite3' 'taglib' 'libmariadbclient' 'js185')
optdepends=('mariadb: to store your music database in mariadb')
backup=('etc/conf.d/mediatomb')
install=mediatomb.install
source=("http://downloads.sourceforge.net/$pkgname/$pkgname-$pkgver.tar.gz"
        'mediatomb.service'
        'mediatomb-mariadb.service'
        'mediatomb.conf'
        'gcc46.patch'
        'tonewjs.patch'
        'jsparse.patch'
        'libav_0.7_support.patch'
        'libmp4v2_191_p497.patch'
        'libavformat.patch'
        'symlinks.patch')
sha256sums=('31163c34a7b9d1c9735181737cb31306f29f1f2a0335fb4f53ecccf8f62f11cd'
            'e46de674e49aa85116a8ff127908f7bac21198ce7625404004b8b7832eccd3f4'
            '9c917f0d6e568ce0ad77c0ed17e4bbaabc0e7a1c0a3e4772b786fb1565db9768'
            '70e4a4b89cef9a7f6f5f800e1793a6cb807f52b39e5a17d0a91356608b95e62d'
            '0c02a20032f0c296800b1bb9644638970c2dedbc5ab7141d66a637235e9da6ce'
            '2cd8f5628c3a38b290526f008bae351b90211825f86e5959bf95f140748de574'
            'd9a3062858900d32b977f0d50d168fd7d36785b6ecc038c019e661e27f7b1c17'
            'c6523e8bf5e2da89b7475d6777ef9bffe7d089752ef2f7b27b5e39a4130fb0ff'
            'd39c2f9aab051c5447461718fd0ec72cf5982f6c920a4a985a50831f34babe84'
            '76b11706d70ed8f5e157d96ca441c90c46c42176102fcb651b4ab1102b61bfee'
            '72f7532d7cd827ab655df652d2912175739fe16d2b1ad989d987a0b147a1d2e8')

build() {
  cd "$srcdir/$pkgname-$pkgver"
  patch -Np1 -i "$srcdir/gcc46.patch"
  patch -Np1 -i "$srcdir/tonewjs.patch"
  patch -Np1 -i "$srcdir/jsparse.patch"
  patch -Np1 -i "$srcdir/libav_0.7_support.patch"
  patch -Np1 -i "$srcdir/libmp4v2_191_p497.patch"
  patch -Np1 -i "$srcdir/libavformat.patch"
  patch -Np1 -i "$srcdir/symlinks.patch"

  ./configure --prefix=/usr \
              --enable-mysql \
              --enable-libmagic \
              --enable-libjs \
              --enable-ffmpeg
  make
}

package() {
  cd "$srcdir/$pkgname-$pkgver"

  make DESTDIR="$pkgdir/" install

  install -D -m0644 "$srcdir/mediatomb.service" "$pkgdir/usr/lib/systemd/system/mediatomb.service"
  install -D -m0644 "$srcdir/mediatomb-mariadb.service" "$pkgdir/usr/lib/systemd/system/mediatomb-mariadb.service"
  install -D -m0644 "$srcdir/mediatomb.conf" "$pkgdir/etc/conf.d/mediatomb"
}
