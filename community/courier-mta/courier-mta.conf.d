#
# Parameters to be passed to courier-imap
#
#
# Select the service you want started with courier-imap
#
# Available options :
# esmtpd imapd pop3d esmtpd-ssl imapd-ssl pop3d-ssl webmaild
#
CI_DAEMONS="courier esmtpd imapd pop3d"

# If you want authdaemond to be automatically started and
# stopped by courier-imap, set this to "true"
AUTO_AUTHDAEMON="false"

# Courier will start this many seconds after autodaemond if
# AUTO_AUTHDAEMON is set to "true"
AUTO_AUTHDAEMON_LAG=2
