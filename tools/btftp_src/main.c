/*
 *
 *  Bluetooth File Transfer utility
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
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "goep.h"
#include "ftp.h"


static volatile sig_atomic_t terminate = 0;

static void sig_term(int sig)
{
	terminate = 1;
}


static void cmd_quit(int od, int argc, char **argv)
{
	terminate = 1;
}

static void cmd_list(int od, int argc, char **argv)
{
	unsigned char *buf;
	size_t len;

	if (argc > 0)
		obex_setpath(od, argv[0], 0x02);

	if (obex_get(od, "x-obex/folder-listing", NULL, &buf, &len) != 0)
		buf = NULL;

	if (argc > 0)
		obex_setpath(od, NULL, 0x03);

	if (buf)
		print_folder_listing((char *) buf, len);
}

static void cmd_chdir(int od, int argc, char **argv)
{
	switch (argc) {
	case 0:
		obex_setpath(od, "", 0x02);
		break;
	case 1:
		if (!strcmp(argv[0], ".."))
			obex_setpath(od, NULL, 0x03);
		else
			obex_setpath(od, argv[0], 0x02);
		break;
	default:
		printf("Usage: cd remote-directory\n");
		return;
	}
}

static void cmd_mkdir(int od, int argc, char **argv)
{
	if ((argc != 1) || (*argv[0] == '.')) {
		printf("Usage: mkdir remote-directory\n");
		return;
	}

	if (strpbrk(argv[0], "/\\")) {
		printf("Unable to create subdiretories\n");
		return;
	}

	obex_setpath(od, argv[0], 0x00);
	obex_setpath(od, NULL, 0x01);
}

static void cmd_get(int od, int argc, char **argv)
{
	unsigned char *buf;
	size_t len;
	int fd;

	if (argc != 1) {
		printf("Usage: get remote-file\n");
		return;
	}

	if (obex_get(od, NULL, argv[0], &buf, &len) == 0) {
		//printf("%d bytes received in %d secs (%d kB/s)\n");
		printf("%d bytes received\n", len);

		fd = open(argv[0], O_WRONLY | O_CREAT);
		if (fd < 0) {
			printf("Can't open/create file %s\n", argv[0]);
			return;
		}

		write(fd, buf, len);
		close(fd);
	}
}

static void cmd_put(int od, int argc, char **argv)
{
	struct stat st;
	unsigned char *data;
	int fd;

	if (argc != 1) {
		printf("Usage: put local-file\n");
		return;
	}

	fd = open(argv[0], O_RDONLY);
	if (fd < 0) {
		printf("Can't open file %s\n", argv[0]);
		return;
	}

	if (fstat(fd, &st) < 0) {
		printf("Can't get size of file %s\n", argv[0]);
		close(fd);
		return;
	}

	data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (!data) {
		printf("Can't mmap file %s\n", argv[0]);
		close(fd);
		return;
	}

	if (obex_put(od, NULL, basename(argv[0]), data, st.st_size) == 0) {
		printf("%lu bytes sent\n", st.st_size);
	}

	munmap(data, st.st_size);
	close(fd);
}

static void cmd_del(int od, int argc, char **argv)
{
	if (argc != 1) {
		printf("Usage: del remote-file\n");
		return;
	}

	obex_put(od, NULL, argv[0], NULL, 0);
}

struct {
	char *cmd;
	char *alt;
	void (*func)(int od, int argc, char **argv);
	char *opt;
	char *doc;
} command[] = {
	{ "quit",  "exit",  cmd_quit,  0,        "Quit program"         },
	{ "ls",    "dir",   cmd_list,  "[dir]",  "Directory listing"    },
	{ "cd",    "chdir", cmd_chdir, "[dir]",  "Change directory"     },
	{ "mkdir", "",      cmd_mkdir, "<dir>",  "Create new directory" },
	{ "get",   "mget",  cmd_get,   "<file>", "Retrieve file"        },
	{ "put",   "mput",  cmd_put,   "<file>", "Send file"            },
	{ "del",   "rm",    cmd_del,   "<file>"  "Delete file"          },
	{ NULL, NULL, NULL, 0, 0 }
};


static int event_hook(void)
{
	if (terminate)
		rl_done = 1;

	return 0;
}

static char *completion_entry(const char *text, int state)
{
	char *tmp[] = { NULL };

	if (!tmp[state])
		return NULL;

	return strdup(tmp[state]);
}

static char **attempted_completion(const char *text, int start, int end)
{
	char **matches = NULL;

	if ((start > 0) && (strncmp(rl_line_buffer, "cd ", 3) == 0)) {
		matches = rl_completion_matches(text, completion_entry);
	}

	rl_attempted_completion_over = 1;

	return matches;
}


static void usage(void)
{
	printf("Bluetooth File Transfer utility ver %s\n", VERSION);
	printf("Usage:\n"
		"\tbtftp [options] <bdaddr> [channel]\n"
		"\n");

	printf("Options:\n"
		"\t-i [hciX|bdaddr]   Local HCI device or BD Address\n"
		"\t-h, --help         Display help\n"
		"\n");
}

static struct option main_options[] = {
	{ "help",	0, 0, 'h' },
	{ "device",	1, 0, 'i' },
	{ 0, 0, 0, 0 }
};

#define MAX_ARGS 20

int main(int argc, char *argv[])
{
	struct sigaction sa;
	bdaddr_t src, dst;
	uint8_t channel = 0;
	char *line, *hist, *cmd, *tmp, *args[MAX_ARGS];
	int i, opt, num, mode, od;

	bacpy(&src, BDADDR_ANY);

	while ((opt = getopt_long(argc, argv, "+i:h", main_options, NULL)) != -1) {
		switch(opt) {
		case 'i':
			if (!strncmp(optarg, "hci", 3) && strlen(optarg) >= 4)
				hci_devba(atoi(optarg + 3), &src);
			else
				str2ba(optarg, &src);
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

	switch (argc) {
	case 2:
		channel = atoi(argv[1]);
	case 1:
		str2ba(argv[0], &dst);
		break;
	default:
		usage();
		exit(0);
	}

	if ((!channel) && (!sdp_get_ftp_channel(&src, &dst, &channel))) {
		printf("Can't retrieve RFCOMM channel\n");
		exit(1);
	}

	if ((od = obex_open(&src, &dst, channel)) < 0) {
		perror("Can't establish OBEX connection");
		exit(1);
	}

	mode = OBEX_FILE_TRANSFER;
	obex_connect(od, mode);

	printf("Connected to %s.\n", argv[0]);

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);

	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);

	rl_readline_name = "btftp";
	rl_event_hook = event_hook;
	rl_attempted_completion_function = attempted_completion;

	while (!terminate) {
		if (!(line = readline("ftp> ")))
			continue;

		hist = strdup(line + strspn(line, " "));

		if (!(cmd = strtok_r(line, " ", &tmp))) {
			free(line);
			free(hist);
			continue;
		}

		add_history(hist);
		free(hist);

		for (i = 0; command[i].cmd; i++) {
			if (strcmp(command[i].cmd, cmd) && strcmp(command[i].alt, cmd))
				continue;

			for (num = 0; num < MAX_ARGS; num++) {
				switch (*tmp) {
				case '\"':
					args[num] = strtok_r(NULL, "\"", &tmp);
					break;
				case '\'':
					args[num] = strtok_r(NULL, "\'", &tmp);
					break;
				default:
					args[num] = strtok_r(NULL, " ", &tmp);
					break;
				}

				if (!args[num])
					break;
			}

			command[i].func(od, num, args);
			break;
		}

		if (!command[i].cmd)
			printf("Invalid command\n");

		free(line);
	}

	obex_disconnect(od);

	printf("Disconnected.\n");

	obex_close(od);

	return 0;
}
