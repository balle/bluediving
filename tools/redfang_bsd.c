/* 
	RedFang - The Bluetooth Device Hunter
	Copyright (C) 2003 @stake inc

	Written 2003 by Ollie Whitehouse <ollie@atstake.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation;

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
	IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY CLAIM,
	OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
	RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
	USE OR PERFORMANCE OF THIS SOFTWARE.

	ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, COPYRIGHTS,
	TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS SOFTWARE IS DISCLAIMED.
*/

/*
 * $Id: redfang_bsd.c,v 1.1 2007/04/09 11:59:21 bytebeater Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include <termios.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <bluetooth.h>

extern int optind,opterr,optopt;
extern char *optarg;

#define for_each_opt(opt, long, short) while ((opt=getopt_long(argc, argv, short ? short:"+", long, NULL)) != -1)

static void usage(void);

static struct option hunt_options[] = {
	{"help",    0,0, 'h'},
	{0, 0, 0, 0}
};

static char *hunt_help =
	"Usage:\n"
	"\thunt <timeout>\n";

static void cmd_hunt(int dev_id, int argc, char **argv)
{
	bdaddr_t bdaddr;
	char name[248];
	
	int opt, dd, num=0, num2=0, num3=0, num4=0, num5=0, num6=0;
	int btout=50000;

	unsigned char lame[16][2] = {"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F", };

	char addtobrute[248];

	printf("redfang - the bluetooth hunter ver 1.00.alpha\n");
	printf("(c)2003 \@stake Inc\n");
	printf("author: Ollie Whitehouse (ollie@atstake.com)\n");

	argc -= optind;
        argv += optind;

	if (argc < 2) {
		printf(hunt_help);
		exit(1);
	}

	if (argc >= 1) {
		btout=atoi(argv[1]);
	}

	printf("timeout: %d\n", btout);

	printf("starting...\n");

	while (num <= 15)
	{	
		while(num2 <= 15)
		{
			while(num3 <= 15)
			{
				while(num4 <= 15)
				{
					while(num5 <= 15)
					{
						while(num6 <= 15)
						{
							strcpy(addtobrute,"00:80:98:");
							strcat(addtobrute,lame[num]);
							strcat(addtobrute,lame[num2]);
							strcat(addtobrute,":");
							strcat(addtobrute,lame[num3]);
							strcat(addtobrute,lame[num4]);
							strcat(addtobrute,":");
							strcat(addtobrute,lame[num5]);
							strcat(addtobrute,lame[num6]);		
						
							/* debug purposes */	
							printf("%s\n",addtobrute);

							baswap(&bdaddr, strtoba(addtobrute));
											
							dev_id = hci_get_route(&bdaddr);
							if (dev_id < 0) {
								fprintf(stderr,"Device not availible");	
								exit(1);
							}
							

							
							dd = hci_open_dev(dev_id);
							if (dd < 0) {
								fprintf(stderr,"HCI device open failed");
								exit(1);
							}
							
							
							/* try to get name of remote device - timeout is the int) */
							if (hci_read_remote_name(dd,&bdaddr,sizeof(name), name, btout) == 0)
								printf("\n.start--->\naddress :- %s\nname    :- %s\n<.end-----\n",batostr(&bdaddr),name);
							
							close(dd);

							num6++;
							}
							num6=0;
							num5++;

						}
						num5=0;
						num4++;
					}
					num4=0;
					num3++;
			}
			num3=0;
			num2++;
		}
		num2=0;
		num++;
	}
}

struct {
	char *cmd;
	void (*func)(int dev_id, int argc, char **argv);
	char *doc;
} command[] = {
	{ "hunt", cmd_hunt, "Get name from remote device"        },
	{ NULL, NULL, 0}
};

static void usage(void)
{
	int i;

	printf("redfang - the bluetooth hunter ver 1.00.alpha\n");
	printf("usage:\n"
		"\tfang [options] <command> [command parameters]\n");
	printf("options:\n"
		"\t--help\tDisplay help\n"
		"\t-i dev\tHCI device\n");
	printf("commands:\n");
	for (i=0; command[i].cmd; i++)
		printf("\t%-4s\t%s\n", command[i].cmd,
		command[i].doc);
	printf("\n"
		"For more information on the usage of each command use:\n"
		"\tfang <command> --help\n" );
}

static struct option main_options[] = {
	{"help",        0,0, 'h'},
	{"device",      1,0, 'i'},
	{0, 0, 0, 0}
};

int main(int argc, char **argv)
{
	int opt, i, dev_id = -1;
	bdaddr_t ba;

	while ((opt=getopt_long(argc, argv, "+i:h", main_options, NULL)) != -1) {
		switch(opt) {
		case 'i':
			dev_id = hci_devid(optarg);
			if (dev_id < 0) {
				perror("Invalid device");
				exit(1);
			}
			break;

		case 'h':
		default:
			usage();
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;
	optind = 0;

	if (argc < 1) {
		usage();
		exit(0);
	}

	if (dev_id != -1 && hci_devba(dev_id, &ba) < 0) {
		perror("Device is not available");
		exit(1);
	}

	for (i=0; command[i].cmd; i++) {
		if (strncmp(command[i].cmd, argv[0], 3))
			continue;
		command[i].func(dev_id, argc, argv);
		break;
	}
	return 0;
}
