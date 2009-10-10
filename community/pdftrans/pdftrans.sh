#!/bin/sh

PROG_NAME=pdftrans
PROG_MAIN=PDFTrans

CP=/usr/share/java/$PROG_NAME

for jar in /usr/share/java/$PROG_NAME/*.jar; do
  CP=$CP:$jar
done

CP=$CP:/usr/share/java/itext/itext.jar
CP=$CP:/usr/share/java/bcprov.jar

java -cp $CP $PROG_MAIN "$@"
