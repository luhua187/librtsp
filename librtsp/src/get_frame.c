#include "rtsp_os.h"
#include "get_frame.h"
#include "comm_func.h"


int vhd_get_i_frame(rtsp_client * client)
{
    int fd;
    socklen_t len = sizeof(struct sockaddr_in);
    struct sockaddr_in ser_addr;
    unsigned char net_visca_get_key_frame[] = {0x81, 0x0B, 0x01, 0x04, 0x0D, 0x00, 0xFF};
    
    fd = _create_udp_socket(&ser_addr, client->ipaddr, 1259);
    
    if(fd < 0)
        return -1;

    len = sendto(fd, (char *)&net_visca_get_key_frame, 7, 0, (struct sockaddr*)&ser_addr, len);
    if(len != 7)
        return -1;

    socket_close(fd);
    return 0;
}



void get_ipc_i_frame(rtsp_client * client)
{
    time_t curr_time = time(NULL);

    if(curr_time - client->v_sess->last_iframe_time >= 1)
        client->v_sess->last_iframe_time = curr_time;
    else
        return;       


    switch(client->ipc_type)
    {
        case IPC_TYPE_UNKNOW:
            client->get_i_f = 1;
            break;
        case IPC_TYPE_VHD:
            vhd_get_i_frame(client);
            break;
        default:
            break;
    }
}


