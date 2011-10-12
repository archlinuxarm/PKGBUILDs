# Contributor: Mikhail Vorozhtsov <mikhail.vorozhtsov@gmail.com>
# Maintainer: Gustavo Alvarez <sl1pkn07@gmail.com>

# ALARM changes were made to the GYP_DEFINES and CFLAGS and to the
# target architecture.
# macau: fix v15.x for armhf building

plugrel=2
noautobuild=1

pkgname=chromium15
pkgver=15.0.858.0
pkgrel=1
_buildtype=Release
test "x${DEBUG}" = "xyes" && _buildtype=Debug
pkgdesc='The open-source project behind Google Chrome (Dev channel)'
arch=('armv7h')
url='http://www.chromium.org/'
license=('BSD')
depends=('alsa-lib' 'xdg-utils' 'hicolor-icon-theme' 'bzip2' 'libevent' 'libxss' 'libpng' 'libjpeg' 'cairo' 'dbus-glib'
         'glib2' 'gtk2' 'nss' 'nspr' 'ffmpeg' 'libvpx' 'libxml2' 'libxslt' 'libxtst' 'icu')
makedepends=('git' 'python2' 'gperf' 'yasm' 'mesa' 'gcc>=4.5.0-6' 'libgnome-keyring')
optdepends=('libgnome-keyring')
replaces=('chromium-dev')
conflicts=('chromium-dev')
options=()
test ${_buildtype} = Debug && options[${#options[@]}]=!strip
install="${pkgname}.install"
source=(http://build.chromium.org/official/chromium-${pkgver}.tar.bz2
        ${pkgname}.desktop 
        ${pkgname}.sh)
md5sums=('c36aa5e0c97050a27208a83810230411'
	'7547bf60abed76fa2d3c314ceb9a4865'
	'0c526aae227cf26b73160dcfd6a52dbe')
_use_gconf=0

if test -x /usr/bin/gconftool-2; then
  _use_gconf=1
  depends[${#depends[@]}]=gconf
fi

build() {
  cd "${srcdir}/chromium-${pkgver}"

  msg "Patching sources..."

  msg "Save configuration in ~/.config/${pkgname}"
  sed -e "s/'filename': 'chromium-browser'/'filename': '${pkgname}'/" -e "s/'confdir': 'chromium'/'confdir': '${pkgname}'/" -i chrome/chrome_exe.gypi
  sed \
    -e "s/config_dir\.Append(\"chromium\")/config_dir.Append(\"${pkgname}\")/" \
    -e "s/config_dir\.Append(\"chrome-frame\")/config_dir.Append(\"chrome-frame-${pkgname#chromium-}\")/" \
    -i chrome/common/chrome_paths_linux.cc
  msg2 "Done"

  msg "Force usage of python2"
  #find -type f -a -name '*.py' -exec sed -i -e 's|#![ ]*/usr/bin/python$|#!/usr/bin/python2|' -e 's|#![ ]*/usr/bin/env python$|#!/usr/bin/env python2|' {} \;
  #find -type f -a -name '*.gyp*' -exec sed -i -e 's|<!(python |<!(python2 |g' -e "s|'python'|'python2'|g" {} \;
   rm -rf "${srcdir}"/python
   mkdir "${srcdir}"/python
   ln -s /usr/bin/python2 "${srcdir}"/python/python
   export PATH="${srcdir}"/python:$PATH
  msg2 "Done"

  msg "Fixing vpx config"
  sed s/libvpx.gyp:libvpx/libvpx.gyp:libvpx_include/g -i remoting/remoting.gyp
  msg2 "Done"
  
  msg "Patching Sources Sucessfull"
  
  msg "Building Chromium..."
  chromium_arch=arm
  GYP_DEFINES="\
    gcc_version=46 \
    arm_thumb=1 \
    no_strict_aliasing=1 \
    build_ffmpegsumo=1 \
    linux_sandbox_path=/usr/lib/${pkgname}/chromium-sandbox \
    linux_sandbox_chrome_path=/usr/lib/${pkgname}/chromium \
    release_extra_cflags='${CFLAGS} -lvpx -I/usr/include -DUSE_EABI_HARDFLOAT -Wno-error=unused-but-set-variable' \
    disable_nacl=1 \
    use_system_ffmpeg=0 \
    use_system_vpx=1 \
    proprietary_codecs=1 \
    use_system_libjpeg=1 \
    use_system_libxslt=1 \
    use_system_libxml=1 \
    use_system_bzip2=1 \
    use_system_zlib=1 \
    use_system_libpng=1 \
    use_system_yasm=1 \
    use_system_libevent=1 \
    use_system_icu=0 \
    use_system_ssl=0 \
    use_gconf=${_use_gconf} \
    use_cups=0 \
    werror= \
    target_arch=${chromium_arch} \
    linux_use_tcmalloc=0 \
    armv7=1 \
    arm_neon=0 \
    arm_fpu=vfpv3-d16 \
    enable_webrtc=0 \
    remoting=0 \
    disable_sse2=1"
  test ${_buildtype} = Release \
    && GYP_DEFINES="${GYP_DEFINES} \
                    linux_strip_binary=1 \
                    remove_webcore_debug_symbols=1"
  export GYP_DEFINES
  echo "${pkgver} ${GYP_DEFINES}" > current.config
  if test -f "last.config"; then
    if cmp last.config current.config; then
      msg2 "Configuration has not changed, reusing output files..."
    else
      msg2 "Configuration has changed, removing output files..."
      rm -rf out
    fi
  fi
  mv current.config last.config
  python2 build/gyp_chromium -f make --depth=. build/all.gyp -d general
  make BUILDTYPE=${_buildtype} ${MAKEFLAGS} chrome chrome_sandbox
}

package() {
  cd "${srcdir}/chromium-${pkgver}"

  chromium_home="${pkgdir}/usr/lib/${pkgname}"
  install -Dm755 -D out/${_buildtype}/chrome "${chromium_home}/chromium"
  install -Dm4555 -o root -g root -D out/${_buildtype}/chrome_sandbox "${chromium_home}/chromium-sandbox"
  install -Dm644 out/${_buildtype}/chrome.pak "${chromium_home}/chrome.pak"
  install -Dm644 out/${_buildtype}/resources.pak "${chromium_home}/resources.pak"
  
  if test ${_use_ffmpeg_system_libs} = 1; then
    for n in avcodec avdevice avfilter avformat avutil postproc swscale; do
      if test -e /usr/lib/lib${n}.so.[0-9]; then
        f=`echo /usr/lib/lib${n}.so.[0-9]`
      else
        f=`echo /usr/lib/lib${n}.so.[0-9][0-9]`
      fi
      f=`basename "$f"`
      ln -s ../$f "${chromium_home}/${f}"
    done
  else
    if test -e out/${_buildtype}/libffmpegsumo.so; then
      install -Dm644 out/${_buildtype}/libffmpegsumo.so "${chromium_home}/libffmpegsumo.so"
    fi
  fi

  cp -a out/${_buildtype}/locales out/${_buildtype}/resources "${chromium_home}/"
  find "${chromium_home}" -type f -name '*.d' -delete
  install -Dm644 out/${_buildtype}/chrome.1 "${pkgdir}/usr/share/man/man1/${pkgname}.1"

  install -Dm644 "${srcdir}/${pkgname}.desktop" "${pkgdir}/usr/share/applications/${pkgname}.desktop"
  for size in 16 22 24 32 48 128 256; do
    install -Dm644 chrome/app/theme/chromium/product_logo_${size}.png "${pkgdir}/usr/share/icons/hicolor/${size}x${size}/apps/${pkgname}.png"
  done
  install -Dm755 ${srcdir}/${pkgname}.sh "${pkgdir}/usr/bin/${pkgname}"

  install -Dm644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}




