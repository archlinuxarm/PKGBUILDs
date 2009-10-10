# Maintainer: Douglas Soares de Andrade <dsa@aur.archlinux.org>
# Contributor: phrakture < aaron m griffin * gmail::com >
# Contributor: tardo <tardo@nagi-fanboi.net>

pkgname=beautiful-soup
pkgver=3.1.0.1
pkgrel=1
pkgdesc="A Python HTML/XML parser designed for quick turnaround projects like screen-scraping"
arch=('i686' 'x86_64')
url="http://www.crummy.com/software/BeautifulSoup/index.html"
license=('PSF')
depends=('python')
source=(http://www.crummy.com/software/BeautifulSoup/download/BeautifulSoup-$pkgver.tar.gz)

build() {
    cd $startdir/src/BeautifulSoup-$pkgver
    python setup.py install --root=$startdir/pkg
}
md5sums=('bcffef3eda6e06e6d1e18c06a9db8a24')
