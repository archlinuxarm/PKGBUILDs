#!/bin/sh

# Priority order for IDEA_JDK:
# 1. Use IDEA_JDK if provided from external variable
# 2. Use package JBR if it exists
# 3. Fall back to /usr/lib/jvm/java-21-openjdk/
if [ -z "$IDEA_JDK" ] ; then
  if [ -d "/usr/share/idea/jbr/" ] ; then
    IDEA_JDK="/usr/share/idea/jbr/"
  else
    IDEA_JDK="/usr/lib/jvm/java-21-openjdk/"
  fi
fi
# open-jfx location that should match the JDK version
if [ -z "$IDEA_JFX" ] ; then
  IDEA_JFX="/usr/lib/jvm/java-21-openjfx/"
fi
# classpath according to defined JDK/JFX
if [ -z "$IDEA_CLASSPATH" ] ; then
  IDEA_CLASSPATH="${IDEA_JDK}/lib/*:${IDEA_JFX}/lib/*"
fi

exec env IDEA_JDK="$IDEA_JDK" IDEA_CLASSPATH="$IDEA_CLASSPATH" /usr/share/idea/bin/idea "$@"

# vim: ts=2 sw=2 et:
