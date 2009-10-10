#
# Settings for ethernet bonding
#
# For each bond interface declared in INTERFACES (in rc.conf), declare
# a bond_${IF} variable that contains the real ethernet interfaces that
# should be bonded to the bond interface with the ifenslave utility.
# Then list the bond interface name in the BOND_INTERFACES array.
#

#bond_bond0="eth0 eth1"
#BOND_INTERFACES=(bond0)

