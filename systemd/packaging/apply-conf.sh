#!/bin/bash

SRC=$1
TAR=$2

ORIGIFS=$IFS
IFS=
for line in `cat $SRC`; do
        CHANGE=$(echo $line | awk -F= '$1 ~ /^([a-zA-Z0-9 ])/ { print $1"=" }')
done

IFS=$ORIGIFS
for line in `echo $CHANGE`; do
        sed -i s/"$(grep $line $TAR)"/"$(grep $line $SRC)"/g $TAR
done

