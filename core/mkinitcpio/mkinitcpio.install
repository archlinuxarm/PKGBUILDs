#!/bin/sh

post_upgrade() {
  if [ "$(vercmp 0.9.0 "$2")" -eq 1 ]; then
    printf '==> If your /usr is on a separate partition, you must add the "usr" hook\n'
    printf '    to /etc/mkinitcpio.conf and regenerate your images before rebooting\n'
  fi

  if [ "$(vercmp 0.12.0 "$2")" -eq 1 ]; then
    printf '==> The "block" hook has replaced several hooks:\n'
    printf '       fw, sata, pata, scsi, virtio, mmc, usb\n'
    printf '    Replace any and all of these in /etc/mkinitcpio.conf with a single\n'
    printf '    instance of the "block" hook\n'
  fi
}
