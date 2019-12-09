#include "rtsp_os.h"
#include "rtsp_def.h"
#include "list.h"
#include "rtsp.h"
#include "rtsp_msg_parse.h"
#include "rtsp_process.h"
#include "log.h"
#include "get_frame.h"


list_rtsp_client zj_rtsp_client;
onErrorCallback rtsp_error_cb = NULL;
onStatisticsCallback rtsp_statis_cb = NULL;
int cb_interval_time = 10;
unsigned char *p_no_video_buf = NULL;
int len_no_video_buf = 0;


list_rtsp_client* get_globle_list()
{
    return &zj_rtsp_client;
}

int InitRtspIpc()
{
#if defined (WIN32) || defined(_WIN32)
    zj_rtsp_client.lock = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutexattr_t mutexattr;   
    memset(&mutexattr, 0, sizeof(pthread_mutexattr_t));
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&(zj_rtsp_client.lock), &mutexattr);
#endif


    list_init(&(zj_rtsp_client.list));

    log_info("rtsp library is inited...............!!!");

    return 0;
}

int SetRtspIpcRun()
{
    return 0;
}


int AddRtspSource(const char* rtspurl, const char* srcuuid, enum ipc_type type)
{
    rtsp_client *client = NULL;
    list_rtsp_client *client_list = NULL;
    struct list_head *pos;
    

    if(rtspurl == NULL || srcuuid == NULL)
    {
        return RTSP_RET_ERR_PARAM;
    }


    client_list = get_globle_list();
    mutex_lock(&(client_list->lock));
    list_for_each(pos, &(client_list->list))
    {
        if(pos)
            client = list_entry(pos, rtsp_client, list);
        else
            break;
        
        if(strcmp(client->uuid, srcuuid) == 0)
        {
            mutex_unlock(&(client_list->lock));
            return RTSP_RET_ERR_DUP_UUID;
        }
    }
    mutex_unlock(&(client_list->lock));
     
    client = (rtsp_client *)malloc(sizeof(rtsp_client));
    memset(client, 0, sizeof(rtsp_client));
    rtsp_uri_parse(client, rtspurl);
    client->uuid  = strdup(srcuuid);
    client->v_sess = (rtsp_session *)malloc(sizeof(rtsp_session));
    memset(client->v_sess, 0, sizeof(rtsp_session));
    client->a_sess = (rtsp_session *)malloc(sizeof(rtsp_session));
    memset(client->a_sess, 0, sizeof(rtsp_session));
    client->ipc_type = (unsigned int)type;
    client->a_sess->rtp_fd = -1;
    client->a_sess->rtcp_fd = -1;
    client->v_sess->rtp_fd = -1;
    client->v_sess->rtcp_fd = -1;
    client->sig_fd = -1;
    client->is_running = 1;
    client->is_active  = RTSP_RET_ERR_CONNECT;
    client->base_uri = NULL;
    client->nonce  = NULL;
    client->realm  = NULL;
    client->md5    = NULL;
    client->session_id = NULL;
    client->sdp =  NULL;
    client->rtp_decode = NULL;
    client->v_rtsp_st = (RtspSrcStatistics *)malloc(sizeof(RtspSrcStatistics));
    client->a_rtsp_st = NULL;
    memset(client->v_rtsp_st, 0, sizeof(RtspSrcStatistics));
    memcpy(client->v_rtsp_st->srcuuid, srcuuid, strlen(srcuuid));
    

    mutex_lock(&(client_list->lock));
    list_add_tail(&(client->list), &(client_list->list));
    mutex_unlock(&(client_list->lock));


#if defined (WIN32) || defined(_WIN32)
    client->thread_id = (thread_t)CreateThread(NULL, 0, thread_handle_rtsp, (void*)client, 0, NULL);
    if(client->thread_id <= 0)
    {
        log_error("create rtsp thread erroe");
        return -1;
    }
#else 
    if(pthread_create(&(client->thread_id), NULL, thread_handle_rtsp, client) != 0)
    {
        log_error("create rtsp thread erroe");
        return -1;
    }
#endif

    log_info("add an rtsp client %s:%s", srcuuid, rtspurl);     
    return 0;

}



int GetRtspSourceStatus(const char* srcuuid, int* status)
{  
    rtsp_client *client = NULL;
    list_rtsp_client *client_list = NULL;
    struct list_head *pos;
    int flag = 0;

	if(srcuuid == NULL)
		return RTSP_RET_ERR_PARAM;


    client_list = get_globle_list();
    mutex_lock(&(client_list->lock));
    list_for_each(pos, &(client_list->list))
    {
        client = list_entry(pos, rtsp_client, list);
        if(strcmp(client->uuid, srcuuid) == 0)
        {
            *status = client->is_active;
            flag = 1;
            break;
        }
    }
    mutex_unlock(&(client_list->lock));

    if(flag == 0)
    {
        return RTSP_RET_ERR_NOT_FOUND;
    }

    return 0;
}


int GetIframeOfH264Output(const char* srcuuid)
{
    rtsp_client *client = NULL;
    list_rtsp_client *client_list = NULL;
    struct list_head *pos;
    int flag = 0;

    if(srcuuid == NULL)
        return RTSP_RET_ERR_PARAM;


    client_list = get_globle_list();
    mutex_lock(&(client_list->lock));
    list_for_each(pos, &(client_list->list))
    {
        client = list_entry(pos, rtsp_client, list);
        if(strcmp(client->uuid, srcuuid) == 0)
        {
            flag = 1;
            break;
        }
    }
    mutex_unlock(&(client_list->lock));
    
    if(flag == 0)
        return RTSP_RET_ERR_NOT_FOUND;

    get_ipc_i_frame(client);

    return 0;
}


int SetRtspSrcOfH264Output(const char* srcuuid, onFramebufCallback func, void *user_param)
{
    rtsp_client *client = NULL;
    list_rtsp_client *client_list = NULL;
    struct list_head *pos;
    int flags = 0;

	if(srcuuid == NULL)
        return RTSP_RET_ERR_PARAM;

    client_list = get_globle_list();
    mutex_lock(&(client_list->lock));
    list_for_each(pos, &(client_list->list))
    {
        client = list_entry(pos, rtsp_client, list);
        if(strcmp(client->uuid, srcuuid) == 0)
        {
            client->frame_cb = func;
            client->user_param = user_param;
            flags = 1;
            client->is_start_cb = 0;
            break;
        }
    }
    mutex_unlock(&(client_list->lock));

    if(flags == 0)
    {
        return RTSP_RET_ERR_NOT_FOUND;
    }
    else
    {
        reinit_rtp_statisc(client);
        get_ipc_i_frame(client);
    }

    return 0;
}


int SetErrorCallback(onErrorCallback func)
{
    if(func == NULL)
        return RTSP_RET_ERR_PARAM;

    rtsp_error_cb = func;

    return 0;
}


int SetStatisticsCallback(onStatisticsCallback func, int seconds)
{
    if(func == NULL || seconds <= 0)
        return RTSP_RET_ERR_PARAM;

    rtsp_statis_cb = func;
    cb_interval_time = seconds;

    return 0;
}



int  SetNovideoImage(const char* h264fname)
{
	FILE *fp_h264 = NULL;
    int len = 0;

    if(h264fname == NULL)
        return RTSP_RET_ERR_PARAM;
    
    fp_h264 = fopen(h264fname, "rb");
    if(fp_h264 == NULL)
        return RTSP_RET_ERR_PARAM;

    fseek(fp_h264, 0L, SEEK_END);
    len = ftell(fp_h264);
    rewind(fp_h264);

    if(p_no_video_buf)
        free(p_no_video_buf);

    p_no_video_buf = (unsigned char *)malloc(len);
    len_no_video_buf = fread(p_no_video_buf, 1, len, fp_h264);

    fclose(fp_h264);
    log_info("set no video file %s", h264fname);
    return 0;
}

int SetLogOutput(FILE* fp, int loglevel)
{
    if(fp == NULL)
        return RTSP_RET_ERR_PARAM;

    log_set_fp(fp);
    
    log_set_level(loglevel);

    return 0;
}



int DelRtspSource(const char* srcuuid)
{
    rtsp_client *client = NULL;
    list_rtsp_client *client_list = NULL;
    struct list_head *pos, *n;
    int flags = 0;

	if(srcuuid == NULL)
        return RTSP_RET_ERR_PARAM;

    client_list = get_globle_list();
    mutex_lock(&(client_list->lock));
    list_for_each_safe(pos, n, &(client_list->list))
    {
        client = list_entry(pos, rtsp_client, list);
        if(strcmp(client->uuid, srcuuid) == 0)
        {
            flags = 1;
            client->is_running = 0;
            list_del(pos);
            break;
        }
    }
    mutex_unlock(&(client_list->lock));

    if(flags == 0)
        return RTSP_RET_ERR_NOT_FOUND;
   
    
    log_info("del an rtsp client %s:%s", srcuuid, client->uri);
    return 0;
}


void DestoryRtspIpc()
{
    rtsp_client *client = NULL;
    list_rtsp_client *client_list = NULL;
    struct list_head *pos, *n;
    int flags = 0;

    client_list = get_globle_list();
    mutex_lock(&(client_list->lock));
    list_for_each_safe(pos, n, &(client_list->list))
    {
        client = list_entry(pos, rtsp_client, list);
        client->is_running = 0;
    }
    mutex_unlock(&(client_list->lock));

    sleep(3);
    free(p_no_video_buf);
    
    log_info("rtsp library is exit...............!!!");
}




int GetErrorCode(const int ncode, char** info)
{
    switch(ncode)
    {
        case RTSP_RET_ERR_PARAM:
            *info = "Function param error!";
            break;
        case RTSP_RET_ERR_DUP_UUID:
            *info = "Srcuuid duplicate!";
            break;
        case RTSP_RET_ERR_URI:
            *info = "Can`t parse rtsp uri!";
            break;
        case RTSP_RET_ERR_RTSP_MSG:
            *info = "again rtsp msg error,again send!";
            break;
        case RTSP_RET_ERR_REVC_RESP:
            *info = "ERROR";
            break;
        case RTSP_RET_ERR_AUTH_FAIL:
            *info = "ERROR";
            break;
        case RTSP_RET_ERR_NOT_FOUND:
            *info = "Not found the srcuuid!";
            break;
        case RTSP_RET_ERR_NO_RTP:
            *info = "Can`t receive rtp packet!";
            break;
        case RTSP_RET_ERR_CONNECT:
            *info = "Again connect rstp server!";
            break;
    }
    
    
    return 0;
}