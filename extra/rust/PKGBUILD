# Maintainer: Johannes Löthberg <johannes@kyriasis.com>
# Maintainer: Jan Alexander Steffens (heftig) <heftig@archlinux.org>
# Contributor: Alexander F Rødseth <xyproto@archlinux.org>
# Contributor: Daniel Micay <danielmicay@gmail.com>
# Contributor: userwithuid <userwithuid@gmail.com>

# ALARM: Kevin Mihelich <kevin@archlinuxarm.org>
#  - remove lib32, musl, and wasm packages and related bits
#  - remove wasm-component-ld from makedepends
#  - add a link to g++ to compensate for broken cross-compiler decisions
#  - build v6/v7 with -j2 - RAM constraints
#  - set llvm-config in config.toml for ARM architectures
#  - set debuginfo-level = 0 in config.toml - RAM constraints
#  - build aarch64 with 16k page support

highmem=1

pkgbase=rust
pkgname=(
  rust
  rust-src
)
pkgver=1.99.0
pkgrel=1
epoch=1
pkgdesc="Systems programming language focused on safety, speed and concurrency"
url=https://www.rust-lang.org/
arch=(x86_64)
license=("Apache-2.0 OR MIT")
options=(
  !emptydirs
  !lto
)
_llvmver=23.1.1
depends=(
  "compiler-rt=$_llvmver"
  "lld=$_llvmver"
  "llvm-libs=$_llvmver"
  bash
  curl
  gcc
  glibc
  libgcc
  libgit2
  libssh2
  libstdc++
  openssl
  sqlite
  zlib
)
makedepends=(
  "clang=$_llvmver"
  "llvm=$_llvmver"
  cmake
  libffi
  musl
  ninja
  perl
  python
  rust
)
checkdepends=(
  gdb
  procps-ng
)
source=(
  "https://static.rust-lang.org/dist/rustc-$pkgver-src.tar.xz"{,.asc}
  "https://github.com/llvm/llvm-project/releases/download/llvmorg-$_llvmver/llvm-project-$_llvmver.src.tar.xz"

  # Patch bootstrap so that rust-analyzer-proc-macro-srv
  # is in /usr/lib instead of /usr/libexec
  0001-bootstrap-Change-libexec-dir.patch

  # Put bash completions where they belong
  0002-bootstrap-Change-bash-completion-dir.patch

  # Fix build with system rustc
  # https://github.com/rust-lang/rust/issues/143735
  0003-bootstrap-Workaround-for-system-stage0.patch

  # Use our *-pc-linux-gnu targets, making LTO with clang simpler
  0004-compiler-Change-LLVM-targets.patch

  # Use our ld.lld
  0005-compiler-Use-ld.lld-by-default.patch

  # Prefer "lib" over "lib64"
  0008-compiler-Swap-primary-and-secondary-lib-dirs.patch
)
b2sums=('f4e2d97047c42067d11c31484d320f9daaaf881c78d22473b7f9ac2289a8f5979de6e75c277c49a0e3e33cd80f18aecace39f25ae78645976945798f3e512f16'
        'SKIP'
        '31d0ad202f4ad38d001baaf638a756508b164229b02a6e41b9689508a33b3a7baecaa10398e20f3661cacb712663fe379eeb94e50e017e40ae546a7e7de8049d'
        '3736aa3f6f891cb94dbe549d5b2c74bc56e9d484a1598c2a446fbfd4db10a90a34f2e371c65743cf01e1266470fd6fe2fc52cbc365564c62ba61e20ea11c8e22'
        'eb57683802be1114452d0864bdabd1f2a42f83c0c45532ff968c693cb98dd5cf61342880bd75e731e91fde318b0f61cc30183374348a56db883ba32c900a4949'
        'd39a31a9260ba1e60e34f2481a4ef19ce18178736539450e57df19b9a3364a35f23b9055eecb36175de0e01a154b04f86c2645cffc7d637aab8b38dbf2eb0832'
        '8a5238d5807673d498ca330b8d1d7c3c14d12be1bd0dec361613b09fd04527060a7f661acd9205f6a03586e2ea4a650c890d2549eed2e207fc03cce3bdb1cf03'
        'd36a29127349f02fdd3327a36ebfe57b1ce7f64bb24de2bb85369b377e362ee389662b5a5f281bc2a82b7fca2eee234ca6de5a923f5030e3570ab1203653fdb2'
        'd5fcda00c46e1e1ea9ed1d269d3ef549d4ad96148d7d0744acd673de802b3bf3747ceec1fa506d3e7c128cdff742410b1e69218d59ef20a5f246755c1bd02694')
validpgpkeys=(
  108F66205EAEB0AAA8DD5E1C85AB96E6FA1BE5FE  # Rust Language (Tag and Release Signing Key) <rust-key@rust-lang.org>
)

# Make sure the duplication in rust-wasm is found
COMPRESSZST+=(--long)

prepare() {
  cd rustc-$pkgver-src

  local src
  for src in "${source[@]}"; do
    src="${src%%::*}"
    src="${src##*/}"
    src="${src%.zst}"
    [[ $src = *.patch ]] || continue
    echo "Applying patch $src..."
    patch -Np1 < "../$src"
  done

  rm -rf src/llvm-project
  ln -sr "$srcdir/llvm-project-$_llvmver.src" src/llvm-project

  local clangdir
  clangdir="$(clang -print-resource-dir)"

  cat >bootstrap.toml <<END
# see src/bootstrap/defaults/
profile = "dist"

# see src/bootstrap/src/utils/change_tracker.rs
change-id = 160100


[llvm]
download-ci-llvm = false
link-shared = true

[gcc]
download-ci-gcc = false

[build]
description = "Arch Linux $pkgbase $epoch:$pkgver-$pkgrel"
cargo = "/usr/bin/cargo"
rustc = "/usr/bin/rustc"
rustfmt = "/usr/bin/rustfmt"
locked-deps = true
vendor = true
tools = [
  "cargo",
  "clippy",
  "rustdoc",
  "rustfmt",
  "rust-analyzer-proc-macro-srv",
  "analysis",
  "src",
]
sanitizers = false
profiler = true

# Generating docs fails with the wasm32-* targets
docs = false

[install]
prefix = "/usr"

[rust]
codegen-units = 16
codegen-units-std = 1
debuginfo-level = 0
debuginfo-level-std = 0
channel = "stable"
rpath = false
lld = false
bootstrap-override-lld = "external"
llvm-bitcode-linker = false
deny-warnings = false
backtrace-on-ice = true
parallel-frontend-threads = 0

[dist]
compression-formats = ["gz"]
compression-profile = "fast"

[target.x86_64-unknown-linux-gnu]
cc = "/usr/bin/gcc"
cxx = "/usr/bin/g++"
ar = "/usr/bin/gcc-ar"
ranlib = "/usr/bin/gcc-ranlib"
llvm-config = "/usr/bin/llvm-config"
optimized-compiler-builtins = "$clangdir/lib/linux/libclang_rt.builtins-x86_64.a"

[target.i686-unknown-linux-gnu]
cc = "/usr/bin/gcc"
cxx = "/usr/bin/g++"
ar = "/usr/bin/gcc-ar"
ranlib = "/usr/bin/gcc-ranlib"
optimized-compiler-builtins = "$clangdir/lib/linux/libclang_rt.builtins-i386.a"

[target.aarch64-unknown-linux-gnu]
cc = "/usr/bin/gcc"
cxx = "/usr/bin/g++"
ar = "/usr/bin/gcc-ar"
ranlib = "/usr/bin/gcc-ranlib"
llvm-config = "/usr/bin/llvm-config"

[target.armv7-unknown-linux-gnueabihf]
cc = "/usr/bin/gcc"
cxx = "/usr/bin/g++"
ar = "/usr/bin/gcc-ar"
ranlib = "/usr/bin/gcc-ranlib"
llvm-config = "/usr/bin/llvm-config"

[target.arm-unknown-linux-gnueabihf]
cc = "/usr/bin/gcc"
cxx = "/usr/bin/g++"
ar = "/usr/bin/gcc-ar"
ranlib = "/usr/bin/gcc-ranlib"
llvm-config = "/usr/bin/llvm-config"
END

  if [[ $CARCH == armv7h ]]; then
    mkdir path
    ln -s /usr/bin/g++ path/arm-linux-gnueabihf-g++
    export PATH="$srcdir/path:$PATH"
    jobs="2"
  else
    jobs="$(nproc)"
  fi
}

_pick() {
  local p="$1" f d; shift
  for f; do
    d="$srcdir/$p/${f#$pkgdir/}"
    mkdir -p "$(dirname "$d")"
    mv "$f" "$d"
    rmdir -p --ignore-fail-on-non-empty "$(dirname "$f")"
  done
}

build() {
  cd rustc-$pkgver-src

  [[ $CARCH == "aarch64" ]] && export JEMALLOC_SYS_WITH_LG_PAGE=16
  export LIBGIT2_NO_VENDOR=1
  export LIBSSH2_SYS_USE_PKG_CONFIG=1
  export LIBSQLITE3_SYS_USE_PKG_CONFIG=1
  export RUST_BACKTRACE=1
  unset CFLAGS CXXFLAGS LDFLAGS

  DESTDIR="$srcdir/dest-rust" python ./x.py install -j $jobs

  cd ../dest-rust

  # delete unnecessary files, e.g. files only used for the uninstall script
  rm -v etc/target-spec-json-schema.json
  rm -v usr/lib/rustlib/{components,install.log,rust-installer-version,uninstall.sh}
  rm -v usr/lib/rustlib/manifest-*

  # licenses for main rust package
  local ldir="usr/share/licenses/rust" f d
  mkdir -p "$ldir"
  for f in usr/share/doc/*/{COPYRIGHT,LICENSE}*; do
    d="$(dirname "$f")"
    case $f in
      */LICENSE-APACHE) rm -v "$f" ;;
      *) mv -v "$f" "$ldir/${f##*/}.${d##*/}" ;;
    esac
    rmdir -p --ignore-fail-on-non-empty "$d"
  done

  # rustbuild always installs copies of the shared libraries to /usr/lib,
  # overwrite them with symlinks to the per-architecture versions
  #mkdir -pv usr/lib32
  #ln -srvft usr/lib   usr/lib/rustlib/x86_64-unknown-linux-gnu/lib/*.so
  #ln -srvft usr/lib32 usr/lib/rustlib/i686-unknown-linux-gnu/lib/*.so

  #_pick dest-i686 usr/lib/rustlib/i686-unknown-linux-gnu usr/lib32
  #_pick dest-musl usr/lib/rustlib/x86_64-unknown-linux-musl
  #_pick dest-wasm usr/lib/rustlib/wasm32-*
  _pick dest-src  usr/lib/rustlib/src
}

package_rust() {
  depends+=(
    libcurl.so
    libgcc_s.so
    libgit2.so
    libsqlite3.so
    libssh2.so
    libssl.so
  )
  optdepends=(
    'gdb: rust-gdb script'
    'lldb: rust-lldb script'
  )
  provides=(
    cargo
    rustfmt
  )
  conflicts=(
    cargo
    'rust-docs<1:1.56.1-3'
    rustfmt
  )
  replaces=(
    cargo
    cargo-tree
    'rust-docs<1:1.56.1-3'
    rustfmt
  )

  cp -a dest-rust/* "$pkgdir"
}

package_rust-src() {
  pkgdesc="Source code for the Rust standard library"
  depends=(rust)

  cp -a dest-src/* "$pkgdir"

  install -Dt "$pkgdir/usr/share/licenses/$pkgname" -m644 \
    rustc-$pkgver-src/{COPYRIGHT,LICENSE-MIT}
}

# vim:set ts=2 sw=2 et:
