# Maintainer: Ruben Silveira <ruben@silveira.ws>

pkgname=qcontrol
pkgver=0.5.2
pkgrel=3
pkgdesc="Daemon and command line tool which control the various peripherals that are present on some NAS devices"
arch=('i686' 'arm' 'armv7h')
buildarch=6
url="https://gitorious.org/qcontrol/pages/Home"
license=('GPL3')
depends=('lua51')
source=(http://www.hellion.org.uk/${pkgname}/releases/${pkgver}/${pkgname}-${pkgver}.tar.xz)
sha1sums=('f6029e69f6f427aac589c37c9ba9baf1369b735d')

build() {
  cd ${pkgname}-${pkgver}
  make all
}

package() {
  cd ${pkgname}-${pkgver}

  install -Dm750 qcontrol "$pkgdir/usr/sbin/qcontrol"
  install -dm755 "$pkgdir/etc/qcontrol"
  install -Dm644 examples/* "$pkgdir/etc/qcontrol/"
  install -dm755 "$pkgdir/etc/qcontrol.d"
  install -dm755 "$pkgdir/usr/lib/systemd/system"
  install -Dm644 systemd/* "$pkgdir/usr/lib/systemd/system/"

  sed -i '/^Requires=/i\
\
# Mantainer Note: As the device never becomes active, this has no effect

' "$pkgdir/usr/lib/systemd/system/qcontrold.service"
  sed -i 's/^Requires=\(.*\)$/#Requires=\1/gi' "$pkgdir/usr/lib/systemd/system/qcontrold.service"
  sed -i 's/^After=\(.*\)$/#After=\1\
/gi' "$pkgdir/usr/lib/systemd/system/qcontrold.service"

cat >> "$pkgdir/usr/lib/systemd/system/qcontrold.service" << EOF

# Mantainer Note: The service should start only at the end of the startup sequence
Type=idle

# Mantainer Note: Shamelessly ripped off from the Debian package
ExecStartPre=-/usr/bin/sh -c '{ grep "QNAP TS-119/TS-219\|QNAP TS-41x" /proc/cpuinfo > /dev/null 2>&1 && /sbin/qcontrol --direct watchdog off > /dev/null 2>&1; } || /usr/bin/true'
EOF
}
