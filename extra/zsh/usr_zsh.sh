#!/bin/sh

/bin/echo "WARNING: you should be calling zsh with /bin/zsh,"
/bin/echo "not with /usr/bin/zsh. Please execute chsh to fix"
/bin/echo "this. Legacy /usr/bin/zsh support will go away!"
/bin/echo ""

exec /bin/zsh $@

