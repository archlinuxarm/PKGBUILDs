#!/bin/sh

#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# 	Dockstar/Pogoplug u-Boot Installer v0.3
# 	by Jeff Doozan
#
#   Based on Pogoplug u-Booter Installer v0.2
#   by IanJB, original method and inspiration from aholler:
# 			http://ahsoftware.de/dockstar/
#
# 	This is a script to write a newer u-boot to Pogoplug/DockStar mtd3
#   and configure the u-boot on mtd0 to pass control to our new u-boot


UBOOT_URL=http://jeff.doozan.com/debian/uboot/uboot.mtd3.bin
UBOOT_ENV_URL=http://jeff.doozan.com/debian/uboot/uboot.environment
BLPARAM_URL=http://jeff.doozan.com/debian/uboot/blparam
NANDDUMP_URL=http://jeff.doozan.com/debian/uboot/nanddump
FLASH_ERASE_URL=http://jeff.doozan.com/debian/uboot/flash_erase
FW_PRINTENV_URL=http://jeff.doozan.com/debian/uboot/fw_printenv
FW_CONFIG_URL=http://jeff.doozan.com/debian/uboot/fw_env.config

UBOOT=/tmp/uboot.mtd3.bin
UBOOT_ENV=/tmp/uboot.environment
BLPARAM=/usr/local/cloudengines/bin/blparam
NANDDUMP=/usr/bin/nanddump
FLASH_ERASE=/usr/bin/flash_erase
FW_PRINTENV=/usr/bin/fw_printenv
FW_SETENV=/usr/bin/fw_setenv
FW_CONFIG=/etc/fw_env.config


function verify_md5 {
  local file=$1
  local md5=$2

  if [[ $(cat "$md5" | cut -d' ' -f1) == $(md5sum "$file" | cut -d' ' -f1) ]]; then
    echo 1;
  else
    echo 0;
  fi
}

function download_and_verify
{
  local file_dest=$1
  local file_url=$2

  local md5_dest="$file_dest.md5"
  local md5_url="$file_url.md5"

  # Always download a fresh MD5, in clase a newer version is available
  if [ -e "$md5_dest" ]; then rm -f "$md5_dest"; fi
  wget -O "$md5_dest" "$md5_url"
  # retry the download if it failed
  if [ ! -e "$md5_dest" ]; then
    wget -O "$md5_dest" "$md5_url"
    if [ ! -e "$md5_dest" ]; then
      echo 0 # Could not get md5
      return
    fi
  fi

  # If the file already exists, check the MD5
  if [ -e "$file_dest" ]; then
    if [[ $( verify_md5 "$file_dest" "$md5_dest" ) == 1 ]]; then
      rm -f "$md5_dest"
      echo 1
      return
    else
      rm -f "$file_dest"
    fi
 fi

  # Download the file
  wget -O "$file_dest" "$file_url"
  # retry the download if it failed
  if [[ $( verify_md5 "$file_dest" "$md5_dest" ) == 0 ]]; then

    # Download failed or MD5 did not match, try again
    if [ -e "$file_dest" ]; then rm -f "$file_dest"; fi
    wget -O "$file_dest" "$file_url"
    if [[ $( verify_md5 "$file_dest" "$md5_dest" ) == 0 ]]; then
      rm -f "$md5_dest"
      echo 0
      return
    fi
  fi

  rm -f "$md5_dest"
  echo 1
}


if [[ "$1" != "--noprompt" ]]; then

  echo ""
  echo ""
  echo ""
  echo ""
  echo "This script will install an alternate uBoot on /dev/mtd3."
  echo "It will also modify the boot parameters of the original uBoot"
  echo "to pass control to the alternate uBoot."
  echo ""
  echo "Your new uBoot will attempt to boot from the first USB device"
  echo "and, failing that, will pass control back to the original"
  echo "uBoot to start the default Pogoplug installation"
  echo ""
  echo "This script will DESTROY ALL EXISTING DATA on /dev/mtd3"
  echo "(which is fine -- it's an empty partition, unless you've"
  echo "knowingly put something there)"
  echo ""
  echo -n "If this is ok, type 'ok' and press ENTER to continue: "

  read IS_OK
  if [ "$IS_OK" != "OK" -a "$IS_OK" != "Ok" -a "$IS_OK" != "ok" ];
  then
    echo "Exiting..."
    exit
  fi

fi

# Install blparam if it doesn't already exist
if [ ! -e "$BLPARAM" ]; then
  echo "# Installing blparam..."
  mount -o rw,remount /

  if [[ $( download_and_verify "$BLPARAM" "$BLPARAM_URL" ) == 0 ]]; then
    mount -o ro,remount /
    echo "## Could not install blparam, exiting."
    exit 1
  fi

  chmod +x "$BLPARAM"
  mount -o ro,remount /

  echo "# Successfully installed blparam."
fi

# Install nanddump if it doesn't already exist
if [ ! -e "$NANDDUMP" ]; then
  echo ""
  echo "# Installing nanddump..."
  mount -o rw,remount /

  if [[ $( download_and_verify "$NANDDUMP" "$NANDDUMP_URL" ) == 0 ]]; then
    mount -o ro,remount /
    echo "## Could not install nanddump, exiting."
    exit 1
  fi

  chmod +x "$NANDDUMP"
  mount -o ro,remount /

  echo "# Successfully installed nanddump."
fi

# Install flash_erase if it doesn't already exist
if [ ! -e "$FLASH_ERASE" ]; then
  echo ""
  echo "# Installing flash_erase..."
  mount -o rw,remount /

  if [[ $( download_and_verify "$FLASH_ERASE" "$FLASH_ERASE_URL" ) == 0 ]]; then
    mount -o ro,remount /
    echo "## Could not install flash_erase, exiting."
    exit 1
  fi

  chmod +x "$FLASH_ERASE"
  mount -o ro,remount /

  echo "# Successfully installed flash_erase."
fi
                                  
# Install fw_env tools if they doesn't already exist
if [ ! -e "$FW_PRINTENV" ]; then
  echo ""
  echo "# Installing fw_printenv and fw_setenv utils..."
  mount -o rw,remount /

  if [[ $( download_and_verify "$FW_PRINTENV" "$FW_PRINTENV_URL" ) == 0 ]]; then
    mount -o ro,remount /
    echo "## Could not install fw_printenv and fw_setenv, exiting."
    exit 1
  fi

  chmod +x "$FW_PRINTENV"
  ln -s "$FW_PRINTENV" "$FW_SETENV"
  mount -o ro,remount /

  echo "## Successfully installed fw_printenv and fw_setenv."
fi

# Install fw_env.config if it doesn't already exist
if [ ! -e "$FW_CONFIG" ]; then
  echo ""
  echo "# Installing fw_env.config..."
  mount -o rw,remount /

  if [[ $( download_and_verify "$FW_CONFIG" "$FW_CONFIG_URL" ) == 0 ]]; then
    mount -o ro,remount /
    echo "## Could not install fw_env.config, exiting."
    exit 1
  fi
  mount -o ro,remount /

  echo "## Successfully installed fw_env.config"
fi




# Attempt to auto-detect the device by looking at the existing bootcmd parameters
#
# If the bootcmd parameters matches a known default string, we assume the device is clean
#
# If the bootcmd paramater does not match a known string, but instead has a bootcmd_original
# string that matches a known default, then we assume the device is modified and check
# for the existance of our bootloader
#
# If neither the bootcmd nor the bootcmd_original strings match a known default, we ask the user
# to select his device

echo ""
echo -n "# Attempting to auto-detect your device..."

dockstar='nand read.e 0x800000 0x100000 0x300000; setenv bootargs $(console) $(bootargs_root); bootm 0x800000'
pogoplug1='nand read 0x2000000 0x100000 0x200000; setenv bootargs $(console) $(bootargs_root); bootm 0x2000000'
pogoplug2='nand read.e 0x800000 0x100000 0x200000; setenv bootargs $(console) $(bootargs_root); bootm 0x800000'

bootcmd=`$BLPARAM | grep "^bootcmd=" | cut -d'=' -f 2-`
bootcmd_original=`$BLPARAM | grep "^bootcmd_original=" | cut -d'=' -f 2-`

if   [ "$bootcmd" = "$dockstar" ]; then
  echo "Dockstar detected"
  bootcmd_original=$bootcmd
elif [ "$bootcmd" = "$plugbox1" ]; then
  echo "Pogoplug v1 detected"
  bootcmd_original=$bootcmd
elif [ "$bootcmd" = "$plugbox2" ]; then  
  echo "Pogoplug v2 detected"
  bootcmd_original=$bootcmd

elif [ "$bootcmd_original" = "$dockstar" ]; then
  echo "Dockstar with modified bootcmd detected"
elif [ "$bootcmd_original" = "$plugbox1" ]; then
  echo "Pogoplug v1 with modified bootcmd detected"
elif [ "$bootcmd_original" = "$plugbox2" ]; then  
  echo "Pogoplug v2 with modified bootcmd detected"


# Auto detect failed, ask the user what device he has
else
  echo "failed!"

  bootcmd_original=

  while [ "$bootcmd_original" == "" ]; do
    echo ""
    echo "############################################"
    echo "Your device could not be auto-detected."
    echo ""
    echo "What device are you using? Type the number of your device and press ENTER."
    echo "1 - Pogoplug v1 - Brick"
    echo "2 - Pogoplug v2 - Pink"
    echo "3 - DockStar"
    read device

    if [ "$device" == "1" ]; then
      echo "Selected Pogoplug v1"
      bootcmd_original=$pogoplug1
    elif [ "$device" == "2" ]; then
      echo "Selected Pogoplug v2"
      bootcmd_original=$pogoplug2
    elif [ "$device" == "3" ]; then
      echo "Selected Dockstar"
      bootcmd_original=$dockstar
    else
      echo "Invalid Input"
    fi
  done
  
fi


UPDATE_UBOOT=1

# If this is not a clean device, check to see if our bootloader is already installed

if [ "$bootcmd" != "$bootcmd_original" ]; then
  echo ""
  echo "# Checking for existing installation of uBoot on mtd3..."
  wget -O "$UBOOT.md5" "$UBOOT_URL.md5"

  # dump the area of mtd3 where an existing uboot would be to /tmp/mtd3.uboot
  $NANDDUMP -no -l 0x80000 -f /tmp/mtd3.uboot /dev/mtd3
  
  if [[ $( verify_md5 "/tmp/mtd3.uboot" "$UBOOT.md5" ) == 1 ]]; then
    echo "## The newest uBoot is already installed on mtd3."
    UPDATE_UBOOT=0
  else
    echo "## Latest uBoot not found on mtd3."
  fi  
  
fi


# uBoot is not yet installed to mtd3,
# download the new uBoot and install it

if [[ $UPDATE_UBOOT == 1 ]]; then

  echo ""
  echo "# Installing uBoot"

  if [[ $( download_and_verify "$UBOOT" "$UBOOT_URL" ) == 0 ]]; then
    echo "## uBoot could not be downloaded, or the MD5 does not match."
    echo "## Exiting. No changes were made to mtd3."
    exit 1
  fi
  
  # Write new uBoot to mtd3
  flash_eraseall /dev/mtd3
  nandwrite /dev/mtd3 $UBOOT

  # dump mtd3 and compare the checksum, to make sure it installed properly  
  $NANDDUMP -no -l 0x80000 -f /tmp/mtd3.uboot /dev/mtd3
  echo "## Verifying new uBoot..."
  wget -O "$UBOOT.md5" "$UBOOT_URL.md5"
  
  if [[ $( verify_md5 "/tmp/mtd3.uboot" "$UBOOT.md5" ) == 0 ]]; then
    rm -f "$UBOOT.md5"
    echo "## VERIFICATION FAILED!"
    echo "## uBoot was not properly installed to mtd3.  Please re-run this installer."
    exit 1
  else
    echo "# Verified successfully!"
  fi
  rm -f "$UBOOT.md5"
  
fi


echo ""
echo "# Installing uBoot environment"

# Install the uBoot environment
if [[ $( download_and_verify "$UBOOT_ENV" "$UBOOT_ENV_URL" ) == 0 ]]; then
  echo "## Could not install uBoot environment, exiting"
  exit 1
fi
$FLASH_ERASE /dev/mtd0 0xc0000 1
nandwrite -s 786432 /dev/mtd0 "$UBOOT_ENV"

echo ""
echo "# Verifying uBoot environment"

# Verify the uBoot environment
$NANDDUMP -of "/tmp/uboot.environment" -s 0xc0000 -l 0x20000 /dev/mtd0
wget -O "$UBOOT_ENV.md5" "$UBOOT_ENV_URL.md5"
if [[ $( verify_md5 "/tmp/uboot.environment" "$UBOOT_ENV.md5" ) == 0 ]]; then
  rm -f "$UBOOT_ENV.md5"
  echo "## VERIFICATION FAILED!"
  echo "## uBoot environment was not properly written to mtd0.  Please re-run this installer."
  exit 1
fi
rm -f "$UBOOT_ENV.md5"


# Preserve the MAC address 
ETHADDR=`$BLPARAM | grep "^ethaddr=" | cut -d'=' -f 2-`

$FW_SETENV ethaddr $ETHADDR
$FW_SETENV bootargs_root root=/dev/mtdblock2 ro
$FW_SETENV bootcmd_original $bootcmd_original

# Configure the mtd0 uBoot parameters

$BLPARAM 'bootcmd_usb=nand read.e 0xc00000 0x2500000 0x80000; go 0xc00000' > /dev/null 2>&1
$BLPARAM 'bootcmd1=setenv bootcmd run bootcmd2; saveenv; run bootcmd_usb' > /dev/null 2>&1
$BLPARAM 'bootcmd2=setenv bootcmd run bootcmd1; saveenv; run bootcmd_original' > /dev/null 2>&1
$BLPARAM "bootcmd_original=$bootcmd_original" > /dev/null 2>&1
$BLPARAM 'bootcmd=run bootcmd1' > /dev/null 2>&1


echo ""
echo "# uBoot installation has completed successfully."

