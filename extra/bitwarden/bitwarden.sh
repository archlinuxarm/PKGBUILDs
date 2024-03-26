#!/bin/sh
export ELECTRON_IS_DEV=0
cd /usr/lib/bitwarden
exec electron@electronversion@ /usr/lib/bitwarden/app.asar $@
