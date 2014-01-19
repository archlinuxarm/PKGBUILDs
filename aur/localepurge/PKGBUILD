# Maintainer: Hyacinthe Cartiaux <hyacinthe.cartiaux @ free.fr>
# Contributor: Francesco Groccia <frgroccia gmail.com>
# Contributor: Dincer Celik <dincer@bornovali.com>

pkgname=localepurge
pkgver=0.7.3.1
pkgrel=1
pkgdesc="Script to remove disk space wasted for unneeded localizations."
arch=('any')
url="http://packages.debian.org/source/sid/localepurge"
license=('GPL')
backup=('etc/locale.nopurge')
source=("http://ftp.de.debian.org/debian/pool/main/l/localepurge/${pkgname}_${pkgver}.tar.gz"
        "${pkgname}.diff"
        "${pkgname}.8.diff"
        "${pkgname}.config.diff"
        "locale.nopurge")

prepare()
{
    patch -uN ${srcdir}/${pkgname}/usr/sbin/localepurge < ${srcdir}/localepurge.diff
    patch -uN ${srcdir}/${pkgname}/debian/localepurge.8 < ${srcdir}/localepurge.8.diff
    patch -uN ${srcdir}/${pkgname}/debian/localepurge.config < ${srcdir}/localepurge.config.diff
}

package()
{
    install -D -m755 ${srcdir}/${pkgname}/usr/sbin/localepurge ${pkgdir}/usr/bin/localepurge
    install -D -m644 ${srcdir}/${pkgname}/debian/localepurge.8 ${pkgdir}/usr/share/man/man8/localepurge.8
    install -D -m755 ${srcdir}/${pkgname}/debian/localepurge.config ${pkgdir}/usr/bin/localepurge-config
    install -D -m644 locale.nopurge ${pkgdir}/etc/locale.nopurge
    if [ ! -e /var/cache/localepurge/localelist ]; then
	find /usr/share/locale -maxdepth 1 -type d -name "*" -printf "%f\n" | grep "^[a-z]" | cut -d" " -f1 | sort -u > ${srcdir}/localelist
    else
	cp /var/cache/localepurge/localelist ${srcdir}/localelist
    fi
    install -D -m644 ${srcdir}/localelist ${pkgdir}/var/cache/localepurge/localelist
}
sha256sums=('d3b9f7a280211b049647b0c873be10830bb1e035adb9646a7211fae0809e87d0'
            'b8704eec22d0e84ada9a524277322f9892c11f3f4b84ec0dd4f50f0ca2dc7474'
            '82bd40594ef0646465eed6e525368e87694322513c0d3280879fcfc5c40cb6a7'
            'dbed1ae1f1514f14c00fdb48d09ac8cd3407dbc3f1a1fed84f1e8735da1f9678'
            'b9c28be93fa47d4f0315972159e501d9eef28bbab7ffe6e8e7c4a13c359f35e8')
