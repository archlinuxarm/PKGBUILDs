# Maintainer: Kyle Keen <keenerd@gmail.com>
# Contributor: Gilrain <pierre.buard+aur gmail com>
# Contributor: bruenig
pkgname=packer
pkgver=20150808
pkgrel=1
pkgdesc="Bash wrapper for pacman and aur"
url="http://github.com/keenerd/packer"
license=("GPL3")
arch=('any')
makedepends=('git')
depends=('grep' 'sed' 'bash' 'curl' 'pacman' 'jshon' 'expac')
optdepends=('sudo: install and update packages as non-root'
            'customizepkg: apply customizepkg modifications')
source=('git+https://github.com/keenerd/packer.git')
md5sums=('SKIP')

pkgver() {
    cd "$pkgname"
    git show -s --format="%ci" HEAD | sed -e 's/-//g' -e 's/ .*//'
}

package() {
  install -Dm755 "$srcdir/$pkgname/packer" "$pkgdir/usr/bin/packer"
  install -Dm644 "$srcdir/$pkgname/packer.8" "$pkgdir/usr/share/man/man8/packer.8"
}

