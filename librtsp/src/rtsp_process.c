#include "rtsp_os.h"
#include "rtsp_process.h"
#include "rtsp_def.h"
#include "rtsp_msg_parse.h"
#include "comm_func.h"
#include "rtp-payload.h"
#include "rtp.h"
#include "log.h"

extern onErrorCallback rtsp_error_cb;
extern onStatisticsCallback rtsp_statis_cb;
extern int cb_interval_time;
extern unsigned char *p_no_video_buf;
extern int len_no_video_buf;

void free_client_node(rtsp_client *client)
{
    if(client == NULL)
        return;
    
    if(client->uri)
        free(client->uri);
    if(client->base_uri)
        free(client->base_uri);
    if(client->username)
        free(client->username);
    if(client->passwd)
        free(client->passwd);
    if(client->uuid)
        free(client->uuid);
    if(client->ipaddr)
        free(client->ipaddr);
    if(client->nonce)
        free(client->nonce);
    if(client->realm)
        free(client->realm);
    if(client->md5)
        free(client->md5);
    if(client->session_id)
        free(client->session_id);


    if(client->sdp)
        sdp_destroy(client->sdp);
    if(client->v_sess)
    {
        if(client->v_sess->rtp_fd > 0)
            socket_close(client->v_sess->rtp_fd);
        if(client->v_sess->rtcp_fd)
            socket_close(client->v_sess->rtcp_fd);
        free(client->v_sess);
    }
    if(client->a_sess)
    {
        if(client->a_sess->rtp_fd > 0)
            socket_close(client->a_sess->rtp_fd);
        if(client->a_sess->rtcp_fd)
            socket_close(client->a_sess->rtcp_fd);
        free(client->a_sess);
    }

    if(client->rtp_decode)
        rtp_payload_decode_destroy(client->rtp_decode);

    if(client->v_rtsp_st)
        free(client->v_rtsp_st);
    if(client->a_rtsp_st)
        free(client->a_rtsp_st);
    

    free(client);
    
}

void reinit_client_node(rtsp_client *client)
{
    if(client == NULL)
        return;
    
    if(client->base_uri)
    {
        free(client->base_uri);
        client->base_uri = NULL;
    }
    if(client->nonce)
    {
        free(client->nonce);
        client->nonce = NULL;
    }
    if(client->realm)
    {
        free(client->realm);
        client->realm = NULL;
        
    }
    if(client->md5)
    {
        free(client->md5);
        client->md5 = NULL;
    }
    if(client->session_id)
    {
        free(client->session_id);
        client->session_id = NULL;
    }

    client->timeout   = 0;
    client->is_active = RTSP_RET_ERR_CONNECT;
    client->is_auth   = 0;
    client->v_rtsp_st->height = 0;
    client->v_rtsp_st->width  = 0;

    if(client->rtp_decode)
    {
        rtp_payload_decode_destroy(client->rtp_decode);
        client->rtp_decode = NULL;
    }
    if(client->sdp)
    {
        sdp_destroy(client->sdp);
        client->sdp = NULL;
    }
    if(client->v_sess)
    {
        if(client->v_sess->rtp_fd > 0)
            socket_close(client->v_sess->rtp_fd);
        if(client->v_sess->rtcp_fd)
            socket_close(client->v_sess->rtcp_fd);

        memset(client->v_sess, 0, sizeof(rtsp_session));
        client->v_sess->rtp_fd  = -1;
        client->v_sess->rtcp_fd = -1;
        client->v_sess->last_seq = -1;
    }
    if(client->a_sess)
    {
        if(client->a_sess->rtp_fd > 0)
            socket_close(client->a_sess->rtp_fd);
        if(client->a_sess->rtcp_fd)
            socket_close(client->a_sess->rtcp_fd);

        memset(client->v_sess, 0, sizeof(rtsp_session));
        client->a_sess->rtp_fd  = -1;
        client->a_sess->rtcp_fd = -1;
        client->a_sess->last_seq = -1;
    }
}

void reinit_rtp_statisc(rtsp_client *client)
{
    if(client->v_rtsp_st)
    {
        client->v_rtsp_st->secondFrom = time(NULL);
        client->v_rtsp_st->duration = 0;
        client->v_rtsp_st->bytes = 0;       
        client->v_rtsp_st->frames = 0;      
        client->v_rtsp_st->iframes = 0;     
        client->v_rtsp_st->ptktotal = 0;    
        client->v_rtsp_st->pktlosts = 0;    
        client->v_rtsp_st->jitter = 0;      
        client->v_rtsp_st->width = 0;       
        client->v_rtsp_st->height = 0;
    }

    if(client->a_rtsp_st)
    {
        client->a_rtsp_st->secondFrom = time(NULL);
        client->a_rtsp_st->duration = 0;
        client->a_rtsp_st->bytes = 0;       
        client->a_rtsp_st->frames = 0;      
        client->a_rtsp_st->iframes = 0;     
        client->a_rtsp_st->ptktotal = 0;    
        client->a_rtsp_st->pktlosts = 0;    
        client->a_rtsp_st->jitter = 0;      
        client->a_rtsp_st->width = 0;       
        client->a_rtsp_st->height = 0;
    }
    
}


int handle_rtp_message(rtsp_client *client, unsigned char *buf, int media_type)
{
    struct sockaddr_in client_addr;
    socklen_t addrlen=sizeof(client_addr);
    int len = 0;
    int i = 0;

    memset(buf, 0, RTP_BUF_LEN);
    
    if(media_type == MEDIA_TYPE_AUDIO)
        len = recvfrom(client->a_sess->rtp_fd, buf, RTP_BUF_LEN, 0, (struct sockaddr *)&client_addr, &addrlen);
    else
        len = recvfrom(client->v_sess->rtp_fd, buf, RTP_BUF_LEN, 0, (struct sockaddr *)&client_addr, &addrlen);

    if(len <= 0)
        return 0;

    
    if(media_type == MEDIA_TYPE_VEDIO)
    {
        rtp_packet_lost_check(client, buf, MEDIA_TYPE_VEDIO);
        client->v_rtsp_st->jitter = rtp_calc_jitter(client, buf, MEDIA_TYPE_VEDIO);
        add_rtp_packet(client, buf, len);
        client->v_rtsp_st->ptktotal++;
        client->v_rtsp_st->bytes += len;
    }
    else
    {
        
    }

    return 0;
}

int handle_rtcp_message(rtsp_client *client, int media_type)
{
    struct sockaddr_in client_addr;
    socklen_t addrlen=sizeof(client_addr);
    char buf[4096];
    int len = 0;
    int i = 0;

    if(media_type == MEDIA_TYPE_AUDIO)
        len = recvfrom(client->a_sess->rtcp_fd, buf, 4096, 0, (struct sockaddr *)&client_addr, &addrlen);
    else
        len = recvfrom(client->v_sess->rtcp_fd, buf, 4096, 0, (struct sockaddr *)&client_addr, &addrlen);


    return 0;
}


int handle_rtsp_recv_msg(rtsp_client *client, char *buf, int msg_type)
{
    int ret = 0;

    memset(buf, 0, RECV_BUF_LEN);

    ret = tcp_recv_msg(client->sig_fd, buf);
    if(ret == RTSP_RET_ERR_REVC_TIMEOUT)
        goto RECV_TIME_OUT;
    
    ret = rtsp_msg_parse(client, buf, msg_type);
    if(ret == 401)
        return 401;
    if(ret !=  200)
        goto NOT_200_OK;

    return 0;

RECV_TIME_OUT:
    socket_close(client->sig_fd);
    client->sig_fd = -1;
    client->need_reinit = 1;
    return -1;

NOT_200_OK:
    if(ret > 200 && ret != 401)
    {
        if(rtsp_error_cb)
            rtsp_error_cb(client->uuid, -ret);       
    }
    return -1;
    
}


int handle_rtsp_signal(rtsp_client *client)
{
    char *buf = (char *)malloc(RECV_BUF_LEN);
    int ret;

    if(client->frame_cb && len_no_video_buf)
        client->frame_cb(p_no_video_buf, len_no_video_buf, NALU_TYPE_IDR, client->user_param);

    
    if(client->sig_fd < 0)
    {
        ret = _create_tcp_socket(&(client->sig_fd), client->ipaddr, client->port);
        if(ret < 0)
        {
            client->sig_fd = -1;
            if(rtsp_error_cb)
                rtsp_error_cb(client->uuid, RTSP_RET_ERR_CONNECT);
            free(buf);
            return -1;
        }
    }

    client->need_reinit = 0;

    ret = rtsp_options_msg(client);
    if(ret == -1)
        goto RET;
    ret = handle_rtsp_recv_msg(client,buf, RTSP_MSG_OPTIONS);
    if(ret == -1)
        goto RET;
    

    rtsp_describe_msg(client);
    if(ret == -1)
        goto RET;
    ret = handle_rtsp_recv_msg(client,buf, RTSP_MSG_DESCRIBE);
    if(ret == -1)
       goto RET;
    if(ret == 401)
    {
        rtsp_describe_msg(client);
        if(ret == -1)
            goto RET;
        ret = handle_rtsp_recv_msg(client,buf, RTSP_MSG_DESCRIBE);
        if(ret == -1)
            goto RET;
        if(ret == 401 && rtsp_error_cb)
        {
            rtsp_error_cb(client->uuid, -401);
            goto RET;
        }           
    }
    
    ret = rtsp_setup_msg(client, MEDIA_TYPE_VEDIO);
    if(ret == -1)
       goto RET;
    ret = handle_rtsp_recv_msg(client, buf, RTSP_MSG_SETUP);
    if(ret == -1)
        goto RET;
    
    ret = rtsp_play_msg(client);
    if(ret == -1)
        goto RET;
    ret = handle_rtsp_recv_msg(client, buf, RTSP_MSG_PLAY);
    if(ret == -1)
        goto RET;

    client->is_active = 0;
    free(buf);
    return 0; 

RET:
    client->need_reinit = 1;
    free(buf);
    return -1;

}


void handle_lose_rtp_error(rtsp_client *client)
{
    if(client->sig_fd)
        rtsp_teardown_msg(client);
    
    client->need_reinit = 1;
}


#if defined (WIN32) || defined(_WIN32)
DWORD WINAPI thread_handle_rtsp(void *param)
#else
void * thread_handle_rtsp(void *param)
#endif
{
    rtsp_client *client = (rtsp_client *)(param);
    char *msg      = (char *)malloc(RECV_BUF_LEN);
    char *v_rtp_buf  = (char *)malloc(RTP_BUF_LEN);
    int ret, maxfd = -1, no_recv_count = 0;
    int time_count = 0;
    struct timeval tv= {1, 0};
    fd_set rfds;
    time_t current_time = 0;
    time_t statisc_time = 0;
    
    ret = handle_rtsp_signal(client);
    while(client->is_running && ret == -1)
    {
        
        reinit_client_node(client);
        ret = handle_rtsp_signal(client);
        if(ret == -1)
        {
            if(rtsp_error_cb)
                    rtsp_error_cb(client->uuid, RTSP_RET_ERR_RTSP_MSG);
            sleep(1);
        }
    }
    create_rtp_decoder(client);
    
    while(client->is_running)
    {
        maxfd = -1;
        FD_ZERO(&rfds);
        FD_SET(client->sig_fd, &rfds);
        maxfd = client->sig_fd>maxfd? client->sig_fd : maxfd;
        FD_SET(client->v_sess->rtp_fd, &rfds);
        maxfd = client->v_sess->rtp_fd>maxfd? client->v_sess->rtp_fd : maxfd;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if(ret == 0)
        {
            if(rtsp_error_cb)
                rtsp_error_cb(client->uuid, RTSP_RET_ERR_NO_RTP);

            if(client->frame_cb && len_no_video_buf)
                client->frame_cb(p_no_video_buf, len_no_video_buf, NALU_TYPE_IDR, client->user_param);

            if(time_count++ > 5)
            {
                handle_lose_rtp_error(client);
                time_count = 0;
            }
        }
        else if(ret < 0)
        {
            client->need_reinit = 1;
            socket_close(client->sig_fd);
            client->sig_fd = -1;
        }
        
        
        if(FD_ISSET(client->sig_fd, &rfds))
        {
            handle_rtsp_recv_msg(client, msg, RTSP_MSG_GET_PARAMETER);
        }
        if(FD_ISSET(client->v_sess->rtp_fd, &rfds))
        {
            handle_rtp_message(client, v_rtp_buf, MEDIA_TYPE_VEDIO);
        }
        if(FD_ISSET(client->v_sess->rtcp_fd, &rfds))
        {
            handle_rtcp_message(client, MEDIA_TYPE_VEDIO);
        }

        
        while(client->need_reinit == 1 && client->is_running)   // repeat connect;
        {
            log_warn("rtsp error repeat connect srcuuid:[%s] uri:[%s]\n",client->uuid, client->uri);
            reinit_client_node(client);
            ret = handle_rtsp_signal(client);
            if(ret == 0)
            {
                create_rtp_decoder(client);
            }
            else
            {
                if(ret ==-1 && rtsp_error_cb)
                    rtsp_error_cb(client->uuid, RTSP_RET_ERR_RTSP_MSG);

                if(client->frame_cb && len_no_video_buf)
                    client->frame_cb(p_no_video_buf, len_no_video_buf, NALU_TYPE_IDR, client->user_param);
                
                sleep(1);
            }
        }

        
        current_time = time(NULL);
        if(client->timeout && (current_time - client->last_keeplive_time >= (client->timeout-3)) )
        {
            client->last_keeplive_time = current_time;
            rtsp_get_parameter_msg(client);
        }
        

        if(rtsp_statis_cb && current_time - statisc_time >= cb_interval_time )
        {
            rtsp_statis_cb((char *)client->v_rtsp_st, sizeof(RtspSrcStatistics));
            statisc_time = current_time;
        }
        
    }


    if(msg)
        free(msg);
    if(v_rtp_buf)
        free(v_rtp_buf);
    
    rtsp_teardown_msg(client);
    free_client_node(client); 
    client = NULL;

    return NULL;
}



