#include "tcp.h"


Fcontrol fcontrol = {
	.srvIp   = "192.168.3.254",
	.srvPort = 9999,
	.macName = "eth1",

	.uhport  = 52360,
};

extern char uhIp[17];
extern int ALARM_WAKEUP;
int uhfd = -1;
int connect_flag = 0;

int main(void)
{
	int srvfd;
	int ret;
	char urlmsg[TCPSIZE];

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

		connect_flag = 0;
		while(1) {
			memset(urlmsg, '\0', sizeof(urlmsg));
			if((ret = read(srvfd, urlmsg, TCPSIZE)) < 0)
				continue;

			if(ret == 0) {
				close(srvfd);
				break;
			}

			printf("###################\nwrite urlmsg=%s", urlmsg);

			if(strlen(urlmsg) > 0){
				/* connect uhttpd */
				if(connect_flag == 0) {
					uhfd = sock_client(uhIp, fcontrol.uhport);
					if(uhfd < 0)
						break;

					connect_flag = 1;
				}

				/* write to uhttpd */
				if((ret = write(uhfd, urlmsg, strlen(urlmsg))) <= 0){		// send TCPSIZE
					ALARM_WAKEUP = 1;
					alarm(0);			// cannel previous alarm
					close(uhfd);
				}

				/* read form uhttpd */
				memset(urlmsg, '\0', sizeof(urlmsg));
				if((ret = read(uhfd, urlmsg, TCPSIZE)) <= 0){		// read do not all TCPSIZE
					close(uhfd);
				}

				printf("*********************\nread urlmsg=%s", urlmsg);

				/* write back to server */
				if((ret = write(srvfd, urlmsg, strlen(urlmsg))) <= 0) {
					close(srvfd);
					close(uhfd);
					break;
				}

				/* response say connect: close */
				if(response_close(urlmsg) == CONCLOSE) {
					connect_flag = 0;
					close(uhfd);
					printf("\n\n################  Connection: close  #################\n\n");
				}


			}
		}

	}

	return 0;
}
