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

# if this is set you can have the courierdaemon start and stop the
# authdaemon automatically
# the lag means the seconds to wait between start authdamond and courier
#AUTO_AUTHDAEMON=true
#AUTO_AUTHDAEMON_LAG=2

