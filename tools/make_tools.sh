#!/bin/bash

make clean
rm -f bccmd btftp btobex bss carwhisper greenplague

make

echo -en "\n<<< Compiling bccmd\n"
cd bccmd_src
make
mv bccmd ..
cd ..

echo -en "\n<<< Compiling btftp\n"
cd btftp_src
make
mv btftp ..
cd ..

echo -en "\n<<< Compiling btobex\n"
cd btobex_src
make
mv btobex ..
cd ..

echo -en "\n<<< Compiling bss\n"
cd bss-0.8
make
mv bss ..
cd ..

echo -en "\n<<< Compiling carwhisperer\n"
cd carwhisperer-0.2
make
mv carwhisperer ..
cd ..

echo -en "\n<<< Compiling greenplaque\n"
cd greenplaque_src/
make
mv src/greenplaque ..
cd ..

echo -en "\n<<< Compiling hidattack\n"
cd  hidattack01
make
mv hidattack ..
cd ..

echo -en "\n<<< Compiling redfang\n"
tar xfvz redfang.tar.gz
mv redfang redfang_src
cd redfang_src
gcc -lbluetooth fang.c -o fang
cp fang ../redfang
cd ..
