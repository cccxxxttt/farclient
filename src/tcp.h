#ifndef __TCP_H
#define __TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>


#define TCPSIZE 65535
#define BUFSIZE 1024
#define CONCLOSE 100

typedef struct fcon {
	char srvIp[17];		// server ip
	int srvPort;		// server port
	char macName[6];
	char mac[17];

	char uhtIp[17];		// uhttpd ip
	int uhport;			// uhttpd port
}Fcontrol;

void getlocalip(void);
int getlocalmac(void);
ssize_t tcp_read(int fd, void *buf, size_t count);
int sock_client(char *ip, int port);
int response_close(char urlmsg[]);

#endif
