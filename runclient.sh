#!/bin/bash

make
log=$(date +"%Y-%m-%d_%H-%M-%S")
./client -s 0.0.0.0 -p 8008 -l "logs/client_${log}.log" -f ""
errno $?