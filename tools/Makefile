all: atshell attest bdaddr hcidump-crash hstest l2cap-packet l2cap_headersize_overflow rfcomm_shell

atshell: atshell.c
	echo -en "\n<<< Compiling atshell\n"
	gcc -lbluetooth -lreadline -lncurses atshell.c -o atshell

attest: attest.c
	echo -en "\n<<< Compiling attest\n"
	gcc -lbluetooth attest.c -o attest

bdaddr: bdaddr.c
	echo -en "\n<<< Compiling bdaddr\n"
	gcc -lbluetooth bdaddr.c -o bdaddr

hcidump-crash: hcidump-crash.c
	echo -en "\n<<< Compiling hcidump-crash\n"
	gcc -lbluetooth hcidump-crash.c -o hcidump-crash

hstest: hstest.c
	echo -en "\n<<< Compiling hstest\n"
	gcc -lbluetooth hstest.c -o hstest

l2cap-packet: l2cap-packet.c
	echo -en "\n<<< Compiling l2cap-packet\n"
	gcc -lbluetooth l2cap-packet.c -o l2cap-packet

l2cap_headersize_overflow: l2cap_headersize_overflow.c
	echo -en "\n<<< Compiling l2cap_headersize_overflow\n"
	gcc -lbluetooth l2cap_headersize_overflow.c -o l2cap_headersize_overflow

rfcomm_shell: rfcomm_shell.c
	echo -en "\n<<< Compiling rfcomm_shell\n"
	gcc -lbluetooth -lreadline -lncurses rfcomm_shell.c -o rfcomm_shell

clean:
	rm -f atshell attest bdaddr hcidump-crash hstest l2cap-packet l2cap_headersize_overflow rfcomm_shell


