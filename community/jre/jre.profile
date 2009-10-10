export PATH=$PATH:/opt/java/jre/bin
if [ ! -f /etc/profile.d/jdk.sh ]; then
        export JAVA_HOME=/opt/java/jre
fi
