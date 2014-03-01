# Maintainer:   Huulivoide      <gmail.com: jesse.jaara>
# Contributor:  Jonathan Hudson <daria.co.uk: jh+arch>


# If you are building this on the original 256MB Raspberry Pi
# you need to enable some swap (100MB ?) or the build will fail
# on OMXPlayerSubtitles.cpp


pkgname=omxplayer-git
pkgver=285.1091db2
pkgrel=1
pkgdesc="omxplayer is a command line media player for the RaspberryPi"
arch=('arm' 'armv6h')
url="https://github.com/popcornmix/omxplayer"
license=(GPL2)
depends=('ffmpeg' 'raspberrypi-firmware-tools' 'fbset')
makedepends=('git' 'boost')
optdepends=('ttf-freefont')
provides=(omxplayer)
conflicts=('omxplayer' 'omxplayer-bin')
source=(git://github.com/popcornmix/omxplayer.git
        Makefile.arch Makefile.include.arch)

pkgver() {
  cd omxplayer
  echo $(git rev-list --count HEAD).$(git rev-parse --short HEAD)
}

prepare() {
  cd "${srcdir}/omxplayer"

  cp ../Makefile.arch Makefile
  cp ../Makefile.include.arch Makefile.include

  sed 's|truetype/freefont|TTF|g' -i omxplayer.cpp
}

build() {
  cd "${srcdir}/omxplayer"

  make
}

package() {
  cd "${srcdir}/omxplayer"

  make DESTDIR="${pkgdir}" install
}

md5sums=('SKIP'
         '462e40e8da2e2b10cb32138e7885d285'
         'a8635442d02665944c04c18606453a04')
