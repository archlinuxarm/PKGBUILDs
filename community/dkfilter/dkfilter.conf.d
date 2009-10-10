#
# UID and GID for dkfilter
#
DKFILTER_USER=dkfilter
DKFILTER_GROUP=dkfilter

#
# Private key
#
DKFILTER_PRIVATE_KEY=/etc/dkfilter/private.key

#
# Hostname (Who verified incoming email)
#
DKFILTER_HOSTNAME=`hostname -f`

#
# Domain (sign outbound email for this domain)
#
DKFILTER_DOMAIN=`hostname -d`

#
# Selector (you may choose random name)
#
DKFILTER_SELECTOR=server1
