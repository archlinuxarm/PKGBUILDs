#!/bin/bash

build() {
    add_binary /usr/lib/systemd/systemd-timestamp /usr/bin/systemd-timestamp
}

help() {
    cat <<HELPEOF
Provides support for RD_TIMESTAMP in early userspace, which can be read by a
program such as systemd-analyze to determine boot time.
HELPEOF
}

# vim: set ft=sh ts=4 sw=4 et:
