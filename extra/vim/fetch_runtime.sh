# the purpose of this magic is to pull in the latest runtime files for vim
# we start withe theruntime provoded by the tarballs and compare MD5s against
# the latest runtime
# if this fails we look in the local source cache if they have been fetched
# for an earlier build and compare those MD5 files
# if this fails, we fetch the stuff from online and store it in the local src
# cache.
# The local cache has to be set (makepkg.conf) AND it has to be writable

update_runtime() {
  _OLDDIR=$(pwd) #get absolute path
  _errors=0
  _ftp="ftp://ftp.vim.org/pub/vim/runtime"

  # we're gonna be sneaky and grok the A-A-P recipe for the files we need
  _recipe="getunix.aap"
  _srccache="${SRCDEST}/vim-${_srcver}/"

  echo "getting runtime recipe"
  cd ${srcdir}
  [ -f "${_recipe}" ] && rm "${_recipe}"
  wget "${_ftp}/${_recipe}" >/dev/null 2>&1

  cd "${_runtimedir}"
  _runtimedir=$(pwd) #get absolute path

  # change IFS to loop line-by-line
  _OLDIFS=$IFS
  IFS="
"
  echo "begin fetching updated runtime files..."
  for _file in $(grep "file = " "${srcdir}/${_recipe}"); do
    _file=$(echo ${_file} | sed "s|.*file = \(.*\)|\1|")
    _md5=$(grep -A2 "file = ${_file} *$" "${srcdir}/${_recipe}" | \
          grep "get_md5" | \
          sed 's|@if get_md5(file) != "\(.*\)":|\1|g')
    _dir=$(dirname "${_file}")

    mkdir -p "${_dir}"

    echo -e "\t${_file}"
    _havefile=0
    # if we have the file and the MD5sum fails, we technically don't have the file
    if [ -f ${_file} ]; then
      # MD5 fails ? ... we don't have the file
      if [ $(echo "${_md5}  ${_file}" | md5sum --status -c -) ]; then
        rm ${_file}
      else
        _havefile=1
      fi
    fi
    # look files that were not copied from the unzipped sources
    _cachefile=${srcdir}/${_versiondir}/runtime/${_file}
    if [ ${_havefile} -ne 1 -a -f ${_cachefile} ]; then
      # MD5 fails ? ... we lookup if we downloaded another version earlier
      if [ $(echo "${_md5}  ${_cachefile}" | md5sum --status -c -) ]; then
        _cachefile=${_srccache}/${_file}
        if [ -f ${_cachefile} ]; then
          if [ $(echo "${_md5}  ${_cachefile}" | md5sum --status -c -) ]; then
            rm ${_cachefile}
          else
            cp ${_cachefile} ${_dir}
            _havefile=1
          fi
        fi
      else
        cp ${_cachefile} ${_dir}
        _havefile=1
      fi
    fi
    # look up the local $SRCDEST
    _cachefile=${_srccache}/${_file}
    if [ ${_havefile} -ne 1 -a -f ${_cachefile} ]; then
      # MD5 fails ? ... we don't have the file
      if [ $(echo "${_md5}  ${_cachefile}" | md5sum --status -c -) ]; then
        rm ${_cachefile}
      else
        cp ${_cachefile} ${_dir}
        _havefile=1
      fi
    fi
    # so we finally have to fetch it and store it to $SRCDEST (cache)
    if [ ${_havefile} -ne 1 ]; then
      echo -n -e "\t ... fetching file ${_file} ..."
      cd "${_dir}"
      wget "${_ftp}/${_file}" >/dev/null 2>&1
      cd "${_runtimedir}"
      # store freshly downloaded file in SRCDEST
      mkdir -p ${_srccache}/${_dir}
      cp ${_file} ${_srccache}/${_dir}
      echo -e " done!"
    fi

   # check the MD5 sum finally
    if [ $(echo "${_md5}  ${_file}" | md5sum --status -c -) ]; then
      echo "!!!! md5sum check for ${_file} failed !!!!"
      errors=$((${_errors} + 1))
    fi
  done
  IFS=${_OLDIFS}

  echo "vim runtime got updated"

  if [ ${_errors} -gt 0 ]; then
    echo "${_errors} failed MD5 checks while updating runtime files -> build can't be completed"
    return 1
  else
    echo -e "\tpatching filetype.vim for better handling of pacman related files ..."
    sed -i "s/rpmsave/pacsave/;s/rpmnew/pacnew/;s/,\*\.ebuild/\0,PKGBUILD*,*.install/" filetype.vim
    sed -i "/find the end/,+3{s/changelog_date_entry_search/changelog_date_end_entry_search/}" ftplugin/changelog.vim
  fi
  # make Aaron happy
  wget http://www.vim.org/scripts/download_script.php\?src_id=10872 \
    -O autoload/pythoncomplete.vim
  cd "${_OLDDIR}"
  return 0
}
