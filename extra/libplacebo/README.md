# libplacebo

## Soname bump on new major releases

libplacebo new major releases (e.g. going from 6.xxx to 7.xxx) include soname bumps (which should be raised by `pkgctl` has a warning).  
In such cases, you can run `sogrep` on the built `libplacebo` package to identify the list of packages to rebuilb against it (e.g. `for repo in core extra; do for lib in $(find-libprovides libplacebo-7.349.0-1-x86_64.pkg.tar.zst | sed 's/=.*//g'); do sogrep -r $repo $lib; done; done | sort | uniq`).

The list currently consist of:

- ffmpeg
- jellyfin-ffmpeg
- mpv

`libplacebo` support has been disabled in `vlc` for the time being (see [the related MR](https://gitlab.archlinux.org/archlinux/packaging/packages/vlc/-/merge_requests/1)), because of incompatibilities with newer `libplacebo` versions (hence why `vlc` does not appear in the above list).

Creating ToDos to track those rebuilds (in `staging`) is encouraged. For instance: <https://archlinux.org/todo/libplace-7-rebuild/>
