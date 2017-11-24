#!/bin/bash

FILENAME=uname.h
FULL_VERSION=$(uname -r)
OLDIFS=$IFS
IFS='-'
SHORT_VERSION=$(echo $FULL_VERSION | awk '{print $1}')

echo $FULL_VERSION $SHORT_VERSION

IFS='.'
VERSION=$(echo $SHORT_VERSION | awk '{print $1}')
PATCHLEVEL=$(echo $SHORT_VERSION | awk '{print $2}')
SUBLEVEL=$(echo $SHORT_VERSION | awk '{print $3}')

IFS=$OLDIFS

echo "#ifndef _UNAME_CONTROLLER_H" > $FILENAME
echo "#define _UNAME_CONTROLLER_H" >> $FILENAME
echo "/* this file is generated automatically in the makefile */" >> $FILENAME
echo "const char *cont_long_version = \"$FULL_VERSION\";" >> $FILENAME
echo "const char *cont_short_version = \"$SHORT_VERSION\";" >> $FILENAME
echo "const int version = $VERSION;" >> $FILENAME
echo "const int patchlevel = $PATCHLEVEL;" >> $FILENAME
echo "const int sublevel = $SUBLEVEL;" >> $FILENAME


if (( $VERSION >= 4 )) ; then
    if (( $PATCHLEVEL < 9 )); then
	API=old
	echo "#define OLD_API 1;" >> $FILENAME
    else
	API=new
	echo "#define NEW_API 1;" >> $FILENAME
    fi
fi

echo "const char *cont_api = \"$API\";" >> $FILENAME
echo "#endif /* _UNAME_CONTROLLER_H */" >> $FILENAME

echo $API
