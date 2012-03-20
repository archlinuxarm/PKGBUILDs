# the TCP port that Dropbear listens on
DROPBEAR_PORT="127.0.0.1:22" # Default to local-only.

# any additional arguments for Dropbear
DROPBEAR_EXTRA_ARGS="-w" # Default to no-root logins.

# specify an optional banner file containing a message to be
# sent to clients before they connect, such as "/etc/issue.net"
DROPBEAR_BANNER=""

# RSA hostkey file (default: /etc/dropbear/dropbear_rsa_host_key)
#DROPBEAR_RSAKEY="/etc/dropbear/dropbear_rsa_host_key"

# DSS hostkey file (default: /etc/dropbear/dropbear_dss_host_key)
DROPBEAR_DSSKEY="/etc/dropbear/dropbear_dss_host_key"
