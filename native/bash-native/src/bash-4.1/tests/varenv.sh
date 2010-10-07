#
# varenv.sh
#
# Test the behavior of the shell with respect to variable and environment
# assignments
#
expect()
{
	echo expect "$@"
}

a=1
b=2
c=3
d=4
e=5
f=6 g=7 h=8

a=3 b=4 $CHMOD $MODE $FN

# This should echo "3 4" according to Posix.2
expect "3 4"
echo $a $b

set -k

# Assignment statements made when no words are left affect the shell's
# environment
a=5 b=6 $CHMOD c=7 $MODE d=8 $FN e=9

expect "5 6 7 8 9"
echo $a $b $c $d $e

$CHMOD f=7 $MODE g=8 $FN h=9
expect "7 8 9"
echo $f $g $h

set +k

# The temporary environment does not affect variable expansion, only the
# environment given to the command

export HOME=/usr/chet
expect $HOME
echo $HOME

expect $HOME
HOME=/a/b/c /bin/echo $HOME

expect $HOME
echo $HOME

# This should echo /a/b/c
expect /a/b/c
HOME=/a/b/c printenv HOME

set -k

# This should echo $HOME 9, NOT /a/b/c 9

expect "$HOME"
HOME=/a/b/c /bin/echo $HOME c=9
expect "$HOME 7"
echo $HOME $c

# I claim the next two echo calls should give identical output.
# ksh agrees, the System V.3 sh does not

expect "/a/b/c 9 /a/b/c"
HOME=/a/b/c $ECHO a=$HOME c=9
echo $HOME $c $a

expect "/a/b/c 9 /a/b/c"
HOME=/a/b/c a=$HOME c=9
echo $HOME $c $a
set +k

# How do assignment statements affect subsequent assignments on the same
# line?
expect "/a/b/c /a/b/c"
HOME=/a/b/c a=$HOME
echo $HOME $a

# The system V.3 sh does this wrong; the last echo should output "1 1",
# but the system V.3 sh has it output "2 2".  Posix.2 says the assignment
# statements are processed left-to-right.  bash and ksh output the right
# thing
c=1
d=2
expect "1 2"
echo $c $d
d=$c c=$d
expect "1 1"
echo $c $d

# just for completeness
unset d c
expect unset
echo ${d-unset}

# no output
export a
a=bcde
export a
/bin/true 2>/dev/null

func()
{
	local YYZ

	YYZ="song by rush"
	echo $YYZ
	echo $A
}

YYZ="toronto airport"
A="AVAR"
echo $YYZ
echo $A
A=BVAR func
echo $YYZ
echo $A

export A
# Make sure expansion doesn't use assignment statements preceding a builtin
A=ZVAR echo $A

XPATH=/bin:/usr/bin:/usr/local/bin:.
func2()
{
	local z=yy
	local -a avar=( ${XPATH//: } )
	echo ${avar[@]}
	local
}

avar=42
echo $avar
func2
echo $avar

# try to set an attribute for an unset variable; make sure it persists
# when the variable is assigned a value
declare -i ivar

ivar=10

declare -p ivar
unset ivar

# export an unset variable, make sure it is not suddenly set, but make
# sure the export attribute persists when the variable is assigned a
# value
export ivar
echo ${ivar-unset}

ivar=42
declare -p ivar

# make sure set [-+]o ignoreeof and $IGNOREEOF are reflected
unset IGNOREEOF
set +o ignoreeof
set -o ignoreeof
if [ "$IGNOREEOF" -ne 10 ]; then
	echo "./varenv.sh: set -o ignoreeof is not reflected in IGNOREEOF" >&2
fi
unset IGNOREEOF
set +o ignoreeof

# older versions of bash used to not reset RANDOM in subshells correctly
[[ $RANDOM -eq $(echo $RANDOM) ]] && echo "RANDOM: problem with subshells"

# make sure that shopt -o is reflected in $SHELLOPTS
# first, get rid of things that might be set automatically via shell
# variables
set +o posix
set +o ignoreeof
set +o monitor
echo $-
echo ${SHELLOPTS}
shopt -so physical
echo $-
echo ${SHELLOPTS}

# and make sure it is readonly
readonly -p | grep SHELLOPTS

# This was an error in bash versions prior to bash-2.04.  The `set -a'
# should cause the assignment statement that's an argument to typeset
# to create an exported variable
unset FOOFOO
FOOFOO=bar
set -a
typeset FOOFOO=abcde

printenv FOOFOO

# test out export behavior of variable assignments preceding builtins and
# functions
$THIS_SH ./varenv1.sub

# more tests; bugs in bash up to version 2.05a
$THIS_SH ./varenv2.sub

# make sure variable scoping is done right
tt() { typeset a=b;echo a=$a; };a=z;echo a=$a;tt;echo a=$a
