#! /bin/sh
# Test lookup path optimization.
# Copyright (C) 2000 Free Software Foundation, Inc.
# This file is part of the GNU C Library.
#

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

common_objpfx=$1
run_program_prefix=$2

test -e ${common_objpfx}elf/will-be-empty &&
  rm -fr ${common_objpfx}elf/will-be-empty
test -d ${common_objpfx}elf/for-renamed ||
  mkdir ${common_objpfx}elf/for-renamed

cp ${common_objpfx}elf/pathoptobj.so ${common_objpfx}elf/for-renamed/renamed.so

LOCPATH=${common_objpfx}localedata GCONV_PATH=${common_objpfx}iconvdata \
LC_ALL=C LD_LIBRARY_PATH=${common_objpfx}elf/will-be-empty:${common_objpfx}elf/for-renamed:${common_objpfx}.:${common_objpfx}dlfcn \
  ${common_objpfx}elf/ld.so ${common_objpfx}elf/tst-pathopt \
    > ${common_objpfx}elf/tst-pathopt.out

exit $?
