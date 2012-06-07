/*
  RFCOMM Socket Shell
  -------------------

  Connect to a remote bluetooth device on an RFCOMM channel,
  read data from it and send data to it like using telnet
  to connect to a TCP port.

  Programmed by Bastian Ballmann [ balle@chaostal.de ]
  Last update: 09.11.2004
  BSD Port: 22.12.2005

  This code is part of the Bluechase project
  http://bluechase.chaostal.de

  Compile with gcc -lbluetooth -lreadline <source> -o <executable>
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/readline.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <bluetooth.h>

#define BUFSIZE 128
#define TIMEOUT 5

int sock;
void sig_alrm(int code);


int main(int argc, char *argv[])
{
  int wfd, rfd;
  int got = BUFSIZE;
  int got_answer = 0;
  struct sockaddr_rfcomm laddr, raddr;
  struct timeval timeout;
  fd_set writefds;
  fd_set readfds;
  char remote[BUFSIZE];
  char prompt[] = ">>> ";
  char *input;

  // Not enough arguments?!
  if(argc < 3)
    {
      printf("%s <btaddr> <channel>\n", argv[0]);
      exit(0);
    }

  // Set the ALRM signal handler
  signal(SIGALRM, sig_alrm);

  // Some information
  printf("/* Chaostal RFCOMM socket shell */\n\n");
  printf("Connecting to %s channel %s\n", argv[1], argv[2]);

  // Address the local socket
  laddr.rfcomm_family = AF_BLUETOOTH;
  bacpy(&laddr.rfcomm_bdaddr, NG_HCI_BDADDR_ANY);
  laddr.rfcomm_channel = 0;

  // Address the remote socket
  raddr.rfcomm_family = AF_BLUETOOTH;
  str2ba(argv[1],&raddr.rfcomm_bdaddr);
  raddr.rfcomm_channel = atoi(argv[2]);  

  // Set a socket timeout
  timeout.tv_sec = TIMEOUT;
  timeout.tv_usec = 0;

  // Create the socket
  if( (sock = socket(AF_BLUETOOTH, SOCK_STREAM, BLUETOOTH_PROTO_RFCOMM)) < 0)
    {
      perror("socket");
      exit(1);
    }

  // Bind it to the local address
  if(bind(sock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0)
    {
      perror("bind");
      exit(1);
    }

  // And try to connect to the remote address
  if(connect(sock, (struct sockaddr *)&raddr, sizeof(raddr)) < 0)
    {
      perror("connect");
      exit(1);
    }
  
  // O.k. We're in. Let's kick it! :)
  printf("Ok. Let's kick it :)\n\n");

  // Set the socket to non-blocking I/O
  fcntl(sock,F_SETFL,O_NONBLOCK);

  // Clear the file descriptor sets and monitor the socket for
  // ready to read or ready to write actions
  FD_ZERO(&writefds);
  FD_ZERO(&readfds);
  FD_SET(sock,&writefds);
  FD_SET(sock,&readfds);
  
  // Endless working loop
  while(1)
    {
      // What string shall we send?
      input = readline(prompt);
      
      // Get our ass outta here?
      if( (!strcmp(input,"exit")) || (!strcmp(input,"quit")) )
	{
	  printf("Disconnecting.\n");
	  FD_CLR(sock,&writefds);
	  FD_CLR(sock,&readfds);
	  close(sock);
	  exit(0);
	}
      else
	{
	  // Is the socket ready for writing?
	  if( (wfd = select(sock+1,NULL,&writefds,NULL,&timeout)) == -1)
	    {
	      perror("select");
	    }
	  else
	    {
	      // Send the string to the remote side
	      if( (write(sock, input, strlen(input))) < 0)
		{
		  perror("write");
		  exit(1);
		}
	    }
	}


      // Is the socket ready for reading?
      if( (rfd = select(sock+1,NULL,&readfds,NULL,&timeout)) == -1)
	{
	  perror("select");
	}
      else
	{
	  memset(remote, 0, BUFSIZE);
	  got_answer = 0;

	  // Read chunk by chunk
	  alarm(TIMEOUT);

	  while(got_answer == 0)
	    {
	      while((got = read(sock, remote, BUFSIZE)) > 0)
		{	
		  got_answer = 1;
		  printf("%s", remote);
		  memset(remote, '\0', BUFSIZE);
		}
	    }

	  alarm(0);
	  printf("\n");
	}
    }

  return 0;
}


// Catch the alarm signal
void sig_alrm(int code)
{
  printf("Lost connection. :(\n");
  close(sock);
  exit(0);
}
