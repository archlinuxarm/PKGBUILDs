#!/bin/sh

PLAN9=${PLAN9:-/opt/plan9}
export PLAN9

case "$PATH" in
    $PLAN9/bin:*) ;;
    *) export PATH=$PLAN9/bin:$PATH ;;
esac

if [ $# -gt 0 ]; then
    exec "$@"
fi
