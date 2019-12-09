#include "rtsp_os.h"
#include "rtsp_def.h"
#include "comm_func.h"
#include "md5.h"
#include "base64.h"
#include "log.h"

#define RTSP_USER_AGENT_STR "User-Agent: <ZjJing RTSP>\r\n"
#define RTSP_CSEQ_STR "CSeq: "

int md5sum_response(rtsp_client *client, char *cmd, char *uri, unsigned char *response)
{
    char buf[2048],ha1[512],ha2[512];
    int len = 0;

    memset(buf, 0, sizeof(buf));
    memset(ha1, 0, sizeof(ha1));
    memset(ha2, 0, sizeof(ha2));
    
    
    len = sprintf(buf, "%s:%s:%s", client->username, client->realm, client->passwd);
    md5_sum32(buf, ha1, len, 32);

    memset(buf, 0, sizeof(buf));
    len = sprintf(buf, "%s:%s",cmd, uri);
    md5_sum32(buf, ha2, len, 32);

    memset(buf, 0, sizeof(buf));
    len = sprintf(buf,  "%s:%s:%s", ha1, client->nonce, ha2);
    md5_sum32(buf, response, len, 32);


    return 0;
}

int base64_response(rtsp_client *client, char *response)
{
    char buf[2048];
    int len = 0;

    memset(buf, 0, sizeof(buf));
    
    len = sprintf(buf, "%s:%s", client->username, client->passwd);
    base64_encode(buf, len, response);

    return 0;
}


int set_rtp_port(rtsp_session *session, int port)
{
	int rtp_sockfd, rtcp_sockfd;
	int rtpport;
	int rtcpport;
	struct sockaddr_in servaddr;
	int rtp_port_from;

#if defined (WIN32) || defined(_WIN32)
	WORD sockVersion = MAKEWORD(2,2);
	WSADATA wsa;
	if (NOERROR != WSAStartup(MAKEWORD(1, 1), &wsa))
	{
		return -1;
	}
#endif

    
	if(0 != port && (port % 2 == 0)) 
		rtp_port_from = port;
    else
		rtp_port_from = 59000;

    if(session->rtp_fd > 0)
        socket_close(session->rtp_fd);
    if(session->rtcp_fd > 0)
        socket_close(session->rtcp_fd);
	

	for(rtpport = rtp_port_from; rtpport < rtp_port_from+1000; rtpport = rtpport + 2) 
    {
		if((rtp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        {
			perror("creat rtp fd error!\n");
			return 0;
		}	
		
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(rtpport);
		if(bind(rtp_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
        {
			socket_close(rtp_sockfd);
			continue;
		}

		// Bind RTCP Port
		rtcpport = rtpport + 1;
		if((rtcp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
        {
			socket_close(rtp_sockfd);
			perror("creat rtcp fd Error\n");
			return 0; // Create failed
		}	

		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(rtcpport);

		if(bind(rtcp_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
        {
			socket_close(rtp_sockfd);
			socket_close(rtcp_sockfd);
			continue;
		}

        session->rtp_fd = rtp_sockfd;
        session->client_rtp_port = rtpport;
        session->rtcp_fd = rtcp_sockfd;
        session->client_rtcp_port = rtcpport;

		return 0;
	}


    session->rtp_fd = -1;
    session->rtcp_fd = -1;
	return -1;
}


int get_media_sdp_control(rtsp_client *client, int media_type, char *uri)
{
    struct sdp_media *m   = NULL;
    struct sdp_info *info = NULL;
    struct sdp_payload *sdp = client->sdp;
    unsigned int i,j;
    char *p1;
    int len = 0;

	if(client->sdp == NULL)
        return -1;

    for(i = 0; i < sdp->medias_count; i++)
    {
        m = &sdp->medias[i];
        info = &m->info;

        if(media_type == MEDIA_TYPE_VEDIO && memcmp(info->type, "video", 5) == 0)
        {
            for(j = 0; j < m->attributes_count; j++)
            {
                p1 = strstr(m->attributes[j], "control:");
                if(p1)
                {
                    if(memcmp(p1+8, "rtsp://", 7) == 0)
                        sprintf(uri, "%s", p1+8);
                    else
                    {
                        len = strlen(client->base_uri);
                        if(client->base_uri[len-1] == '/')
                            sprintf(uri, "%s%s", client->base_uri, p1+8);
                        else
                            sprintf(uri, "%s/%s", client->base_uri, p1+8);
                    }
                    return 0;
                }
            }
        }

        if(media_type == MEDIA_TYPE_AUDIO && memcmp(info->type, "audio", 5) == 0)
        {
            for(j = 0; j < m->attributes_count; j++)
            {
                p1 = strstr(m->attributes[j], "control:");
                if(p1)
                {
                    if(memcmp(p1, "rtsp://", 7) == 0)
                        sprintf(uri, "%s", p1+8);
                    else
                    {
                        len = strlen(client->base_uri);
                        if(client->base_uri[len-1] == '/')
                            sprintf(uri, "%s%s", client->base_uri, p1+8);
                        else
                            sprintf(uri, "%s/%s", client->base_uri, p1+8);
                    }
                    return 0;
                }
            }
        }

        
    }

    return -1;
}


int rtsp_options_msg(rtsp_client *client)
{
	char msg[2048];
	int  offset = 0;
    int ret = 0;

	memset(msg, 0, sizeof(msg));
	offset += sprintf(msg+offset, "OPTIONS %s RTSP/1.0\r\n", client->uri);
	offset += sprintf(msg+offset, "%s %d\r\n", RTSP_CSEQ_STR, client->cseq++);
	offset += sprintf(msg+offset, "%s", RTSP_USER_AGENT_STR);
	offset += sprintf(msg+offset, "\r\n");

    ret = tcp_write_msg(client->sig_fd, msg, offset);
    if(ret == -1)
    {
        socket_close(client->sig_fd);
        client->sig_fd = -1;
        return -1;
    }

    log_debug("send rtsp: %s", msg);
	return 0;
}


int rtsp_describe_msg(rtsp_client *client)
{

	unsigned char msg[2048], response_auth[512];
	int  offset = 0;
    int ret;

    memset(response_auth, 0,  sizeof(response_auth));
	memset(msg, 0, sizeof(msg));
    
	offset += sprintf(msg+offset, "DESCRIBE %s RTSP/1.0\r\n", client->uri);
	offset += sprintf(msg+offset, "%s %d\r\n", RTSP_CSEQ_STR, client->cseq++);
    if(client->is_auth == 2)
    {
        md5sum_response(client, "DESCRIBE", client->uri, response_auth);
        offset += sprintf(msg+offset, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n", 
                                            client->username, client->realm, client->nonce, client->uri, response_auth);
    }
    else if(client->is_auth == 1)
    {
        base64_response(client, response_auth);
        offset += sprintf(msg+offset, "Authorization: Basic %s\r\n", response_auth);
    }
	offset += sprintf(msg+offset, "%s", RTSP_USER_AGENT_STR);
	offset += sprintf(msg+offset, "Accept: application/sdp\r\n");
	offset += sprintf(msg+offset, "\r\n");

    
	ret = tcp_write_msg(client->sig_fd, msg, offset);
    if(ret == -1)
    {
        socket_close(client->sig_fd);
        client->sig_fd = -1;
        return -1;
    }

    log_debug("send rtsp: %s", msg);
	return 0;
}


int rtsp_setup_msg(rtsp_client *client, int media_type)
{

	unsigned char msg[2048], buf[512];
	int  offset = 0;
    int ret;
    
	memset(msg, 0, sizeof(msg));

    if(media_type == MEDIA_TYPE_AUDIO)
        set_rtp_port(client->a_sess, 0);
    else
        set_rtp_port(client->v_sess, 0);
    

    memset(buf, 0,  sizeof(buf));
    get_media_sdp_control(client, media_type, buf);
	offset += sprintf(msg+offset, "SETUP %s RTSP/1.0\r\n", buf);
	offset += sprintf(msg+offset, "%s %d\r\n", RTSP_CSEQ_STR, client->cseq++);
    if(client->is_auth == 2)
    {   
        memset(buf, 0, sizeof(buf));
        md5sum_response(client, "SETUP", client->base_uri, buf);
        offset += sprintf(msg+offset, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n", 
                                            client->username, client->realm, client->nonce, client->base_uri, buf);
    }
    else if(client->is_auth == 1)
    {
        memset(buf, 0, sizeof(buf));
        base64_response(client, buf);
        offset += sprintf(msg+offset, "Authorization: Basic %s\r\n", buf);
    }
	offset += sprintf(msg+offset, "%s", RTSP_USER_AGENT_STR);
    
    if(media_type == MEDIA_TYPE_AUDIO)
	    offset += sprintf(msg+offset, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n", client->a_sess->client_rtp_port,client->a_sess->client_rtcp_port);
    else
        offset += sprintf(msg+offset, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n", client->v_sess->client_rtp_port,client->v_sess->client_rtcp_port);

    if(client->session_id)
	    offset += sprintf(msg+offset, "Session: %s\r\n", client->session_id);
    
	offset += sprintf(msg+offset, "\r\n");

	ret = tcp_write_msg(client->sig_fd, msg, offset);
    if(ret == -1)
    {
        socket_close(client->sig_fd);
        client->sig_fd = -1;
        return -1;
    }

    log_debug("send rtsp: %s", msg);
	return 0;
}


int rtsp_play_msg(rtsp_client *client)
{
	unsigned char msg[2048], buf[512];
	int  offset = 0;
    int ret;

	memset(msg, 0, sizeof(msg));
	offset += sprintf(msg+offset, "PLAY %s RTSP/1.0\r\n", client->base_uri);
	offset += sprintf(msg+offset, "%s %d\r\n", RTSP_CSEQ_STR, client->cseq++);
    if(client->is_auth == 2)
    {   
        memset(buf, 0, sizeof(buf));
        md5sum_response(client, "PLAY", client->base_uri, buf);
        offset += sprintf(msg+offset, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n", 
                                            client->username, client->realm, client->nonce, client->base_uri, buf);
    }
    else if(client->is_auth == 1)
    {
        memset(buf, 0, sizeof(buf));
        base64_response(client, buf);
        offset += sprintf(msg+offset, "Authorization: Basic %s\r\n", buf);
    }
	offset += sprintf(msg+offset, "%s", RTSP_USER_AGENT_STR);
	offset += sprintf(msg+offset, "Session: %s\r\n", client->session_id);
	offset += sprintf(msg+offset, "Range: npt=0.000-\r\n");
	offset += sprintf(msg+offset, "\r\n");

	ret = tcp_write_msg(client->sig_fd, msg, offset);
    if(ret == -1)
    {
        socket_close(client->sig_fd);
        client->sig_fd = -1;
        return -1;
    }

    log_debug("send rtsp: %s", msg);
	return 0;
}


int rtsp_teardown_msg(rtsp_client *client)
{
    unsigned char msg[2048], response_auth[512];
	int  offset = 0;
    int ret;

    memset(response_auth, 0,  sizeof(response_auth));
	memset(msg, 0, sizeof(msg));
    
	offset += sprintf(msg+offset, "TEARDOWN %s RTSP/1.0\r\n", client->base_uri);
	offset += sprintf(msg+offset, "%s %d\r\n", RTSP_CSEQ_STR, client->cseq++);
    if(client->is_auth == 2)
    {
        md5sum_response(client, "TEARDOWN", client->uri, response_auth);
        offset += sprintf(msg+offset, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n", 
                                            client->username, client->realm, client->nonce, client->uri, response_auth);
    }
    else if(client->is_auth == 1)
    {
        base64_response(client, response_auth);
        offset += sprintf(msg+offset, "Authorization: Basic %s\r\n", response_auth);
    }
	offset += sprintf(msg+offset, "%s", RTSP_USER_AGENT_STR);
	offset += sprintf(msg+offset, "Session: %s\r\n", client->session_id);
	offset += sprintf(msg+offset, "\r\n");
    
	ret = tcp_write_msg(client->sig_fd, msg, offset);
    if(ret == -1)
    {
        socket_close(client->sig_fd);
        client->sig_fd = -1;
        return -1;
    }  

    log_debug("send rtsp: %s", msg);
    return 0;

	return 0;
}

int rtsp_pause_msg(rtsp_client *client)
{
    

	return 0;
}

int rtsp_get_parameter_msg(rtsp_client *client)
{
    unsigned char msg[2048], response_auth[512];
	int  offset = 0;
    int ret;

    memset(response_auth, 0,  sizeof(response_auth));
	memset(msg, 0, sizeof(msg));
    
	offset += sprintf(msg+offset, "GET_PARAMETER %s RTSP/1.0\r\n", client->base_uri);
	offset += sprintf(msg+offset, "%s %d\r\n", RTSP_CSEQ_STR, client->cseq++);
    if(client->is_auth == 2)
    {
        md5sum_response(client, "GET_PARAMETER", client->uri, response_auth);
        offset += sprintf(msg+offset, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n", 
                                            client->username, client->realm, client->nonce, client->uri, response_auth);
    }
    else if(client->is_auth == 1)
    {
        base64_response(client, response_auth);
        offset += sprintf(msg+offset, "Authorization: Basic %s\r\n", response_auth);
    }
	offset += sprintf(msg+offset, "%s", RTSP_USER_AGENT_STR);
	offset += sprintf(msg+offset, "Session: %s\r\n", client->session_id);
	offset += sprintf(msg+offset, "\r\n");
    
	ret = tcp_write_msg(client->sig_fd, msg, offset);
    if(ret == -1)
    {
        socket_close(client->sig_fd);
        client->sig_fd = -1;
        return -1;
    }  

    log_debug("send rtsp: %s", msg);
    return 0;
}

