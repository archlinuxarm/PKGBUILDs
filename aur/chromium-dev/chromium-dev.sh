#!/bin/sh
export CHROME_WRAPPER=/usr/lib/chromium-dev/chromium
export CHROME_DESKTOP=chromium-dev.desktop
exec /usr/lib/chromium-dev/chromium "$@"
