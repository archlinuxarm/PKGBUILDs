#!/bin/bash

GLESTDIR="/opt/glest"

[ -d ~/.glest ] || mkdir ~/.glest
[ -e ~/.glest/glest ] || ln -s $GLESTDIR/glest ~/.glest/glest
[ -e ~/.glest/glest.ini ] || cp $GLESTDIR/glest.ini ~/.glest/glest.ini

for d in data techs maps tilesets scenarios tutorials; do
    [ -d ~/.glest/$d ] || ln -s $GLESTDIR/$d ~/.glest/$d
done

grep -E 'Lang=.*.lng' ~/.glest/glest.ini && sed -i 's#.lng##' ~/.glest/glest.ini

cd ~/.glest

export LD_LIBRARY_PATH=/opt/xerces-c-2/lib
exec ./glest "$@"
