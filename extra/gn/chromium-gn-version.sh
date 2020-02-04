#!/bin/bash

set -eo pipefail

chromium_version=${1:-$(curl -s https://omahaproxy.appspot.com/linux)}

curl -s https://chromium.googlesource.com/chromium/src/+/$chromium_version/DEPS?format=TEXT |
	base64 -d | grep -Po "'gn_version': 'git_revision:\K[^']*"
