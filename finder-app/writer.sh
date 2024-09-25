#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

if [ $# -lt 2 ]
then
	echo "Please specify both the file dir and string to write, only get "$#" and "$1""
    exit 1
else
	FILEDIR=$1
    WRITESTR=$2
    DIR="$(dirname "$FILEDIR")"
    if [ -d "$DIR" ]; then
        echo "$WRITESTR" > "$FILEDIR"
        exit 0
    else
        mkdir -p "$DIR"
        echo "$WRITESTR" > "$FILEDIR"
	fi
fi

if [ $? != 0 ]; then
    echo "file could not be created"
    exit 1
fi
