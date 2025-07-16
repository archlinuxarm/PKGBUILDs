# NGINX Packaging

## Overview

Due to the nature of NGINX's module architecture, it is important to understand
the intricacies involved in maintaining compatibility across different
versions.

## Dynamic Modules

When we compile a dynamic module, the raw output is a shared object (.so
file). At startup and reload, NGINX loads each of the shared objects named by a
[load_module] directive (which by convention are in the main configuration file,
`nginx.conf`).

Dynamic modules are binary‑compatible with the official builds of NGINX and
NGINX Plus. However, this binary compatibility has limitations. Dynamic modules
must be compiled against the same version of NGINX they are loaded into. This
means that upgrading NGINX without rebuilding all dynamic modules built against
the matching version results in a failure during load time.

### Distro flags

Due to the default configuration of the NGINX dynamic module `configure`
scripts and `Makefile`, our distribution's `LDFLAGS` are ignored. To address
this, the `configure` scripts accept a `--with-ld-opt` option, allowing you to
pass along our distribution's flags. Additionally, for dynamic modules
compatibility, the `--with-compat` option should always be used.

```sh
/usr/src/nginx/configure \
    --with-compat \
    --with-ld-opt="${LDFLAGS}" \
    --add-dynamic-module=../modsecurity-nginx-v$pkgver
```

### Depending on nginx

Taking the incompatibility into account, dynamic module packages should depend
on the exact nginx version used during compilation to avoid breakage after a
system upgrade in case a rebuild has been missed. To achieve this, the dynamic
module packages should `makedepends` on `nginx` as well as in their respective
`package()` function add a `depends` on the precise `nginx` version:

```sh
makedepends=(
  nginx
  nginx-src
)

package() {
  local _nginx_version=$(nginx -v 2>&1)
  _nginx_version=${_nginx_version/* nginx\/}
  depends+=("nginx=${_nginx_version}")
}
```

### Rebuilding packages

The easiest way to find the rebuild targets is to lookout for dependencies on
the `nginx-src` split package, which contains the require source code for dynamic
modules to compile against. The rebuild targets can be double checked against the
[ArchWeb frontend].

Using `pkgctl` to get a list of packages having a makedepends on `nginx-src`:

```sh
pkgctl search --json '"makedepends = nginx-src"' | jq --raw-output '.[].project_name'
```

[load_module]: https://nginx.org/en/docs/ngx_core_module.html#load_module
[ArchWeb frontend]: https://archlinux.org/packages/extra/x86_64/nginx-src/
