# python

## Benchmarking

The Python interperter can be benchmarked using the [python-pyperformance] package. Note that this tool creates virtualenv's to run the benchmarks in so all packages
have to be available for the Python version under test on PyPi. To run these benchmarks:

```bash
pyperformance run -o baseline.json
pyperformance run -o new.json
pyperformance compare -O table baseline.json new.json
```

## Bootstrapping new Python interpreter version

When a new Python interpreter version (e.g. a minor version update) is released, all dependent packages must be rebuild against it.

To make this possible, an initial set of packages (required for a [PEP 517] based package build workflow), must be bootstrapped, as they depend on each other during build time.
For bootstrapping we rely on the [python-bootstrap] project, which offers a custom integration for building the minimum required set of packages for initial bootstrap.
With it, packages are installed including all dependencies vendored by upstream.

The following packages are currently part of the initial bootstrap set:

- [python-build]
- [python-flit-core]
- [python-installer]
- [python-packaging]
- [python-pyproject-hooks]
- [python-setuptools]
- [python-wheel]

### Prepare python-bootstrap

To be able to bootstrap the exact versions of the packages in the initial bootstrap set, the git submodules of the [python-bootstrap] project need to be updated to the respective versions.

Afterwards, a [relase for python-bootstrap] is created, so that the specific version can be relied upon when bootstrapping the packages of the initial bootstrap set.

### Prepare ticket for devendoring of initial bootstrap set

To allow the devendoring of the initial bootstrap set, a ticket should be created for the [python] package, to track the required steps for devendoring the initial bootstrap set (e.g. similar to how it is done in [python-bootstrap#2]).

### Update Python package

Once the [python-bootstrap] prerequisites are met, the [python] package is updated in the staging environment:

```bash
pkgctl version upgrade
pkgctl build --updpkgsums --staging
pkgctl release --db-update --staging
```

### Bootstrap initial set of packages

Afterwards, the packages in the initial bootstrap set are bootstrapped against the updated Python interpreter in the staging environmet, by following the instructions in the respective README of each of those packages.

### Devendor initial bootstrap set

Once all packages in the initial bootstrap set are in the staging environment, they are devendored by again following the instructions in the respective README of each of those packages.

### Rebuild all packages without tests

Once the packages in the initial bootstrap set are devendored, all packages requiring [python] are rebuilt without tests (i.e. `pkgctl build --nocheck --rebuild --staging`).

In this first pass it may already be possible for some packages to be rebuilt with tests enabled (i.e. `pkgctl build --rebuild --staging`).

### Rebuild packages with tests

All packages, that have been rebuilt against the new interpreter version without tests, must be rebuilt with tests in a second pass.

Running the [no_check_pkgs.sh] script against all package files in the staging environment of a local mirror may be used to detect those packages not yet built with tests enabled (note, that the script may output false-positives!).

### Move to testing

Once all dependent packages have been rebuilt with tests enabled, all packages are moved from the staging to the testing environment.

[PEP 517]: https://peps.python.org/pep-0517/
[python-bootstrap]: https://gitlab.archlinux.org/archlinux/python-bootstrap/
[release for python-bootstrap]: https://gitlab.archlinux.org/archlinux/python-bootstrap/#releases
[python-build]: https://gitlab.archlinux.org/archlinux/packaging/packages/python-build/
[python-flit-core]: https://gitlab.archlinux.org/archlinux/packaging/packages/python-flit-core/
[python-installer]: https://gitlab.archlinux.org/archlinux/packaging/packages/python-installer/
[python-packaging]: https://gitlab.archlinux.org/archlinux/packaging/packages/python-packaging/
[python-pyperformance]: https://gitlab.archlinux.org/archlinux/packaging/packages/python-pyperformance/
[python-pyproject-hooks]: https://gitlab.archlinux.org/archlinux/packaging/packages/python-pyproject-hooks/
[python-setuptools]: https://gitlab.archlinux.org/archlinux/packaging/packages/python-setuptools/
[python-wheel]: https://gitlab.archlinux.org/archlinux/packaging/packages/python-wheel/
[python]: https://gitlab.archlinux.org/archlinux/packaging/packages/python/
[python-bootstrap#2]: https://gitlab.archlinux.org/archlinux/python-bootstrap/-/issues/2
[no_check_pkgs.sh]: https://gitlab.archlinux.org/archlinux/python-bootstrap/-/blob/b45ea748b507323500f8fda5a7eb0d40a53c46a1/scripts/no_check_pkgs.sh
