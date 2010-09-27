jj ()
{
	p=$(jobs $1);
	echo $p

	case "$p" in
	[*)	echo matches '[*'
		;;
	*)	echo not a match\?
		;;
	esac
}
