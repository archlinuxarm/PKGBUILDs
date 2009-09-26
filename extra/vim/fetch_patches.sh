# the external logic for pulling in patches

get_patches() {
  _patchdir=${srcdir}/patches
  cd ${srcdir}/${_versiondir}
  if [ -d ${_patchdir} ]; then
    rm -rf ${_patchdir}
    echo -e "\tremove patches from old build"
  fi
  mkdir ${_patchdir} && cd ${_patchdir}
  _rpath=ftp://ftp.vim.org/pub/vim/patches/${_srcver}

  # change IFS to loop line-by-line
  _OLDIFS=$IFS
  IFS="
"
  echo -e "\tfetching checksumfile for patches"
  wget ${_rpath}/MD5SUMS >/dev/null 2>&1

  _downloads=0
  for _line in $(/bin/cat MD5SUMS); do
    _file=$(echo $_line | cut -d ' ' -f3)
    [ ${_file##*.} == "gz" ] && continue
    _downloads=$((${_downloads} + 1))
    _md5=$(echo $_line | cut -d ' ' -f1)
    if [ -f ${SRCDEST}/vim-${_srcver}/${_file} ]; then
      echo -e "\thaving patch file:${_file}"
      cp ${SRCDEST}/vim-${_srcver}/${_file} ./
    else
      echo -n -e "\t... fetching patch file: ${_file} ..."
      wget ${_rpath}/${_file} >/dev/null 2>&1
      if [ -w ${SRCDEST} ]; then
        if [ ! -d ${SRCDEST}/vim-${_srcver} ]; then
          mkdir -p ${SRCDEST}/vim-${_srcver}
        fi
        cp ${_file} ${SRCDEST}/vim-${_srcver}/
        echo " done!"
      fi
    fi

    if [ $(echo "${_md5}  ${_file}" | md5sum --status -c -) ]; then
      echo ${_file} md5sums do not match
      return 1
    fi
  done

  ########

  if [ ${_downloads} != ${_patchlevel} ]; then
    echo ""
    echo -e "\t\tWARNING!"
    echo "You are not building the latest available version! A newer patchlevel"
    echo "seems to be available. Please edit the PKGBUILD and add the latest"
    echo "${_downloads} as pkgrel number!"
    echo ""
    sleep 10
  fi
  IFS=$_OLDIFS
  rm MD5SUMS
  cd ${srcdir}/${_versiondir}
  for _patchnum in $(/usr/bin/seq 1 ${_patchlevel}); do
    _patch=${_srcver}.$(printf "%03d" ${_patchnum})
    patch -Np0 -i ${_patchdir}/${_patch} || return 1
  done
  rm -rf ${_patchdir}
  return 0
}
