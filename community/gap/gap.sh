#!/bin/sh

GAP_ROOT="/usr/lib/gap"
GAP_DIR=$GAP_ROOT
GAP_EXE=$GAP_ROOT

exec "$GAP_EXE/gap" -l "$GAP_DIR" "$@"
