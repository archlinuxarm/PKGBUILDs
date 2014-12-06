PLEX_MEDIA_SERVER_USER=plex

PLEX_MEDIA_SERVER_HOME=/opt/plexmediaserver

# the number of plugins that can run at the same time
PLEX_MEDIA_SERVER_MAX_PLUGIN_PROCS=6

# ulimit -s $PLEX_MEDIA_SERVER_MAX_STACK_SIZE
PLEX_MEDIA_SERVER_MAX_STACK_SIZE=3000

# where the mediaserver should store the transcodes
PLEX_MEDIA_SERVER_TMPDIR=/var/tmp

# Change this to be what you like
PLEX_MEDIA_SERVER_APPLICATION_SUPPORT_DIR="${PLEX_MEDIA_SERVER_HOME}/Library/Application Support"

# Logs live in /opt/plexmediaserver/Library/Application Support/Plex Media Server/Logs
# Uncomment this to send to syslog-ng
#PLEX_MEDIA_SERVER_USE_SYSLOG=true
