#
# Parameters to be passed to distccd
#
# You must explicitly add IPs (or subnets) that are allowed to connect,
# using the --allow switch.  See the distccd manpage for more info.
#

DISTCC_ARGS="--allow 127.0.0.1"
#DISTCC_ARGS="--allow 192.168.0.0/24 --log-level error --log-file /tmp/distccd.log"
