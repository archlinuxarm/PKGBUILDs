# ODROID-XU3/XU4 Mali GL Driver
# Maintainer: Kevin Mihelich <kevin@archlinuxarm.org>

buildarch=4

pkgbase=odroid-xu3-libgl
pkgname=("${pkgbase}-x11" "${pkgbase}-fb" "${pkgbase}-headers")
pkgver=r17p0
pkgrel=1
_commit=e3fbb79c1df377da21b38bd4f57ae1289c59e480
arch=('armv7h')
url="http://www.hardkernel.com/"
license=('Proprietary')
depends=('mesa-libgl')
makedepends=('git')
source=("git+https://github.com/mdrjr/5422_mali.git#commit=${_commit}"
        'mali-xu3.conf')
md5sums=('SKIP'
         '40f5104200cfceb12b4623d219646d4e')

package_odroid-xu3-libgl-x11() {
  pkgdesc="ODROID-XU3/XU4 Mali driver (X11)"
  conflicts=('odroid-xu3-libgl')
  provides=('odroid-xu3-libgl')

  install -d "${pkgdir}"/usr/lib/mali-egl
  install -d "${pkgdir}"/etc/ld.so.conf.d
  cp -a 5422_mali/x11/lib* "${pkgdir}"/usr/lib/mali-egl
  cp "${srcdir}"/mali-xu3.conf "${pkgdir}"/etc/ld.so.conf.d
}

package_odroid-xu3-libgl-fb() {
  pkgdesc="ODROID-XU3/XU4 Mali driver (framebuffer)"
  conflicts=('odroid-xu3-libgl')
  provides=('odroid-xu3-libgl')

  install -d "${pkgdir}"/usr/lib/mali-egl
  install -d "${pkgdir}"/etc/ld.so.conf.d
  cp -a 5422_mali/fbdev/lib* "${pkgdir}"/usr/lib/mali-egl
  cp "${srcdir}"/mali-xu3.conf "${pkgdir}"/etc/ld.so.conf.d
}

package_odroid-xu3-libgl-headers() {
  pkgdesc="ODROID-XU3/XU4 Mali driver headers"
  conflicts=('opencl-headers')
  provides=('opencl-headers')

  install -d "${pkgdir}"/usr/include
  cp -a 5422_mali/headers/CL{,_1_2,_2_0} "${pkgdir}"/usr/include
}
