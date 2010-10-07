if [ `uname -m` = "armv5tel" ]; then
  # if in the chroot use the cross toolchain prior to anything else
  export PATH="/usr/arm-softfloat-linux-gnueabi/bin:${PATH}"
else
  export PATH="${PATH}:/usr/arm-softfloat-linux-gnueabi/bin"
fi
