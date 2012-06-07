/*
 * L2CAP packetgenerator
 *
 * By Bastian Ballmann <balle@chaostal.de>
 * http://www.datenterrorist.de
 * Last update: 04.02.2007
 *
 * Some code borrowed from bss - thx 2 Pierre
 *
 * License: GPLv3 
 */

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>

// Functions
void usage(void);

// MAIN PART
int main(int argc, char *argv[])
{
	l2cap_cmd_hdr *cmd;		// struct detailed in kernel_source/include/net/bluetooth/l2cap.h
        struct sockaddr_l2 laddr, raddr;
        struct hci_dev_info di;
	char *buf, *remote_address = NULL;
        char dummy_payload[] = "greets from ccc easterhegg 2007";
        char *payload = dummy_payload;
	int sock, c, i;
        int l2_code = 0x08;
        int l2_ident = 23;
        int l2_hsize = 42;

        // Get params
        while ((c = getopt (argc, argv, "a:c:i:p:s:")) != -1)
        {
         	switch (c)
                {
                	case 'a':
                             remote_address = optarg;
                             break;

                        case 'c':
                             l2_code = atoi(optarg);
                             break;

                        case 'i':
                             l2_ident = atoi(optarg);
                             break;

                        case 'p':
                             payload = (char *)optarg;                             
                             break;

                        case 's':
                             l2_hsize = atoi(optarg);
                             break;
           
                        default:
                        	usage();
                                break;
                }
        }

        if(remote_address == NULL)
        {
        	printf(">>> I need at least a remote btaddr...\n\n");
         	usage();
                exit(EXIT_FAILURE);
        }

        if(l2_hsize == 42)
        {
        	l2_hsize = strlen(payload);
        }

        // Get local device info
        if(hci_devinfo(0, &di) < 0)
        {
        	perror("HCI device info failed");
                exit(EXIT_FAILURE);
        }

        printf("Local device %s\n", batostr(&di.bdaddr));
        printf("Remote device %s\n", remote_address);
        printf("L2CAP code %d\n", l2_code);
        printf("L2CAP ident %d\n", l2_ident);
        printf("L2CAP header length %d\n", l2_hsize);

        /* Construct local addr */
        laddr.l2_family = AF_BLUETOOTH;
        laddr.l2_bdaddr = di.bdaddr;
        laddr.l2_psm = 0;

        /* Construct remote addr */
	memset(&raddr, 0, sizeof(raddr));        
	raddr.l2_family = AF_BLUETOOTH;
	str2ba(remote_address, &raddr.l2_bdaddr);

        /* Create a Bluetooth raw socket */	
	if ((sock = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_L2CAP)) < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

        /* ...and bind it to the local device */
	if (bind(sock, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

        /* Let's try to connect */
	if (connect(sock, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

        /* Init packet buffer */
	if( ! (buf = (char *) malloc (L2CAP_CMD_HDR_SIZE + strlen(payload))) ) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

        /* Set L2CAP header properties */
	cmd = (l2cap_cmd_hdr *) buf;
	cmd->code = l2_code;
	cmd->ident = l2_ident;
	cmd->len = htobs(l2_hsize);

        /* Copy payload after l2cap header */
        strncpy((buf + L2CAP_CMD_HDR_SIZE), payload, strlen(payload));

        /* Throw the packet into the air */
	if(send(sock, buf, L2CAP_CMD_HDR_SIZE + strlen(payload), 0) <= 0)
	{
        	perror("send");
	}

        printf("L2CAP packet was send\n");

        /* Disconnect */
        close(sock);

        return EXIT_SUCCESS;
}


// Print usage
void usage(void)
{
	printf("l2cap-packet -a <bdaddr> -c <l2cap_code> -i <l2cap_ident> -p <payload> -s <l2cap_headersize>\n");
        printf("L2CAP command codes:\n"); 
        printf("L2CAP_COMMAND_REJ 0x01\n");
        printf("L2CAP_CONN_REQ    0x02\n");
        printf("L2CAP_CONN_RSP    0x03\n");
        printf("L2CAP_CONF_REQ    0x04\n");
        printf("L2CAP_CONF_RSP    0x05\n");
        printf("L2CAP_DISCONN_REQ 0x06\n");
        printf("L2CAP_DISCONN_RSP 0x07\n");
        printf("L2CAP_ECHO_REQ    0x08\n");
        printf("L2CAP_ECHO_RSP    0x09\n");
        printf("L2CAP_INFO_REQ    0x0a\n");
        printf("L2CAP_INFO_RSP    0x0b\n");
        exit(EXIT_SUCCESS);
}
