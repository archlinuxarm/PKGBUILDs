# Maintainer: Super Tecno Gym <banana@grrlz.net>

pkgname="tqftpserv"
pkgdesc="Trivial File Transfer Protocol server over AF_QIPCRTR"
pkgver=1.1.1
pkgrel=1
arch=(aarch64 x86_64)
url="https://github.com/linux-msm/tqftpserv"
license=("BSD-3-Clause")
conflicts=("${pkgname%-git}")
groups=(qcom-icnss-wlan)
_srcname="${pkgname}-${pkgver}"
source=("${_srcname}.tar.gz::https://github.com/linux-msm/tqftpserv/archive/refs/tags/v${pkgver}.tar.gz")
sha256sums=('7232cfdc76de42e20d4efa45a0206ab95513fb31c63148452d44c745a462789d')
makedepends=(meson)

build() {
        cd "$_srcname"

        mkdir -p build
        meson setup build --prefix=/usr
        meson compile -C build

}

package() {
        cd "$_srcname"

        meson install -C build --destdir "$pkgdir"
        install -Dm644 LICENSE "$pkgdir"/usr/share/licenses/$pkgname/COPYING
}
