# Mali Utgard GL Driver for Meson Platforms
# Maintainer: Joseph Kogut <joseph.kogut@gmail.com>

buildarch=8

pkgbase=mali-utgard-meson-libgl
pkgname=("${pkgbase}-x11" "${pkgbase}-fb")
pkgver=r6p1
pkgrel=1
arch=('aarch64')
url="http://github.com/jakogut/mali-utgard-meson-libgl.git"
license=('Proprietary')
depends=('mesa-libgl')
makedepends=('git')
source=("http://openlinux.amlogic.com:8000/download/ARM/filesystem/arm-buildroot-2016-08-18-5aaca1b35f.tar.gz"
        'mali.conf'
        '99-mali.rules')
md5sums=('4f391268947279d5d90f74066c27e5cf'
         '40f5104200cfceb12b4623d219646d4e'
         '8e2a04645c507b4128a8e8554f8cfd66')

# libMali gets linked to provide these
_gles_libs=('libEGL.so'
	    'libEGL.so.1'
	    'libEGL.so.1.4'
	    'libGLESv1_CM.so.1'
	    'libGLESv1_CM.so.1.1'
	    'libGLESv2.so'
	    'libGLESv2.so.2'
	    'libGLESv2.so.2.0')

package_mali-utgard-meson-libgl-x11() {
  pkgdesc="Mali Utgard Meson driver (X11)"
  conflicts=('odroid-c2-libgl' 'mali-utgard-meson-libgl')
  provides=('mali-utgard-meson-libgl')

  install -d "${pkgdir}"/usr/lib/mali-egl
  install -d "${pkgdir}"/etc/ld.so.conf.d
  install -d "${pkgdir}"/usr/lib/udev/rules.d
  cp -a buildroot/package/opengl/src/lib/arm64/r6p1/m450-X/lib* "${pkgdir}"/usr/lib/mali-egl
  cp "${srcdir}"/mali.conf "${pkgdir}"/etc/ld.so.conf.d
  cp "${srcdir}"/99-mali.rules "${pkgdir}"/usr/lib/udev/rules.d

  for lib in ${_gles_libs[@]}
  do
	  ln -sf /usr/lib/mali-egl/libMali.so "${pkgdir}/usr/lib/mali-egl/$lib"
  done
}

package_mali-utgard-meson-libgl-fb() {
  pkgdesc="Mali Utgard Meson driver (framebuffer)"
  conflicts=('odroid-c2-libgl mali-utgard-meson-libgl')
  provides=('mali-utgard-meson-libgl')

  install -d "${pkgdir}"/usr/lib/mali-egl
  install -d "${pkgdir}"/etc/ld.so.conf.d
  install -d "${pkgdir}"/usr/lib/udev/rules.d
  cp -a buildroot/package/opengl/src/lib/arm64/r6p1/m450/lib* "${pkgdir}"/usr/lib/mali-egl
  cp "${srcdir}"/mali.conf "${pkgdir}"/etc/ld.so.conf.d
  cp "${srcdir}"/99-mali.rules "${pkgdir}"/usr/lib/udev/rules.d

  for lib in ${_gles_libs[@]}
  do
	  ln -sf /usr/lib/mali-egl/libMali.so "${pkgdir}/usr/lib/mali-egl/$lib"
  done
}
