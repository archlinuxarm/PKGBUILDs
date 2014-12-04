pkgname=sabnzbd
_pkgname=SABnzbd
pkgver=0.7.20
pkgrel=1
pkgdesc="A web-interface based binary newsgrabber with NZB file support"
url="http://www.sabnzbd.org"
arch=("any")
license=("GPL")
depends=("curl" "par2cmdline"
         "python2" "python2-cheetah" "python2-yenc"
         "sqlite" "unrar" "unzip")
optdepends=("xdg-utils: registration of .nzb files" "python2-feedparser: rss support" "python2-pyopenssl: ssl support" "par2cmdline-tbb: par2 multi-threading")
install="${pkgname}.install"
backup=("etc/conf.d/sabnzbd" "opt/${pkgname}/${pkgname}.ini")
source=("http://downloads.sourceforge.net/sabnzbdplus/${_pkgname}-${pkgver}-src.tar.gz"
        "${pkgname}" "${pkgname}.desktop" "addnzb.sh" "nzb-2.png" "sab2_64.png" "x-nzb.xml" "${pkgname}.service" "${pkgname}.confd")
md5sums=('7972b2cdad0a3431262611c271ce5747'
         '48d60a1c626503c7fef1bc5374390513'
         'f9bd5485072714b11f8c30a28024dc4d'
         '69b9bcbcf67ff3e7a4cdd9f26e001341'
         '2a49c07b1e3e6448eabe92644315f983'
         'fdc878dd0f6f25617e627b04409abbbd'
         '11fb2cd1451e3725b08bfc2bd045be54'
         '7da9222f3b86abfed590950f48dc2e77'
         '8fc2607a7961fc643ef4f6640166322a')

package() {
  mkdir -p "${pkgdir}/opt/${pkgname}"
  touch "${pkgdir}/opt/${pkgname}/${pkgname}.ini"
  cp -rv "${srcdir}/${_pkgname}-${pkgver}/"* "${pkgdir}/opt/${pkgname}"

  # Fix for issues with Python 3
  find "${pkgdir}/opt/${pkgname}" -type f -exec sed -i 's/python/python2/g' {} \;
  find "${pkgdir}/opt/${pkgname}" -type d -exec chmod 755 {} \;
  find "${pkgdir}/opt/${pkgname}" -type f -exec chmod 644 {} \;
  chmod 755 "${pkgdir}/opt/${pkgname}/${_pkgname}.py"
  chmod 755 "${pkgdir}/opt/${pkgname}/Sample-PostProc.sh"

  install -Dm755 "${srcdir}/${pkgname}"       "${pkgdir}/usr/bin/${pkgname}"
  install -Dm644 "${srcdir}/${pkgname}.confd" "${pkgdir}/etc/conf.d/${pkgname}"
  install -Dm644 "${srcdir}/${pkgname}.service" "${pkgdir}/usr/lib/systemd/system/${pkgname}.service"
  install -Dm755 "${srcdir}/${pkgname}.desktop" \
    "${pkgdir}/usr/share/applications/${pkgname}.desktop"
  install -Dm755 "${srcdir}/addnzb.sh"    "${pkgdir}/opt/${pkgname}/addnzb.sh"
  install -Dm644 "${srcdir}/nzb-2.png"    "${pkgdir}/opt/${pkgname}/nzb-2.png"
  install -Dm644 "${srcdir}/sab2_64.png"  "${pkgdir}/opt/${pkgname}/sab2_64.png"
  install -Dm770 "${srcdir}/x-nzb.xml"    "${pkgdir}/opt/${pkgname}/x-nzb.xml"
}
