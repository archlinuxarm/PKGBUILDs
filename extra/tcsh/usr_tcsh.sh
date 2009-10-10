#!/bin/sh

echo "WARNING: you should be calling tcsh with /bin/tcsh,"
echo "not with /usr/bin/tcsh. Please execute chsh to fix"
echo "this. Legacy /usr/bin/tcsh support will go away!"
echo ""

exec /bin/tcsh $@

