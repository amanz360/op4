#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
	rm upload
	exit 0
}

if [ "$#" -ne 2 ]; then
    echo "Usage is: $0 <address> <port>"
    exit 1
fi

mkfifo upload
echo 'Use "cat <filename> >>upload" to send a file'
nc $1 $2 <upload &
cat >>upload
rm upload
