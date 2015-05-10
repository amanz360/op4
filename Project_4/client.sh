#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
	rm upload
	exit 0
}

mkfifo upload
echo 'Use cat "<filename> >>bin" to send a file'
nc $1 $2 <upload &
cat >>upload
rm upload
