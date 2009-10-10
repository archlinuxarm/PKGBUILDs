# Maintainer: Aaron Schaefer <aaron@elasticdog.com>
pkgname=vim-pastie
pkgver=2.0
_scriptid=10260
pkgrel=3
pkgdesc='A Vim plugin that lets you read and create pastes at http://pastie.org/'
arch=('any')
url='http://www.vim.org/scripts/script.php?script_id=1624'
license=('custom:vim')
depends=('ruby' 'vim')
source=("http://www.vim.org/scripts/download_script.php?src_id=$_scriptid" license.txt)
md5sums=('4ba364a003903ace6fd51ccbe6b86d78' 'cd49910a3dde6fe0a7e24ebe8cbf7066')

build() {
  cd "$srcdir"
  mv "download_script.php?src_id=$_scriptid" "pastie-$pkgver.vim"

  installpath="$pkgdir/usr/share/vim/vimfiles"

  install -D -m644 "pastie-$pkgver.vim" "$installpath/plugin/pastie.vim" || return 1
  install -Dm644 ${srcdir}/license.txt ${pkgdir}/usr/share/licenses/${pkgname}/license.txt
}
