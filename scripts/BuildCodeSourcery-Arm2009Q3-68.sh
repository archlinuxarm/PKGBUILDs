#! /usr/bin/env bash
##########################################################
# Modified by cbxbiker61 on 2010.01.25 to build native
# cross-compiler tools suitable to building an arm kernel.
##########################################################
# This file contains the complete sequence of commands
# CodeSourcery used to build this version of Sourcery G++.
# 
# For each free or open-source component of Sourcery G++, the
# source code provided includes all of the configuration
# scripts and makefiles for that component, including any and
# all modifications made by CodeSourcery.  From this list of
# commands, you can see every configuration option used by
# CodeSourcery during the build process.
# 
# This file is provided as a guideline for users who wish to
# modify and rebuild a free or open-source component of
# Sourcery G++ from source. For a number of reasons, though,
# you may not be able to successfully run this script directly
# on your system. Certain aspects of the CodeSourcery build
# environment (such as directory names) are included in these
# commands. CodeSourcery uses Canadian cross compilers so you
# may need to modify various configuration options and paths
# if you are building natively. This sequence of commands
# includes those used to build proprietary components of
# Sourcery G++ for which source code is not provided.
# 
# Please note that Sourcery G++ support covers only your use
# of the original, validated binaries provided as part of
# Sourcery G++ -- and specifically does not cover either the
# process of rebuilding a component or the use of any binaries
# you may build.  In addition, if you rebuild any component,
# you must not use the --with-pkgversion and --with-bugurl
# configuration options that embed CodeSourcery trademarks in
# the resulting binary; see the "CodeSourcery Trademarks"
# section in the Sourcery G++ Software License Agreement.
set -e

scratch=/var/tmp/CodeSourceryArmTmp
respin=$scratch/respin
nativepath=/usr/bin:/opt/texlive/texmf/bin
srcdir=$PWD
arch=$(arch)
host_gcc=gcc
host_ar=ar
host_ranlib=ranlib
ane='arm-none-eabi'
ver='2009q3-68'
codesourcery=http://www.codesourcery.com/sgpp/lite/arm/portal/package5352/public/$ane
bigarc=arm-$ver-$ane.src.tar.bz2

makejobs=$(grep -Ec "^cpu[0-9]+" /proc/stat || :)
if ! grep -qs QEMU /proc/cpuinfo; then
	(( makejobs *= 2 ))
else
	(( makejobs++ )) # use fewer jobs on QEMU
fi
makej="make -j$makejobs"

inform_fd=2
umask 022
exec < /dev/null

error_handler ()
{
    exit 1
}

check_status() {
    local status="$?"
    if [ "$status" -ne 0 ]; then
	error_handler
    fi
}

check_pipe() {
    local -a status=("${PIPESTATUS[@]}")
    local limit=$1
    local ix

    if [ -z "$limit" ] ; then
	limit="${#status[@]}"
    fi
    for ((ix=0; ix != $limit ; ix++)); do
	if [ "${status[$ix]}" != "0" ] ; then
	    error_handler
	fi
    done
}

error () {
    echo "$script: error: $@" >& $inform_fd
    exit 1
}

warning () {
    echo "$script: warning: $@" >& $inform_fd
}

verbose () {
    if $gnu_verbose; then
	echo "$script: $@" >& $inform_fd
    fi
}

copy_dir() {
    mkdir -p "$2"

    (cd "$1" && tar cf - .) | (cd "$2" && tar xf -)
    check_pipe
}

copy_dir_clean() {
    mkdir -p "$2"
    (cd "$1" && tar cf - \
	--exclude=CVS --exclude=.svn --exclude=.git --exclude=.pc \
	--exclude="*~" --exclude=".#*" \
	--exclude="*.orig" --exclude="*.rej" \
	.) | (cd "$2" && tar xf -)
    check_pipe
}

update_dir_clean() {
    mkdir -p "$2"


    (cd "$1" && tar cf - \
	--exclude=CVS --exclude=.svn --exclude=.git --exclude=.pc \
	--exclude="*~" --exclude=".#*" \
	--exclude="*.orig" --exclude="*.rej" \
	--after-date="$3" \
	. 2> /dev/null) | (cd "$2" && tar xf -)
    check_pipe
}

copy_dir_exclude() {
    local source="$1"
    local dest="$2"
    local excl="$3"
    shift 3
    mkdir -p "$dest"
    (cd "$source" && tar cfX - "$excl" "$@") | (cd "$dest" && tar xf -)
    check_pipe
}

copy_dir_only() {
    local source="$1"
    local dest="$2"
    shift 2
    mkdir -p "$dest"
    (cd "$source" && tar cf - "$@") | (cd "$dest" && tar xf -)
    check_pipe
}

clean_environment() {
    local env_var_list
    local var




    unset BASH_ENV CDPATH POSIXLY_CORRECT TMOUT

    env_var_list=$(export | \
	grep '^declare -x ' | \
	sed -e 's/^declare -x //' -e 's/=.*//')

    for var in $env_var_list; do
	case $var in
	    HOME|HOSTNAME|LOGNAME|PWD|SHELL|SHLVL|SSH_*|TERM|USER)


		;;
	    LD_LIBRARY_PATH|PATH| \
		FLEXLM_NO_CKOUT_INSTALL_LIC|LM_APP_DISABLE_CACHE_READ)


		;;
	    MAKEINFO)

		;;
	    *_LICENSE_FILE)












		if [ "" ]; then
		    local license_file_envvar
		    license_file_envvar=

		    if [ "$var" != "$license_file_envvar" ]; then
			export -n "$var" || true
		    fi
		else
		    export -n "$var" || true
		fi
		;;
	    *)

		export -n "$var" || true
		;;
	esac
    done


    export LANG=C
    export LC_ALL=C


    export CVS_RSH=ssh



    user_shell=$SHELL
    export SHELL=$BASH
    export CONFIG_SHELL=$BASH
}

pushenv() {
    pushenv_level=$(($pushenv_level + 1))
    eval pushenv_vars_${pushenv_level}=
}


pushenv_level=0
pushenv_vars_0=



pushenvvar() {
    local pushenv_var="$1"
    local pushenv_newval="$2"
    eval local pushenv_oldval=\"\$$pushenv_var\"
    eval local pushenv_oldset=\"\${$pushenv_var+set}\"
    local pushenv_save_var=saved_${pushenv_level}_${pushenv_var}
    local pushenv_savep_var=savedp_${pushenv_level}_${pushenv_var}
    eval local pushenv_save_set=\"\${$pushenv_savep_var+set}\"
    if [ "$pushenv_save_set" = "set" ]; then
	error "Pushing $pushenv_var more than once at level $pushenv_level"
    fi
    if [ "$pushenv_oldset" = "set" ]; then
	eval $pushenv_save_var=\"\$pushenv_oldval\"
    else
	unset $pushenv_save_var
    fi
    eval $pushenv_savep_var=1
    eval export $pushenv_var=\"\$pushenv_newval\"
    local pushenv_list_var=pushenv_vars_${pushenv_level}
    eval $pushenv_list_var=\"\$$pushenv_list_var \$pushenv_var\"
}

prependenvvar() {
    local pushenv_var="$1"
    local pushenv_newval="$2"
    eval local pushenv_oldval=\"\$$pushenv_var\"
    pushenvvar "$pushenv_var" "$pushenv_newval$pushenv_oldval"
}

popenv() {
    local pushenv_var=
    eval local pushenv_vars=\"\$pushenv_vars_${pushenv_level}\"
    for pushenv_var in $pushenv_vars; do
	local pushenv_save_var=saved_${pushenv_level}_${pushenv_var}
	local pushenv_savep_var=savedp_${pushenv_level}_${pushenv_var}
	eval local pushenv_save_val=\"\$$pushenv_save_var\"
	eval local pushenv_save_set=\"\${$pushenv_save_var+set}\"
	unset $pushenv_save_var
	unset $pushenv_savep_var
	if [ "$pushenv_save_set" = "set" ]; then
	    eval export $pushenv_var=\"\$pushenv_save_val\"
	else
	    unset $pushenv_var
	fi
    done
    unset pushenv_vars_${pushenv_level}
    if [ "$pushenv_level" = "0" ]; then
	error "Popping environment level 0"
    else
	pushenv_level=$(($pushenv_level - 1))
    fi
}

prepend_path() {
    if $(eval "test -n \"\$$1\""); then
	prependenvvar "$1" "$2:"
    else
	prependenvvar "$1" "$2"
    fi
}

pushenvvar PATH $nativepath
#pushenvvar LD_LIBRARY_PATH /usr/local/tools/gcc-4.3.3/$arch-pc-linux-gnu/lib:/usr/local/tools/gcc-4.3.3/lib64:/usr/local/tools/gcc-4.3.3/lib
pushenvvar MAKEINFO 'makeinfo --css-ref=../cs.css'
clean_environment

echo task [001/052] /init/dirs
pushenv
pushenvvar CC_FOR_BUILD $host_gcc

# start from scratch
mkdir -p $scratch

if [[ ! -f $bigarc ]]; then
	wget -c $codesourcery/$bigarc
fi

if [[ $(md5sum $bigarc | cut -d' ' -f1) != "121805e970e78291247ab6bd29bcab73" ]]; then
	echo "md5sum of $bigarc doesn't match"
	exit 1
fi

if [[ ! -f $scratch/arc/zlib-$ver.tar.bz2 ]]; then
	pushd $scratch
	tar x --strip-components=1 -f $srcdir/$bigarc
	mkdir arc
	mv *.tar.bz2 *.sh *.txt arc
	popd
fi

# clean out prior run
rm -rf $respin/{install,logs,obj,pkg}

# make necessary directories
mkdir -p $respin/src
mkdir -p $respin/install/share/doc/arm-$ane/{html,pdf}
mkdir -p $respin/logs/data
mkdir -p $respin/obj
mkdir -p $respin/pkg
if [[ ! -d $respin/src/zlib-1.2.3 ]]; then
	pushd $respin/src
	tar xf $scratch/arc/zlib-$ver.tar.bz2
	tar xf $scratch/arc/gmp-$ver.tar.bz2
	tar xf $scratch/arc/mpfr-$ver.tar.bz2
	tar xf $scratch/arc/ppl-$ver.tar.bz2
	tar xf $scratch/arc/cloog-$ver.tar.bz2
	tar xf $scratch/arc/binutils-$ver.tar.bz2
	tar xf $scratch/arc/gcc-$ver.tar.bz2
	tar xf $scratch/arc/newlib-$ver.tar.bz2
	tar xf $scratch/arc/expat-$ver.tar.bz2
	tar xf $scratch/arc/gdb-$ver.tar.bz2
popd
fi
popenv

echo task [002/052] /init/cleanup
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
rm -f $respin/pkg/arm-$ver-$ane.src.tar.bz2 $respin/pkg/arm-$ver-$ane.backup.tar.bz2
rm -rf $respin/obj/pkg-$ver-$ane/arm-$ver-$ane
mkdir -p $respin/obj/pkg-$ver-$ane/arm-$ver-$ane
#ln -s $srcdir/*.tar.bz2 $respin/obj/pkg-$ver-$ane/arm-$ver-$ane
popenv

echo task [003/052] /$arch-pc-linux-gnu/host_cleanup
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
popenv

echo task [004/052] /$arch-pc-linux-gnu/zlib_first/copy
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
rm -rf $respin/obj/zlib-first-$ver-$ane-$arch-pc-linux-gnu
copy_dir_clean $respin/src/zlib-1.2.3 $respin/obj/zlib-first-$ver-$ane-$arch-pc-linux-gnu
chmod -R u+w $respin/obj/zlib-first-$ver-$ane-$arch-pc-linux-gnu
popenv

echo task [005/052] /$arch-pc-linux-gnu/zlib_first/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushd $respin/obj/zlib-first-$ver-$ane-$arch-pc-linux-gnu
pushenv
pushenvvar CC "$host_gcc "
pushenvvar AR "ar rc"
pushenvvar RANLIB $host_ranlib
./configure --prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr
popenv
popd
popenv

echo task [006/052] /$arch-pc-linux-gnu/zlib_first/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushd $respin/obj/zlib-first-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv

echo task [007/052] /$arch-pc-linux-gnu/zlib_first/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushd $respin/obj/zlib-first-$ver-$ane-$arch-pc-linux-gnu
make install
popd
popenv

echo task [008/052] /$arch-pc-linux-gnu/gmp/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CFLAGS '-g -O2'
rm -rf $respin/obj/gmp-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/gmp-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/gmp-$ver-$ane-$arch-pc-linux-gnu
$respin/src/gmp-stable/configure --build=$arch-pc-linux-gnu --target=$arch-pc-linux-gnu --prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --disable-shared --host=$arch-pc-linux-gnu --enable-cxx --disable-nls
popd
popenv
popenv
popenv

echo task [009/052] /$arch-pc-linux-gnu/gmp/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CFLAGS '-g -O2'
pushd $respin/obj/gmp-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv
popenv
popenv

echo task [010/052] /$arch-pc-linux-gnu/gmp/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CFLAGS '-g -O2'
pushd $respin/obj/gmp-$ver-$ane-$arch-pc-linux-gnu
make install
popd
popenv
popenv
popenv

echo task [011/052] /$arch-pc-linux-gnu/gmp/postinstall
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CFLAGS '-g -O2'
pushd $respin/obj/gmp-$ver-$ane-$arch-pc-linux-gnu
make check
popd
popenv
popenv
popenv

echo task [012/052] /$arch-pc-linux-gnu/mpfr/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
rm -rf $respin/obj/mpfr-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/mpfr-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/mpfr-$ver-$ane-$arch-pc-linux-gnu
$respin/src/mpfr-stable/configure --build=$arch-pc-linux-gnu --target=$ane --prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --disable-shared --host=$arch-pc-linux-gnu --disable-nls --with-gmp=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr
popd
popenv
popenv
popenv

echo task [013/052] /$arch-pc-linux-gnu/mpfr/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/mpfr-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv
popenv
popenv

echo task [014/052] /$arch-pc-linux-gnu/mpfr/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/mpfr-$ver-$ane-$arch-pc-linux-gnu
make install
popd
popenv
popenv
popenv

echo task [015/052] /$arch-pc-linux-gnu/mpfr/postinstall
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/mpfr-$ver-$ane-$arch-pc-linux-gnu
make check
popd
popenv
popenv
popenv

echo task [016/052] /$arch-pc-linux-gnu/ppl/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
rm -rf $respin/obj/ppl-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/ppl-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/ppl-$ver-$ane-$arch-pc-linux-gnu
$respin/src/ppl-0.10.2/configure --build=$arch-pc-linux-gnu --target=$ane --prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --disable-shared --host=$arch-pc-linux-gnu --disable-nls --with-libgmp-prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr
popd
popenv
popenv
popenv

echo task [017/052] /$arch-pc-linux-gnu/ppl/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/ppl-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv
popenv
popenv

echo task [018/052] /$arch-pc-linux-gnu/ppl/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/ppl-$ver-$ane-$arch-pc-linux-gnu
make install
popd
popenv
popenv
popenv

echo task [019/052] /$arch-pc-linux-gnu/cloog/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
rm -rf $respin/obj/cloog-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/cloog-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/cloog-$ver-$ane-$arch-pc-linux-gnu
$respin/src/cloog-0.15/configure --build=$arch-pc-linux-gnu --target=$ane --prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --disable-shared --host=$arch-pc-linux-gnu --disable-nls --with-ppl=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --with-gmp=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr
popd
popenv
popenv
popenv

echo task [020/052] /$arch-pc-linux-gnu/cloog/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/cloog-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv
popenv
popenv

echo task [021/052] /$arch-pc-linux-gnu/cloog/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/cloog-$ver-$ane-$arch-pc-linux-gnu
make install
popd
popenv
popenv
popenv

echo task [022/052] /$arch-pc-linux-gnu/cloog/postinstall
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/cloog-$ver-$ane-$arch-pc-linux-gnu
make check
popd
popenv
popenv
popenv

echo task [023/052] /$arch-pc-linux-gnu/toolchain/binutils/copy
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
rm -rf $respin/obj/binutils-src-$ver-$ane-$arch-pc-linux-gnu
copy_dir_clean $respin/src/binutils-stable $respin/obj/binutils-src-$ver-$ane-$arch-pc-linux-gnu
chmod -R u+w $respin/obj/binutils-src-$ver-$ane-$arch-pc-linux-gnu
touch $respin/obj/binutils-src-$ver-$ane-$arch-pc-linux-gnu/.gnu-stamp
popenv
popenv
popenv

echo task [024/052] /$arch-pc-linux-gnu/toolchain/binutils/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
rm -rf $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu
$respin/obj/binutils-src-$ver-$ane-$arch-pc-linux-gnu/configure --build=$arch-pc-linux-gnu --target=$ane --prefix=/opt/codesourcery --host=$arch-pc-linux-gnu '--with-pkgversion=Sourcery G++ Lite $ver' --with-bugurl=https://support.codesourcery.com/GNUToolchain/ --disable-nls --with-sysroot=/opt/codesourcery/$ane --enable-poison-system-directories
popd
popenv
popenv
popenv

echo task [025/052] /$arch-pc-linux-gnu/toolchain/binutils/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
pushd $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv
popenv
popenv

echo task [026/052] /$arch-pc-linux-gnu/toolchain/binutils/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
pushd $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu
make install prefix=$respin/install exec_prefix=$respin/install libdir=$respin/install/lib htmldir=$respin/install/share/doc/arm-$ane/html pdfdir=$respin/install/share/doc/arm-$ane/pdf infodir=$respin/install/share/doc/arm-$ane/info mandir=$respin/install/share/doc/arm-$ane/man datadir=$respin/install/share
popd
popenv
popenv
popenv

echo task [027/052] /$arch-pc-linux-gnu/toolchain/binutils/postinstall
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
pushd $respin/install
[[ ! -f ./lib64/libiberty.a ]] || rm ./lib64/libiberty.a
[[ ! -f ./lib/libiberty.a ]] || rm ./lib/libiberty.a
[[ ! -d ./lib64 ]] || rmdir ./lib64
[[ ! -d ./lib ]] || rmdir ./lib
popd
copy_dir_clean $respin/src/binutils-stable/include $respin/obj/host-binutils-$ver-$ane-$arch-pc-linux-gnu/usr/include
chmod -R u+w $respin/obj/host-binutils-$ver-$ane-$arch-pc-linux-gnu/usr/include
mkdir -p $respin/obj/host-binutils-$ver-$ane-$arch-pc-linux-gnu/usr/lib
cp $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu/libiberty/libiberty.a $respin/obj/host-binutils-$ver-$ane-$arch-pc-linux-gnu/usr/lib
cp $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu/bfd/.libs/libbfd.a $respin/obj/host-binutils-$ver-$ane-$arch-pc-linux-gnu/usr/lib
cp $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu/bfd/bfd.h $respin/obj/host-binutils-$ver-$ane-$arch-pc-linux-gnu/usr/include
cp $respin/src/binutils-stable/bfd/elf-bfd.h $respin/obj/host-binutils-$ver-$ane-$arch-pc-linux-gnu/usr/include
cp $respin/obj/binutils-$ver-$ane-$arch-pc-linux-gnu/opcodes/.libs/libopcodes.a $respin/obj/host-binutils-$ver-$ane-$arch-pc-linux-gnu/usr/lib
popenv
popenv
popenv

echo task [028/052] /$arch-pc-linux-gnu/toolchain/gcc_first/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenvvar AR_FOR_TARGET $ane-ar
pushenvvar NM_FOR_TARGET $ane-nm
pushenvvar OBJDUMP_FOR_TARET $ane-objdump
pushenvvar STRIP_FOR_TARGET $ane-strip
pushenvvar gcc_cv_as_cfi_directive no
rm -rf $respin/obj/gcc-first-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/gcc-first-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/gcc-first-$ver-$ane-$arch-pc-linux-gnu
$respin/src/gcc-4.4/configure --build=$arch-pc-linux-gnu --host=$arch-pc-linux-gnu --target=$ane --enable-threads --disable-libmudflap --disable-libssp --disable-libstdcxx-pch --enable-extra-sgxxlite-multilibs --with-gnu-as --with-gnu-ld '--with-specs=%{O2:%{!fno-remove-local-statics: -fremove-local-statics}} %{O*:%{O|O0|O1|O2|Os:;:%{!fno-remove-local-statics: -fremove-local-statics}}}' --enable-languages=c,c++ --disable-shared --disable-lto --with-newlib '--with-pkgversion=Sourcery G++ Lite $ver' --with-bugurl=https://support.codesourcery.com/GNUToolchain/ --disable-nls --prefix=/opt/codesourcery --disable-shared --disable-threads --disable-libssp --disable-libgomp --without-headers --with-newlib --disable-decimal-float --disable-libffi --enable-languages=c --with-sysroot=/opt/codesourcery/$ane --with-build-sysroot=$respin/install/$ane --with-gmp=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --with-mpfr=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --with-ppl=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr '--with-host-libstdcxx=-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic -lm' --with-cloog=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --disable-libgomp --enable-poison-system-directories --with-build-time-tools=$respin/install/$ane/bin --with-build-time-tools=$respin/install/$ane/bin
popd
popenv
popenv

echo task [029/052] /$arch-pc-linux-gnu/toolchain/gcc_first/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenvvar AR_FOR_TARGET $ane-ar
pushenvvar NM_FOR_TARGET $ane-nm
pushenvvar OBJDUMP_FOR_TARET $ane-objdump
pushenvvar STRIP_FOR_TARGET $ane-strip
pushenvvar gcc_cv_as_cfi_directive no
pushd $respin/obj/gcc-first-$ver-$ane-$arch-pc-linux-gnu
$makej LDFLAGS_FOR_TARGET=--sysroot=$respin/install/$ane CPPFLAGS_FOR_TARGET=--sysroot=$respin/install/$ane build_tooldir=$respin/install/$ane
popd
popenv
popenv

echo task [030/052] /$arch-pc-linux-gnu/toolchain/gcc_first/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenvvar AR_FOR_TARGET $ane-ar
pushenvvar NM_FOR_TARGET $ane-nm
pushenvvar OBJDUMP_FOR_TARET $ane-objdump
pushenvvar STRIP_FOR_TARGET $ane-strip
pushenvvar gcc_cv_as_cfi_directive no
pushd $respin/obj/gcc-first-$ver-$ane-$arch-pc-linux-gnu
make prefix=$respin/install exec_prefix=$respin/install libdir=$respin/install/lib htmldir=$respin/install/share/doc/arm-$ane/html pdfdir=$respin/install/share/doc/arm-$ane/pdf infodir=$respin/install/share/doc/arm-$ane/info mandir=$respin/install/share/doc/arm-$ane/man install
popd
popenv
popenv

echo task [031/052] /$arch-pc-linux-gnu/toolchain/gcc_first/postinstall
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenvvar AR_FOR_TARGET $ane-ar
pushenvvar NM_FOR_TARGET $ane-nm
pushenvvar OBJDUMP_FOR_TARET $ane-objdump
pushenvvar STRIP_FOR_TARGET $ane-strip
pushenvvar gcc_cv_as_cfi_directive no
pushd $respin/install
rm bin/$ane-gccbug
[[ ! -f ./lib64/libiberty.a ]] || rm ./lib64/libiberty.a
[[ ! -f ./lib/libiberty.a ]] || rm ./lib/libiberty.a
rmdir include
popd
popenv
popenv

echo task [032/052] /$arch-pc-linux-gnu/toolchain/newlib/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CFLAGS_FOR_TARGET '-g -O2 -fno-unroll-loops'
rm -rf $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu
$respin/src/newlib-stable/configure --build=$arch-pc-linux-gnu --target=$ane --prefix=/opt/codesourcery --host=$arch-pc-linux-gnu --enable-newlib-io-long-long --disable-newlib-supplied-syscalls --disable-libgloss --disable-newlib-supplied-syscalls --disable-nls
popd
popenv
popenv
popenv

echo task [033/052] /$arch-pc-linux-gnu/toolchain/newlib/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CFLAGS_FOR_TARGET '-g -O2 -fno-unroll-loops'
pushd $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv
popenv
popenv

echo task [034/052] /$arch-pc-linux-gnu/toolchain/newlib/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CFLAGS_FOR_TARGET '-g -O2 -fno-unroll-loops'
pushd $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu
make install prefix=$respin/install exec_prefix=$respin/install libdir=$respin/install/lib htmldir=$respin/install/share/doc/arm-$ane/html pdfdir=$respin/install/share/doc/arm-$ane/pdf infodir=$respin/install/share/doc/arm-$ane/info mandir=$respin/install/share/doc/arm-$ane/man datadir=$respin/install/share
popd
popenv
popenv
popenv

echo task [035/052] /$arch-pc-linux-gnu/toolchain/newlib/postinstall
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CFLAGS_FOR_TARGET '-g -O2 -fno-unroll-loops'
pushd $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu
make pdf
mkdir -p $respin/install/share/doc/arm-$ane/pdf
cp $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu/$ane/newlib/libc/libc.pdf $respin/install/share/doc/arm-$ane/pdf/libc.pdf
mkdir -p $respin/install/share/doc/arm-$ane/pdf
cp $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu/$ane/newlib/libm/libm.pdf $respin/install/share/doc/arm-$ane/pdf/libm.pdf
make html
mkdir -p $respin/install/share/doc/arm-$ane/html
copy_dir $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu/$ane/newlib/libc/libc.html $respin/install/share/doc/arm-$ane/html/libc
mkdir -p $respin/install/share/doc/arm-$ane/html
copy_dir $respin/obj/newlib-$ver-$ane-$arch-pc-linux-gnu/$ane/newlib/libm/libm.html $respin/install/share/doc/arm-$ane/html/libm
popd
popenv
popenv
popenv

echo task [036/052] /$arch-pc-linux-gnu/toolchain/gcc_final/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenvvar AR_FOR_TARGET $ane-ar
pushenvvar NM_FOR_TARGET $ane-nm
pushenvvar OBJDUMP_FOR_TARET $ane-objdump
pushenvvar STRIP_FOR_TARGET $ane-strip
pushenvvar gcc_cv_as_cfi_directive no
rm -f $respin/install/$ane/usr
ln -s . $respin/install/$ane/usr
rm -rf $respin/obj/gcc-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/gcc-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/gcc-$ver-$ane-$arch-pc-linux-gnu
$respin/src/gcc-4.4/configure --build=$arch-pc-linux-gnu --host=$arch-pc-linux-gnu --target=$ane --enable-threads --disable-libmudflap --disable-libssp --disable-libstdcxx-pch --enable-extra-sgxxlite-multilibs --with-gnu-as --with-gnu-ld '--with-specs=%{O2:%{!fno-remove-local-statics: -fremove-local-statics}} %{O*:%{O|O0|O1|O2|Os:;:%{!fno-remove-local-statics: -fremove-local-statics}}}' --enable-languages=c,c++ --disable-shared --disable-lto --with-newlib '--with-pkgversion=Sourcery G++ Lite $ver' --with-bugurl=https://support.codesourcery.com/GNUToolchain/ --disable-nls --prefix=/opt/codesourcery --with-headers=yes --with-sysroot=/opt/codesourcery/$ane --with-build-sysroot=$respin/install/$ane --with-gmp=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --with-mpfr=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --with-ppl=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr '--with-host-libstdcxx=-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic -lm' --with-cloog=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --disable-libgomp --enable-poison-system-directories --with-build-time-tools=$respin/install/$ane/bin --with-build-time-tools=$respin/install/$ane/bin
popd
popenv
popenv

echo task [037/052] /$arch-pc-linux-gnu/toolchain/gcc_final/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenvvar AR_FOR_TARGET $ane-ar
pushenvvar NM_FOR_TARGET $ane-nm
pushenvvar OBJDUMP_FOR_TARET $ane-objdump
pushenvvar STRIP_FOR_TARGET $ane-strip
pushenvvar gcc_cv_as_cfi_directive no
rm -f $respin/install/$ane/usr
ln -s . $respin/install/$ane/usr
pushd $respin/obj/gcc-$ver-$ane-$arch-pc-linux-gnu
$makej LDFLAGS_FOR_TARGET=--sysroot=$respin/install/$ane CPPFLAGS_FOR_TARGET=--sysroot=$respin/install/$ane build_tooldir=$respin/install/$ane
popd
popenv
popenv

echo task [038/052] /$arch-pc-linux-gnu/toolchain/gcc_final/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenvvar AR_FOR_TARGET $ane-ar
pushenvvar NM_FOR_TARGET $ane-nm
pushenvvar OBJDUMP_FOR_TARET $ane-objdump
pushenvvar STRIP_FOR_TARGET $ane-strip
pushenvvar gcc_cv_as_cfi_directive no
rm -f $respin/install/$ane/usr
ln -s . $respin/install/$ane/usr
pushd $respin/obj/gcc-$ver-$ane-$arch-pc-linux-gnu
make prefix=$respin/install exec_prefix=$respin/install libdir=$respin/install/lib htmldir=$respin/install/share/doc/arm-$ane/html pdfdir=$respin/install/share/doc/arm-$ane/pdf infodir=$respin/install/share/doc/arm-$ane/info mandir=$respin/install/share/doc/arm-$ane/man install
popd
popenv
popenv

echo task [039/052] /$arch-pc-linux-gnu/toolchain/gcc_final/postinstall
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenvvar AR_FOR_TARGET $ane-ar
pushenvvar NM_FOR_TARGET $ane-nm
pushenvvar OBJDUMP_FOR_TARET $ane-objdump
pushenvvar STRIP_FOR_TARGET $ane-strip
pushenvvar gcc_cv_as_cfi_directive no
pushd $respin/install
rm bin/$ane-gccbug
[[ ! -f ./lib64/libiberty.a ]] || rm ./lib64/libiberty.a
[[ ! -f ./lib/libiberty.a ]] || rm ./lib/libiberty.a
rm ./$ane/lib/libiberty.a
rm ./$ane/lib/thumb2/libiberty.a
rm ./$ane/lib/thumb/libiberty.a
rm ./$ane/lib/armv6-m/libiberty.a
rmdir include
popd
rm -f $respin/install/$ane/usr
popenv
popenv

echo task [040/052] /$arch-pc-linux-gnu/toolchain/zlib/0/copy
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
rm -rf $respin/obj/zlib-$ver-$ane-$arch-pc-linux-gnu
copy_dir_clean $respin/src/zlib-1.2.3 $respin/obj/zlib-$ver-$ane-$arch-pc-linux-gnu
chmod -R u+w $respin/obj/zlib-$ver-$ane-$arch-pc-linux-gnu
popenv

echo task [041/052] /$arch-pc-linux-gnu/toolchain/zlib/0/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushd $respin/obj/zlib-$ver-$ane-$arch-pc-linux-gnu
pushenv
pushenvvar CC "$host_gcc "
pushenvvar AR "ar rc"
pushenvvar RANLIB $host_ranlib
./configure --prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr
popenv
popd
popenv

echo task [042/052] /$arch-pc-linux-gnu/toolchain/zlib/0/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushd $respin/obj/zlib-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv

echo task [043/052] /$arch-pc-linux-gnu/toolchain/zlib/0/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushd $respin/obj/zlib-$ver-$ane-$arch-pc-linux-gnu
make install
popd
popenv

echo task [044/052] /$arch-pc-linux-gnu/toolchain/expat/0/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
rm -rf $respin/obj/expat-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/expat-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/expat-$ver-$ane-$arch-pc-linux-gnu
$respin/src/expat-mirror/configure --build=$arch-pc-linux-gnu --target=$ane --prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --disable-shared --host=$arch-pc-linux-gnu --disable-nls
popd
popenv
popenv
popenv

echo task [045/052] /$arch-pc-linux-gnu/toolchain/expat/0/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/expat-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv
popenv
popenv

echo task [046/052] /$arch-pc-linux-gnu/toolchain/expat/0/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushd $respin/obj/expat-$ver-$ane-$arch-pc-linux-gnu
make install
popd
popenv
popenv
popenv

echo task [047/052] /$arch-pc-linux-gnu/toolchain/gdb/0/copy
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
rm -rf $respin/obj/gdb-src-$ver-$ane-$arch-pc-linux-gnu
copy_dir_clean $respin/src/gdb-stable $respin/obj/gdb-src-$ver-$ane-$arch-pc-linux-gnu
chmod -R u+w $respin/obj/gdb-src-$ver-$ane-$arch-pc-linux-gnu
touch $respin/obj/gdb-src-$ver-$ane-$arch-pc-linux-gnu/.gnu-stamp
popenv
popenv
popenv

echo task [048/052] /$arch-pc-linux-gnu/toolchain/gdb/0/configure
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
rm -rf $respin/obj/gdb-$ver-$ane-$arch-pc-linux-gnu
mkdir -p $respin/obj/gdb-$ver-$ane-$arch-pc-linux-gnu
pushd $respin/obj/gdb-$ver-$ane-$arch-pc-linux-gnu
$respin/obj/gdb-src-$ver-$ane-$arch-pc-linux-gnu/configure --build=$arch-pc-linux-gnu --target=$ane --prefix=/opt/codesourcery --host=$arch-pc-linux-gnu '--with-pkgversion=Sourcery G++ Lite $ver' --with-bugurl=https://support.codesourcery.com/GNUToolchain/ --disable-nls --with-libexpat-prefix=$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr --with-system-gdbinit=/opt/codesourcery/$arch-pc-linux-gnu/$ane/lib/gdbinit
popd
popenv
popenv
popenv

echo task [049/052] /$arch-pc-linux-gnu/toolchain/gdb/0/build
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
pushd $respin/obj/gdb-$ver-$ane-$arch-pc-linux-gnu
$makej
popd
popenv
popenv
popenv

echo task [050/052] /$arch-pc-linux-gnu/toolchain/gdb/0/install
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
pushd $respin/obj/gdb-$ver-$ane-$arch-pc-linux-gnu
make install prefix=$respin/install exec_prefix=$respin/install libdir=$respin/install/lib htmldir=$respin/install/share/doc/arm-$ane/html pdfdir=$respin/install/share/doc/arm-$ane/pdf infodir=$respin/install/share/doc/arm-$ane/info mandir=$respin/install/share/doc/arm-$ane/man datadir=$respin/install/share
popd
popenv
popenv
popenv

echo task [051/052] /$arch-pc-linux-gnu/toolchain/gdb/0/postinstall
pushenv
pushenvvar CC_FOR_BUILD $host_gcc
pushenvvar CC $host_gcc
pushenvvar AR $host_ar
pushenvvar RANLIB $host_ranlib
prepend_path PATH $respin/install/bin
pushenv
pushenv
pushenvvar CPPFLAGS -I$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/include
pushenvvar LDFLAGS -L$respin/obj/host-libs-$ver-$ane-$arch-pc-linux-gnu/usr/lib
popenv
popenv
popenv

echo task [052/052] /$arch-pc-linux-gnu/createarchive
pushd $respin/install/bin
rm $ane-gcc $ane-c++
ln -s $ane-gcc-4.4.1 $ane-gcc
ln -s $ane-g++ $ane-c++
pushd ..
mkdir -p $scratch/arm-$ver-$arch
cp -a * $scratch/arm-$ver-$arch
pushd $scratch
tar cjf arm-$ver-$ane-$arch-pc-linux-gnu-custom.tar.bz2 arm-$ver-$arch
rm -rf $scratch/arm-$ver-$arch
mv arm-$ver-$ane-$arch-pc-linux-gnu-custom.tar.bz2 $srcdir
popd
popd
popd

echo
echo "*************************************************************************"
echo "Finished building $srcdir/arm-$ver-$ane-$arch-pc-linux-gnu-custom.tar.bz2"
echo "*************************************************************************"

