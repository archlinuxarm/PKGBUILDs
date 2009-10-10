#!/bin/sh

echo "WARNING: you should be calling ksh with /bin/ksh,"
echo "not with /usr/bin/ksh. Please execute chsh to fix"
echo "this. Legacy /usr/bin/ksh support will go away!"
echo ""

exec /bin/ksh $@

