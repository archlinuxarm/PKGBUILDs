#!/bin/sh

TARGETDIR=$2

mkdir -p $TARGETDIR

echo "<HTML><BODY>" > $TARGETDIR/index.html

$1/index.pl http://wiki.archlinux.org | while read A; do
    TITLE=`echo $A | cut -d \  -f 2- | tr ' ' '_'`
    ID=`echo $A | cut -d \  -f 1`
    echo "$ID => $TITLE"
    echo "<P><A HREF='$ID.html'>$TITLE</A>" >> $TARGETDIR/index.html
    [ -f "$TARGETDIR/$ID.html" ] || wget -q "http://wiki.archlinux.org/index.php?title=$TITLE&printable=yes" -O "$TARGETDIR/$ID.html"
done

echo "</BODY></HTML>" >> $TARGETDIR/index.html
