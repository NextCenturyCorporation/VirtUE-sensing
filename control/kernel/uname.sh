#!/bin/bash

FILENAME=uname.h
VERSION=$(uname -r)
OLDIFS=$IFS
IFS='-'
SHORT_VERSION=$(echo $VERSION | awk '{print $1}')
IFS=$OLDIFS
echo $VERSION $SHORT_VERSION


echo "/* this file is generated automatically in the makefile */" > $FILENAME
echo "const char *cont_long_version = \"$VERSION\";" >> $FILENAME
echo "const char *cont_short_version = \"$SHORT_VERSION\";" >> $FILENAME

if [[ $SHORT_VERSION < "4.13.1" ]] ; then
    API=old
    echo "#define OLD_API = 1;" >> $FILENAME
else
    API=new
    echo "#define NEW_API = 1;" >> $FILENAME
fi
echo "const char *cont_api = \"$API\";" >> $FILENAME
echo $API
