#!/bin/bash

sd_booted() {
  [[ -d run/systemd/system && ! -L run/systemd/system ]]
}

add_privs() {
  if ! setcap "$2" "$1" 2>/dev/null; then
    echo "==> Warning: setcap failed, falling back to setuid root on /$1"
    chmod u+s "$1"
  fi
}

add_journal_acls() {
  # ignore errors, since the filesystem might not support ACLs
  setfacl -Rnm g:wheel:rx,d:g:wheel:rx,g:adm:rx,d:g:adm:rx var/log/journal/ 2>/dev/null
  :
}

maybe_reexec() {
  # don't reexec on 209-1 upgrade due to large infrastructural changes.
  if [[ $(vercmp 209-1 "$2") -eq 1 ]]; then
    echo ':: systemd has not been reexecuted. It is recommended that you'
    echo '   reboot at your earliest convenience.'
    return
  fi

  if sd_booted; then
    systemctl --system daemon-reexec
  fi
}

_dir_empty() {
  set -- "$1"/*
  [[ ! -e $1  && ! -L $1 ]]
}

post_common() {
  systemd-sysusers
  udevadm hwdb --update
  journalctl --update-catalog
}

_204_1_changes() {
  printf '==> The /bin/systemd symlink has been removed. Any references in your\n'
  printf '    bootloader (or elsewhere) must be updated to /usr/lib/systemd/systemd.\n'
}

_205_1_changes() {
  printf '==> systemd 205 restructures the cgroup hierarchy and changes internal\n'
  printf '    protocols. You should reboot at your earliest convenience.\n'
}

_206_1_changes() {
  printf '==> The "timestamp" hook for mkinitcpio no longer exists. If you used\n'
  printf '    this hook, you must remove it from /etc/mkinitcpio.conf. A "systemd"\n'
  printf '    hook has been added which provides this functionality, and more.\n'
}

_208_1_changes() {
  if [[ -e var/lib/backlight && ! -e var/lib/systemd/backlight ]]; then
    mv -T var/lib/backlight var/lib/systemd/backlight
  fi

  if [[ -e var/lib/random-seed && ! -e var/lib/systemd/random-seed ]]; then
    mv -T var/lib/random-seed var/lib/systemd/random-seed
  fi
}

_208_8_changes() {
  add_journal_acls
}

_209_1_changes() {
  # attempt to preserve existing behavior

  local old_rule=etc/udev/rules.d/80-net-name-slot.rules
  local new_rule=etc/udev/rules.d/80-net-setup-link.rules

  echo ":: Network device naming is now controlled by udev's net_setup_link"
  echo "   builtin. Refer to the systemd.link manpage for a full description."

  # not clear what action we can take here, so don't do anything
  [[ -e $new_rule ]] && return 0

  # rename the old rule to the new one so that we preserve the user's
  # existing option.
  if [[ -e $old_rule ]]; then
    printf ':: Renaming %s to %s in order\n' "${old_rule##*/}" "${new_rule##*/}"
    printf '   to preserve existing network naming behavior.\n'
    mv -v "$old_rule" "$new_rule"
  else
    echo ':: No changes have been made to your network naming configuration.'
    echo '   Interfaces should continue to maintain the same names.'
  fi
}

_210_1_changes() {
  if sd_booted; then
    # If /etc/systemd/network is non-empty, then this is a 209 user who used
    # networkd. Re-enable it for them.
    if ! _dir_empty etc/systemd/network; then
      systemctl enable systemd-networkd
    fi
  fi
}

_213_4_changes() {
  if sd_booted; then
    # if /etc/resolv.conf is a symlink, just assume that it was being managed
    # by systemd-networkd, and re-enable systemd-resolved.
    if [[ -L etc/resolv.conf ]]; then
      systemctl enable systemd-resolved
    fi
  fi
}

_214_2_changes() {
  # /run/systemd/network/resolv.conf -> /run/systemd/resolve/resolv.conf
  if [[ etc/resolv.conf -ef run/systemd/network/resolv.conf ]]; then
    ln -sf /run/systemd/resolve/resolv.conf /etc/resolv.conf

    if sd_booted; then
      if [[ ! -d run/systemd/resolve ]]; then
        mkdir run/systemd/resolve
      fi

      if [[ -f run/systemd/network/resolv.conf ]]; then
        mv run/systemd/{network,resolve}/resolv.conf
      fi
    fi
  fi

  echo ':: coredumps are no longer sent to the journal by default. To re-enable:'
  echo '   echo >/etc/sysctl.d/50-coredump.conf \'
  echo '       "kernel.core_pattern=|/usr/lib/systemd/systemd-coredump %p %u %g %s %t %e"'
}

_215_2_changes() {
  # create at least the symlink from /etc/os-release to /usr/lib/os-release
  systemd-tmpfiles --create etc.conf
}

_216_2_changes() {
  echo ':: Coredumps are handled by systemd by default. Collection behavior can be'
  echo '   tuned in /etc/systemd/coredump.conf.'
}

_219_2_changes() {
  if mkdir -m2755 var/log/journal/remote 2>/dev/null; then
    chgrp systemd-journal-remote var/log/journal/remote
  fi
}

_219_4_changes() {
  if ! systemctl is-enabled -q remote-fs.target; then
    systemctl enable -q remote-fs.target
  fi
}

post_install() {
  systemd-machine-id-setup

  post_common "$@"

  add_journal_acls

  # enable some services by default, but don't track them
  systemctl enable getty@tty1.service remote-fs.target

  echo ":: Append 'init=/usr/lib/systemd/systemd' to your kernel command line in your"
  echo "   bootloader to replace sysvinit with systemd, or install systemd-sysvcompat"

  # group 'systemd-journal-remote' is created by systemd-sysusers
  mkdir -m2755 var/log/journal/remote
  chgrp systemd-journal-remote var/log/journal/remote
}

post_upgrade() {
  post_common "$@"

  maybe_reexec "$@"

  local v upgrades=(204-1
                    205-1
                    206-1
                    208-1
                    208-8
                    209-1
                    210-1
                    213-4
                    214-2
                    215-2
                    216-2
                    219-2
                    219-4)

  for v in "${upgrades[@]}"; do
    if [[ $(vercmp "$v" "$2") -eq 1 ]]; then
      "_${v//-/_}_changes"
    fi
  done
}

# vim:set ts=2 sw=2 et:
