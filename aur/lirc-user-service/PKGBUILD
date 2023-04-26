# Maintainer: graysky <graysky AT archlinux DOT us>
# vim:set ts=2 sw=2 et:

pkgname=lirc-user-service
pkgver=1.6
pkgrel=1
pkgdesc="Run lircd as an unprivileged user for better stability and security"
arch=(any)
url="https://www.lirc.org/html/configuration-guide.html"
license=(MIT)
depends=(lirc systemd)
install=readme.install
source=(60-lirc.rules sysusers.conf)
md5sums=('cbed0097426c746550c687ae3d0310ec'
         '2a19b64c02e12256624cc15906bcf65a')

package() {
  install -Dm644 60-lirc.rules "$pkgdir/usr/lib/udev/rules.d/60-lirc.rules"
  install -Dm644 sysusers.conf "$pkgdir/usr/lib/sysusers.d/$pkgname.conf"
}
