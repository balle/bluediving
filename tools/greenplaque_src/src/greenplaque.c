/*
 *  GreenPlaque - The Bluetooth multi-dongle Discovery scanner
 *  Yellow and Blue make Green!
 *  Pretty much written by Ollie Whitehouse <ollie@atstake.com> in 2003 ala Redfang
 *  Modded 2005 by KF <kf@digitalmunition.com>
 *
 *  version 1.4f
 *
 * patched by bastian ballmann <balle@chaostal.de>
 * - removed logging for use in bluediving
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <termios.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <netinet/in.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <time.h>
#include "list.h"

#define LOGTXTFILE      2

static void usage(void)
{
   	printf("usage:\ngreenplaque [options]\n\n");
   	printf("options:\n"
        "   -t\ttimeout    The connect timeout, this is 10000 by default\n"
	"     \t           Which is quick and yields results, increase for\n"
	"     \t           reliability\n"
        "   -n\tnum        The number of dongles\n"
        "   -l\tnum        Use loop? 1 = yes, 0 = no\n"
        "   -h              Display help\n\n");
}


typedef struct 
{
	int btout;
   	int threads;
   	int loop;
} 
options;
options opts;

char tmpmanf[2000];
char *findManf (char *address)
{
   int i;

   for (i=0; manf[i].addr != NULL; i++)
   {
      if (strncmp(manf[i].addr, address, 8))
      {
        continue;
      }
      snprintf(tmpmanf,sizeof(tmpmanf), "%s",manf[i].desc);
      return(tmpmanf);
   }
   return("non listed device");
}

static void hciDiscovery(int dev_id) 
{
	int length, flags, dd, i;
	bdaddr_t bdaddr;
	char name[248];
	inquiry_info *info = NULL;
	int num_rsp = 100;

	dd = hci_open_dev(dev_id);
        if (dd < 0) 
	{
	   	fprintf(stderr, "HCI device open failed");
		free(info);
	   	exit(1);
	}

	int ctl;
        if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) 
	{
                perror("Can't open HCI socket.");
        }
        struct hci_dev_req dr;
        dr.dev_id  = dev_id;
        dr.dev_opt = SCAN_DISABLED;
	// Stop the dongles from discovering each other!
        ioctl(ctl, HCISETSCAN, (unsigned long) &dr);

        /* Reset HCI device with a stop[ and start*/
	printf("resetting hci device. \n");
        ioctl(ctl, HCIDEVDOWN, dev_id);
        ioctl(ctl, HCIDEVUP, dev_id);

	close(ctl);

	// Need to field test small values. 
	length  = 4;  /* multiply by 1.2 seconds */

	flags = IREQ_CACHE_FLUSH;
	printf("sending inquiry\n");
	num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);
	
	if(num_rsp > 0)
	{
  		printf("hci%d saw %d responses\n", dev_id, num_rsp);
	}

	// we need to come up with a device class lookup function
	// this is just incase we drive or walk by too quick to get a devname... this should be logged somewhere. 
	char addr[18];
        for (i = 0; i < num_rsp; i++) 
	{
                ba2str(&(info+i)->bdaddr, addr);
                printf("%s\tclass: 0x%2.2x%2.2x%2.2x\n",addr,
                (info+i)->dev_class[2],
                (info+i)->dev_class[1],
                (info+i)->dev_class[0]);
        }

	// this may need to be done differently. 
        for(i = 0; i < num_rsp; i++) 
        {
	   	memset(name, 0, sizeof(name));
	   	if (hci_read_remote_name(dd, &(info+i)->bdaddr, sizeof(name), name, opts.btout) == 0) 
           	{
	      		baswap(&bdaddr, &(info+i)->bdaddr);  
       			printf("hci%d Discovered: %s [%s] - %s\n", dev_id, name, batostr(&bdaddr), findManf(batostr(&bdaddr)));
			sdptool(name,findManf(batostr(&bdaddr)),batostr(&bdaddr),dev_id);
	   	}
	}
	// where does the sdptool code go? 

      	hci_close_dev(dd);
	free(info);
}
static void hciDiscoverThread(void **args)
{
   	int *dev_id = (int *)args[0];
   	hciDiscovery(*dev_id);
}

int main(int argc, char **argv)
{
   	int opt, i;
   	static int total = 0;
   	extern int optind,opterr,optopt;
   	extern char *optarg;
   	opts.btout = 18000; // We originally used 25000, hcitool used 10000
   	opts.threads = 1;
   
   	printf("greenplaque - the bluetooth hunter killer\n");

   	while ((opt=getopt(argc, argv, "h:g:n:t:l:")) != -1) 
	{
      		switch(opt) 
		{
         		case 't':
            			opts.btout = atoi(optarg);
            			break;
         		case 'n':
	    			opts.threads = atoi(optarg);
            			break;
         		case 'l':
	    			opts.loop = atoi(optarg);
	    			break;
         		case 'h':
         			default:
            			usage();
            			exit(0);
      		}
   	}
	begin:
   	if (opts.threads > 1) 
	{
      		int count;
      		int ret;
      		int threads = opts.threads;
      		pthread_t tid[opts.threads];

      		printf("Using %d threads \n", threads);
      
      		for(count = 0; count < threads; count++) 
		{
	 		void **args = malloc(5*sizeof(void *));
	 		char hcidev[8];

         		args[0] = malloc(sizeof(int));
         		args[1] = malloc(sizeof(int));

	 		memcpy(args[0], &count, sizeof(int));
	 		memcpy(args[1], &total, sizeof(int));

	 		ret  = pthread_create(&tid[count], NULL, (void *)&(hciDiscoverThread), args);
	 		if (ret != 0) 
			{
            			printf("Failed to create thread %d\n", count+1);
	    			perror("pthread_create");
	    			exit(-1);
	 		}
	 		else
	 		{
   	    			total=total+1;
   	    			printf("Started[dev %d][total %d]\n", count, total);
	 		}
      		}

      		for (count = 0; count < threads; count++) 	
		{
         		ret = pthread_join(tid[count], NULL);
      		}
   	} 
  	else 
	{
		printf("Sorry no single device discovery, run with -h for help\n");
		printf("Try ./hciemu DE:AD:00:00:BE:EF and run with 2 threads\n");
		exit(0);
   	}

   	if (opts.loop >0) 
	{
   		printf("looping...\n");
		goto begin;
   	}
}

int sdptool(char *name, char *descrip,char *bluetoothaddress, int *dev)
{
        FILE *f;

	// this code needs to be cleaned up or depreciated
	char sdpcmd[512];
        char tmpdev[20];
        char tmpfile[512];

        memset(tmpfile, 0, sizeof(tmpfile));
	snprintf(tmpdev,sizeof(tmpdev)-1, "%d",dev);

        strncat(tmpfile,bluetoothaddress,sizeof(tmpfile)-1);
        strncat(tmpfile,"_hci",sizeof(tmpfile)-1);
        strncat(tmpfile,tmpdev,sizeof(tmpfile)-1);
        strncat(tmpfile,"-",sizeof(tmpfile)-1);
	snprintf(tmpdev, sizeof(tmpdev)-1,"%hx",time(NULL)); // helps a bit incase of multiple discoveries of same device
        strncat(tmpfile,tmpdev,sizeof(tmpfile)-1);
        strncat(tmpfile,".txt",sizeof(tmpfile)-1);

	f = fopen(tmpfile, "w");
	int handle;
	handle=dup(fileno(stdout));
	dup2(fileno(f),fileno(stdout));

        // for some reason the system() code worked better than the popen() ?
        memset(sdpcmd, 0, sizeof(sdpcmd));
        strncat(sdpcmd,"echo '", sizeof(sdpcmd)-1);
        strncat(sdpcmd,name, sizeof(sdpcmd)-1);
        strncat(sdpcmd," ", sizeof(sdpcmd)-1);
        strncat(sdpcmd,descrip, sizeof(sdpcmd)-1);
        strncat(sdpcmd,"';", sizeof(sdpcmd)-1); // lazy ass hack to put device name in output logs
        strncat(sdpcmd,"sdptool browse '", sizeof(sdpcmd)-1);
        strncat(sdpcmd,bluetoothaddress, sizeof(sdpcmd)-1);
        strncat(sdpcmd,"' &", sizeof(sdpcmd)-1); // this is to help with stdout being nabbed at the wrong time. 

	// need clean input before this point and before our own insertion of ';' and '&'
        system(sdpcmd);

	dup2(handle,fileno(stdout));
	// sometimes this was too slow... but not recently... 
	fflush(stdout);
	fflush(f);
	fclose(f);

}
