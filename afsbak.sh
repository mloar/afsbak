#!/bin/sh -e

VOS=/usr/bin/vos
TIME="0"
VOSARGS=-localauth
TARVOL=/usr/local/sbin/afsbak

# Need the useless short option to make getopt not go into stupid mode
ARGS=`getopt -o "q" -s sh -l "newer:" -- "$@"`
if [[ $? -ne 0 ]]; then
    echo "getopt failed"
    exit 1
fi

eval set -- "$ARGS"

while [[ "$1" != "--" ]]; do
    if [[ "$1" = "--newer" ]]; then
        TIME=`date "--date=$2" "+%m/%d/%Y %H:%M"`
        shift
    fi
    shift
done
shift

$VOS backup $1 $VOSARGS >&2
$VOS dump $1.backup -time "$TIME" -omitdirs $VOSARGS | $TARVOL -cv
