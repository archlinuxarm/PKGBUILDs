# Contributor: Johannes Dewender  arch at JonnyJD dot net
pkgname=lio-utils
pkgver=4.1
pkgrel=8
epoch=
pkgdesc="a simple low-level configuration tool set for LIO (Target and iSCSI)"
arch=('i686' 'x86_64')
url="http://linux-iscsi.org/wiki/Lio-utils"
license=('GPL2')
depends=('python2')
# for binary packages including lio-snmp in this package might be fine
#makedepends=('net-snmp')
#optdepends=("net-snmp: monitor lio with snmp")
provides=()
conflicts=('targetcli-fb')
replaces=()
backup=('etc/target/tcm_start.sh' 'etc/target/lio_start.sh')
options=()
install=
source=(rc-arch.patch logfile.patch)
md5sums=('1afad3e324d0bdde01d0383b482fac5f'
         '43ab5e1a31f6d331dceef5bfa41a91a6')

build() {
  if [ ! -e "$pkgname-$pkgver.tgz" ]; then
    wget "http://www.risingtidesystems.com/git/?p=$pkgname.git;a=snapshot;h=595cd43e38ebd19813b91a3db35f3a707d2ba475;sf=tgz" -O "$pkgname-$pkgver.tgz"
  fi
  bsdtar -xf "$srcdir/$pkgname-$pkgver.tgz"

  cd "$srcdir/$pkgname/"
  patch -p1 < "$srcdir/logfile.patch"
  patch -p1 < "$srcdir/rc-arch.patch"

  cd "$srcdir/$pkgname/"
	cd tcm-py ; python2 setup.py build
  cd "$srcdir/$pkgname/"
	cd lio-py ; python2 setup.py build
  cd "$srcdir/$pkgname/tools"; make
}

package() {
  cd "$srcdir/$pkgname/"

  install -d $pkgdir/etc/rc.d
  install -m 0755 scripts/rc.target $pkgdir/etc/rc.d/target

  make DESTDIR="$pkgdir/" conf_install

  SITE_PACKAGES=`python2 ./get-py-modules-path.py`
  install -d "$pkgdir/usr/sbin"

  cd "$srcdir/$pkgname/tcm-py"
  python2 setup.py install --skip-build --root="$pkgdir/" --optimize=1
  for file in "$pkgdir$SITE_PACKAGES"/tcm_*.py; do
    sed -i '1s/python/python2/' "$file"
  done
  for part in {node,dump,loop,fabric}; do
    chmod a+x "$pkgdir$SITE_PACKAGES/tcm_$part.py"
    if [ ! -f "$pkgdir/usr/sbin/tcm_$part"  ]; then
      ln -s "$SITE_PACKAGES/tcm_$part.py" "$pkgdir/usr/sbin/tcm_$part"
    fi
  done

  cd "$srcdir/$pkgname/lio-py"
  python2 setup.py install --skip-build --root="$pkgdir/" --optimize=1
  for file in "$pkgdir$SITE_PACKAGES"/lio_*.py; do
    sed -i '1s/python/python2/' "$file"
  done
  for part in {dump,node}; do
    chmod a+x "$pkgdir$SITE_PACKAGES/lio_$part.py"
    if [ ! -f "$pkgdir/usr/sbin/lio_$part"  ]; then
      ln -s "$SITE_PACKAGES/lio_$part.py" "$pkgdir/usr/sbin/lio_$part"
    fi
  done


  if [ -d "$srcdir/$pkgname/tools" ]; then
    cd "$srcdir/$pkgname/tools"
    make DESTDIR="$pkgdir/" install
  fi
}

# vim:set ts=2 sw=2 et:
