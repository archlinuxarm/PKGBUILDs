#!/bin/sh
export CHROME_WRAPPER=/usr/lib/chromium15/chromium
export CHROME_DESKTOP=chromium15.desktop
exec /usr/lib/chromium15/chromium "$@"
