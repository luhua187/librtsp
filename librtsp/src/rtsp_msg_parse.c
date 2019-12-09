#include "rtsp_os.h"
#include "rtsp.h"
#include "rtsp_def.h"
#include "sdp.h"

static inline char* eat_space_end(const char* p, const char* pend)
{
	for(;(p<pend)&&(*p==' ' || *p=='\t') ;p++);
	return (char *)p;
}

static inline char* eat_token_end(const char* p, const char* pend)
{
	for (;(p<pend)&&(*p!=' ')&&(*p!='\t')&&(*p!='\n')&&(*p!='\r'); p++);
	return (char *)p;
}

static inline char* q_memchr(char* p, int c, unsigned int size)
{
	char* end;
	end=p+size;
	for(;p<end;p++){
		if (*p==(unsigned char)c) return p;
	}
	return 0;
}
char* eat_line(char* buffer, unsigned int len)
{
	char* nl;
	nl=(char *)q_memchr( buffer, '\n', len );
	if ( nl ) { 
		if ( nl + 1 < buffer+len)  nl++;
		if (( nl+1<buffer+len) && * nl=='\r')  nl++;
	} else  nl=buffer+len;
	return nl;
}

static inline char* get_quotmark(char *p, char **end)
{
    char *tmp1, *tmp2;
    char *buf = NULL;
    
    tmp1 = strchr(p, '"');
    if(tmp1)
    {
        tmp2 = strchr(tmp1+1, '"');
        if(tmp2)
        {   
            buf = (char *)malloc(tmp2-tmp1);
            memset(buf, 0, tmp2-tmp1);
            memcpy(buf, tmp1+1, tmp2-tmp1-1);
            *end = tmp2+1;
            return buf;
        }
            
    }

    return NULL;
}


int rtsp_uri_parse(rtsp_client *client, const char *uri)
{
    char *str = strdup(uri);
    char *p1, *p2, *p3;

    p1 = strstr(str, "://");
    if (p1 == NULL)
        return RTSP_RET_ERR_URI;
    if(memcmp(str, "rtsp", p1-str) != 0)
        return RTSP_RET_ERR_URI;

    p1 += 3;
    p2 = strchr(p1, '@');
    if(p2)
    {
        p3 = strchr(p1, ':');
        if(p3 == NULL)
            return RTSP_RET_ERR_URI;
        *p3 = 0;
        client->username = strdup(p1);
        *p2 = 0;
        client->passwd  = strdup(p3+1);

        p1 = p2+1;
    }

    client->ipaddr = (char *)malloc(128);
    memset(client->ipaddr, 0, 128);
    p2 = strchr(p1, '/');
    p3 = strchr(p1, ':');
    if(p2 != NULL)
    {
        if(p3 && p3 < p2)
        {
            client->port = atoi(p3+1);
            memcpy(client->ipaddr, p1, p3-p1);
        }
        else
        {
            client->port = 554;
            memcpy(client->ipaddr, p1, p2-p1);
        }
    }
    else
    {
        if(p3)
        {
            client->port = atoi(p3+1);
            memcpy(client->ipaddr, p1, p3-p1);
        }
        else
        {
            client->port = 554;
            strcpy(client->ipaddr, p1);
        }
    }

    
    client->uri = (char *)malloc(256);
    snprintf(client->uri, 256, "rtsp://%s", p1);

    free(str);
    return 0;
}

int rtsp_msg_fl_parse(rtsp_client *client, char *msg)
{
    char first_line[128];
    char *ptr, *p1, *p2, *p3;


    p1 = strstr(msg,"\r\n");
    if(p1 == NULL)
        return RTSP_RET_ERR_REVC_RESP;

    memset(first_line, 0, sizeof(first_line));
    memcpy(first_line, msg, p1-msg);
    ptr  = first_line;
    p1   = first_line + (p1-msg);
    
    p2 = eat_space_end(ptr, p1);
    p3 = eat_token_end(p2, p1);
    if((memcmp(p2,"RTSP", 4)!=0) &&(memcmp(p2,"rtsp", 4)!=0)  )
        return RTSP_RET_ERR_REVC_RESP;

    p2 = eat_space_end(p3, p1);

    return atoi(p2);
}


int rtsp_msg_parse(rtsp_client *client, const char *msg, int msg_type)
{
    char *str = strdup(msg);
    char *p1=NULL, *p2=NULL, *p3=NULL, *p4 =NULL;
    char *start, *end;
    int ret = 0, len = 0;
	char *buf;
	char *tmp;

    len = strlen(str);
    end = str+len;
    p1 = str;
    
    ret = rtsp_msg_fl_parse(NULL, str);
    if(ret < 0)
        goto RET;
    
    if(ret == 401)
    {
        while(p1 < end)
        {
            p1 = eat_line(p1, len);
            p1 = eat_space_end(p1, end);
            if(*p1 == 'W' || *p1 == 'w')
            {
                p3 = p1;
                for(p2 = p1; (*p2 != '\r' && *p2 !='\n'); p2++)
                {
                    if(*p2 == ' ' || *p2 == '\t' || *(p2+1) == '\r' || *(p2+1) == '\n')
                    {
                        if((*p3 == 'b' || *p3 == 'B') && client->is_auth != 2)
                            client->is_auth = 1;
                        if(*p3 == 'd' || *p3 == 'D')
                            client->is_auth = 2;
                        if(*p3 == 'r' || *p3 == 'R')    
                            client->realm = get_quotmark(p3, &p2);
                        if(*p3 == 'n' || *p3 == 'N')
                            client->nonce = get_quotmark(p3, &p2);
                        
                        if(p2 == NULL)
                            return RTSP_RET_ERR_REVC_RESP;
                        for(; *p2==' ' || *p2 == '\t'; p2++);
                        p3 = p2;
                    }
                    
                }
            }
        }

        goto RET;
    }

    if(msg_type == RTSP_MSG_OPTIONS)
    {
        goto RET;
    }
    

    
    if(msg_type == RTSP_MSG_DESCRIBE)
    {
        int sdp_len = 0;
        while(p1 < end)
        {
            p1 = eat_line(p1, len);
            p1 = eat_space_end(p1, end);
            if(memcmp(p1, "Content-Length:", 15) == 0)
            {
                p2 = p1 + 15;
                for(; *p2==' ' || *p2 == '\t'; p2++);
                sdp_len = atoi(p2);
                p2 = strstr(p2, "\r\n\r\n");
                if(p2 && (p2+4) < end)
                {
                    buf = (char *)malloc(sdp_len +1);
                    memcpy(buf, p2+4, sdp_len);
                    buf[sdp_len] = 0;
                    client->sdp = sdp_parse(buf);
                    free(buf);
                }
            }
            
            if(memcmp(p1, "Content-Base:", 13) == 0)
            {
                p1 = p1 + 13;
                p1 = eat_space_end(p1, end);
                for(p2 = p1; *p2!=' ' && *p2 != '\t' && *p2 != ';' && *p2 != '\r' && *p2 != '\n'; p2++);
                buf = (char *)malloc(p2 -p1 + 1);
                memset(buf, 0, p2-p1+1);
                memcpy(buf, p1, p2-p1);
                client->base_uri = buf;
                
            }
        }

        goto RET;
    }


    if(msg_type == RTSP_MSG_SETUP)
    {
        while(p1 < end)
        {
            p1 = eat_line(p1, len);
            p1 = eat_space_end(p1, end);
            if(memcmp(p1, "Transport:", 10) == 0)
            {
                p1 = p1 + 10;
                p1 = eat_space_end(p1, end);
                for(p2 = p1; (*p2 != '\r' && *p2 !='\n' && *p2 != '\t' && *p2 !=';'); p2++);
                {
                    buf = (char *)malloc(p2-p1+1);
                    memcpy(buf, p1, p2-p1);
                    buf[p2-p1] = '\0';
                    if(strncmp(buf, "RTP/AVP/TCP", 11) == 0)
                        client->ser_is_tcp = 1;
                    else if(!strncmp(buf, "RTP/AVP/UDP", 11) || !strncmp(buf, "RTP/AVP", 7))
                        client->ser_is_tcp = 0;
                    free(buf);
                }
                 

                p1 = p2+1;
                p3 = p1;
                for(p2 = p1; (*p2 != '\r' && *p2 !='\n'); p2++)
                {
                    if(*p2 == ' ' || *p2 == '\t' || *p2 == ';' || *(p2+1) == '\r' || *(p2+1) == '\n')
                    {
                        if(memcmp(p3, "server_port=", 12) == 0)
                        {
                            p4 = p3 +  12;
                            client->v_sess->server_rtp_port = atoi(p4);
                            p4 = strchr(p4, '-');
                            if(p4)
                                client->v_sess->server_rtcp_port = atoi(p4+1);
                        }
                        if(memcmp(p3, "ssrc=", 5) == 0)
                        {
                            p4 = p3 + 5;
                            client->v_sess->ssrc = strtol(p4, &tmp, 16);
                       }
                 
                        for(; *p2==' ' || *p2 == '\t' || *p2 == ';' ; p2++);
                        p3 = p2;
                   }      
                }
            }


            if(memcmp(p1, "Session:", 8) == 0)
            {
                
                p1 = p1 + 8;
                p1 = eat_space_end(p1, end);
                for(p2 = p1; (*p2 != '\r' && *p2 !='\n' && *p2 != '\t' && *p2 !=';'); p2++);
                client->session_id = (char *)malloc(p2-p1 + 1);
                memcpy(client->session_id, p1, p2-p1);
                client->session_id[p2-p1] = 0;
                
                p1 = p2+1;
                p3 = p1;
                for(p2 = p1; (*p2 != '\r' && *p2 !='\n'); p2++)
                {
                    if(*p2 == ' ' || *p2 == '\t' || *p2 == ';' || *(p2+1) == '\r' || *(p2+1) == '\n')
                    {
                        if(memcmp(p3, "timeout=", 8) == 0)
                        {
                            p4 = p3 +  8;
                            client->timeout = atoi(p4);
                        }
                 
                        for(; *p2==' ' || *p2 == '\t' || *p2 == ';' ; p2++);
                        p3 = p2;
                   }      
                }
                
            }
        }

        goto RET;
    }

    if(msg_type == RTSP_MSG_PLAY)
    {

        goto RET;
    }

    if(msg_type == RTSP_MSG_TEARDOWN)
    {
        goto RET;
    }

    if(msg_type == RTSP_MSG_GET_PARAMETER)
    {
        goto RET;
    }

    
    
RET:
    free(str);
    return ret;
}



