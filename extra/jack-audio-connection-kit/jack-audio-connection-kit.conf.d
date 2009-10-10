# Jack Audio Connection Kit options


################################################ general server options
# output form `jackd --help`
# extend the switches in the OPTIONS variable
# usage: jackd [ --realtime OR -R [ --realtime-priority OR -P priority ] ]
#              [ --no-mlock OR -m ]
#              [ --timeout OR -t client-timeout-in-msecs ]
#              [ --port-max OR -p maximum-number-of-ports]
#              [ --verbose OR -v ]
#              [ --silent OR -s ]
#              [ --version OR -V ]
#          -d driver [ ... driver args ... ]
#              driver can be `alsa', `dummy', `oss' or `portaudio'

SERVER_PARAMS="-s -d alsa"


#################################################  options passed to the driver
# currently only options for alsa are available


# Parameters for driver 'alsa' (all parameters are optional):
#   -C, --capture   Provide only capture ports.  Optionally set device (default: none)
#   -P, --playback  Provide only playback ports.  Optionally set device (default: none)
#   -d, --device    ALSA device name (default: hw:0)
#   -r, --rate      Sample rate (default: 48000)
#   -p, --period    Frames per period (default: 1024)
#   -n, --nperiods  Number of periods in hardware buffer (default: 2)
#   -H, --hwmon     Hardware monitoring, if available (default: false)
#   -M, --hwmeter   Hardware metering, if available (default: false)
#   -D, --duplex    Provide both capture and playback ports (default: true)
#   -s, --softmode  Soft-mode, no xrun handling (default: false)
#   -m, --monitor   Provide monitor ports for the output (default: false)
#   -z, --dither    Dithering mode (default: n)
#   -i, --inchannels        Number of capture channels (defaults to hardware max) (default: 0)
#   -o, --outchannels       Number of playback channels (defaults to hardware max) (default: 0)
#   -S, --shorts    Try 16-bit samples before 32-bit (default: false)

DRIVER_PARAMS="-d hw:0 -p 1024"

