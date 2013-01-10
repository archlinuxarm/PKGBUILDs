# Maintainer: Marti Raudsepp <marti@juffo.org>

pkgname=flashbench-git
pkgver=20110219
pkgrel=1
pkgdesc="Tool for benchmarking and classifying flash memory drives"
arch=(i686 x86_64)
license=('unknown')
url="https://lwn.net/SubscriberLink/428584/354d16fe00c90072/"
depends=()
makedepends=()
replaces=('flashbench')
provides=('flashbench')
conflicts=('flashbench')
source=()

_gitroot="git://git.linaro.org/people/arnd/flashbench.git"
_gitname="flashbench"

build() {
  cd "$srcdir"
  msg "Connecting to GIT server...."

  if [ -d $_gitname ] ; then
    cd $_gitname && git pull origin
    msg "The local files are updated."
  else
    git clone $_gitroot $_gitname
  fi

  msg "GIT checkout done or server timeout"
  msg "Starting make..."

  rm -rf "$srcdir/$_gitname-build"
  git clone "$srcdir/$_gitname" "$srcdir/$_gitname-build"
  cd "$srcdir/$_gitname-build"

  make
  mkdir -p $pkgdir/usr/bin
  install -m755 flashbench erase $pkgdir/usr/bin/
}
