#include "tcp.h"


extern Fcontrol fcontrol;
char uhIp[17];
int bin_flag = 0;

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

int read_line(int sockfd, char buf[])
{
	char *temp=buf;
	char c;
	int ret;

	while(1)
	{
		ret = read(sockfd, &c, 1);
		if(ret <= 0)
			return ret;
		else {
			*temp++ = c;

			if(c == '\n')
				break;
		}
	}
	*temp = '\0';

	return strlen(buf);
}


ssize_t http_write(int fd, char buf[], size_t count)
{
	int ret;
	char temp[TCPSIZE];

	/* send count first */
	memset(temp, '\0', TCPSIZE);
	sprintf(temp, "UrlSize = %d\r\n", count);
	ret = write(fd, temp, strlen(temp));
	if(ret <= 0)
		return ret;

	/* send reponse */
	ret = write(fd, buf, count);
	if(ret <= 0)
		return ret;

	//printf("write-len = %d\n", strlen(buf));

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
#if 0
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
	p = strstr(urlmsg, "Keep-Alive:");
	if(p != NULL) {
		p = strstr(p, "timeout");
		if(p != NULL) {
			while(*(p++) != '=');	// drop '='

			i = 0;
			while(*p != '\n')
				timebuf[i++] = *(p++);
			timebuf[i] = '\0';

			time = atoi(timebuf);

			/* set a alarm */
			signal(SIGALRM, timeout_handler);
			alarm(time);
		}
	}

	return ret;
}
#endif


ssize_t pc_read(int fd, char buf[])
{
	int ret, i;
	unsigned long urlsize, retsize = 0;
	char msgsize[TCPSIZE], size[6];
	char temp[TCPSIZE];
	char *p;
	int firstFlag = 0;

	ret = read_line(fd, msgsize);
	if(ret <= 0)
		return ret;

	/* get urlmsg size: UrlSize = xxx\r\n */
	if(strncmp(msgsize, "UrlSize", 7) != 0)
		return -1;

	i = 0;
	p = msgsize + strlen("UrlSize = ");
	while(*p != '\r')
		size[i++] = *(p++);
	size[i] = '\0';
	urlsize = atoi(size);

	/* read url msg */
	while(urlsize > 0) {
		memset(temp, '\0', TCPSIZE);

		if(urlsize > TCPSIZE)
			ret = read(fd, temp, TCPSIZE);
		else
			ret = read(fd, temp, urlsize);

  		if(ret > 0) {
			// don't use strcat and strncpy, because sometimes jgp etc msg has '\0'
			if(ret == strlen(temp))
				strcat(buf, temp);
			else {
				int i = 0;
				while(i < ret) {
					*(buf+retsize+i) = temp[i];
					i++;
				}
			}
		}
		else if(ret <= 0)
			return ret;

		urlsize -= ret;
		retsize += ret;

#if 1
		/* debug use: get the first head */
		if(firstFlag == 0) {
			p = strstr(temp, "HTTP/1.0");
			if(p != NULL) {
				while(*(p++) != '\n');		// zhu: post data don't has  \r\n
				*p = '\0';

				printf("pc-%d:  %s", fd, temp);
			}

			firstFlag = 1;
		}
#endif
	}

	return retsize;
}

int server_to_route(int srvfd, int uhfd)
{
	int ret;
	char urlmsg[TCPSIZE];

	while(1) {
		memset(urlmsg, '\0', sizeof(urlmsg));
		if((ret = pc_read(srvfd, urlmsg)) <= 0)
			return ret;

		printf("pc-%d-%d\n", srvfd, ret);

		if(strcmp(urlmsg, "end\r\n") == 0)
			break;

		/* debug, read fireware write to cxt.bin, compile result is ok */
		#if 0
		if(bin_flag == 1) {
			FILE *fp;
			fp = fopen("/tmp/cxt.bin", "ab");
			fwrite(urlmsg, ret, 1, fp);
			fclose(fp);
		}
		#endif

		/* write to uhttpd */
		if((ret = write(uhfd, urlmsg, ret)) <= 0)
			return ret;
	}
	printf("pc-end\n");
	return ret;
}

int route_to_server(int srvfd, int uhfd)
{
	int ret;
	char urlmsg[TCPSIZE], lineBuf[BUFSIZE];
	int retsize = 0;

	/* read head msg */
	memset(urlmsg, '\0', TCPSIZE);
#if 1
		/* debug use */
		ret = read_line(uhfd, lineBuf);
		if(ret <= 0)
			return ret;

		strcat(urlmsg, lineBuf);
		retsize += ret;

		printf("route-%d:  %s", uhfd, urlmsg);
#endif

	while(1) {
		ret = read_line(uhfd, lineBuf);
		if(ret < 0)
			return -1;
		else
			strcat(urlmsg, lineBuf);

		retsize += ret;

		if(strcmp(lineBuf, "\r\n") == 0)		// head end
			break;
	}
	//printf("route-%d:  %s", uhfd, urlmsg);
	/* write back to server */
	if((ret = http_write(srvfd, urlmsg, retsize)) <= 0)
		return ret;

	/* server close mean end */
	while(1) {
		/* read form uhttpd */
		memset(urlmsg, '\0', TCPSIZE);
		ret = read(uhfd, urlmsg, TCPSIZE);

		if(ret <= 0)
			break;

		retsize += ret;

		printf("route-%d-%d\n", ret, strlen(urlmsg));
		if(ret != strlen(urlmsg))
			printf("urlmsg=%s\n", urlmsg);
		//printf("\nread urlmsg-%d=%s\n", ret, urlmsg);

		/* write back to server */
		if((ret = http_write(srvfd, urlmsg, ret)) <= 0)
			return ret;
	}

	printf("read-end-%d\n\n", retsize);

	/* send end msg */
	if((ret = http_write(srvfd, "end\r\n", strlen("end\r\n"))) <= 0)
		return ret;

	return retsize;
}
