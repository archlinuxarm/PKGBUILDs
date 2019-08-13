#!/bin/bash

set -euo pipefail

echo "vim"

pkgfile -rd "^/usr/lib/perl5/" | sed 's#^.*/##' | sort -u

for repo in core extra community multilib; do
	ssh dragon.archlinux.org sogrep "$repo" libperl.so
done

# this one is optional
#pkgfile -r '^/usr/share/perl5/'  | sed 's#^.*/##' | sort -u

