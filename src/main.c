#include "tcp.h"


Fcontrol fcontrol = {
	.srvIp   = "203.195.154.171",
	//.srvIp   = "192.168.0.130",
	.srvPort = 9999,
	.macName = "eth1",

	//.uhport  = 52360,
	.uhport  = 80,
};

extern char uhIp[17];
extern int ALARM_WAKEUP;

int main(void)
{
	int srvfd;
	int ret;
	char sendmsg[BUFSIZE];
	int uhfd = -1;

	getlocalip();

	/* while(1) : reconnection */
	while(1)
	{
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
			/* connect uhttpd */
			uhfd = sock_client(uhIp, fcontrol.uhport);
			if(uhfd < 0)
				continue;

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
