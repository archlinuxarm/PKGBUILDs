#From: "Simon J. Gerraty" <sjg@zen.void.oz.au>
#Message-Id: <199510091130.VAA01188@zen.void.oz.au>
#Subject: Re: a shell idea?
#Date: Mon, 09 Oct 1995 21:30:20 +1000


# NAME:
#	add_path.sh - add dir to path
#
# DESCRIPTION:
#	These functions originated in /etc/profile and ksh.kshrc, but
#	are more useful in a separate file.
#
# SEE ALSO:
#	/etc/profile
#
# AUTHOR:
#	Simon J. Gerraty <sjg@zen.void.oz.au>

#	@(#)Copyright (c) 1991 Simon J. Gerraty
#
#	This file is provided in the hope that it will
#	be of use.  There is absolutely NO WARRANTY.
#	Permission to copy, redistribute or otherwise
#	use this file is hereby granted provided that
#	the above copyright notice and this notice are
#	left intact.

# is $1 missing from $2 (or PATH) ?
no_path() {
	eval "case :\$${2-PATH}: in *:$1:*) return 1;; *) return 0;; esac"
}
# if $1 exists and is not in path, append it
add_path () {
  [ -d ${1:-.} ] && no_path $* && eval ${2:-PATH}="\$${2:-PATH}:$1"
}
# if $1 exists and is not in path, prepend it
pre_path () {
  [ -d ${1:-.} ] && no_path $* && eval ${2:-PATH}="$1:\$${2:-PATH}"
}
# if $1 is in path, remove it
del_path () {
  no_path $* || eval ${2:-PATH}=`eval echo :'$'${2:-PATH}: |
    sed -e "s;:$1:;:;g" -e "s;^:;;" -e "s;:\$;;"`
}
