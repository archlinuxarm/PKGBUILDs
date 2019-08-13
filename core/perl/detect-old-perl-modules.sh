#!/bin/bash

basedir=/usr/lib/perl5
perlver=$(perl -e '$v = $^V->{version}; print $v->[0].".".($v->[1]);')

dir_empty() {
	local dir=$1
	[[ $(find $dir -maxdepth 0 -empty -exec echo empty \;) = "empty" ]] && return 0 || return 1
}

print_unowned_files() {
	local dir=$1
	LC_ALL=C find "$dir" -type f -exec pacman -Qqo {} + |& sed -n 's/^error: No package owns \(.*\)$/\1/p'
}

for dir in "$basedir/"*; do
	if [[ "${dir##*/}" != "$perlver" ]]; then
		if [[ -d "$dir" ]] && ! dir_empty "$dir"; then
			pkgcount=$(pacman -Qqo "$dir" | wc -l)
			if ((pkgcount > 0)); then
				printf "WARNING: '%s' contains data from at least %d packages which will NOT be used by the installed perl interpreter.\n" "$dir" "$pkgcount"
				printf " -> Run the following command to get a list of affected packages: pacman -Qqo '%s'\n" "$dir"
			fi

			unowned_count=$(print_unowned_files "$dir" | wc -l)
			if ((unowned_count > 0)); then
				printf "WARNING: %d file(s) in %s are not tracked by pacman and need to be rebuilt.\n" "$unowned_count" "$dir"
				printf " -> These were most likely installed directly by cpan or a similar tool.\n"
				printf "    Run the following command to get a list of these files:\n"
				printf "    LC_ALL=C find \"%s\" -type f -exec pacman -Qqo {} + |& sed -n 's/^error: No package owns \(.*\)$/\\\1/p'\n" "$dir"
			fi
		fi
	fi
done


