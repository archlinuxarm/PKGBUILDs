# pkgbuilds.git - README - 2012-12-03
This repository hosts
[PKGBUILDs](https://wiki.archlinux.org/index.php/PKGBUILD) that have been
modified from the upstream [Arch Linux](http://archlinux.org) release in order
to build on architectures supported by [Arch Linux
ARM](http://archlinuxarm.org).  Unmodified upstream packages do not belong
here.

These packages are meant to be built on Arch Linux ARM, found at
http://archlinuxarm.org


## Layout
New packages should be placed in the correct locations, with the packages base
folder name reflecting the 'pkgname' for single-package PKGBUILDs, or 'pkgbase'
for multiple-package PKGBUILDs.  In the case of non-alarm packages, naming
should exactly match the base folder or package name as used upstream or in the
AUR, respectively.  This will ensure correct package->version matching in the
build system update routines.

PKGBUILDs modified from upstream, AUR, or custom PKGBUILDs for the alarm repo
must have the author's name and email in the header along with a changelog of
what modifications have been done to have the package build correctly.  This
allows us to identify and merge these changes into newer versions of the
package.

### core, extra and community
These folders contain packages only found in the upstream repositories by the
same name.  Exceptions are packages such as our kernels or other packages we
feel belong within the scope of these repos.

### aur
Contains a selection of packages from the [AUR](https://aur.archlinux.org/
"Arch User Repository") that have been requested by the community to be
pre-compiled and easily installed.  However, you're free to use the AUR just as
you would on Arch, and we have included Yaourt to compile and install directly
from AUR (`yaourt -AS packagename`).

### alarm
Contains packages we have created or have been submitted to us, to enable
functionality on ARM systems in use by the community.  These are our own or
have changed significantly enough from upstream or AUR to no longer qualify as
being in those respective locations here.


##Custom PKGBUILD variables for the build system

### noautobuild
If non-zero, the build system will mark the package as done and not build it
for any architecture.

### buildarch
A bitmask of architectures to build the package for.  This must be set to the
decimal equivalent of the bitmask (the number in parenthesis).
Sub-architectures must be specifically requested, they won't build by default.

* `0000 0001` (1) = *the default*, package will be built for all architectures
* `0000 0010` (2) = the package will be built only for armv5
* `0000 0100` (4) = the package will be built only for armv7h
* `0000 1000` (8) = reserved
* `0001 0000` (16) = the package will be build only for armv6h

### highmem
If non-zero, the build system will mark the package as requiring a builder that
has more than 1GB of RAM/swap to build successfully.
