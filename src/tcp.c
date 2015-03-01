#include "tcp.h"


extern Fcontrol fcontrol;
char uhIp[17];

void getlocalip(void)
{
    int sfd, intr;
    struct ifreq buf[16];
    struct ifconf ifc;
	char *ip;

    sfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sfd < 0)
        return ;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (ioctl(sfd, SIOCGIFCONF, (char *)&ifc))
        return ;

    intr = ifc.ifc_len / sizeof(struct ifreq);
    while (intr-- > 0 && ioctl(sfd, SIOCGIFADDR, (char *)&buf[intr]));

    close(sfd);

    ip = inet_ntoa(((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr);
	strcpy(uhIp, ip);
}

int getlocalmac(void)
{
	struct ifreq ifreq;
	int sock;

    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0)
    {

    }

	strcpy(ifreq.ifr_name, fcontrol.macName);
	if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) {

	}

	sprintf(fcontrol.mac, "%02x:%02x:%02x:%02x:%02x:%02x",
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[0],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[1],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[2],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[3],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[4],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[5]);

	return 0;
}

ssize_t tcp_read(int fd, void *buf, size_t count)
{
	int ret, len;
	char tmp[BUFSIZE];

	len = count;
	while(len > 0) {
		memset(tmp, '\0', sizeof(tmp));

		if(len > BUFSIZE) {
			if((ret = read(fd, tmp, BUFSIZE)) < 0)
				continue;
		}
		else {
			if((ret = read(fd, tmp, len)) < 0)
				continue;
		}

		len -= ret;
		strcat(buf, tmp);
	}

	return ret;
}


int sock_client(char *ip, int port)
{
	int cfd;
	struct sockaddr_in des_addr;

	/* socket */
	cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd < 0) {
        return -1;
    }

	/* connect */
    des_addr.sin_family = AF_INET;
    des_addr.sin_port = htons(port);
    des_addr.sin_addr.s_addr = inet_addr(ip);
    bzero(&(des_addr.sin_zero), 8);
	if (connect(cfd, (struct sockaddr *)&des_addr, sizeof(struct sockaddr)) < 0) {
		return -1;
	}

	return cfd;
}


int ALARM_WAKEUP = 1;
void timeout_handler(int signo)
{
	if(signo == SIGALRM)
		if(ALARM_WAKEUP == 2)
			ALARM_WAKEUP = 0;
}

/* analyse response url data, judge connect */
int response_close(char urlmsg[])
{
	char *p = NULL, *q = NULL;
	int ret = 0, i;
	char buf[BUFSIZE];
	char timebuf[5];
	int time;

	p = strstr(urlmsg, "Connection:");
	if(p != NULL) {
		/* get line --  Connection: .....*/
		q = p;
		i = 0;
		while(*(q) != '\n') {
			buf[i++] = *q;
			q++;
		}
		buf[i] = '\0';

		p = strstr(buf, "close");
		if(p != NULL)
			ret = CONCLOSE;
	}

	/* timeout */
//	p = strstr(urlmsg, "Keep-Alive:");
//	if(p != NULL) {
//		p = strstr(p, "timeout");
//		if(p != NULL) {
//			while(*(p++) != '=');	// drop '='

//			i = 0;
//			while(*p != '\n')
//				timebuf[i++] = *(p++);
//			timebuf[i] = '\0';

//			time = atoi(timebuf);

//			/* set a alarm */
//			signal(SIGALRM, timeout_handler);
//			alarm(time);
//		}
//	}

	return ret;
}

/* change http1.1 long connect, Connection: close */
void modify_connect_close(char urlmsg[])
{
	char *p, *q;
	char tempmsg[TCPSIZE];

	strcpy(tempmsg, urlmsg);

	p = strstr(tempmsg, "Connection: Keep-Alive");		// request: keep-alive; repose: Keep-Alive
	if(p != NULL) {
		q = p;
		while(*(q++) != '\n');
	} else {
		return ;
	}

	memset(urlmsg, '\0', TCPSIZE);
	strncpy(urlmsg, tempmsg, p-tempmsg);
	strcat(urlmsg, "Connection: Close\n");
	strcat(urlmsg, q);
}
