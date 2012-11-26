# Contributor tomasgroth at yahoo.dk
# Contributor WarheadsSE <max@warheads.net>
pkgname=xbmc-rbp-git
pkgver=20121125
pkgrel=1
buildarch=16

pkgdesc="A software media player and entertainment hub for digital media for the Raspberry Pi"
arch=('armv6h')
url="http://xbmc.org"
license=('GPL' 'custom')
depends=('hicolor-icon-theme' 'fribidi' 'lzo2' 'smbclient' 'libtiff' 'libva' 'libpng' 'libcdio' 'yajl' 'libmysqlclient' 'libjpeg-turbo' 'libsamplerate' 'libssh' 'libmicrohttpd' 'sdl_image' 'python2' 'libass' 'libmpeg2' 'libmad' 'libmodplug' 'jasper' 'rtmpdump' 'unzip' 'xorg-xdpyinfo' 'libbluray' 'libnfs' 'afpfs-ng' 'libshairport' 'avahi' 'bluez' 'tinyxml' 'raspberrypi-firmware' 'libcec-rpi' 'libplist' 'swig' 'taglib' 'ffmpeg')

makedepends=('boost' 'cmake' 'gperf' 'nasm' 'zip' 'udisks' 'upower' 'bluez' 'git' 'autoconf' 'openjdk6')
optdepends=(
  'lirc: remote controller support'
  'udisks: automount external drives'
  'upower: used to trigger suspend functionality'
  'unrar: access compressed files without unpacking them'
)
source=(xbmc-ae04d99-321-texturepacker-hostflags-and-rework.patch)

md5sums=('fc6a925a09ba1b13d84daf1121b42ab9')

_gitroot="git://github.com/xbmc"
_gitname="xbmc"

_prefix=/usr

build() {
  cd "${srcdir}"

  msg2 "Connecting to GIT server..."
  if [[ -d "${_gitname}" ]]; then
    cd "${_gitname}" && git pull origin
    msg2 "The local files are updated."
  else
    git clone --depth 1 "${_gitroot}/${_gitname}"
  fi
  msg2 "GIT checkout done or server timeout."

  cd "${srcdir}/${_gitname}"

  # fix lsb_release dependency
  sed -i -e 's:/usr/bin/lsb_release -d:cat /etc/arch-release:' xbmc/utils/SystemInfo.cpp

  # Patch to fix TexturePacker build. 
  patch -i ${srcdir}/xbmc-ae04d99-321-texturepacker-hostflags-and-rework.patch -p1 

  # Bootstrapping XBMC
  ./bootstrap

  # Configuring XBMC
  export PYTHON_VERSION=2  # external python v2
  # we need to compile for armv6 instead of armv5 to avoid problems compiling assembler code
  export CFLAGS="-O3 -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -mabi=aapcs-linux -pipe -fstack-protector --param=ssp-buffer-size=4 -D_FORTIFY_SOURCE=2 -I/opt/vc/include/ -I/opt/vc/include/IL -I/opt/vc/include/interface/vcos/pthreads"
  export CXXFLAGS="-O3 -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -mabi=aapcs-linux -fstack-protector --param=ssp-buffer-size=4 -D_FORTIFY_SOURCE=2 -I/opt/vc/include/ -I/opt/vc/include/IL -I/opt/vc/include/interface/vcos/pthreads"
  export LDFLAGS="$LDFLAGS -L/opt/vc/lib"
#  export MAKEFLAGS="-j1"
  ./configure --prefix=$_prefix --exec-prefix=$_prefix  \
  --enable-gles --disable-sdl --disable-x11 --disable-xrandr --disable-openmax \
  --disable-optical-drive --disable-dvdcss --disable-joystick --disable-debug \
  --disable-crystalhd --disable-vtbdecoder --disable-vaapi --disable-vdpau \
  --disable-pulse --disable-projectm --with-platform=raspberry-pi --enable-optimizations \
  --enable-libcec --enable-player=omxplayer --enable-external-ffmpeg

  make
}

package() {
  cd "${srcdir}/${_gitname}"
  # Running make install
  make DESTDIR="${pkgdir}" install

  # run feh with python2
  sed -i -e 's/python/python2/g' ${pkgdir}${_prefix}/bin/xbmc

  # Remove checks that doesn't apply to the raspberry pi
  head -n 171 "${pkgdir}${_prefix}/share/xbmc/FEH.py" > "${pkgdir}${_prefix}/share/xbmc/FEH.py.new"
  mv "${pkgdir}${_prefix}/share/xbmc/FEH.py.new"  "${pkgdir}${_prefix}/share/xbmc/FEH.py"

  # lsb_release fix
  sed -i -e 's/which lsb_release > \/dev\/null/\[ -f \/etc\/arch-release ]/g' "${pkgdir}${_prefix}/bin/xbmc"
  sed -i -e "s/lsb_release -a 2> \/dev\/null | sed -e 's\/\^\/    \/'/cat \/etc\/arch-release/g" "${pkgdir}${_prefix}/bin/xbmc"

  # Tools
  install -D -m 0755 "${srcdir}/${_gitname}/tools/TexturePacker/TexturePacker" "${pkgdir}${_prefix}/share/xbmc/"

  # Licenses
  install -d -m 0755 "${pkgdir}${_prefix}/share/licenses/${pkgname}"
  for licensef in LICENSE.GPL copying.txt; do
    mv "${pkgdir}${_prefix}/share/doc/xbmc/${licensef}" "${pkgdir}${_prefix}/share/licenses/${pkgname}"
  done

  # Create lib links
  mkdir -p "${pkgdir}"/etc/ld.so.conf.d/
 
  # ensure we can load libs
  echo "/opt/vc/lib/" > "${pkgdir}"/etc/ld.so.conf.d/xbmc-rbp-git.conf
}
