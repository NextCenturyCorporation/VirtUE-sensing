#!/bin/bash

for FILE in $@ ; do
    if [[ $FILE = *".o" ]] ; then
	objdump -w -S $FILE > $FILE.asm
    fi
done
