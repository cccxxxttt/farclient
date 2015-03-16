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
#include <errno.h>
#include <sys/wait.h>


#define TCPSIZE 65535
#define BUFSIZE 1024
#define CONCLOSE 100

#define DEBUG
#ifdef DEBUG
	#define DEBUG_PRINT(format, ...)	printf(format, ##__VA_ARGS__)
#else
	#define DEBUG_PRINT(format, ...)
#endif

typedef struct fcon {
	char srvIp[16];		// server ip
	int srvPort;		// server port
	char macName[6];
	char mac[18];

	char uhIp[16];		// uhttpd ip
	int uhport;			// uhttpd port
}Fcontrol;

int getlocalip();
int getIfaceName(char *iface_name);
int getlocalmac(char *iface_mac, char *iface_name);
int getUhttpdPort(int *uhttpd_port);
ssize_t http_read(int fd, char buf[], size_t count);
ssize_t http_write(int fd, char buf[], size_t count);
int sock_client(char *ip, int port);
int uhttpd_connect(char *ip, int port);
int response_close(char urlmsg[]);
void modify_connect_close(char urlmsg[]);
void modify_http_head(char urlmsg[]);

ssize_t pc_read(int fd, char buf[]);
int server_to_route(int srvfd, int uhfd);
int route_to_server(int srvfd, int uhfd);

#endif
