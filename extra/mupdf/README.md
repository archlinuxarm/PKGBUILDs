# mupdf

## Soname bump on new upstream releases

Each new upstream release includes a soname bump in `libmupdf` (which is a part of this `mupdf` split package).  
You can run `sogrep` on the built `libmupdf` package to identify the list of packages to rebuilb against it (e.g. `for repo in core extra; do for lib in $(find-libprovides libmupdf-1.24.8-1-x86_64.pkg.tar.zst | sed 's/=.*//g'); do sogrep -r $repo $lib; done; done | sort | uniq`).

Creating ToDos to track those rebuilds (in `staging`) is encouraged. For instance: <https://archlinux.org/todo/mupdf-1248-rebuild/>

*PS: It happens sometimes that upstream do not sync `mupdf` and `(python-)pymupdf` releases, creating some incompatibilities between them. See the [python-pymupdf package README](https://gitlab.archlinux.org/archlinux/packaging/packages/python-pymupdf) for more details.*
