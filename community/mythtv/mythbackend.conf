#!/bin/bash
#
# Rename this file to:
#
#    /etc/config.d/mythbackend
#
###############################################################################
#
# Copyright (c) by the MythTV Development Team.
#
# Derived from work by:
#
#     Michael Thomson <linux at m-thomson dot net>
#     Stu Tomlinson <stu at nosnilmot dot com>
#     Axel Thimm <axel.thimm at atrpms dot net>
# Adopted for ArchLinux:
#     Jürgen Hoetzel <juergen@archinux.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
###############################################################################
#
# Config variables for the mythbackend startup script, which is usually
# located in /etc/rc.d/mythbackend
#
# When the startup script is executed, it sources this file if it exists,
# otherwise it will fall back on default values.
#
# Leave variables commented out to use default values in init script
# (/etc/rc.d/mythbackend).
#
# To override defaults, uncomment the relevant variable definition and
# edit as required.
#

#
# User who should start the mythbackend processes
#
# Running mythbackend as non-root requires you to ensure that audio/video
# devices used for recording have suitable user permissions. One way
# to achieve this is to modify existing or create new udev rules which
# assign these devices to a non-root group with rw permissions and add
# your mythbackend user to that group. Be aware that console.perms can 
# also affect device permissions and may need additional configuration.
# Running as non-root may also introduce increased process latency.
#
# MBE_USER='root'

#
# Directory holding the mythbackend binary (empty means autodetect).
#
# MBE_DIR=''

#
# Name of mythbackend binary.
#
# MBE_PROG='mythbackend'

#
# Other startup options for mythbackend (see 'mythbackend --help' for a list).
#
# MBE_OPTIONS=''

#
# Directory holding the mythbackend log file
#
# LOG_DIR='/var/log/mythtv'

#
# Name of mythbackend log file.
#
# NOTE: If you are running as non-root take care to ensure the mythbackend user
# has permission to write to this log file.
#
# LOG_FILE='mythbackend.log'

#
# Logging options for mythbackend (see 'mythbackend -v help' for a list)
#
# LOG_OPTS=''
