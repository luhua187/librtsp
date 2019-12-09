#ifndef __COMM_FUNC__H__
#define __COMM_FUNC__H__

#include "rtsp_os.h"


int _create_tcp_socket(int *sock_fd, char *ip, int port);
int _create_udp_socket(struct sockaddr_in *ser_addr,char *ip, int port);

int tcp_write_msg(int fd, char *msg, int len);
int tcp_recv_msg(int fd, char *msg);
int udp_write_msg(int fd, char *msg, int len);
int udp_recv_msg(int fd, char *msg);





unsigned int get_ticks();
















#endif