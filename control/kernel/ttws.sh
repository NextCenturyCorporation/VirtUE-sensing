#!/bin/bash

for FILE in $@ ; do 
    before=$(wc -m $FILE | awk '{print $1}')
    sed --in-place 's/[[:space:]]\+$//' $FILE
    after=$(wc -m $FILE | awk '{print $1}')
    echo "removed $(( before - after )) trailing spaces from ${FILE}"
done
