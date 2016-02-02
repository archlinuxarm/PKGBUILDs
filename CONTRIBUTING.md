# Rules for Pull Requests

* **Descriptive titles.** Pull requests must have a title that gives the package name the PR is for, and a short description about the PR.
* **Detailed description.** The body of the PR must contain a detailed description of what changes are being introduced, and most importantly, *why* this PR should be merged.
* **One package per request.** PRs must be for a single package only.  A PR addressing multiple packages without merit risks having a delayed merge or being closed.
* **Squash commits.** Your commits must be meaningful. If you make incremental changes or fixes, they must [be squashed](https://git-scm.com/book/en/v2/Git-Tools-Rewriting-History#Squashing-Commits) before the pull request will be merged.
* **Check that your changes build.** Your submission must build using the [clean chroot](https://wiki.archlinux.org/index.php/DeveloperWiki:Building_in_a_Clean_Chroot) method on *all* supported architectures that the package is to be built for.  No exceptions.
* **Work within one PR.** Do not close a PR and open another with new changes.  Amend your commit and force push to your branch to update the changes in the pull request.
* Ensure your PR addresses three of the most common problems:
  * Correctly update the pkgver or pkgrel of the package (see below).
  * Update the checksums if external files have been added or modified.
  * Is the package only for specific architectures?  Set the `buildarch` variable (see [the README](https://github.com/archlinuxarm/PKGBUILDs/blob/master/README.md))

Pull requests that fail to meet these requirements may be summarily closed without response.

## Submitting new packages
* If the pull request is for a new package, review the [README](https://github.com/archlinuxarm/PKGBUILDs/blob/master/README.md) to ensure the package is going into the correct repository and meets all the stated requirements.

## Updating existing packages
### Upstream x86 Arch Linux packages
* If you changed the PKGBUILD or related files, detail your changes in the comment header at the top.  Review the packages in this repository for examples of what this looks like.
* Increment the pkgrel variable by a decimal number if the version of the package is already in the package repositories.  This is to prevent interference tracking against upstream versions.  For example:
  * If `pkgrel=1`, change to `pkgrel=1.1`
  * If `pkgrel=1.1`, change to `pkgrel=1.2`
* At no time should an upstream package tracked here exceed the version upstream.
* If the package is only supposed to be built for specific architectures, ensure the `buildarch` variable is set correctly (see [the README](https://github.com/archlinuxarm/PKGBUILDs/blob/master/README.md))

### Arch Linux ARM specific packages
* If the change increases the version of the package, ensure `pkgver` is updated and `pkgrel` is reset to 1.
* Otherwise, increment `pkgrel` by 1.
* If the package is only supposed to be built for specific architectures, ensure the `buildarch` variable is set correctly (see [the README](https://github.com/archlinuxarm/PKGBUILDs/blob/master/README.md))
