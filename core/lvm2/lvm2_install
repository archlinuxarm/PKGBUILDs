#!/bin/bash

build() {
    local mod
    for mod in dm-mod dm-snapshot dm-mirror; do
        add_module "$mod"
    done

    add_binary "/sbin/lvm"
    add_binary "/sbin/dmsetup"
    add_file "/usr/lib/udev/rules.d/10-dm.rules"
    add_file "/usr/lib/udev/rules.d/13-dm-disk.rules"
    add_file "/usr/lib/udev/rules.d/95-dm-notify.rules"
    add_file "/usr/lib/udev/rules.d/11-dm-lvm.rules"
    add_file "/usr/lib/initcpio/udev/11-dm-initramfs.rules" "/usr/lib/udev/rules.d/11-dm-initramfs.rules"

    add_runscript
}

help() {
  cat <<HELPEOF
This hook loads the necessary modules for an LVM2 root device.

The optional lvmwait= parameter followed by a comma-separated
list of device names can be given on the command line.
It will cause the hook to wait until all given devices exist
before trying to scan and activate any volume groups.
HELPEOF
}

# vim: set ft=sh ts=4 sw=4 et:
