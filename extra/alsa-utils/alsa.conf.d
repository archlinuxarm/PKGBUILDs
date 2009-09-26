# Arguments for alsactl
# example: ALSA_ARGS="--file /var/run/alsa-state"
ALSA_ARGS=""

# Enables powersaving mode for AC97 and hda_intel audio chips.
# Set to 1 to enable powersaving.
# Set to 0 to disable powersaving (default).
POWERSAVE=0

# Whether to save volume levels when stopped ("yes" or "no").
SAVE_VOLUME="yes"

# Whether to mute the master volume when stopped ("yes" or "no").
# Useful for bad audio cards which make a noise on system poweroff.
MUTE_VOLUME="no"
