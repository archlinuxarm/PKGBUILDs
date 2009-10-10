#
# Settings for layer-2 bridges
#
# For each bridge interface declared in INTERFACES (in rc.conf), declare
# a bridge_${IF} variable that contains the real ethernet interfaces that
# should be bridged togeether.
#
# Then list the bridge interface name in the BRIDGE_INTERFACES array.
#

# example:
#
# in /etc/rc.conf:
#    eth0="eth0 up"
#    eth1="eth1 up"
#    br0="br0 192.168.0.2 netmask 255.255.255.0 up"
#    INTERFACES=(lo eth0 eth1 br0)
#
# in /etc/conf.d/bridges
#    bridge_br0="eth0 eth1"
#    BRIDGE_INTERFACES=(br0)
#


#bridge_br0="eth0 eth1"
#BRIDGE_INTERFACES=(br0)

