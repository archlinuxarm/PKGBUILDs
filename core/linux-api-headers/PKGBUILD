# Maintainer: Giancarlo Razzolini <grazzolini@archlinux.org>
# Maintainer: Frederik Schwan <freswa at archlinux dot org>
# Contributor: Bartłomiej Piotrowski <bpiotrowski@archlinux.org>
# Contributor: Allan McRae <allan@archlinux.org>

# toolchain build order: linux-api-headers->glibc->binutils->gcc->glibc->binutils->gcc

pkgname=linux-api-headers
pkgver=6.16
pkgrel=2
pkgdesc='Kernel headers sanitized for use in userspace'
arch=('x86_64')
url='https://www.gnu.org/software/libc'
license=(GPL-2.0-only)
makedepends=(rsync)
source=(https://www.kernel.org/pub/linux/kernel/v${pkgver:0:1}.x/linux-${pkgver}.tar.{xz,sign})
sha256sums=('1a4be2fe6b5246aa4ac8987a8a4af34c42a8dd7d08b46ab48516bcc1befbcd83'
            'SKIP')
validpgpkeys=('ABAF11C65A2970B130ABE3C479BE3E4300411886'   # Linus Torvalds
              '647F28654894E3BD457199BE38DBBDC86092693E')  # Greg Kroah-Hartman

build() {
  cd linux-$pkgver

  make mrproper
}

package() {
  cd linux-$pkgver
  make INSTALL_HDR_PATH="$pkgdir/usr" headers_install
}
