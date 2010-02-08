#! /bin/sh
# Testing the implementation of localedata.
# Copyright (C) 1998, 2000 Free Software Foundation, Inc.
# This file is part of the GNU C Library.
# Contributed by Andreas Jaeger, <aj@arthur.rhein-neckar.de>, 1998.
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

common_objpfx=$1; shift
localedef=$1; shift

test_locale ()
{
    charmap=$1
    input=$2
    out=$3
    rep=$4
    if test $rep; then
      rep="--repertoire-map $rep"
    fi
    I18NPATH=. GCONV_PATH=${common_objpfx}iconvdata \
    LOCPATH=${common_objpfx}localedata LC_ALL=C LANGUAGE=C \
    ${localedef} --quiet -c -f $charmap -i $input \
      ${rep} ${common_objpfx}localedata/$out

    if [ $? -ne 0 ]; then
	echo "Charmap: \"${charmap}\" Inputfile: \"${input}\"" \
	     "Outputdir: \"${out}\" failed"
	exit 1
    else
	echo "locale $out generated succesfully"
    fi
}

test_locale IBM437 de_DE de_DE.437
test_locale tests/test1.cm tests/test1.def test1
test_locale tests/test2.cm tests/test2.def test2
test_locale tests/test3.cm tests/test3.def test3
test_locale tests/test4.cm tests/test4.def test4
test_locale tests/test5.cm tests/test5.def test5 tests/test5.ds
test_locale tests/test6.cm tests/test6.def test6 tests/test6.ds
test_locale tests/test7.cm tests/test7.def test7

exit 0

# Local Variables:
#  mode:shell-script
# End:
