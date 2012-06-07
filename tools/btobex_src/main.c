/*
 *
 *  Bluetooth Object Push utility
 *
 *  Copyright (C) 2003  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation;
 *
 *  Software distributed under the License is distributed on an "AS
 *  IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *  implied. See the License for the specific language governing
 *  rights and limitations under the License.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#define VERSION "0.1"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "goep.h"


static int opp_open(bdaddr_t *src, bdaddr_t *dst, uint8_t channel)
{
	uint8_t chn = channel;
	int od;

	if ((!channel) && (!sdp_get_opp_channel(src, dst, &chn)))
		return -1;

	if ((od = obex_open(src, dst, chn)) < 0)
		return -1;

	obex_connect(od, 0);

	return od;
}

static void opp_close(int od)
{
	obex_disconnect(od);

	obex_close(od);
}


static void cmd_generic_put(bdaddr_t *bdaddr, int argc, char **argv, char *type, char *file)
{
	bdaddr_t device;
	char *name, *filename;
	unsigned char *buf;
	size_t len;
	struct stat st;
	int od, fd;

	if (argc < 2)
		exit(1);

	str2ba(argv[0], &device);
	filename = argv[1];

	if (!(name = file)) {
		if (!(name = strrchr(filename, '/')))
			name = filename;
		else
			name++;
	};

	if (stat(filename, &st) < 0) {
		perror("Can't get file size");
		exit(1);
	}

	len = st.st_size;
	if (!(buf = malloc(len))) {
		perror("Can't alloc memory for file");
		exit(1);
	}

	if ((fd = open(filename, O_RDONLY)) < 0) {
		perror("Can't open input file");
		exit(1);
	}

	read(fd, buf, len);
	close(fd);

	if ((od = opp_open(bdaddr, &device, 0)) < 0) {
		perror("Can't establish OBEX connection");
		exit(1);
	}

	if (obex_put(od, type, name, buf, len) < 0)
		printf("OBEX put operation failed\n");

	opp_close(od);
}

static void cmd_generic_get(bdaddr_t *bdaddr, int argc, char **argv, char *type, char *file)
{
	bdaddr_t device;
	uint8_t channel = 0;
	unsigned char *buf;
	size_t len;
	int od;

	switch (argc) {
	case 4:
	case 3:
	case 2:
		channel = atoi(argv[1]);
	case 1:
		str2ba(argv[0], &device);
		break;
	default:
		exit(1);
	}

	if ((od = opp_open(bdaddr, &device, channel)) < 0) {
		perror("Can't establish OBEX connection");
		exit(1);
	}

	if (obex_get(od, type, file, &buf, &len) == 0) {
		printf("%s", buf);
		free(buf);
	}

	opp_close(od);
}

static void cmd_push(bdaddr_t *bdaddr, int argc, char **argv)
{
	char *name = (argc > 2) ? argv[2] : NULL;
	char *mime = (argc > 3) ? argv[3] : NULL;

	cmd_generic_put(bdaddr, argc, argv, mime, name);
}

static void cmd_pull(bdaddr_t *bdaddr, int argc, char **argv)
{
	char *name = (argc > 2) ? argv[2] : NULL;
	char *mime = (argc > 3) ? argv[3] : NULL;

	cmd_generic_get(bdaddr, argc, argv, mime, name);
}

static void cmd_devinfo(bdaddr_t *bdaddr, int argc, char **argv)
{
	cmd_generic_get(bdaddr, argc, argv, NULL, "telecom/devinfo.txt");
}

static void cmd_pbinfo(bdaddr_t *bdaddr, int argc, char **argv)
{
	cmd_generic_get(bdaddr, argc, argv, NULL, "telecom/pb/info.log");
}

static void cmd_calinfo(bdaddr_t *bdaddr, int argc, char **argv)
{
	cmd_generic_get(bdaddr, argc, argv, NULL, "telecom/cal/info.log");
}

static void cmd_getpb(bdaddr_t *bdaddr, int argc, char **argv)
{
	cmd_generic_get(bdaddr, argc, argv, NULL, "telecom/pb.vcf");
}

static void cmd_getcal(bdaddr_t *bdaddr, int argc, char **argv)
{
	cmd_generic_get(bdaddr, argc, argv, NULL, "telecom/cal.vcs");
}

static void cmd_getcap(bdaddr_t *bdaddr, int argc, char **argv)
{
	cmd_generic_get(bdaddr, argc, argv, "x-obex/capability", NULL);
}

static void cmd_getcard(bdaddr_t *bdaddr, int argc, char **argv)
{
	cmd_generic_get(bdaddr, argc, argv, "text/x-vcard", NULL);
}

struct {
	char *cmd;
	char *alt;
	void (*func)(bdaddr_t *bdaddr, int argc, char **argv);
	char *opt;
	char *doc;
} command[] = {
	{ "push",    "push",   cmd_push,    "<bdaddr> [file]   ", "Push object to Inbox"      },
	{ "pull",    "pull",   cmd_pull,    "<bdaddr> [channel]", "Pull object from Inbox"    },
	{ "devinfo", "info",   cmd_devinfo, "<bdaddr> [channel]", "Get device information"    },
	{ "pbinfo",  "pbinf",  cmd_pbinfo,  "<bdaddr> [channel]", "Get phonebook information" },
	{ "calinfo", "calinf", cmd_calinfo, "<bdaddr> [channel]", "Get calendar information"  },
	{ "getpb",   "pb",     cmd_getpb,   "<bdaddr> [channel]", "Get entire phonebook"      },
	{ "getcal",  "cal",    cmd_getcal,  "<bdaddr> [channel]", "Get entire calendar"       },
	{ "getcap",  "cap",    cmd_getcap,  "<bdaddr> [channel]", "Get capability object"     },
	{ "getcard", "card",   cmd_getcard, "<bdaddr> [channel]", "Get default vCard"         },
	{ NULL, NULL, NULL, 0, 0 }
};

static void usage(void)
{
	int i;

	printf("Bluetooth Object Push utility ver %s\n", VERSION);
	printf("Usage:\n"
		"\tbtobex [options] <command>\n"
		"\n");

	printf("Options:\n"
		"\t-i [hciX|bdaddr]   Local HCI device or BD Address\n"
		"\t-h, --help         Display help\n"
		"\n");

	printf("Commands:\n");
	for (i = 0; command[i].cmd; i++)
		printf("\t%-8s %-20s\t%s\n",
			command[i].cmd,
			command[i].opt ? command[i].opt : " ",
		command[i].doc);
	printf("\n");
}

static struct option main_options[] = {
	{ "help",	0, 0, 'h' },
	{ "device",	1, 0, 'i' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char *argv[])
{
	bdaddr_t bdaddr;
	int i, opt;

	bacpy(&bdaddr, BDADDR_ANY);

	while ((opt = getopt_long(argc, argv, "+i:h", main_options, NULL)) != -1) {
		switch(opt) {
		case 'i':
			if (!strncmp(optarg, "hci", 3) && strlen(optarg) >= 4)
				hci_devba(atoi(optarg + 3), &bdaddr);
			else
				str2ba(optarg, &bdaddr);
			break;

		case 'h':
			usage();
			exit(0);

		default:
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

	for (i = 0; command[i].cmd; i++) {
		if (strcmp(command[i].cmd, argv[0]) && strcmp(command[i].alt, argv[0]))
			continue;
		argc--;
		argv++;
		command[i].func(&bdaddr, argc, argv);
		exit(0);
	}

	usage();
	exit(1);

	return 0;
}
