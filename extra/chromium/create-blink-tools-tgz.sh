#!/bin/bash

pkgver=$(. PKGBUILD; echo $pkgver)
[[ -n $pkgver ]] || exit 1

wget -qO blink-tools-$pkgver.tar.gz \
  https://chromium.googlesource.com/chromium/src/+archive/$pkgver/third_party/blink/tools.tar.gz

rsync --ignore-existing blink-tools-$pkgver.tar.gz pkgbuild.com:public_html/sources/chromium/
