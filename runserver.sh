#!/bin/bash

make
log=$(date +"%Y-%m-%d_%H-%M-%S")
./server -p 8008 -s "logs/server_${log}.log"
errno $?