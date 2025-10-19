# libplacebo

## Soname bump on new releases

libplacebo new releases include soname bumps.  
You can run `sogrep` on the built `libplacebo` package to identify the list of packages to rebuilb against it (e.g. `for repo in core extra; do for lib in $(find-libprovides libplacebo-7.349.0-1-x86_64.pkg.tar.zst | sed 's/=.*//g'); do sogrep -r $repo $lib; done; done | sort | uniq`).

Creating ToDos to track those rebuilds (in `staging`) is encouraged. For instance: <https://archlinux.org/todo/libplace-7-rebuild/>
