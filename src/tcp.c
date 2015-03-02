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

int read_line(int sockfd, char buf[])
{
	char *temp=buf;
	char c;
	int n;

	while(1)
	{
		n=read(sockfd,&c,1);
		if(n<0)
			return -1;
		else {
			*temp++=c;

			if(c=='\n')
				break;
		}
	}
	*temp = '\0';

	return strlen(buf);
}

# if 0
// 如果存在Transfer-Encoding（重点是chunked），则在header中不能有Content-Length，有也会被忽视。
// http_read() 函数无效
ssize_t http_read(int fd, char buf[], size_t count)
{
	FILE *fp;
	char *p, *q;
	int ret, len;
	int textLenght;
	char lenbuf[5];
	char lineBuf[BUFSIZE], textBuf[BUFSIZE];

#if 0
	/* read head msg */
	fp = fdopen(fd, "r");
	if(fp == NULL) {
		fprintf(stderr,"%s: fdopen(s)\n",strerror(errno));
		return -1;
	}
	setlinebuf(fp);

	while(1) {
		memset(lineBuf, '\0', BUFSIZE);
		p = fgets(lineBuf, BUFSIZE, fp);
		if(p != NULL)
			strcat(buf, lineBuf);

		if(strncmp(lineBuf, "Content-Length:", 15) == 0) {
			q = lineBuf + 15;
			while(*(q++) != ' ');	// drop ' '

			int i = 0;
			while(*q != '\r') {
				lenbuf[i++] = *(q++);
			}
			lenbuf[i] = '\0';

			textLenght = atoi(lenbuf);
			printf("lenbuf=%s, textLenght = %d\n", lenbuf, textLenght);
		}

		if(strcmp(lineBuf, "\r\n") == 0)		// head end
			break;
	}
	fclose(fp);
#else

	/* read head msg */
	while(1) {
		ret = read_line(fd, lineBuf);
		if(ret < 0)
			return -1;
		else
			strcat(buf, lineBuf);

		if(strncmp(lineBuf, "Content-Length:", 15) == 0) {
			q = lineBuf + 15;
			while(*(q++) != ' ');	// drop ' '

			int i = 0;
			while(*q != '\r') {
				lenbuf[i++] = *(q++);
			}
			lenbuf[i] = '\0';

			textLenght = atoi(lenbuf);
		}

		if(strcmp(lineBuf, "\r\n") == 0)		// head end
			break;
	}
#endif

	/* read text */
	if(textLenght > 0) {
		len = textLenght;
		while(len > 0) {
			memset(textBuf, '\0', BUFSIZE);
			if(len > BUFSIZE)
				ret = read(fd, textBuf, BUFSIZE);
			else
				ret = read(fd, textBuf, len);

			len -= ret;

			if(ret > 0)
				strcat(buf, textBuf);
			else if(ret < 0)
				continue;
			else
				break;

			printf("textbuf=%s, ret=%d\n", textBuf, ret);
		}
	}

	return 0;

}

#else

// 如果采用短连接，则直接可以通过服务器关闭连接来确定消息的传输长度
ssize_t http_read(int fd, char buf[], size_t count)
{
	int ret;
	char temp[BUFSIZE];

	while(1) {
		memset(temp, '\0', BUFSIZE);
		ret = read(fd, temp, BUFSIZE);

		if(ret > 0)
			strcat(buf, temp);
		if(ret <= 0)
			break;
	}

	return strlen(buf);
}

#endif

ssize_t http_write(int fd, char buf[], size_t count)
{
	int ret, len = 0;
	char temp[BUFSIZE];

	/* send count first */
	sprintf(temp, "UrlSize = %d\r\n", count);
	ret = write(fd, temp, strlen(temp));
	if(ret <= 0)
		return -1;

	//printf("urlsize = %s\n", temp);

	/* send reponse */
	ret = write(fd, buf, strlen(buf));

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
	char *src = "Connection: Keep-Alive";
	char *dst = "Connection: close";

	strcpy(tempmsg, urlmsg);

	p = strstr(tempmsg, src);		// request: keep-alive; repose: Keep-Alive
	if(p != NULL) {
		q = p + strlen(src);
	} else {
		return ;
	}

	memset(urlmsg, '\0', TCPSIZE);
	strncpy(urlmsg, tempmsg, p-tempmsg);
	strcat(urlmsg, dst);
	strcat(urlmsg, q);
}


void modify_http_head(char urlmsg[])
{
	char *p, *q;
	char tempmsg[TCPSIZE];
	char *src = "HTTP/1.1";
	char *dst = "HTTP/1.0";

	strcpy(tempmsg, urlmsg);

	p = strstr(tempmsg, src);		// strstr can find first HTTP/1.1
	if(p != NULL) {
		q = p + strlen(src);
	} else {
		return ;
	}

	memset(urlmsg, '\0', TCPSIZE);
	strncpy(urlmsg, tempmsg, p-tempmsg);
	strcat(urlmsg, dst);
	strcat(urlmsg, q);
}