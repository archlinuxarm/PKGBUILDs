# Parameters to be passed to the iscsid daemon.
ISCSID_ARGS=""

# iSCSI server IP which will be discovered by iscsiadm
SERVER=""

# mountpoints from fstab to mount (first entry will be mounted first, and so on)
# example: MOUNT=(/dev/sdb1 /dev/sdb2)
MOUNT=()

# wait n seconds before mounting connected nodes
SEC_BEFORE_MOUNT=2

# Extra arguments to pass to iscsiadm
ISCSIADM_EXTRAARGS=""
