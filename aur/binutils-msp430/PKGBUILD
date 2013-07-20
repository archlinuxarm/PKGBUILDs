# Contributor: Rick W. Chen <stuffcorpse at archlinux dot us>

pkgname=binutils-msp430
pkgver=2.21.1a
pkgrel=3
pkgdesc="A set of programs to assemble and manipulate binary and object files for the MSP430 architecture"
arch=('i686' 'x86_64')
url="http://sourceforge.net/projects/mspgcc/"
license=('GPL')
depends=('zlib')
options=('!emptydirs' '!libtool')

_mspgcc_ver=20120406
_gnu_mirror="http://ftpmirror.gnu.org"
_sf_base="http://sourceforge.net/projects/mspgcc/files"
_patches_base="${_sf_base}/Patches/LTS/${_mspgcc_ver}"

_patches=()

source=("${_sf_base}/mspgcc/mspgcc-${_mspgcc_ver}.tar.bz2"
        "${_gnu_mirror}/binutils/binutils-${pkgver}.tar.bz2"
        "0001_ld_makefile_libdir.patch"
        "0002_binutils-texinfo-5.0-gas-doc.patch"
        "0003-binutils-texinfo-5.0.patch")
sha1sums=('cc96a7233f0b1d2c106eff7db6fc00e4ed9039a8'
          '525255ca6874b872540c9967a1d26acfbc7c8230'
          '7a5d78fd94fd99dd544816db75a14c326c494e68'
          'b92aba28a090f214a650102ad0f0862c37b45e58'
          '417b1cdb35c41a08d3ca967c165c0d79588986e9')

_builddir="${srcdir}/build"

build() {
  _patch_name="msp430-binutils-${pkgver}-${_mspgcc_ver}.patch"
  (cd "${srcdir}/binutils-2.21.1" &&
    patch -p1 < "${srcdir}/mspgcc-${_mspgcc_ver}/${_patch_name}" &&
    patch -p0 < "${srcdir}/0001_ld_makefile_libdir.patch" &&
    patch -p1 < "${srcdir}/0002_binutils-texinfo-5.0-gas-doc.patch" &&
    patch -p1 < "${srcdir}/0003-binutils-texinfo-5.0.patch" &&
    for patch in ${_patches[@]} ; do
      msg "Applying ${patch}"
      patch -p1 < "${srcdir}/${patch}"
    done)

  rm -frv ${_builddir}
  mkdir -p ${_builddir} && cd ${_builddir}

  "${srcdir}/binutils-2.21.1/configure" \
      --prefix=/usr \
      --program-prefix="msp430-" \
      --disable-multilib \
      --disable-nls \
      --enable-install-libbfd \
      --infodir=/usr/share/info \
      --libdir=/usr/msp430/lib \
      --mandir=/usr/share/man \
      --target=msp430

  # This checks the host environment and makes sure all the necessary
  # tools are available to compile Binutils.
  make configure-host

  make tooldir=/usr all
}

check() {
  cd ${_builddir}

  # do not abort on errors - manually check log files
  make -k -j1 check || true
}

package() {
  cd ${_builddir}
  make DESTDIR=${pkgdir} tooldir=/usr install

  rm -f ${pkgdir}/usr/lib/libiberty.a
  rm -f ${pkgdir}/usr/man/man1/{dlltool,nlmconv,windres}*
  rm -f ${pkgdir}/usr/share/info/dir

  cd ${pkgdir}/usr/share/info
  for file in as bfd binutils configure gprof ld standards ; do
    mv ${file}.info "msp430-${file}.info"
  done

  for bin in addr2line ar as c++filt gprof ld nm objcopy \
             objdump ranlib readelf size strings strip
  do
    rm -f ${pkgdir}/usr/bin/${bin}
  done

  install -Dm644 "${srcdir}/binutils-2.21.1/COPYING" \
    "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}

# vim:set sts=2 ts=2 sw=2 et:
