#! /bin/sh
# Creates root symlink in /dev
# for Arch Linux by Roman Kyrylych <Roman.Kyrylych@gmail.com>

if ! [ -L /dev/root ]; then 
ln -s $(cat /proc/cmdline | sed "s: :\n:g" | grep root= | sed "s:root=::") /dev/root
fi