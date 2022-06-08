#!/bin/bash

set -eo pipefail

readonly CURL='curl -s --compressed'

gn_revision_from_chrome_version() {
  $CURL "https://chromium.googlesource.com/chromium/src/+/$1/DEPS?format=TEXT" \
    | base64 -d | grep -Po "'gn_version': 'git_revision:\K[^']*"
}

{
  echo channel version gn_revision
  echo ------- ------- -----------
  while read -r channel version; do
    echo "$channel $version $(gn_revision_from_chrome_version "$version")"
  done < <(
    $CURL https://omahaproxy.appspot.com/json \
      | jq -r '.[] | select ( .os == "linux" ) | .versions | .[] | "\(.channel) \(.version)"'
  )
} | column -t
