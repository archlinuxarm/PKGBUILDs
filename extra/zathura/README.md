# zathura

## Silent ABI bump on new relases and plugins rebuilds

New zathura releases often include a *silent* ABI bump which require all the `zathura-*` plugin packages to be rebuilt (see <https://github.com/pwmt/zathura/commit/17b9271243a4d4fb4230868600247f829d482930> and <https://github.com/pwmt/zathura/issues/653>).  
Sometimes, upstream creates new tags for plugins in order to highlight this (see <https://github.com/pwmt/zathura/issues/653#issuecomment-2267421357>), but it's not always the case.

As a matter of precaution, it is encouraged to always rebuild all zathura plugins against new zathura releases.

The list of zathura plugin packages currently consist of:

- zathura-cb
- zathura-djvu
- zathura-pdf-mupdf
- zathura-pdf-poppler
- zathura-ps

Creating ToDos to track those rebuilds (in `staging`) is encouraged. For instance: <https://archlinux.org/todo/zathura-057/>
