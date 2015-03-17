#include "tcp.h"

#define IPPATH		"/etc/config/serverip.conf"
#define PORTPATH	"/etc/config/serverport.conf"
#define MSGFILE		"/tmp/faclient_log"


Fcontrol fcontrol = {
	.srvIp   = "203.195.154.171",
	//.srvIp   = "192.168.0.130",
	.srvPort = 9999,

	//.uhport  = 52360,
	.uhport  = 80,
};

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

	getlocalip(fcontrol.uhIp);
	getIfaceName(fcontrol.macName);
	getlocalmac(fcontrol.mac, fcontrol.macName);
	getUhttpdPort(&fcontrol.uhport);

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
//	printf("Farclient: ip=%s, port=%d, macName=%s, mac=%s, uhip=%s, uhport=%d\n",
//		fcontrol.srvIp, fcontrol.srvPort, fcontrol.macName, fcontrol.mac,
//		fcontrol.uhIp, fcontrol.uhport);
	FILE *fp;

	sprintf(buf, "Farclient: ip=%s, port=%d, macName=%s, mac=%s, uhip=%s, uhport=%d\n",
		fcontrol.srvIp, fcontrol.srvPort, fcontrol.macName, fcontrol.mac,
		fcontrol.uhIp, fcontrol.uhport);
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
	int uhfd = -1;
	int time = 0;

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

		DEBUG_PRINT("connect ok - %d~~~~\n", srvfd);

/*########## send message first #############*/
		memset(sendmsg, 0, BUFSIZE);
		sprintf(sendmsg, "%s\r\n",
			fcontrol.mac);
		if (write(srvfd, sendmsg, sizeof(sendmsg)) <= 0) {
			close(srvfd);
			continue;
		}
/*########## end #############*/

		while(1) {
			DEBUG_PRINT("\n@@start~~~\n");
			/* read start msg, check timeout to keep tcp alive */
			time = 15*60;	// 15*60 s
			ret = pc_start_ping(srvfd, time);
			if(ret < 0)
				break;

			/* connect uhttpd */
			uhfd = uhttpd_connect(fcontrol.uhIp, fcontrol.uhport);
			if(uhfd < 0)
				break;

			/* read from server, write to route */
			ret = server_to_route(srvfd, uhfd);
			if(ret <= 0)
				break;

			/* read from route, write to server */
			ret = route_to_server(srvfd, uhfd);
			if(ret <= 0)
				break;

			close(uhfd);

			DEBUG_PRINT("@@end~~~\n");
		}

		if(uhfd > 2)
			close(uhfd);
		if(srvfd > 2){
 			// a tcp connect close, 4 shake hands in TIME_WAIT,
			// socket while use util 2 min
			close(srvfd);
		}
		//sleep(2);	// let server socket all close
	}

	return 0;
}
