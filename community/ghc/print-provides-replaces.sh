#!/bin/bash

. PKGBUILD

if [[ ! -d src/ghc-${pkgver}/libraries ]]; then
  echo "error: no directory src/ghc-${pkgver}/libraries: You must extract the source tarball under src/"
  exit 1
fi

declare -A exclude
# no Win32 cause we're not building on windows
exclude['Win32']=1
# no integer-simple because we use integer-gmp
exclude['integer-simple']=1
# integer-gmp was replaced with integer-gmp2
exclude['integer-gmp']=1
# extract excluded libraries from ghc.mk
for exclude_pkg in $(sed 's/PKGS_THAT_ARE_INTREE_ONLY := //p' -n src/ghc-${pkgver}/ghc.mk); do
  exclude[${exclude_pkg}]=1
done

cd src/ghc-${pkgver}/libraries

# $1 is the name of the variable
# $2 is the string for the test, either '=' or '<'
print_var() {
  printf "$1=("
  for path in $(ls ./*/*.cabal); do
    dirname=$(echo $path | awk -F '/' '{ print $2 }')
    cabalfile=$(echo $path | awk -F '/' '{ print $3 }')
    cabalname=$(basename $cabalfile .cabal)
    [[ ${exclude[${dirname}]} ]] && continue
    version=$(awk 'tolower($0) ~ /^version:/ {print $2 }' $path)
    printf "'haskell-$cabalname"
    [[ -n "$2" ]] && printf "$2$version"
    printf "'\n          "
  done
  # also add cabal
  version=$(awk 'tolower($0) ~ /^version:/ { print $2 }' Cabal/Cabal/Cabal.cabal)
  printf "'haskell-cabal"
  [[ -n "$2" ]] && printf "$2$version"
  printf "'\n          "
  echo -e '\b)'
}

print_var 'provides' '='
print_var 'replaces'
