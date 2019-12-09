#include "rtsp_def.h"
#include "log.h"


int _create_tcp_socket(int *sock_fd, char *ip, int port)
{
	struct sockaddr_in ser_addr;
	int _fd = -1;
    unsigned long ul;
    int ret;

#if defined (WIN32) || defined(_WIN32)
	WORD sockVersion = MAKEWORD(2,2);
	WSADATA wsa;
	if (NOERROR != WSAStartup(MAKEWORD(1, 1), &wsa))
	{
		return -1;
	}
#endif


	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port   = htons(port);
	ser_addr.sin_addr.s_addr = (inet_addr(ip));

	_fd = socket(AF_INET,SOCK_STREAM, 0);
	if(_fd < 0)
	{
		log_warn("creat tcp socket error, ip:%s port:%d", ip, port);
		return -1;
	}
    
#ifdef WIN32
    ul = 1;
	ioctlsocket(_fd, FIONBIO, &ul);
#else
	fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK);
#endif

    ret = connect(_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if(ret != 0)
    {
        fd_set fds;
        struct timeval tm = {5, 0};
        FD_ZERO(&fds);
        FD_SET(_fd, &fds);

        if(select(_fd+1, NULL, &fds, NULL, &tm) <= 0)
        {
            log_warn("connect ip:%s port:%d error!!!", ip, port);
            socket_close(_fd);
            return -2;
        }
        else
        {            
            int error_num = -1;  
            int opt_len = sizeof(int);  
            getsockopt(_fd, SOL_SOCKET, SO_ERROR, (char*)&error_num, &opt_len);
            if(error_num != 0)
            {
                log_warn("connect ip:%s port:%d error!!!", ip, port);
                socket_close(_fd);
                return -2;
            }
        }    
    }

    *sock_fd = _fd;
    
#ifdef WIN32
    ul = 0;
	ioctlsocket(_fd, FIONBIO, &ul);
#else
	fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) & (~O_NONBLOCK));
#endif

    {
        int opt = 1;
	    int opt_len = sizeof(int);
	    setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, opt_len);
    }

	
	return 0;
}


int _create_udp_socket(struct sockaddr_in *ser_addr, char *ip, int port)
{
	int _fd = -1;
#if defined (WIN32) || defined(_WIN32)
	WORD sockVersion = MAKEWORD(2,2);
	WSADATA wsa;
	if (NOERROR != WSAStartup(MAKEWORD(1, 1), &wsa))
	{
		return -1;
	}
#endif


	ser_addr->sin_family = AF_INET;
	ser_addr->sin_port   = htons(port);
	ser_addr->sin_addr.s_addr = inet_addr(ip);

	_fd = socket(AF_INET,SOCK_DGRAM, 0);
	if(_fd < 0)
	{
		log_warn("creat udp socket error, ip:%s port:%d", ip, port);
        socket_close(_fd);
		return -1;
	}
    
	return _fd;
}


int tcp_write_msg(int fd, char *msg, int len)
{
    int ret = 0;
    int count = 0;
    struct timeval tv= {0, 1000*100};

    ret = send(fd, msg, len, 0);
    while(ret == 0)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 1000*100;
        if(count == 3)
        {
            log_warn("send  msg:[%s] error!\n",msg);
            return -1;
        }
        if(select(0, NULL, NULL, NULL, &tv) == 0)
            ret = send(fd, msg, len, 0);
    }
    
    if(ret < 0)
    {
        log_warn("send  msg:[%s] error!\n", msg);
        return -1;
    }    

	return 0;
}

int tcp_recv_msg(int fd, char *msg)
{
    int ret;
    struct timeval time_out= {2, 0};
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    
    if(select(fd+1, &rfds, NULL, NULL, &time_out) <= 0)
    {
        return -17;
    }

    ret = recv(fd, msg, RECV_BUF_LEN, 0);
    if(ret <= 0)
        return -17;

	return ret;
}

int udp_write_msg(int fd, char *msg, int len)
{
	return 0;
}

int udp_recv_msg(int fd, char *msg)
{


	return 0;
}

unsigned int get_ticks()
{
	unsigned long long v;
#if defined(WIN32)
	FILETIME ft;
	GetSystemTimeAsFileTime((FILETIME*)&ft);
	v = (((__int64)ft.dwHighDateTime << 32) | (__int64)ft.dwLowDateTime) / 10000; // to ms
	v -= 0xA9730B66800; /* 11644473600000LL */ // January 1, 1601 (UTC) -> January 1, 1970 (UTC).
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	v = tv.tv_sec;
	v *= 1000;
	v += tv.tv_usec / 1000;
#endif
	return (unsigned int)v;
}