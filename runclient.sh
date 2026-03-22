#!/bin/bash

make
log=$(date +"%Y-%m-%d_%H-%M-%S")
ipstr=$(<addr)
./client -s ${ipstr} -p 8008 -l "logs/client_${log}.log" -f ""
errno $?