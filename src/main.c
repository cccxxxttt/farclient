#include "tcp.h"

#define IPPATH		"/etc/config/serverip.conf"
#define PORTPATH	"/etc/config/serverport.conf"
#define MSGFILE		"/tmp/faclient_log"


Fcontrol fcontrol = {
	.srvIp   = "203.195.154.171",
	//.srvIp   = "192.168.0.130",
	.srvPort = 9999,
	.macName = "eth1",

	//.uhport  = 52360,
	.uhport  = 80,
};

extern char uhIp[16];
extern int ALARM_WAKEUP;
extern int config_read(char buf[], char *path);
extern int main_farclient(void);

int main(void)
{
	pid_t pid;

	while(1) {
		/* program auto reboot */
		pid = fork();

		if(pid > 0)
			pid = wait(NULL);
		else if(pid == 0) {
			main_farclient();
		}
	}

	return 0;
}

int config_read(char buf[], char *path)
{
	FILE *fp;

	fp = fopen(path, "r");
	if(fp == NULL)
		return -1;

	fread(buf, 1024, 1, fp);
	fclose(fp);

	/* read one line end of '\n' */
	if(buf[strlen(buf) -1] == '\n')
		buf[strlen(buf) -1] = '\0';

	return strlen(buf);
}

void get_config()
{
	int ret;
	char buf[1024];

	/* get server ip and port */
	memset(buf, '\0', 1024);
	ret = config_read(buf, IPPATH);
	if(ret > 0) {
		strcpy(fcontrol.srvIp, buf);
	}

	memset(buf, '\0', 1024);
	ret = config_read(buf, PORTPATH);
	if(ret > 0) {
		fcontrol.srvPort = atoi(buf);
	}

#if 1
	//printf("Farclient: ip=%s, port=%d\n", fcontrol.srvIp, fcontrol.srvPort);
	FILE *fp;

	sprintf(buf, "Farclient: ip=%s, port=%d\n", fcontrol.srvIp, fcontrol.srvPort);
	fp = fopen(MSGFILE, "w");
	if(fp != NULL) {
		fwrite(buf, 1024, 1, fp);
		fclose(fp);
	}
#endif
}

int main_farclient(void)
{
	int srvfd;
	int ret;
	char sendmsg[BUFSIZE];
	char urlmsg[TCPSIZE];
	int uhfd = -1;

	getlocalip();

	/* while(1) : reconnection */
	while(1)
	{
		get_config();

		/* create socket */
		srvfd = sock_client(fcontrol.srvIp, fcontrol.srvPort);
	    if (srvfd < 0) {
			DEBUG_PRINT("connect to server failed!!! wait 10s ...\n");
	        sleep(10);		// 10s connect again
	        continue;
	    }

		DEBUG_PRINT("connect ok~~~~\n");

/*########## send message first #############*/
		getlocalmac();

		memset(sendmsg, 0, BUFSIZE);
		sprintf(sendmsg, "%s\r\n",
			fcontrol.mac);
		if (write(srvfd, sendmsg, sizeof(sendmsg)) <= 0) {
			close(srvfd);
			continue;
		}
/*########## end #############*/

		while(1) {
			/* read pc first, afraid uhttpd connect timeout close */
			memset(urlmsg, '\0', sizeof(urlmsg));
			if((ret = pc_read(srvfd, urlmsg)) <= 0)
				return ret;

			DEBUG_PRINT("pc-%d-%d-%d\n", srvfd, ret, strlen(urlmsg));

			/* connect uhttpd */
			uhfd = sock_client(uhIp, fcontrol.uhport);
			if(uhfd < 0)
				continue;

			/* write to uhttpd */
			if((ret = write(uhfd, urlmsg, ret)) <= 0)
				return ret;

			/* read from server, write to route */
			ret = server_to_route(srvfd, uhfd);
			if(ret <= 0)
				break;

			/* read from route, write to server */
			ret = route_to_server(srvfd, uhfd);
			if(ret <= 0)
				break;

			close(uhfd);
		}
	}

	return 0;
}
