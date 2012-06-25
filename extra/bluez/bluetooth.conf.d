# Bluetooth configuraton file

# Bluetooth services (allowed values are "true" and "false")

# Run the bluetoothd daemon (default: true)
#DAEMON_ENABLE="false"

# Run the sdp daemon (default: false)
# If this is disabled, hcid's internal sdp daemon will be used
#SDPD_ENABLE="true"

# Run the bluetooth HID daemon (default: false)
#HIDD_ENABLE="true"

# Activate rfcomm ports (default: false)
#RFCOMM_ENABLE="true"

# Run bluetooth dial-up networking daemon (default: false)
#DUND_ENABLE="true"

# Run bluetooth PAN daemon (default: false)
#PAND_ENABLE="true"

# rfcomm configuration file (default: /etc/bluetooth/rfcomm.conf)
#RFCOMM_CONFIG="/etc/bluetooth/rfcomm.conf"

# Options for hidd, dund and pand (default: none)
HIDD_OPTIONS="--server"
#DUND_OPTIONS=""
#PAND_OPTIONS=""
