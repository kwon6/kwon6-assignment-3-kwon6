#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

if [ $# -lt 2 ]
then
	echo "Please specify both the file dir and search string"
    exit 1
else
	FILESDIR="$1"
    if [ -d "$FILESDIR" ]; then
        echo "Path valid"
    else
        echo "Path invalid"
        exit 1
	fi
    SEARCHSTR="$2"
fi

file_count=$(find "$FILESDIR" -type f | wc -l)
match_count=$(grep -rl "$SEARCHSTR" "$FILESDIR" | wc -l)

echo "The number of files are ${file_count} and the number of matching lines are ${match_count}"
