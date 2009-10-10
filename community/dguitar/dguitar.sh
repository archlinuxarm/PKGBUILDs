#!/bin/sh
for name in /usr/share/java/dguitar/*.jar ; do
  CP=$CP:$name
done
cd /usr/share/dguitar
java -cp $CP -Dapple.laf.useScreenMenuBar=true -Dcom.apple.mrj.application.apple.menu.about.name=DGuitar dguitar.gui.DGuitar
