# Maintainer: taotieren <admin@taotieren.com>

pkgname=qbootctl
pkgver=0.2.2
pkgrel=1
epoch=
pkgdesc="Qualcomm bootctl HAL for Linux."
arch=($CARCH)
url="https://github.com/linux-msm/qbootctl"
license=('GPL-3.0-or-later')
groups=()
depends=(glibc)
makedepends=(
    meson
    ninja)
checkdepends=()
optdepends=()
provides=(${pkgname})
conflicts=(${pkgname})
replaces=()
backup=()
options=()
install=
changelog=
source=("${pkgname}-${pkgver}.tar.gz::${url}/archive/refs/tags/${pkgver}.tar.gz")
noextract=()
sha256sums=('e66b361f86fac413f6a96974b4045a1fbc385f9eb6cee7fa8a26e0ca77c74a51')
#validpgpkeys=()

build() {
    arch-meson ${pkgname}-${pkgver} build
    ninja -C build
}

# check() {
#     meson test -C ${srcdir}/build
# }

package() {
    DESTDIR="${pkgdir}" ninja -C ${srcdir}/build install
}
