## New Issues
This issue tracker is for bugs with packaging, and the PKGBUILDs *only*.  Use [the forum](http://archlinuxarm.org/forum) for *all* other issues.

*Before* filing an issue, copy and paste the following template.  **This is required to be completely filled out.**
```AsciiDoc
Package: 
Architecture: 
Device: 

What it isn't doing:

What it should be doing:

Steps to reproduce:

```
The issue title must contain the package name and a short description of the problem:
```AsciiDoc
package-name: short description
```

#### You can [click this link](https://github.com/archlinuxarm/PKGBUILDs/issues/new?title=package-name:%20short%20description&body=Package:%20%0AArchitecture:%20%0ADevice:%20%0A%0AWhat%20it%20isn%27t%20doing:%0A%0AWhat%20it%20should%20be%20doing:%0A%0ASteps%20to%20reproduce:%0A) to automatically pre-fill a new issue.

Field | Description
---- | -----
Package | The package name of the package with the problem (eg. linux-armv7)
Architecture | The architecture this package is from (armv5, armv6, armv7)
Device | The ARM device used where the problem occurred (eg. Wandboard)
What it isn't doing | A **detailed** account of the problem, including all relevant error messages
What it should be doing | A **detailed** description of what correct operation looks like
Steps to reproduce | A **detailed** guide on how to successfully reproduce the problem

---

## Pull Requests
* Pull requests should have a title that gives the package name the PR is for, and a short description about the PR.
* The body of the PR should contain a **detailed** description of what changes are being introduced, and most importantly, *why* this PR should be merged.
* PRs must be for a single package only.  A PR addressing multiple packages without merit risks having a delayed merge or being closed.
* Ensure your PR addresses three of the most common problems:
  * Correctly update the pkgver or pkgrel of the package (see below).
  * Update the checksums if external files have been added or modified.
  * Is the package only for specific architectures?  Set the `buildarch` variable (see [the README](https://github.com/archlinuxarm/PKGBUILDs/blob/master/README.md))

### New Packages
* If the pull request is for a new package, review the [README](https://github.com/archlinuxarm/PKGBUILDs/blob/master/README.md) to ensure the package is going into the correct repository and meets all the stated requirements.

### Existing packages
#### Upstream x86 Arch Linux packages
* If you changed the PKGBUILD or related files, detail your changes in the comment header at the top.
* Increment the pkgrel variable by a decimal number.  This is to prevent interference tracking against upstream versions.  For example:
  * If `pkgrel=1`, change to `pkgrel=1.1`
  * If `pkgrel=1.1`, change to `pkgrel=1.2`
* At no time should an upstream package tracked here exceed the version upstream.
* If the package is only supposed to be built for a specific architecture, ensure the `buildarch` variable is set correctly (see [the README](https://github.com/archlinuxarm/PKGBUILDs/blob/master/README.md))

#### Arch Linux ARM specific packages
* If the change increases the version of the package, ensure `pkgver` is updated and `pkgrel` is reset to 1.
* Otherwise, increment `pkgrel` by 1.
* If the package is only supposed to be built for a specific architecture, ensure the `buildarch` variable is set correctly (see [the README](https://github.com/archlinuxarm/PKGBUILDs/blob/master/README.md))
