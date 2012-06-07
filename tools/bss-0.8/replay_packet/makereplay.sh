#!/bin/sh

if [ -z "$1" ]
 then
  echo "[!] usage ./makereplay.sh <filename>"
  echo "[!] i.e. ./makereplay.sh replay_l2cap_packet_11022005100457.6"
  exit 1
fi

gcc -c ../l2ping.c 
gcc -c $1.c  
gcc l2ping.o $1.o -o replay -Wall -lbluetooth
