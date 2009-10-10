

function to()
{
   if test "$2"; then
     cd "$(apparix "$1" "$2" || echo .)";
   else
     cd "$(apparix "$1" || echo .)";
   fi
   pwd
}

function bm()
{
   if test "$2"; then
      apparix --add-mark "$1" "$2";
   elif test "$1"; then
      apparix --add-mark "$1";
   else
      apparix --add-mark;
   fi
}

function portal()
{
   if test "$1"; then
      apparix --add-portal "$1";
   else
      apparix --add-portal;
   fi
}

function _apparix_aliases ()
{   cur=$2
    dir=$3
    COMPREPLY=()
    if [ "$1" == "$3" ]
    then
        COMPREPLY=($(cat $HOME/.apparix{rc,expand}|grep "j,.*$cur.*,"|cut -f2 -d,))
    else
        dir=`apparix -favour lro $dir 2>/dev/null` || return 0
        eval_compreply="
        COMPREPLY=( $(cd "$dir" ; ls -d *$cur* |
            while read r
            do
                [[ -d "$r" ]] && [[ $r == *$cur* ]] && echo \"${r// /\\ }\"
            done))"
        eval $eval_compreply
    fi
    return 0
}
complete -F _apparix_aliases to
