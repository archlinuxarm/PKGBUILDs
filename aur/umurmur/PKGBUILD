# Maintainer: Robert Knauer <robert@privatdemail.net>
# Contributor : xav <xav at ethertricks dot net>

pkgname=umurmur
pkgver=0.2.13
pkgrel=1
pkgdesc="A minimalistic Mumble server"
arch=('i686' 'x86_64')
url="http://code.google.com/p/umurmur/"
license=('custom')
depends=('openssl' 'libconfig' 'protobuf-c')
source=(
  "http://${pkgname}.googlecode.com/files/${pkgname}-${pkgver}.tar.gz"
  "${pkgname}.service"
)
sha256sums=(
  'ac1595fa47ce6bd1e0706dd609293f745d73a59af00a7a04ddc0f9ae243997be'
  '86f396a042d02b9f9ed9f23596ed650ae7604d2de3e0f560f1f2fe03167fbbba'
)
install="${pkgname}.install"
backup=(
  'etc/umurmur/umurmur.conf'
)

#prepare() {
#  cd "${srcdir}/${pkgname}-${pkgver}"
#  patch 'src/client.c' "${srcdir}/opus.patch"
#}

build() {
  cd "${srcdir}/${pkgname}-${pkgver}"
  ./configure --prefix=/usr --mandir=/usr/share/man --with-ssl=openssl
  make
}

package() {
  cd "${srcdir}/${pkgname}-${pkgver}"
  make DESTDIR=${pkgdir} install || return 1
  install -Dm644 'umurmur.conf.example' "${pkgdir}/etc/umurmur/umurmur.conf"
  install -Dm644 "${srcdir}/umurmur.service" "${pkgdir}/usr/lib/systemd/system/umurmur.service"
  install -Dm644 'LICENSE' "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
}
