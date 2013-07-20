# Contributor: Rick W. Chen <stuffcorpse at archlinux dot us>
# Contributor: nboichat <nicolas-aur at boichat.ch>

pkgname=msp430-gdb
pkgver=7.2a
_gdbver=7.2
pkgrel=5
pkgdesc="GNU debugger for MSP430"
arch=('i686' 'x86_64')
url="http://mspgcc4.sourceforge.net/"
license=('GPL')
depends=('gcc-msp430' 'python2')
makedepends=('gcc-msp430')

_mspgcc_ver=20120406
_gdb_patch_ver=20111205
_gnu_mirror="http://ftpmirror.gnu.org"

source=("http://sourceforge.net/projects/mspgcc/files/mspgcc/mspgcc-${_mspgcc_ver}.tar.bz2"
        "${_gnu_mirror}/gdb/gdb-${pkgver}.tar.bz2"
        "0001_bfd-doc-texinfo-5.0.patch"
        "0002_bfdio-pass-correct-size.patch")
sha1sums=('cc96a7233f0b1d2c106eff7db6fc00e4ed9039a8'
          '14daf8ccf1307f148f80c8db17f8e43f545c2691'
          '1670af85f2b78d794559e81cfcadf6f10e17a032'
          'db00a6d342cb5aab00e61fd795de4c499ae0e820')

build() {
  _builddir="${srcdir}/build"

  _patch_name="msp430-gdb-${_gdbver}a-${_gdb_patch_ver}.patch"
  (cd "${srcdir}/gdb-${_gdbver}" &&
    patch -p1 < "${srcdir}/mspgcc-${_mspgcc_ver}/${_patch_name}" &&
    patch -p1 < "${srcdir}/0001_bfd-doc-texinfo-5.0.patch" &&
    patch -p1 < "${srcdir}/0002_bfdio-pass-correct-size.patch")

  # Fix configure problem if CPPFLAGS contains "-D_FORTIFY_SOURCE=2"
  export CPPFLAGS="$CPPFLAGS -O2"
  rm -frv ${_builddir}
  mkdir -p ${_builddir} && cd ${_builddir}
  "${srcdir}/gdb-${_gdbver}/configure" \
    --target=msp430 \
    --prefix=/usr \
    --program-prefix="msp430-" \
    --enable-languages=c,c++ \
    --mandir=/usr/share/man \
    --infodir=/usr/share/info \
    --with-python=python2 \
    --disable-nls
  make
}

package() {
  _builddir="${srcdir}/build"

  cd ${_builddir}
  make DESTDIR=${pkgdir} install

  cd ${pkgdir}/usr/share/info
  for file in annotate stabs ; do
    mv ${file}.info "msp430-${file}.info"
  done

  for file in gdb.info* gdbint.info* ; do
    mv ${file} "msp430-${file}"
  done

  rm -fr ${pkgdir}/usr/lib/libiberty.a
  rm -fr ${pkgdir}/usr/share/info/dir
  rm -fr ${pkgdir}/usr/share/info/bfd.info
  rm -fr ${pkgdir}/usr/share/info/configure.info
  rm -fr ${pkgdir}/usr/share/info/standards.info

  rm -fr ${pkgdir}/usr/share/gdb/syscalls/amd64-linux.xml
  rm -fr ${pkgdir}/usr/share/gdb/syscalls/gdb-syscalls.dtd
  rm -fr ${pkgdir}/usr/share/gdb/syscalls/i386-linux.xml
  rm -fr ${pkgdir}/usr/share/gdb/syscalls/ppc-linux.xml
  rm -fr ${pkgdir}/usr/share/gdb/syscalls/ppc64-linux.xml
  rm -fr ${pkgdir}/usr/share/gdb/syscalls/sparc-linux.xml
  rm -fr ${pkgdir}/usr/share/gdb/syscalls/sparc64-linux.xml
}

# vim:set sts=2 ts=2 sw=2 et:
