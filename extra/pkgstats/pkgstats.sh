#!/bin/bash

pkgstatsver=1.0
showonly=false

usage() {
	echo "usage: ${0} [option]"
	echo 'options:'
	echo '	-v	show the version of pkgstats'
	echo '	-d	enable debug mode'
	echo '	-h	show this help'
	echo '	-s	show what information would be sent'
	echo '		(but do not send anything)'
	echo ''
	echo 'pkgstats sends a list of all installed packages and'
	echo 'the architecture you are using to the Arch Linux project.'
}

while getopts 'vdhs' option; do
	case $option in
		v)	echo "pkgstats, version ${pkgstatsver}"; exit 0;;
		d)	debug='-v';;
		s)	showonly=true;;
		*)	usage; exit 0;;
	esac
done

pkglist=$(mktemp --tmpdir pkglist.XXXXXX)
echo 'Creating package list...'
pacman -Qq > ${pkglist}

if $showonly; then
	echo 'packages='
	cat ${pkglist}
	echo ''
	echo "arch=$(uname -m)"
	echo "pkgstatsver=${pkgstatsver}"
else
	echo 'Submitting data...'
	curl ${debug} -f -H 'Expect: ' \
		--data-urlencode "packages@${pkglist}" \
		--data-urlencode "arch=$(uname -m)" \
		--data-urlencode "pkgstatsver=${pkgstatsver}" \
		'http://www.archlinux.de/?page=PostPackageList' \
		|| echo 'Sorry, package list could not be sent.'
fi

rm -f ${pkglist}
