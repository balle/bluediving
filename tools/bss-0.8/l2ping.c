/* 	
 *  	taken from the Bluez Source - http://www.bluez.org/
 * 	hacked into an include by Ollie Whitehouse - ol at uncon dot org
*/ 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>

#include <asm/types.h>
#include <asm/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

	/* Defaults */
	 bdaddr_t bdaddr;
	 int size  = 20;
	 int ident = 200;
	 int delay = 1;
	 int count = 1;		// how many pings

 	/* Stats */
	 int sent_pkt = 0, recv_pkt = 0;
 

// --------------------------------------------------------------------
// function	: tv2fl
// role		:
// --------------------------------------------------------------------
 static float tv2fl(struct timeval tv)
 {
          return (float)(tv.tv_sec*1000.0) + (float)(tv.tv_usec/1000.0);
 }
 
// --------------------------------------------------------------------
// function	: stat handler
// --------------------------------------------------------------------
static void stat(int sig)
 {
          int loss = sent_pkt ? (float)((sent_pkt-recv_pkt)/(sent_pkt/100.0)) : 0;
	  fprintf(stdout,"\n");
	  fprintf(stdout,"%d sent, %d received, %d%% loss\n", sent_pkt, recv_pkt, loss);
          exit(0);
 }
 
// --------------------------------------------------------------------
// function	: l2ping
// takes	: bluetooth address, debug mode, continue on errors
// role		: perform an l2ping
// returns	: 1 if host is up, 0 if down, can cause code to exist as well
// --------------------------------------------------------------------
int l2ping(char *svr, int debug, int cont)
 {

         struct sockaddr_l2 addr;
         struct sigaction sa;
         char buf[2048];
         char str[18];
         int s, i, opt, lost;
         uint8_t id;
 
         memset(&sa, 0, sizeof(sa));

	 // calls the pretty stat handler when we exit
	 sa.sa_handler = stat;
         sigaction(SIGINT, &sa, NULL);
 
	 
	 if (debug){
         	fprintf(stdout,"\n[d] l2ping being called with address %s\n", svr);
	 }     
	 
	 if ((s = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)) < 0) {
                 	perror("[!] l2ping: Can't create socket.");
                 	exit(1);
         }
	 
	 memset(&addr, 0, sizeof(addr));
         addr.l2_family = AF_BLUETOOTH;
         addr.l2_bdaddr = bdaddr;
         if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                 perror("[!] l2ping: Can't bind socket.");
                 exit(1);
         }
 
         str2ba(svr, &addr.l2_bdaddr);
         if (connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                 perror("[!] l2ping: Can't connect.");
                 exit(1);
         }
 
         /* Get local address */
         opt = sizeof(addr);
         if (getsockname(s, (struct sockaddr *)&addr, &opt) < 0) {
                 perror("[!] l2ping: Can't get local address.");
                 exit(1);
         }
         ba2str(&addr.l2_bdaddr, str);
 
         if (debug) printf("[*] l2ping ping: %s from %s (data size %d) ...\n", svr, str, size);
 
         /* Initialize buffer */
         for (i = L2CAP_CMD_HDR_SIZE; i < sizeof(buf); i++)
                 buf[i]=(i%40)+'A';
 
         id = ident;
 
         while( count == -1 || count-- > 0 ){
                 struct timeval tv_send, tv_recv, tv_diff;
                 l2cap_cmd_hdr *cmd = (l2cap_cmd_hdr *) buf;
 
                 /* Build command header */
                 cmd->code  = L2CAP_ECHO_REQ;
                 cmd->ident = id;
                 cmd->len   = __cpu_to_le16(size);
 
                 gettimeofday(&tv_send, NULL);
 
                 /* Send Echo Request */
                 if (send(s, buf, size + L2CAP_CMD_HDR_SIZE, 0) <= 0) {
                         perror("[!] l2ping: send failed");
			 close(s);
                         exit(1);
                 }
 
                 /* Wait for Echo Response */
                 lost = 0;
                 while (1) {
                         struct pollfd pf[1];
                         register int err;
 
                         pf[0].fd = s; pf[0].events = POLLIN;
                         if ((err = poll(pf, 1, 10*1000)) < 0) {
				 if (!debug) fprintf(stdout,"\n");
                                 perror("[!] l2ping: poll failed");
				 close(s);
				 if (cont) return 1;
                                 exit(1);
                         }
 
                         if (!err) {
                                 lost = 1;
                                 break;
                         }
 
                         if ((err = recv(s, buf, sizeof(buf), 0)) < 0) {
                                 if (!debug) fprintf(stdout,"\n");
				 perror("[!] l2ping: Recv failed");
                                 if (cont) {
					 if (debug) fprintf(stderr,"[d] l2ping: continue anyway\n");
					 close(s);
					 return 1;
				 }
				 // exit(1);
				 // it is up it just reset the connection
				 close(s);
				 return 1; 
                         }
 
                         if (!err){
				 if (!debug) fprintf(stdout,"\n");
                                 printf("[!] l2ping: disconnected\n");
				 close(s);
				 if (cont) return 1;
                                 exit(1);
                         }
 
                         cmd->len = __le16_to_cpu(cmd->len);
 
                         /* Check for our id */
                         if( cmd->ident != id )
                                 continue;
 
                         /* Check type */
                         if (cmd->code == L2CAP_ECHO_RSP)
                                 break;
			 
                         if (cmd->code == L2CAP_COMMAND_REJ) {
                                 if (!debug) fprintf(stdout,"\n");
				 fprintf(stdout,"[!] l2ping: peer doesn't support Echo packets\n");
				 close(s);
                                 exit(1);
                         }
 
                 }
                 sent_pkt++;
 
		 close(s);
                 if (!lost) {
                         recv_pkt++;
 
                         gettimeofday(&tv_recv, NULL);
                         timersub(&tv_recv, &tv_send, &tv_diff);
 
                         if (debug) fprintf(stdout,"[*] l2ping: %d bytes from %s id %d time %.2fms\n", cmd->len, svr, id, tv2fl(tv_diff));
			 return 1;
                         if (delay) sleep(delay);
                 } else {
                         if (debug) fprintf(stdout,"[!] l2ping: no response from %s: id %d\n", svr, id);
			 return 0;
                 }
 
		 
                 if (++id > 254) id = ident;
         }
         //stat(0);
 }
