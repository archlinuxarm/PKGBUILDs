#!/bin/sh

for jar in /usr/share/java/sportstracker/*.jar; do
  CP=$CP:$jar
done

java -cp $CP de.saring.sportstracker.gui.STMain
