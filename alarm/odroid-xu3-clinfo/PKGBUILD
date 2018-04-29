# Maintainer: Mateusz Kaczanowski <kaczanowski.mateusz@gmail.com>

pkgname=odroid-xu3-clinfo
pkgver=2.2.18.04.06
pkgrel=1
pkgdesc="A simple OpenCL application that enumerates all available platform and device properties for ODROID-XU3/XU4 Mali driver"
arch=('armv7h')
url="https://github.com/Oblomov/clinfo"
license=('custom:Public Domain')
depends=('odroid-xu3-libgl')
conflicts=('clinfo odroid-xu3-clinfo')
makedepends=('odroid-xu3-libgl-headers')
source=(clinfo-${pkgver}.tar.gz::"${url}/archive/${pkgver}.tar.gz")
sha256sums=('f77021a57b3afcdebc73107e2254b95780026a9df9aa4f8db6aff11c03f0ec6c')

build() {
    CFLAGS="${CFLAGS} -I /usr/include/CL_1_2/"
    LDFLAGS="${LDFLAGS} -L /usr/lib/mali-egl"

    cd clinfo-${pkgver}
    sed -i "s/OpenCL/mali/g" Makefile
    make
}

package() {
    cd clinfo-${pkgver}
    install -Dm755 clinfo -t "${pkgdir}"/usr/bin/
    install -Dm644 LICENSE -t "${pkgdir}"/usr/share/licenses/${pkgname}
    install -Dm644 man1/clinfo.1 -t "${pkgdir}"/usr/share/man/man1/
}
