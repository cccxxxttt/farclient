/********************************************************************\
 *                                                                  *
 * $Id$                                                             *
 * @file tcp.c                                                      *
 * @brief tcp functions                                             *
 * @author Copyright (C) 2015 cxt <xiaotaohuoxiao@163.com>          *
 * @start 2015-2-28                                                 *
 * @end   2015-3-18                                                 *
 *                                                                  *
\********************************************************************/

#include "tcp.h"

extern Fcontrol fcontrol;
int bin_flag = 0;

int getlocalip(char *uhip)
{
    int sfd, intr;
    struct ifreq buf[16];
    struct ifconf ifc;
	char *ip;

    sfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sfd < 0)
        return -1;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (ioctl(sfd, SIOCGIFCONF, (char *)&ifc))
        return -1;

    intr = ifc.ifc_len / sizeof(struct ifreq);
    while (intr-- > 0 && ioctl(sfd, SIOCGIFADDR, (char *)&buf[intr]));

    close(sfd);

    ip = inet_ntoa(((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr);

	strcpy(uhip, ip);

	return 0;
}

int getIfaceName(char *iface_name)
{
    int r = -1;
    int flgs, ref, use, metric, mtu, win, ir;
    unsigned long int d, g, m;
    char devname[20];
    FILE *fp = NULL;

    if((fp = fopen("/proc/net/route", "r")) == NULL) {
        perror("fopen error!\n");
        return -1;
    }

    if (fscanf(fp, "%*[^\n]\n") < 0) {
        fclose(fp);
        return -1;
    }

    while (1) {
        r = fscanf(fp, "%19s%lx%lx%X%d%d%d%lx%d%d%d\n",
                 devname, &d, &g, &flgs, &ref, &use,
                 &metric, &m, &mtu, &win, &ir);
        if (r != 11) {
            if ((r < 0) && feof(fp)) {
                break;
            }
            continue;
        }

        strcpy(iface_name, devname);
        fclose(fp);
        return 0;
    }

    fclose(fp);

    return -1;
}

int getlocalmac(char *iface_mac, char *iface_name)
{
	struct ifreq ifreq;
	int sock;

    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0)
    {

    }

	strcpy(ifreq.ifr_name, iface_name);
	if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) {

	}

	sprintf(iface_mac, "%02x:%02x:%02x:%02x:%02x:%02x",
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[0],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[1],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[2],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[3],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[4],
        (unsigned   char)ifreq.ifr_hwaddr.sa_data[5]);

	return 0;
}

int getUhttpdPort(int *uhttpd_port)
{
	FILE *fp;
	char buf[10];

	char *cmd = "chmod 777 /sbin/getUhttpdPort.sh";
	fp = popen(cmd, "r");
	if(fp == NULL)
		return -1;
	pclose(fp);

	cmd = "sh /sbin/getUhttpdPort.sh";
	fp = popen(cmd, "r");
	if(fp == NULL)
		return -1;

	*uhttpd_port = 0;
	while(fgets(buf, 1024, fp) != NULL) {
		buf[strlen(buf)] = '\0';
		*uhttpd_port = atoi(buf);

		//printf("buf=%s, uhport=%d\n", buf, *uhttpd_port);
	}

	pclose(fp);

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

	//DEBUG_PRINT("write-len = %d\n", strlen(buf));

	return ret;
}


/* socket set keepalive, timeout can set tcp stat, write() and read() return 0 */
int socket_set_keepalive(int fd, int idle, int intv, int cnt)
{
	int alive;

	/* Set: use keepalive on fd */
	alive = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof alive) != 0)
		return -1;

	/* idle秒钟无数据，触发保活机制，发送保活包 */
	if(setsockopt (fd, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof idle) != 0)
		return -1;

	/* 如果没有收到回应，则intv秒钟后重发保活包 */
	if (setsockopt (fd, SOL_TCP, TCP_KEEPINTVL, &intv, sizeof intv) != 0)
		return -1;

	/* 连续cnt次没收到保活包，视为连接失效 */
	if (setsockopt (fd, SOL_TCP, TCP_KEEPCNT, &cnt, sizeof cnt) != 0)
		return -1;

	return 1;
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

	// keep_alive check time: (60+5)*3
	if(socket_set_keepalive(cfd, 60, 5, 3) < 0)
		return -1;

	return cfd;
}

int uhttpd_connect(char *ip, int port)
{
	int uhfd;
	int count = 0;

	while(1) {
		uhfd = sock_client(ip, port);

		if(uhfd > 0)
			break;

		// uhfd < 0
		count++;
		if(count > 10)
			break;
	}

	return uhfd;
}

/* when one socket close, write twice takes a SIGPIPE, exit progrem */
void protect_progrem(void)
{
	signal(SIGPIPE, SIG_IGN);
}

/* like tcp ping, keep tcp alive */
int pc_start(int fd)
{
	int ret = 0;
	char buf[10];

	/* ping: read "start\r\n" */
	ret = read(fd, buf, strlen("start\r\n"));
	if(ret>0 && strncmp(buf, "start\r\n", ret)==0) {
		return ret;		// success
	}
	else
		return PC_MSG_ERR;	// faile
}

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
			memcpy(buf+retsize, temp, ret);
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

				DEBUG_PRINT("pc-%d:  %s", fd, temp);
			}

			firstFlag = 1;

			#if 0
			p = strstr(temp, "POST");
			if(p != NULL)
				bin_flag = 1;
			#endif
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
			return PC_MSG_ERR;

		DEBUG_PRINT("pc-%d-%d-%d\n", srvfd, ret, strlen(urlmsg));

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
			return UH_MSG_ERR;
	}
	DEBUG_PRINT("pc-end\n");
	return OK_MSG;
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
			return UH_MSG_ERR;

		strcat(urlmsg, lineBuf);
		retsize += ret;

		DEBUG_PRINT("route-%d:  %s", uhfd, urlmsg);
#endif

	while(1) {
		ret = read_line(uhfd, lineBuf);
		if(ret < 0)
			return UH_MSG_ERR;
		else
			strcat(urlmsg, lineBuf);

		retsize += ret;

		if(strcmp(lineBuf, "\r\n") == 0)		// head end
			break;
	}
	//DEBUG_PRINT("route-%d:  %s", uhfd, urlmsg);
	/* write back to server */
	if((ret = http_write(srvfd, urlmsg, retsize)) <= 0)
		return PC_MSG_ERR;

	/* server close mean end */
	while(1) {
		/* read form uhttpd */
		memset(urlmsg, '\0', TCPSIZE);
		ret = read(uhfd, urlmsg, TCPSIZE);

		if(ret <= 0)
			break;

		retsize += ret;

		DEBUG_PRINT("route-%d-%d\n", ret, strlen(urlmsg));

		/* write back to server */
		if((ret = http_write(srvfd, urlmsg, ret)) <= 0)
			return PC_MSG_ERR;
	}

	DEBUG_PRINT("read-end-%d\n\n", retsize);

	/* send end msg */
	if((ret = http_write(srvfd, "end\r\n", strlen("end\r\n"))) <= 0)
		return PC_MSG_ERR;

	return retsize;
}
