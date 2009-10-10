#!/bin/sh -x

Q=`echo "$*" | sed 's|- p t h r e a d|-pthread|'`

echo "$Q" | grep 'lmp4ff' && Q="$Q /usr/lib/libmp4ff.a"

exec /bin/g++ $Q
