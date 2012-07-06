#!/bin/sh

if [ ! -r /etc/locale.conf ]; then
  return
fi

. /etc/locale.conf

if [ "${LANG+x}" = 'x' ]; then
  export LANG
fi

if [ "${LC_CTYPE+x}" = 'x' ]; then
  export LC_CTYPE
fi

if [ "${LC_NUMERIC+x}" = 'x' ]; then
  export LC_NUMERIC
fi

if [ "${LC_TIME+x}" = 'x' ]; then
  export LC_TIME
fi

if [ "${LC_COLLATE+x}" = 'x' ]; then
  export LC_COLLATE
fi

if [ "${LC_MONETARY+x}" = 'x' ]; then
  export LC_MONETARY
fi

if [ "${LC_MESSAGES+x}" = 'x' ]; then
  export LC_MESSAGES
fi

if [ "${LC_PAPER+x}" = 'x' ]; then
  export LC_PAPER
fi

if [ "${LC_NAME+x}" = 'x' ]; then
  export LC_NAME
fi

if [ "${LC_ADDRESS+x}" = 'x' ]; then
  export LC_ADDRESS
fi

if [ "${LC_TELEPHONE+x}" = 'x' ]; then
  export LC_TELEPHONE
fi

if [ "${LC_MEASUREMENT+x}" = 'x' ]; then
  export LC_MEASUREMENT
fi

if [ "${LC_IDENTIFICATION+x}" = 'x' ]; then
  export LC_IDENTIFICATION
fi

