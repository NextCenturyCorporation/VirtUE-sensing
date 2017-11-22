#!/bin/bash

FILENAME=uname.h
VERSION=$(uname -r)
OLDIFS=$IFS
IFS='-'
SHORT_VERSION=$(echo $VERSION | awk '{print $1}')
IFS=$OLDIFS
echo $VERSION $SHORT_VERSION


if [[ $SHORT_VERSION < "4.13.1" ]] ; then
    API=old
else
    API=new
fi

echo $API

echo "/* this file is generated automatically in the makefile */" > $FILENAME
echo "const char *long_version = \"$VERSION\";" >> $FILENAME
echo "const char *short_version = \"$SHORT_VERSION\";" >> $FILENAME
echo "const char *api = \"$API\";" >> $FILENAME
