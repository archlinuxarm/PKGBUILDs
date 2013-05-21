# Contributor: David Rosenstrauch <darose@darose.net>
# Contributor: Erik Mank <erik@braindisorder.org>
# Maintainer: Clément Démoulins <clement@archivel.fr>

pkgname=tt-rss
_realname=Tiny-Tiny-RSS
pkgver=1.7.9
pkgrel=1
pkgdesc="Tiny Tiny RSS is an open source web-based news feed (RSS/Atom) aggregator, designed to allow you to read news from any location, while feeling as close to a real desktop application as possible."
arch=('any')
url="http://tt-rss.org/redmine/"
license=('GPL')
depends=('php')
optdepends=('apache' 'mysql' 'postgresql' 'php-curl')
backup=()
install=tt-rss.install

source=(https://github.com/gothfox/Tiny-Tiny-RSS/archive/${pkgver}.tar.gz tt-rss-updated.rc tt-rss-updated.service)
md5sums=('d4980f9646cf588ada3956ca9ffba754'
         'd4dbb037f2392eb9f4a91c6ae96863cc'
         'd3ba69d6f0d0a6bce7fab7f235d6afdf')

package() {
    cd "$srcdir/${_realname}-$pkgver"
    _instdir=${pkgdir}/usr/share/webapps/${pkgname}

    # install tt-rss
    mkdir -p ${_instdir}
    cp -ra * ${_instdir}/
    rm -rf ${_instdir}/debian

    # install config file into /etc/webapps/tt-rss/
    mkdir -p $pkgdir/etc/webapps/tt-rss
    install -m640 -g http config.php-dist $pkgdir/etc/webapps/tt-rss/config.php-dist
    ln -s /etc/webapps/tt-rss/config.php ${_instdir}/config.php

    # cache, feed-icons and lock will be into /var/lib/tt-rss/
    mkdir -p $pkgdir/var/lib/tt-rss
    mv ${_instdir}/{lock,feed-icons,cache} $pkgdir/var/lib/tt-rss
    ln -s /var/lib/tt-rss/lock ${_instdir}/lock
    ln -s /var/lib/tt-rss/feed-icons ${_instdir}/feed-icons
    ln -s /var/lib/tt-rss/cache ${_instdir}/cache
    chown -R http:http $pkgdir/var/lib/tt-rss

    # add a daemon
    install -D -m755 ${srcdir}/tt-rss-updated.rc ${pkgdir}/etc/rc.d/tt-rss-updated
    install -D -m644 ${srcdir}/tt-rss-updated.service ${pkgdir}/usr/lib/systemd/system/tt-rss-updated.service
}
