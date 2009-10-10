# $Id: PKGBUILD 82 2009-07-17 19:56:55Z aaron $
# Maintainer: Allan McRae <mcrae_allan@hotmail.org>

pkgname=sunbird-i18n
pkgver=0.9
pkgrel=1
_languages=ca,cs,da,de,es-AR,es-ES,eu,fr,ga-IE,hu,is,it,ja,ka,ko,lt,nb-NO,nl,nn-NO,pl,pt-BR,pt-PT,ro,ru,sk,sl,sv-SE,uk,zh-CN,zh-TW
pkgdesc="Language packs for Sunbird"
arch=('i686' 'x86_64')
license=('MPL')
url="http://www.mozilla.com/"
depends=("sunbird=${pkgver}")
makedepends=('unzip')
eval source=(ftp://ftp.mozilla.org/pub/mozilla.org/calendar/sunbird/releases/${pkgver}/langpacks/{${_languages}}.xpi)
md5sums=('d3de5b43b25f0d8b68e515dbb06052e7'
         '25152b290f87bfc02445f8297a403644'
         'aa0e4022bd538cd033d94c0571a59b2c'
         '3f549ba15e76abbd153e0b0342f5ebdb'
         '7b8163f0a458ae980fa9d37454289d73'
         '74a9be1292d50c645ff33e1cc2c378ff'
         '7e54e3a2bcf842d14803bda2862b4db2'
         '1b4d394049c17175e27d502d657b5938'
         '21993a02c28c4ee2b9cb8eab483d5198'
         '78c92149bb63a35be40f4dffbd639638'
         '161b183b3723454823d1b5d8d38e46e8'
         '5500c10aa729ccc6002b6a541f7dc9f6'
         'b0207b572fa9775f2088743e1f7dc0aa'
         '9e85fd0d3fa4cd030b255d4c64b88d80'
         'dbbbdc6cf6db7378e05d2ab50f403ea2'
         '94359e18b7271b0dd6b37ded878a4659'
         '15b3a56ca4107da58c2cfc53d88051f5'
         '3ae36d5a1cd2cea41f28bf40bec99f56'
         'd5ee25458f7517f6a801d9f96f12392b'
         'd923f4a955a241c0863ea28a4b794d59'
         'a4d64b260795c9f16bca3a2cc4739990'
         'dda6b745526e85126012e374476c57cc'
         'd871b87338082e8b9ecc5c96ae508d11'
         '961e62525a09271add4d0d9cfe071402'
         '3d891899835d73506ce9c793acccac2b'
         'f05e4a3e93394693be844c90e2ca0f7c'
         '27d1175daf451e4ecaabe648fe76640b'
         'a0a35823fdf3a1fa3f217eebe447b902'
         'ef85d04ebcb24b864fc0357568efbfe8'
         '516dae7254589e17c0fc3bba48ec68c4')

build() {
  cd ${srcdir}
  for lang in $(echo "${_languages}" | sed 's|,| |g'); do
    unzip -o ${lang}.xpi
    sed -i "s|chrome/||" chrome.manifest || return 1
    install -D -m 644 chrome/${lang}.jar \
      ${pkgdir}/usr/lib/sunbird-${pkgver}/chrome/${lang}.jar || return 1
    install -D -m 644 chrome/calendar-${lang}.jar \
      ${pkgdir}/usr/lib/sunbird-${pkgver}/chrome/calendar-${lang}.jar || return 1
    install -D -m 644 chrome.manifest \
      ${pkgdir}/usr/lib/sunbird-${pkgver}/chrome/${lang}.manifest || return 1
  done
}
