/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2001-2005  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation;
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
 *  CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
 *  COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
 *  SOFTWARE IS DISCLAIMED.
 *
 *
 *  $Id: attest_bsd.c,v 1.2 2005/12/31 23:19:59 bytebeater Exp $
 *
/
/*
 * BSD Port
 * Programmed by Bastian Ballmann
 * http://www.datenterrorist.de
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth.h>

static int at_command(int fd, char *cmd, int to)
{
	fd_set rfds;
	struct timeval timeout;
	char buf[1024];
	int sel, len, i, n;

	write(fd, cmd, strlen(cmd));

	for (i = 0; i < 100; i++) {

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		timeout.tv_sec = 0;
		timeout.tv_usec = to;

		if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0) {

			if (FD_ISSET(fd, &rfds)) {
				memset(buf, 0, sizeof(buf));
				len = read(fd, buf, sizeof(buf));
				for (n = 0; n < len; n++)
					printf("%c", buf[n]);
				if (strstr(buf, "\r\nOK") != NULL)
					break;
				if (strstr(buf, "\r\nERROR") != NULL)
					break;
				if (strstr(buf, "\r\nCONNECT") != NULL)
					break;
			}

		}

	}

	return 0;
}

static int open_device(char *device)
{
	struct termios ti;
	int fd;

	fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "Can't open serial port: %s (%d)\n",
							strerror(errno), errno);
		return -1;
	}

	tcflush(fd, TCIOFLUSH);

	/* Switch tty to RAW mode */
	cfmakeraw(&ti);
	tcsetattr(fd, TCSANOW, &ti);

	return fd;
}

static int open_socket(bdaddr_t *bdaddr, uint8_t channel)
{
	struct sockaddr_rfcomm addr;
	int sk;

	sk = socket(PF_BLUETOOTH, SOCK_STREAM, BLUETOOTH_PROTO_RFCOMM);
	if (sk < 0) {
		fprintf(stderr, "Can't create socket: %s (%d)\n",
							strerror(errno), errno);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.rfcomm_family = AF_BLUETOOTH;
	bacpy(&addr.rfcomm_bdaddr, NG_HCI_BDADDR_ANY);

	if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Can't bind socket: %s (%d)\n",
							strerror(errno), errno);
		close(sk);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.rfcomm_family = AF_BLUETOOTH;
	bacpy(&addr.rfcomm_bdaddr, bdaddr);
	addr.rfcomm_channel = channel;

	if (connect(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Can't connect: %s (%d)\n",
							strerror(errno), errno);
		close(sk);
		return -1;
	}

	return sk;
}

static void usage(void)
{
	printf("Usage:\n\tattest <device> | <bdaddr> [channel]\n");
}

int main(int argc, char *argv[])
{
	int fd;

	bdaddr_t bdaddr;
	uint8_t channel;

	switch (argc) {
	case 2:
	  str2ba(argv[1], &bdaddr);
	  channel = 17;
	  break;
	case 3:
	  str2ba(argv[1], &bdaddr);
	  channel = atoi(argv[2]);
	  break;
	default:
	  usage();
	  exit(-1);
	}

	if (bacmp(NG_HCI_BDADDR_ANY, &bdaddr)) {
		printf("Connecting to %s on channel %d\n", argv[1], channel);
		fd = open_socket(&bdaddr, channel);
	} else {
		printf("Opening device %s\n", argv[1]);
		fd = open_device(argv[1]);
	}

	if (fd < 0)
		exit(-2);

	at_command(fd, "ATZ\r\n", 10000);
	at_command(fd, "AT+CPBS=\"ME\"\r\n", 10000);
	at_command(fd, "AT+CPBR=1,100\r\n", 100000);
	at_command(fd, "AT+CMGL\r\n", 10000);
	at_command(fd, "AT+CMGR=1,100\r\n", 10000);
	at_command(fd, "ATI1\r\n", 10000);
	at_command(fd, "ATI2\r\n", 10000);
	at_command(fd, "ATI3\r\n", 10000);
	at_command(fd, "ATI4\r\n", 10000);
	// at_command(fd, "AT&V\r\n", 10000);
	// at_command(fd, "ATST0=0\r\n", 10000);

	close(fd);
	return 0;
}
