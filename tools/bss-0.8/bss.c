/*
 * BSS: Bluetooth Stack Smasher
 * This tool intends to perform several tests on the L2CAP layer 
 * of the bluetooth protocol.
 *
 * Pierre BETOUIN <pierre.betouin@security-labs.org>
 * 
 * You may need to install the libbluetooth (-dev) first. 
 * Debian : apt-get install libbluetooth1-dev
 *
 * Copyright (C) 2006 Pierre BETOUIN
 * 
 * Written 2006 by Pierre BETOUIN <pierre.betouin@security-labs.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE
 * FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS,
 * COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS SOFTWARE
 * IS DISCLAIMED.
*/

// includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/sdp.h>
#include <asm/byteorder.h>
#include <time.h>
#include "l2ping.h"
#include "replace.h"

// defines
#define VERSION			"0.8"
#define MAXSIZE			4096
#define IT			512
#define LENGTH			20
#define	BUFCODE			100
#define SIZE_L2HEADER		10 	// Change it there if you want :
					// Size of len in L2CAP header (mode full L2CAP header fuzzing)
#define MAX_L2CAP_FIELDS	0x1F	// Max value here for "full header fuzzing" is 0xFF but fuzz are tooooooo long with it....
#define DEFAULT_DELAY		500	// Default delay in miliseconds

// variables
int pdelay=DEFAULT_DELAY;
int dontstop=0, vdebug=0, shutnop=0, silent=0, filegen=0, filecnt=0, noping=0;

// functions defs
int banner(int mode);
int genreplay(char *payload, int savedsize);
int usage(char *);
void l2dos(char *, char *, int, int, char);
void l2fuzz(char *bdstr_addr, int maxsize, int maxcrash);
void l2fuzz_header(char *bdstr_addr, int payload_size, char pad);
char *code2define(int code);
bdaddr_t local_bdaddr;


// --------------------------------------------------------------------
// // function     : genreplay
// // role         : generates replay_l2cap_packet_XXX.c for suspected vulns
// // takes        : payload
// // returns      : nothing
// // ---------------------------------------------------------------------
int genreplay(char *payload, int savedsize)
{
	FILE  * hSrc;
	FILE  * hDst;
	struct tm *ptr;
	time_t 	tm;
	char	strSize[20];
	char    strTemp[5];
	char   	strOutFile[255];
	char    *charFileBuffer;
	char	*charOutFileBuffer;
	char	crashpackettmp[7];
	char	*crashpacket;
	long   	iNumBytes;
	int	i;

	tm  = time(NULL);
	ptr = (void *) localtime(&tm);
	strftime(strOutFile ,240 , "replay_packet/replay_l2cap_packet_%d%m%Y%H%M%S.",ptr);
	snprintf(strTemp,5,"%d",filecnt); 
	strcat(strOutFile,strTemp);
	strcat(strOutFile,".c");
	
	// expand the crash packet into C format
    	crashpacket=(char*)calloc((savedsize*4)+100,sizeof(char));
	if(crashpacket==NULL){
		fprintf(stdout,"[!] bss: failed to allocate memory\n");
		return 1;
	}

	strcat(crashpacket, "char replay_buggy_packet[]=\"");
	for(i=0 ; i<savedsize ; i++)
	{
		snprintf(crashpackettmp,6,"\\x%.2X", (unsigned char) payload[i]);
		strcat(crashpacket,crashpackettmp);
	}
	strcat(crashpacket, "\";\n");

	if (vdebug){
		fprintf(stdout,"[d] bss: payload expanded\n");
		fprintf(stdout,"%s",crashpacket);
	}

	// open template file and destination files
	hSrc = fopen("replay_packet/replay_l2cap_packet.c", "r" );
	if (hSrc == NULL) {
		fprintf(stdout,"[!] bss: failed to open source file 'replay_packet/replay_packet.c'\n");
		return 1;
	}

	hDst = fopen(strOutFile,"w");
	if (hDst == NULL) {
		fprintf(stdout,"[!] bss: failed to open destination file '%s'\n",strOutFile);
		fclose(hSrc);
		return 1;
	}

	// Get the number of bytes
	fseek(hSrc, 0L, SEEK_END);
	iNumBytes = ftell(hSrc);

	// reset file seek pointer, allocate memory
	fseek(hSrc, 0L, SEEK_SET);
	charFileBuffer = (char*)calloc(iNumBytes,sizeof(char));
	if (charFileBuffer == NULL) {
		fprintf(stdout,"[!] bss: couldn't allocate memory for replay_packet file generation\n");
		fclose(hSrc);
		fclose(hDst);
		return 1;
	}

	// read the source and close when done
	fread(charFileBuffer, sizeof(char), iNumBytes, hSrc);
	fclose(hSrc);
	
	charOutFileBuffer = replace(charFileBuffer, "XXX", crashpacket);

	sprintf(strSize,"%d",savedsize);
	charOutFileBuffer = replace(charOutFileBuffer,"YYY", strSize);
	if(vdebug) {
		fprintf(stdout,"[d] bss: modified Buffer\n");
		fprintf(stdout,"%s",charOutFileBuffer);
	}
	
	// write out, flush, close
	fprintf(hDst,"%s",charOutFileBuffer);
	fflush(hDst);
	fclose(hDst);
	filecnt++;
	
	if (vdebug) fprintf(stdout,"[d] bss: wrote %s ok\n",strOutFile);
	
	// tidy
	if (vdebug) fprintf(stdout,"[d] bss: free'ing crashpacket\n");
	free(crashpacket);
	if (vdebug) fprintf(stdout,"[d] bss: free'ing charFileBuffer\n");
	free(charFileBuffer);
	return 0;
}


// --------------------------------------------------------------------
// function	: sdpdos
// role		: perform random DoS attempts against SDP
// takes	: bluetooth address
// returns	: nothing
// --------------------------------------------------------------------
void sdpdos(char *bdstraddr){
	
	// TODO ;)


}


// --------------------------------------------------------------------
// function 	: l2dos
// role 	: perform random DoS attempts against known l2cap types
// takes	: src & dost bluetooth addresses , l2cap option number, 
// 		  size, padding byte
// returns	: nothing
// ---------------------------------------------------------------------
void l2dos(char *src_bdaddr, char *bdstr_addr, int cmdnum, int siz, char pad)
{
	char *buf;
	l2cap_cmd_hdr *cmd;		/* struct detailed in /usr/include/bluetooth/l2cap.h */
	struct sockaddr_l2 addr;
	int sock, i, id;
	char *strcode = NULL;
	int deadstop=0;
	bdaddr_t src;
	
	if (vdebug) fprintf(stdout,"[d] l2dos called with cmdnum %d\n",cmdnum);
       		strcode = code2define(cmdnum);
	
	if(strcode == NULL) {
		perror("[!] bss: L2CAP command unknown");
		exit(EXIT_FAILURE);
	} else
		fprintf(stdout, "[i] performing \"%s\" fuzzing...\n",strcode);
	
	if (vdebug) fprintf(stdout,"[d] malloc'ing buf\n");
	if(!(buf = (char *) malloc ((int) siz))) {
		perror("[!] bss: can't malloc");
		exit(EXIT_FAILURE);
	}

// used for cleaning shit up
recoverl2dos:
	if (vdebug) fprintf(stdout,"[d] bss: creating socket\n");
      	if ((sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)) < 0) {
	   	perror("[!] bss: can't create socket");
		exit(EXIT_FAILURE);
	}

        memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, &local_bdaddr);
	if (vdebug) fprintf(stdout,"[d] bss: binding socket\n");
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("[!] bss: can't bind socket");
		exit(EXIT_FAILURE);
	}
	
	
	if (vdebug) fprintf(stdout,"[d] bss: connecting\n");
	memset(&addr, 0, sizeof(addr));
        addr.l2_family = AF_BLUETOOTH;
	str2ba(bdstr_addr, &addr.l2_bdaddr);

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		if (dontstop) { 
			if (silent) fprintf(stdout,"\n");
			perror("[!] bss: can't connect.");
			if (vdebug) fprintf (stdout,"[!] bss: couldn't connect\n");
			if (vdebug) fprintf(stdout,"[d] bss: jumping to recoverl2dos\n");
			goto recoverl2dos;  
		} else {
			if (silent) fprintf(stdout,"\n");
			perror("[!] bss: can't connect.");
			exit(EXIT_FAILURE);
		}
	}

	for(i = L2CAP_CMD_HDR_SIZE; i < siz; i++)
	{
		if( pad == 0 )
			buf[i] = 0x41;		/* Default padding byte */
		else
			buf[i] = pad;
	}
	
	if (!silent) {
		// if (!vdebug) fprintf(stdout,"\n");
		fprintf(stdout, "[i] packet size = %d\n",siz);
	}

	for(i = 0; i < IT; i++){			// Send IT times the packet thru the air
		if (vdebug) fprintf(stdout,"[d] bss: next l2dos i - %d\n",i);
		
		usleep(pdelay*1000);
		cmd = (l2cap_cmd_hdr *) buf;
		cmd->code = cmdnum;
		cmd->ident = (i%250) + 1;		// Identificator 
		cmd->len = __cpu_to_le16(LENGTH);	/* trick found into tanya tool (tbear) */

		if (!vdebug) fprintf(stdout,".");
		fflush(stdout);
		
		if(send(sock, buf, siz?siz:MAXSIZE, 0) <= 0)
		{
			/* Now we check after each sending failure if the host is still responding the pings */
			//if((!silent) || (!vdebug)) fprintf(stdout,"\n");
			
			if(noping == 0)
			{
				if(!l2ping(bdstr_addr,vdebug,dontstop)){
					fprintf(stdout, "[!] bss: l2ping returned that the host is down!\n");
					if(shutnop) deadstop=1;
                                        if(vdebug) fprintf(stdout,"[d] bss: jumping to recoverl2dos\n");
                                        goto recoverl2dos;
				} else {
					if(!silent) fprintf(stdout,"[*] bss: l2ping returned the hose is up!\n");
				}
				

				if(!silent){
					fprintf(stdout, "[i] potential crash detected for %s, check l2ping response above\n",bdstr_addr);
				}


			} else {
	                        if(!silent) {
					fprintf(stdout, "[i] potential crash detected for %s, check with an inquiry scan\n",bdstr_addr);
				}
			}
			
			if(silent){
				fprintf(stdout,"!");
			} else {
				fprintf(stdout, "[i] ----------------------------------------------------\n");
				fprintf(stdout, "[i] host\t\t%s\n", bdstr_addr);
				fprintf(stdout, "[i] code field\t\t%s\n", strcode);
				fprintf(stdout, "[i] ident field\t\t%d\n", id);
				fprintf(stdout, "[i] length field\t%d\n", __cpu_to_le16(LENGTH));
				fprintf(stdout, "[i] packet size\t\t%d\n", siz);
				fprintf(stdout, "[i] ----------------------------------------------------\n");
			}

			if(deadstop){
				fprintf(stdout,"[!] bss: shutting down due to use of -x option - hosts l2cap stack in stack \n");
				exit(EXIT_SUCCESS);
			}
								
		}
		if(++id > 254)
			id = 1;
	
	}
	if (vdebug) fprintf(stdout,"[d] free'ing strcode\n");
	free(strcode);	
	if (vdebug) fprintf(stdout,"[d] free'ing buf\n");
	free(buf);
}

// --------------------------------------------------------------------
// function	: l2fuzz
// role		: sends random() packets via l2cap to device
// takes	: bluetooth address, maximum size, maximum number of crashes
// returns	: nothing
// --------------------------------------------------------------------
void l2fuzz(char *bdstr_addr, int maxsize, int maxcrash)
{
	char *buf, *savedbuf;
	struct sockaddr_l2 addr;
	int sock, i, size;
	int crash_count=0, savedsize;
	int deadstop=0;
	int sndsize=0;


	fprintf(stdout, "[i] performing full random L2CAP fuzzing, take a seat and crack open a beer...\n");
	
	if(noping == 0)
	{
		// if Debugging is turned on just check the host is actually responding to l2cap pings
		if(vdebug){
			if(!l2ping(bdstr_addr,vdebug,dontstop)){
				fprintf(stdout, "[d] [!] bss: (entry to l2fuzz) l2ping returned that the host is down!\n");
			}else{
				fprintf(stdout, "[d] [*] bss: (entry to l2fuzz) l2ping returned that the host is up!\n");
			}
		}
	}

recoverl2fuzz:
	if (vdebug) fprintf(stdout,"[d] bss: creating socket\n");
	if ((sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)) < 0) {
		perror("[!] bss: can't create socket.");
		exit(EXIT_FAILURE);
	}


        memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, &local_bdaddr);
	if (vdebug) fprintf(stdout,"[d] bss: binding socket\n");
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("[!] bss: can't bind socket");
		exit(EXIT_FAILURE);
	}

	str2ba(bdstr_addr, &addr.l2_bdaddr);
	
	if (vdebug) fprintf(stdout,"[d] bss: malloc'ing savedbuf\n");
	if(!(savedbuf = (char *) malloc ((int) maxsize + 1))) {
		perror("[!] bss: bss: can't malloc.");
		exit(EXIT_FAILURE);
	}

	while(1)		// Initite loop (ctrl-c to stop...)
	{

		usleep(pdelay*1000);
		
		memset(&addr, 0, sizeof(addr));
        	addr.l2_family = AF_BLUETOOTH;
		str2ba(bdstr_addr, &addr.l2_bdaddr);

		// bring up the connection to the device
		if (vdebug) fprintf(stdout,"[d] bss: connecting\n");
		if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
			if (dontstop) {
				if (silent) fprintf(stdout,"\n");
				perror("[!] bss: can't connect.");
				if (vdebug) fprintf(stdout,"[!] bss: couldn't connect.\n");
				if (vdebug) fprintf(stdout,"[d] bss: jumping to recover2fuzz\n");
				goto recoverl2fuzz;	
			} else {
				if (silent) fprintf(stdout,"\n");
				perror("[!] bss: can't connect.");
				exit(EXIT_FAILURE);
			}
		} else {
			if(vdebug) fprintf(stdout,"[d] bss: connected\n");
		}
			

		// build random fuzz packet
		size=rand() % maxsize;
		if(size == 0) 
			size=1;
		if (vdebug) fprintf(stdout,"[d] malloc'ing buf\n");
		if(!(buf = (char *) malloc ((int) size + 1))) {
			perror("[!] bss: couldn't malloc");
			exit(EXIT_FAILURE);
		}

		bzero(buf, size);
		for(i=0 ; i<size ; i++)	{
			buf[i] = (char) rand();
		}
		
		if( !vdebug ) 
		{
			putchar('.');
			fflush(stdout);
		}

		// do it now ! :)
		sndsize=send(sock, buf, size, 0);

		if(sndsize <= 0)
		{
			if(vdebug) fprintf(stdout,"[d] bss: send size was <= 0\n");
			// Now we check after each sending failure if the host is still responding the pings
			if((!silent) && (!vdebug)) fprintf(stdout,"\n");
			if(noping == 0)
			{
				if(!l2ping(bdstr_addr,vdebug,dontstop)){
					if(silent) fprintf(stdout,"\n");
					fprintf(stdout, "[!] bss: l2ping returned that the host is down!\n");
					if(shutnop) deadstop=1;
					crash_count++;
				}else{
					if(!silent) fprintf(stdout, "[*] bss: l2ping returned that the host is up!\n");
				}
			}
			
			
			// if it's not runnning in client mode or a bug was detected while -x was supplied
			if((!silent) || (deadstop)){
				if(noping == 0)
				{
					fprintf(stdout, "[i] bss: potential crash detected for %s, check l2ping response above\n",bdstr_addr);
				}
				else
				{
					fprintf(stdout, "[i] bss: potential crash detected for %s, check with an inquiry scan\n",bdstr_addr);
				}
			
				fprintf(stdout, "[i] ----------------------------------------------------\n");
				fprintf(stdout, "[i] host\t\t%s\n", bdstr_addr);
				fprintf(stdout, "[i] packet size\t\t%d\n", savedsize);
 				fprintf(stdout, "[i] ----------------------------------------------------\n");
				fprintf(stdout, "[i] replay buffer:\n\t");
			
				fprintf(stdout, "char replay_buggy_packet[]=\"");
				// dump the offending packet
				for(i=0 ; i<savedsize ; i++)
				{
					fprintf(stdout, "\\x%.2X", (unsigned char) savedbuf[i]);
				}
				fprintf(stdout, "\";\n");
				fprintf(stdout, "\n[i]----------------------------------------------------\n");
			} else {
				fprintf(stdout,"!");
			}

			
			// if file generation is on
			if (filegen) {
				// if told to close on crash and crash is detect OR if not told to close on crash
				if ( ((shutnop) && (deadstop)) || (!shutnop) ) {
					if (vdebug) fprintf(stdout,"[d] generating replay_packet\n");
					if (genreplay(savedbuf,savedsize) == 0){
						if (!silent) fprintf(stdout,"[i] generated replay_packet ok!\n");
						if (silent) fprintf(stdout,"G");
					} else {
						if (!silent) fprintf(stdout,"[!] error generating replay file\n");
					}
				}
			}
			
			// check we aint maxed out
			if((crash_count == maxcrash) && (maxcrash > 0))
			{
				fprintf(stdout,"[i] max crash count hit\n");

				if (vdebug) fprintf(stdout,"[d] free'ing buf\n");
				free(buf);
				if (vdebug) fprintf(stdout,"[d] free'ing savedbuf\n");
				free(savedbuf);
				exit(EXIT_SUCCESS);
			}

			// if we've been told to stop on an error
			if(deadstop){
				fprintf(stdout,"[!] shutting down due to use of -x option - hosts l2cap stack in stack\n");
				exit(EXIT_SUCCESS);
			}
			
		} else { // we sent some data
			if(vdebug) fprintf(stdout,"[d] send size was > 0\n");
			if((vdebug) && (!silent)){
				if(vdebug) fprintf(stdout,"\n");
				fprintf(stdout, "[d] debug packet dump\n");
				fprintf(stdout, "[d] ----------------------------------------------------\n");
                        	fprintf(stdout, "[d] host\t\t%s\n", bdstr_addr);
                        	fprintf(stdout, "[d] packet size\t\t%d\n", sndsize);
                        	fprintf(stdout, "[d] ----------------------------------------------------\n");
	                        for(i=0 ; i<sndsize ; i++)
	                        {       
	                                fprintf(stdout, "%.2X ", (unsigned char) buf[i]);
	                        }
	                        //fprintf(stdout, "\";\n");
	                        fprintf(stdout, "\n[d]----------------------------------------------------\n");
			}
		}
		
		memcpy(savedbuf, buf, size);	// Get the previous packet, not this one...
		savedsize = size;
		if (vdebug) fprintf(stdout,"[d] bss: free'ing buf\n");
		free(buf);
		if(savedbuf!=NULL) {
			if (vdebug) fprintf(stdout,"[d] bss: free'ing savedbuf\n");
			free(savedbuf);
		}

		// Be tidy
		if (vdebug) fprintf(stdout,"[d] bss: closing socket\n");
		close(sock); 
		if (vdebug) fprintf(stdout,"[d] bss: jumping to recoverl2fuzz\n");
		goto recoverl2fuzz;
	}
}

// --------------------------------------------------------------------
// function 	: l2fuzz_header
// role 	: perform random DoS attempts testing ALL L2CAP header fields
// takes	: bluetooth address, payload size, payload padding byte
// returns	: nothing
// ---------------------------------------------------------------------
void l2fuzz_header(char *bdstr_addr, int payload_size, char pad)
{
	char *buf, *savedbuf;
	l2cap_cmd_hdr *cmd;		/* struct detailed in /usr/include/bluetooth/l2cap.h */
	struct sockaddr_l2 addr;
	int sock;
	int code=0, id=0, len=0;
	int code_saved=0, id_saved=0, len_saved=0;
	int deadstop=0, sndsize=0;
	int i;

	if (vdebug) fprintf(stdout,"[d] bss: malloc'ing buf\n");
	if(!(buf = (char *) malloc ((int) payload_size + L2CAP_CMD_HDR_SIZE + 1))) {
		perror("[!] bss: can't malloc");
		exit(EXIT_FAILURE);
	}

	if (vdebug) fprintf(stdout,"[d] bss: malloc'ing savedbuf\n");
	if(!(savedbuf = (char *) malloc ((int) payload_size + L2CAP_CMD_HDR_SIZE + 1))) {
		perror("[!] bss: can't malloc");
		exit(EXIT_FAILURE);
	}

	for(i = L2CAP_CMD_HDR_SIZE; i < (L2CAP_CMD_HDR_SIZE+payload_size); i++)
	{
		if( pad == 0 )
		{
			pad = 0x41;
			buf[i] = 0x41;		/* Default padding byte */
		}
		else
			buf[i] = pad;
	}

	fprintf(stdout, "[i] payload size = %d\n",payload_size);
	fprintf(stdout, "[i] max len field tested = %d\n", SIZE_L2HEADER);
	fprintf(stdout, "[i] max code field tested = %d\n", MAX_L2CAP_FIELDS);
	fprintf(stdout, "[i] max id field tested = %d\n", MAX_L2CAP_FIELDS);
	fprintf(stdout, "[i] performing full L2CAP header fuzzing (%d tests)...\n", SIZE_L2HEADER*MAX_L2CAP_FIELDS*MAX_L2CAP_FIELDS);

// used for cleaning shit up
recoverl2fuzzhdr:
        if (vdebug) fprintf(stdout,"[d] bss: creating socket\n");
      	if ((sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)) < 0) {
	   	perror("[!] bss: can't create socket");
		exit(EXIT_FAILURE);
	}

        memset(&addr, 0, sizeof(addr));
	addr.l2_family = AF_BLUETOOTH;
	bacpy(&addr.l2_bdaddr, &local_bdaddr);
	if (vdebug) fprintf(stdout,"[d] bss: binding socket\n");
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("[!] bss: can't bind socket");
		exit(EXIT_FAILURE);
	}
	
	if (vdebug) fprintf(stdout,"[d] bss: connecting\n");
	memset(&addr, 0, sizeof(addr));
        addr.l2_family = AF_BLUETOOTH;
	str2ba(bdstr_addr, &addr.l2_bdaddr);

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		if (dontstop) { 
			if (silent) fprintf(stdout,"\n");
			perror("[!] bss: can't connect.");
			if (vdebug) fprintf (stdout,"[!] bss: couldn't connect . jumping to recoverl2fuzzhdr\n");
			goto recoverl2fuzzhdr;  
		} else {
			if (silent) fprintf(stdout,"\n");
			perror("[!] bss: can't connect.");
			exit(EXIT_FAILURE);
		}
	}
	
	//if(noping == 0)
	//{
	//	if(!l2ping(bdstr_addr,vdebug,dontstop)){
	//		fprintf(stdout, "[!] bss: l2ping returned that the host is down!\n");
	//		if(shutnop) deadstop=1;
	//	}else{
	//		if (!silent) fprintf(stdout, "[*] bss: l2ping returned that the host is up!\n");
	//	}
	//}

	
	for(len=0; len < SIZE_L2HEADER; len++)
	{
		if (vdebug) fprintf(stdout,"[d] bss: next l2fuzzhdr len %d\n",len);
		for(code = 0; code < MAX_L2CAP_FIELDS; code++)
		{
			if (vdebug) fprintf(stdout,"[d] bss: next l2fuzzhdr code %d\n",code);
			for(id = 0; id < MAX_L2CAP_FIELDS; id++)
			{
				if (vdebug) fprintf(stdout,"[d] bss: next l2fuzzhdr id %d\n",id);
				usleep(pdelay*1000);
				cmd = (l2cap_cmd_hdr *) buf;
				cmd->code = code;
				cmd->ident = id;		// Identificator 
				cmd->len = __cpu_to_le16(len);	/* trick found into tanya tool (tbear) */
			

				if (!vdebug) fprintf(stdout,".");
		
				if( (sndsize=send(sock, buf, L2CAP_CMD_HDR_SIZE+payload_size, 0)) <= 0)
				{
					/* Now we check after each sending failure if the host is still responding the pings */

					if (!vdebug) fprintf(stdout,"\n");
					
					if(noping == 0)
					{
						if(!l2ping(bdstr_addr,vdebug,dontstop)){
							fprintf(stdout, "[!] bss: l2ping returned that the host is down!\n");
							if(shutnop) deadstop=1;
							goto recoverl2fuzzhdr; 
						}else{
							if (!silent) fprintf(stdout, "[*] bss: l2ping returned that the host is up!\n");
						}
					}

					if(noping == 0)
					{
						if((silent) && (!vdebug)){
							fprintf(stdout,"!");
						} else {
							fprintf(stdout, "[i] bss: potential crash detected for %s, check l2ping response above\n",bdstr_addr);
					
						}
					}
					else
					{
						if((silent) && (!vdebug)){
							fprintf(stdout,"!");
						} else {
							fprintf(stdout, "[i] bss: potential crash detected for %s, check with an inquiry scan\n",bdstr_addr);
					
						}
					}

					if(!silent){
						fprintf(stdout, "[i] ----------------------------------------------------\n");
						fprintf(stdout, "[i] host\t\t%s\n", bdstr_addr);
						fprintf(stdout, "[i] code field\t\t%d\n", code_saved);
						fprintf(stdout, "[i] ident field\t\t%d\n", id_saved);
						fprintf(stdout, "[i] length field\t%d\n", __cpu_to_le16(len_saved));
						fprintf(stdout, "[i] packet size\t\t%d\n", L2CAP_CMD_HDR_SIZE+payload_size);
						fprintf(stdout, "[i] payload size\t%d\n", payload_size);
						fprintf(stdout, "[i] payload padding\t\'%c\'\n", pad);
						fprintf(stdout, "[i] ----------------------------------------------------\n");
					}

					
					// if file generation is on
					if (filegen) {
						// if told to close on crash and crash is detect OR if not told to close on crash
						if ( ((shutnop) && (deadstop)) || (!shutnop) ) {
							if (vdebug) fprintf(stdout,"[d] generating replay_packet.c\n");
							if (genreplay(savedbuf,L2CAP_CMD_HDR_SIZE+payload_size) == 0)
							{
								if (!silent) fprintf(stdout,"[d] generated ok!\n");
								if (silent) fprintf(stdout,"G");
							} 
							else 
							{
								if (!silent) fprintf(stdout,"[!] error generating replay file\n");
							}
						}
					}

					if(deadstop){
						fprintf(stdout,"[!] shutting down due to use of -x option - hosts l2cap stack in stack\n");
						exit(EXIT_SUCCESS);
					}

				}
				else
				{
					/* Save previous information */
					id_saved = id;
					code_saved = code;
					len_saved = len;
					memcpy(savedbuf, buf, L2CAP_CMD_HDR_SIZE+payload_size);

					if(vdebug) 
					{
						fprintf(stdout, "[d] debug packet dump\n");
						fprintf(stdout, "[d] ----------------------------------------------------\n");
						fprintf(stdout, "[d] host\t\t%s\n", bdstr_addr);
						fprintf(stdout, "[d] packet size\t\t%d\n", sndsize);
						fprintf(stdout, "[d] code field\t\t%d\n", code);
						fprintf(stdout, "[d] ident field\t\t%d\n", id);
						fprintf(stdout, "[d] length field\t%d\n", __cpu_to_le16(len));
						fprintf(stdout, "[d] payload padding\t\'%c\'\n", pad);
						fprintf(stdout, "[d] ----------------------------------------------------\n");
						for(i=0 ; i<sndsize ; i++)
						{       
							fprintf(stdout, "%.2X ", (unsigned char) buf[i]);
						}
						fprintf(stdout, "\n[d]----------------------------------------------------\n");
					}
				}
			}
		}
	}
	if (vdebug) fprintf(stdout,"[d] bss: free'ing buf\n");
	free(buf);
	if (vdebug) fprintf(stdout,"[d] bss: free'ing savedbuf\n");
	free(savedbuf);
}


// --------------------------------------------------------------------
// function 	: usage
// role 	: print usage information
// takes	: tool name 
// returns	: EXIT_FAILURE
// ---------------------------------------------------------------------
int usage(char *name)
{
	fprintf(stderr, "Usage: %s\t[-i iface] [-d delay] [-c] [-v] [-x] [-P0] [-q] [-o]\n", name);
	fprintf(stderr, "\t\t[-s size] [-m mode] [-p pad_byte] [-M maxcrash_count] <bdaddr>\n\n");
	fprintf(stderr, "[-i iface] - Optional output interface (format hci[X] - check 'hciconfig -a')\n");
	fprintf(stderr, "[-d delay] - Optional delay (miliseconds). Default is %dms\n", DEFAULT_DELAY);
	fprintf(stderr, "[-c]       - Continue even on errors we would normally exit on (except malloc)\n");
	fprintf(stderr, "             This overrides -x in most places\n");
	fprintf(stderr, "[-v]       - Verbose debugging\n");
	fprintf(stderr, "[-x]       - Exit on potential crashes that also don't respond to secondary l2ping's\n");
	fprintf(stderr, "[-P0]      - Do not perform L2CAP ping (some hosts don't respond to such packets\n");
	fprintf(stderr, "             This overrides -x in most places\n");
	fprintf(stderr, "[-q]       - Quiet mode - print minimal output\n");
	fprintf(stderr, "[-o]       - Generate replay_packet.c automatically\n");
	fprintf(stderr, "[-s size]  - L2CAP packet size (bytes)\n");
	fprintf(stderr, "[-M value] - Max crash count before exiting (Mode 13)\n");
	fprintf(stderr, "[-p value] - Padding value (modes 1-12)\n");
	fprintf(stderr, "[-m mode]  - Available modes:\n	\
	0  ALL MODES LISTED BELOW\n	\
	1  L2CAP_COMMAND_REJ\n	\
	2  L2CAP_CONN_REQ\n	\
	3  L2CAP_CONN_RSP\n	\
	4  L2CAP_CONF_REQ\n	\
	5  L2CAP_CONF_RSP\n	\
	6  L2CAP_DISCONN_REQ\n	\
	7  L2CAP_DISCONN_RSP\n	\
	8  L2CAP_ECHO_REQ\n	\
	9  L2CAP_ECHO_RSP\n	\
	10 L2CAP_INFO_REQ\n	\
	11 L2CAP_INFO_RSP\n	\
	12 L2CAP full header fuzzing (-s : payload size) [%d tests]\n	\
	13 L2CAP Random Fuzzing (infinite loop: break with ctrl-c)\n\n", MAX_L2CAP_FIELDS*MAX_L2CAP_FIELDS*SIZE_L2HEADER);	
	exit(EXIT_FAILURE);
}

// --------------------------------------------------------------------
// function 	: code2define
// role 	: Convert L2CAP command type from integer to ASCII string
// takes	: L2CAP command code (int)
// returns	: L2CAP type of command in an ASCII string
// ---------------------------------------------------------------------
char *code2define(int code)
{
	char *strcode= malloc(BUFCODE + 1);
	switch(code)
	{
		case L2CAP_ECHO_REQ:
			strcpy(strcode, "L2CAP echo request");	
			break;

		case L2CAP_COMMAND_REJ:
			strcpy(strcode, "L2CAP command reject");	
			break;
			
		case L2CAP_CONN_REQ:
			strcpy(strcode, "L2CAP connection request");	
			break;
			
		case L2CAP_CONN_RSP:
			strcpy(strcode, "L2CAP connexion response");	
			break;
			
		case L2CAP_CONF_REQ:
			strcpy(strcode, "L2CAP configuration request");	
			break;
			
		case L2CAP_CONF_RSP:
			strcpy(strcode, "L2CAP configuration response");	
			break;
			
		case L2CAP_DISCONN_REQ:
			strcpy(strcode, "L2CAP disconnection request");	
			break;
			
		case L2CAP_DISCONN_RSP:
			strcpy(strcode, "L2CAP disconnection response");	
			break;
			
		case L2CAP_ECHO_RSP:
			strcpy(strcode, "L2CAP echo response");	
			break;
			
		case L2CAP_INFO_REQ:
			strcpy(strcode, "L2CAP info request");	
			break;
			
		case L2CAP_INFO_RSP:
			strcpy(strcode, "L2CAP info response");	
			break;
			
		default:
			strcode=NULL;
	}
	return strcode;
}


// ---------------------------------------------------------------------
// function     : banner
// role         : prints shutup and shutdown banners
// takes        : mode
// returns      : 1 if called correctly / 0 if not
// ---------------------------------------------------------------------
int banner(int mode)
{

	if(mode==1){ // startup
		fprintf(stdout,"------------------------------------------------------------------------------\n");
		fprintf(stdout,"BSS - Bluetooth Stack Smasher - version %s\n",VERSION);
		fprintf(stdout,"------------------------------------------------------------------------------\n");
		return 1;
	} else if(mode==2){ // shutdown
		return 1;
	} else {
		fprintf(stdout,"[!] bss: someone called banner() with an invalid param\n");
		return 0;
	}
}



// --------------------------------------------------------------------
// function 	: main
// role 	: main function (as usual:)
// takes	: Arguments
// returns	: depending of the execution
// ---------------------------------------------------------------------
int main(int argc, char **argv)
{
	int i, siz = 0, mode = 0, maxcrash=1;
	char bdaddr[20], pad=0;

	if(!banner(1)){
		fprintf(stdout,"[!] Something went very wrong with banner()\n");
	}
	
	char *src_bdaddr=NULL;

	int pouet=100;

	if(getuid() != 0)
	{
		fprintf(stderr, "[!] You need to be root to launch %s (raw socket)\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	bacpy(&local_bdaddr, BDADDR_ANY);
	
	if(argc < 2 || argc > 18)
	{
		usage(argv[0]);
	}
	
	for(i = 0; i < argc; i++)
	{
		if(strchr(argv[i], ':'))
			strncpy(bdaddr, argv[i], 18);
		else
		{
		if(!memcmp(argv[i], "-i", 2) && (pouet=hci_devba(atoi(argv[++i]+3), &local_bdaddr)) < 0)
			usage(argv[0]);
					
		if(!memcmp(argv[i], "-S", 2) && (src_bdaddr = argv[++i]) < 0)
			usage(argv[0]);
			
		if(!memcmp(argv[i], "-s", 2) && (siz = atoi(argv[++i])) < 0)
			usage(argv[0]);
		
		if(!memcmp(argv[i], "-m", 2) && (mode = atoi(argv[++i])) < 0)
			usage(argv[0]);
		
		if(!memcmp(argv[i], "-p", 2) && (pad = (*argv[++i])) < 0)
			usage(argv[0]);
		
		if(!memcmp(argv[i], "-M", 2) && (maxcrash = atoi(argv[++i])) < 0)
			usage(argv[0]);

		if(!memcmp(argv[i], "-d", 2) && (pdelay = atoi(argv[++i])) < 0)
			usage(argv[0]);

		if(!memcmp(argv[i], "-c", 2) && (dontstop == 0)) {
			fprintf(stdout,"[*] Always continue mode: on\n");
			dontstop=1;
		}

		if(!memcmp(argv[i], "-v",2) && (vdebug == 0)) {
			fprintf(stdout,"[*] Debugging: on\n");
			vdebug=1;
		}
				
		if(!memcmp(argv[i], "-x",2) && (shutnop == 0)) {
		        fprintf(stdout,"[*] Exit on no response to l2ping: on\n");
		        shutnop=1;
		}

		if(!memcmp(argv[i], "-P0",3) && (noping == 0)) {
		        fprintf(stdout,"[*] Do not use L2CAP ping (some hosts don't respond to such packets): on\n");
		        noping=1;
		}

		if(!memcmp(argv[i], "-q",2) && (silent == 0)) {
		        fprintf(stdout,"[*] Silent mode: on\n");
		        silent=1;
		}
			
    	        if(!memcmp(argv[i], "-o",2) && (filegen == 0)) {
		        fprintf(stdout,"[*] Automatic replay_packet.c generation: on\n");
		        filegen=1;
		fprintf(stdout,"[*] All test cases\n");
		}
		
		}
	}

	if(mode > 13)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if(mode == 0)
	{
		/* L2CAP modes from 0 to 0x0b */
		for(i=1; i <= 0x0b; i++) 
			l2dos(src_bdaddr, bdaddr, i, siz?siz:MAXSIZE, pad);	
		l2fuzz_header(bdaddr, siz?siz:MAXSIZE, pad);
		l2fuzz(bdaddr, siz?siz:MAXSIZE, maxcrash);
	}
	else
	{
		if(mode <= 11) {
			fprintf(stdout,"[*] L2DOS mode:  on\n");
			l2dos(src_bdaddr, bdaddr, mode, siz?siz:MAXSIZE, pad);
		}

		if(mode == 12){
			fprintf(stdout,"[*] L2FUZZ header mode: on\n");
			l2fuzz_header(bdaddr, siz?siz:MAXSIZE, pad);
		}

		if(mode == 13) {
			fprintf(stdout,"[*] L2FUZZ random fuzz mode: on\n");
			l2fuzz(bdaddr, siz?siz:MAXSIZE, maxcrash);
		}
	}
	
	fprintf(stdout, "\n[*do a little dance*] Your bluetooth device didn't crash receiving the packets\n");
	
     	if(!banner(2)){
		fprintf(stdout,"[!] bss: something very wrong with banner()\n");
	}
	
	return EXIT_SUCCESS;
}
