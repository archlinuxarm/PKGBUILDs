# Maintainer: Francesco Groccia <frgroccia gmail.com>
# Contributor: Dincer Celik <dincer@bornovali.com>

pkgname=localepurge
pkgver=0.7.3
pkgrel=1
pkgdesc="Script to remove disk space wasted for unneeded localizations."
arch=('any')
url="http://packages.debian.org/source/sid/localepurge"
license=('GPL')
backup=('etc/locale.nopurge')
source=("http://ftp.de.debian.org/debian/pool/main/l/localepurge/${pkgname}_${pkgver}.tar.gz"
    "http://fgr.bitbucket.org/im/localepurge-arch/${pkgname}.diff"
    "http://fgr.bitbucket.org/im/localepurge-arch/${pkgname}.8.diff"
    "http://fgr.bitbucket.org/im/localepurge-arch/${pkgname}.config.diff"
    "http://fgr.bitbucket.org/im/localepurge-arch/locale.nopurge")

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
sha256sums=('1e5f569c8d5e3d09bf51128f11ebe7c80bccd785b0c028e949df21347ed135f7'
            '84b8abf57a1af27e5b50fcd5a12701a5980d85a187c5a3b3e3c3e5a79e28886d'
            '82bd40594ef0646465eed6e525368e87694322513c0d3280879fcfc5c40cb6a7'
            'dbed1ae1f1514f14c00fdb48d09ac8cd3407dbc3f1a1fed84f1e8735da1f9678'
            'b9c28be93fa47d4f0315972159e501d9eef28bbab7ffe6e8e7c4a13c359f35e8')
sha256sums=('1e5f569c8d5e3d09bf51128f11ebe7c80bccd785b0c028e949df21347ed135f7'
            'b8704eec22d0e84ada9a524277322f9892c11f3f4b84ec0dd4f50f0ca2dc7474'
            '82bd40594ef0646465eed6e525368e87694322513c0d3280879fcfc5c40cb6a7'
            'dbed1ae1f1514f14c00fdb48d09ac8cd3407dbc3f1a1fed84f1e8735da1f9678'
            'b9c28be93fa47d4f0315972159e501d9eef28bbab7ffe6e8e7c4a13c359f35e8')
