# Maintainer: Allan McRae <mcrae_allan@hotmail.com>
# Contributor: Hyperair <hyperair@gmail.com>

pkgname=gnome-keyring-sharp-svn
pkgver=87622
pkgrel=3
pkgdesc="A fully managed implementation of libgnome-keyring"
arch=('i686' 'x86_64')
url="http://www.mono-project.com/Libraries"
license=('custom')
depends=('ndesk-dbus>=0.4')
makedepends=('subversion')
provides=('gnome-keyring-sharp')
conflicts=('gnome-keyring-sharp')
source=()
md5sums=()

_svntrunk=http://anonsvn.mono-project.com/source/trunk/gnome-keyring-sharp
_svnmod=gnome-keyring-sharp

build() {
  export MONO_SHARED_DIR="${srcdir}/.wapi"
  mkdir -p "${MONO_SHARED_DIR}"
  cd "${srcdir}"

  if [ -d ${_svnmod}/.svn ]; then
    (cd ${_svnmod} && svn up -r ${pkgver})
  else
    svn co ${_svntrunk} --config-dir ./ -r ${pkgver} ${_svnmod}
  fi

  msg "SVN checkout done or server timeout"
  msg "Starting make..."

  rm -rf "${srcdir}/${_svnmod}-build"
  svn export ${_svnmod} ${_svnmod}-build
  cd ${_svnmod}-build

  #
  # BUILD
  #
  ./autogen.sh --prefix=/usr --sysconfdir=/etc || return 1
  make || return 1
  make DESTDIR="${pkgdir}" install || return 1
  rm -rf "${MONO_SHARED_DIR}"
  ln -sf gnome-keyring-sharp-1.0.pc "${pkgdir}/usr/lib/pkgconfig/gnome-keyring-sharp.pc"  

  install -Dm644 COPYING ${pkgdir}/usr/share/licenses/gnome-keyring-sharp-svn/license
}

