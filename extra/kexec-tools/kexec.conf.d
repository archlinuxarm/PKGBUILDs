#
# Configuration for Kexec
#

# Path to kernel, default to stock arch kernel
KPATH="/boot/vmlinuz26"

# Root partition
# The default attempts to autodetect
ROOTPART="$(awk '/^[ \t]*[^#]/ { if ($2 == "/") { print $1; }}' /etc/fstab)"

# Additional kernel parameters
KPARAM="ro"

# Path to initrd image, default to stock arch kernel
INITRD="/boot/kernel26.img"
