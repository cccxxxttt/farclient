#include "tcp.h"


Fcontrol fcontrol = {
	//.srvIp   = "192.168.3.254",
	.srvIp   = "192.168.0.130",
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
	char urlmsg[TCPSIZE];
	int uhfd = -1;

	getlocalip();

	/* while(1) : reconnection */
	while(1)
	{
		/* create socket */
		srvfd = sock_client(fcontrol.srvIp, fcontrol.srvPort);
	    if (srvfd < 0) {
	        sleep(10);		// 10s connect again
	        continue;
	    }

		printf("connect ok~~~~\n");

/*########## send message first #############*/
		getlocalmac();

		char sendmsg[BUFSIZE];
		memset(sendmsg, 0, BUFSIZE);
		sprintf(sendmsg, "%s\r\n",
			fcontrol.mac);
		if (write(srvfd, sendmsg, sizeof(sendmsg)) < 0) {
			close(srvfd);
			continue;
		}
/*########## end #############*/

		while(1) {
			memset(urlmsg, '\0', sizeof(urlmsg));
			if((ret = read(srvfd, urlmsg, TCPSIZE)) < 0)
				continue;

			if(ret == 0) {
				close(srvfd);
				break;
			}

			printf("\npc-%d\n", srvfd);
			//printf("###################\nwrite urlmsg=%s", urlmsg);

			if(strlen(urlmsg) > 0){
				/* connect uhttpd */
				uhfd = sock_client(uhIp, fcontrol.uhport);
				if(uhfd < 0)
					break;

				/* write to uhttpd */
				if((ret = write(uhfd, urlmsg, strlen(urlmsg))) <= 0){		// send TCPSIZE
					close(uhfd);
				}

				/* read form uhttpd */
				memset(urlmsg, '\0', sizeof(urlmsg));
				if((ret = http_read(uhfd, urlmsg, TCPSIZE)) <= 0){		// read do not all TCPSIZE
					close(uhfd);
				}

				//modify_connect_close(urlmsg);
				//modify_http_head(urlmsg);

				//printf("read-len = %d\n", ret);
				//printf("*********************\nread urlmsg-%d=%s", uhfd, urlmsg);

				/* write back to server */
				if((ret = http_write(srvfd, urlmsg, strlen(urlmsg))) <= 0) {
					close(srvfd);
					close(uhfd);
					break;
				}

				close(uhfd);
			}
		}
	}

	return 0;
}
