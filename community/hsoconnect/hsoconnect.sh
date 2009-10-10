#!/bin/bash
if [ "$1" = "DEBUG" ]
then
    echo DEBUG mode on
    export HSOCDEBUG=on
fi

export HSOCONF=/usr/share/hsoconnect/hsoconf
python /usr/share/hsoconnect/hsoc/HSOconnect.py
