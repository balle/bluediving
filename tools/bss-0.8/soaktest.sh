#!/bin/sh
# BSS Soak test
# Ollie Whitehouse <ol at uncon dot org>

echo ------------------------------------------------------------------------------
echo [*] BSS soak test
echo [*] soack host $1
echo ------------------------------------------------------------------------------
echo

if [ -z "$1" ]
then
   	echo "[!] usage ./soaktest.sh <host>"
     	echo "[!] i.e. ./soaktest.sh AA:BB:CC:DD:EE:FF"
       	exit 1
fi

echo [s] recompiling
make clean; make

echo
echo [s] Test 1: Mode 13 - quiet 
./bss -m 13 -M 0 -q $1 

echo
echo [s] Test 2: Mode 13 - debug 
./bss -m 13 -M 0 -v $1

echo
echo [s] Test 3: Mode 13 - normal 
./bss -m 13 -M 0 $1

echo
echo [s] Test 4: Mode 12 - quiet 
./bss -m 12 -M 0 -q $1

echo
echo [s] Test 5: Mode 12 - debug 
./bss -m 12 -M 0 -v $1

echo
echo [s] Test 5: Mode 12 - normal 
./bss -m 12 -M 0 $1

echo
echo [s] Test 6: Mode 1 - quiet 
./bss -m 1 -M 0 -q $1

echo
echo [s] Test 7: Mode 1 - debug 
./bss -m 1 -M 0 -v $1

echo
echo [s] Test 8: Mode 1 - normal 
./bss -m 1 -M 0 $1

# more complex
echo [s] Test 9: Mode 12 - quiet, file gen, exit 
./bss -m 12 -q -o -x $1

echo
echo ------------------------------------------------------------------------------
echo [*] BSS soak test finished
echo ------------------------------------------------------------------------------
