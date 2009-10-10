#!/bin/sh

echo "WARNING: you should be calling csh with /bin/csh,"
echo "not with /usr/bin/csh. Please execute chsh to fix"
echo "this. Legacy /usr/bin/csh support will go away!"
echo ""

exec /bin/csh $@

