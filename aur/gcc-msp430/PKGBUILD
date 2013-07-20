# Contributor: Rick W. Chen <stuffcorpse at archlinux dot us>

pkgname=gcc-msp430
pkgver=4.6.3
pkgrel=4
pkgdesc="GNU toolchain for the TI MSP430 processor"
arch=('i686' 'x86_64')
url="http://sourceforge.net/projects/mspgcc/"
license=('GPL')
makedepends=('binutils-msp430')
depends=('elfutils' 'libmpc')
options=(!strip !emptydirs !libtool)

_mspgcc_ver=20120406
_gnu_mirror="http://ftpmirror.gnu.org"
_sf_base="http://sourceforge.net/projects/mspgcc/files"
_patches_base="${_sf_base}/Patches/LTS/${_mspgcc_ver}"

_patches=(msp430-gcc-${pkgver}-20120406-sf3540953.patch
          msp430-gcc-${pkgver}-20120406-sf3559978.patch)

source=("http://sourceforge.net/projects/mspgcc/files/mspgcc/mspgcc-${_mspgcc_ver}.tar.bz2"
        "${_gnu_mirror}/gcc/gcc-${pkgver}/gcc-${pkgver}.tar.bz2"
        "${_patches[0]}::${_patches_base}/${_patches[0]}/download"
        "${_patches[1]}::${_patches_base}/${_patches[1]}/download"
        "0001_gcc-doc-texinfo-5.0.patch")
sha1sums=('cc96a7233f0b1d2c106eff7db6fc00e4ed9039a8'
          'ce317ca5c8185b58bc9300182b534608c578637f'
          '9de4e74d8ceb2005409e03bf671e619f2e060082'
          '3721d13fd9a19df60fe356e082e6cea4ea637dbc'
          '0f5e63dc6a689976014c6cc87d5be28eb4ee922b')

_builddir="${srcdir}/build"

build() {
  _patch_name="msp430-gcc-${pkgver}-${_mspgcc_ver}.patch"
  (cd "${srcdir}/gcc-${pkgver}" &&
    patch -p1 < "${srcdir}/mspgcc-${_mspgcc_ver}/${_patch_name}" &&
    patch -p1 < "${srcdir}/0001_gcc-doc-texinfo-5.0.patch" &&
    for patch in ${_patches[@]} ; do
      msg "Applying ${patch}"
      patch -p1 < "${srcdir}/${patch}"
    done)

  rm -frv ${_builddir}
  mkdir -p ${_builddir} && cd ${_builddir}
  "${srcdir}/gcc-${pkgver}/configure" \
               CFLAGS_FOR_TARGET="-Os" \
               --prefix=/usr \
               --infodir=/usr/share/info \
               --mandir=/usr/share/man \
               --disable-libssp \
               --disable-nls \
               --target=msp430 \
               --enable-languages=c,c++ \
               --with-gnu-as \
               --with-gnu-ld \
               --with-as=/usr/bin/msp430-as \
               --with-ld=/usr/bin/msp430-ld \
               --with-pkgversion="mspgcc_${_mspgcc_ver}"
  make
}

package() {
  cd ${_builddir}
  make DESTDIR=${pkgdir} install

  rm -f  ${pkgdir}/usr/lib/libiberty.a
  rm -rf ${pkgdir}/usr/share/man/man7
  rm -rf ${pkgdir}/usr/share/info

  msg "Stripping debugging symbols from binaries"
  local binary
  find ${pkgdir} -type f 2>/dev/null | while read binary ; do
    case "$(file -biz "$binary")" in
      *compressed-encoding*)      # Skip compressed binarys
        ;;
      *application/x-executable*) # Binaries
        /usr/bin/strip "$binary" >/dev/null 2>&1 ;;
    esac
  done
}

# vim:set sts=2 ts=2 sw=2 et:
