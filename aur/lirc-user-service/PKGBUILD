# Maintainer: graysky <graysky AT archlinux DOT us>
# vim:set ts=2 sw=2 et:

pkgname=lirc-user-service
pkgver=1.5
pkgrel=2
pkgdesc="Run lircd as an unprivileged user for better stability and security"
arch=(any)
url="https://www.lirc.org/html/configuration-guide.html"
license=(MIT)
depends=(lirc systemd)
install=readme.install
source=(60-lirc.rules sysusers.conf tmpfiles.conf)
backup=(etc/systemd/system/lircd.service.d/override.conf)
md5sums=('cbed0097426c746550c687ae3d0310ec'
         '2a19b64c02e12256624cc15906bcf65a'
         '818c7fa6b32ab3edb56ef643450116f2')

package() {
  install -Dm644 60-lirc.rules "$pkgdir/usr/lib/udev/rules.d/60-lirc.rules"
  install -Dm644 sysusers.conf "$pkgdir/usr/lib/sysusers.d/$pkgname.conf"
  # prefer /etc/tmpfiles.d/ over /usr/lib/tmpfiles.d/
  # why?  lirc package inslalls there and if we do to systemd-tmpfiles
  # warns on execution which can confuse users
  install -Dm644 tmpfiles.conf "$pkgdir/etc/tmpfiles.d/$pkgname.conf"
}
