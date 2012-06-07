/*
 *  dump HID keyboard sca codes
 *
 *  (c) Collin R. Mulliner <collin@betaversion.net>
 *  http://www.mulliner.org/bluetooth/
 *
 *  License: GPLv2 
 *
 */

#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// for event values see: /usr/include/linux/input.h

int letter2event(unsigned char l, struct input_event *e)
{
	switch (l) {
	case '0':
		e->code = KEY_0;
		break;
	case 'w':
		e->code = KEY_W;
		break;	
	case 'n':
		e->code = KEY_N;
		break;	
	case 'd':
		e->code = KEY_D;
		break;	
	}
	e->value = 1; // 1=press, 0=release, 2=auto repeat
	e->type = EV_KEY;

	return(1);
}

void main()
{
	const char ATTACK[] = {"0wnd"};
				
	int o;
	int i;
	struct input_event e;
	
	o = open("ha.inp", O_CREAT|O_RDWR);

	for (i = 0; i < strlen(ATTACK); i++) {
		bzero((unsigned char*)&e, sizeof(e));	
		letter2event(ATTACK[i], &e);
		write(o, &e, sizeof(e));
	}

	close(o);
}
