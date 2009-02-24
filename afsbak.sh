#!/bin/sh -e

#
# Written by Matthew Loar <matthew@loar.name>
# This work is hereby placed in the public domain by its author.
#

TARVOL=/usr/local/sbin/tarvol
VOS=/usr/bin/vos
VOSARGS=-localauth

TIME="0"

ERRFILE=`mktemp`
function cleanup
{
    rm $ERRFILE
}

trap cleanup EXIT

# Need the useless short option to make getopt not go into stupid mode
ARGS=`getopt -o "q" -s sh -l "newer:" -- "$@"`
if [[ $? -ne 0 ]]; then
    echo "getopt failed"
    exit 1
fi

eval set -- "$ARGS"

while [[ "$1" != "--" ]]; do
    if [[ "$1" = "--newer" ]]; then
        # GNU date is needed for the --date option
        TIME=`date "--date=$2" "+%m/%d/%Y %H:%M"`
        shift
    fi
    shift
done
shift

$VOS backup $1 $VOSARGS 2>$ERRFILE >&2
$VOS dump $1.backup -time "$TIME" $VOSARGS 2>$ERRFILE | $TARVOL -acv
egrep -v "^Dumped volume|^Created backup volume for" $ERRFILE >&2 || true
