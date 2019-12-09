#ifndef __RTSP_DEF__H__
#define __RTSP_DEF__H__

#include "rtsp_os.h"
#include "list.h"
#include "rtsp.h"
#include "sdp.h"

#define RECV_BUF_LEN 4096
#define RTP_BUF_LEN  1024 * 64


enum {

    RTSP_MSG_OPTIONS,
    RTSP_MSG_DESCRIBE,
    RTSP_MSG_SETUP,
    RTSP_MSG_PLAY,
    RTSP_MSG_PAUSE,
    RTSP_MSG_TEARDOWN,
    RTSP_MSG_GET_PARAMETER,   

};

enum
{
    RTSP_CMD_CMD = 0,
    RTSP_CMD_ADD_URI,
    RTSP_CMD_GET_I_F,
    RTSP_CMD_DEL_URI,
    
        
};

enum
{
    MEDIA_TYPE_AUDIO = 1,
    MEDIA_TYPE_VEDIO,
};


enum NaluType_e
{
	NALU_TYPE_SLICE = 1,
	NALU_TYPE_DPA = 2,
	NALU_TYPE_DPB = 3,
	NALU_TYPE_DPC = 4,
	NALU_TYPE_IDR = 5,
	NALU_TYPE_SEI = 6,
	NALU_TYPE_SPS = 7,
	NALU_TYPE_PPS = 8,
	NALU_TYPE_AUD = 9,
	NALU_TYPE_EOSEQ = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL = 12,

	NALU_TYPE_PREFIX = 14,
	NALU_TYPE_SUB_SPS = 15,
	NALU_TYPE_SLC_EXT = 20,
	NALU_TYPE_VDRD = 24  // View and Dependency Representation Delimiter NAL Unit

};


typedef struct _frame_cache
{
    unsigned char sps_frame[1024];
    int sps_len;
    unsigned char pps_frame[1024];
    int pps_len;
    unsigned char i_frame[RTP_BUF_LEN];
    int i_len;
}frame_cache;


typedef struct _rtsp_session
{
    int rtp_fd;
    int rtcp_fd;
    int client_rtp_port;
    int client_rtcp_port;
    int server_rtp_port;
    int server_rtcp_port;
    unsigned int ssrc;

    unsigned int last_recv;
    unsigned int last_timestamp;
    unsigned int time_zero;
    unsigned int last_sts;
    unsigned int last_rts;
    double       jitter;

    int last_seq;

    time_t  last_iframe_time;

    unsigned int is_frame_end:1;

    char rtp_slice_buf[1024*128];
    int  slice_len;
    
}rtsp_session;

typedef struct _rtsp_client{

	char *uri;
    char *username;
    char *passwd;
    char *uuid;
    char *ipaddr;
    
    char *base_uri;    
    char *nonce;
    char *realm;
    char *md5;
    char *session_id;

    unsigned int ipc_type;
    
    int timeout;
	int  cseq;
    int  port;
    
	int sig_fd;

	unsigned int is_tcp:1; 
    unsigned int ser_is_tcp:1;
    unsigned int is_auth:2;  // 0:no auth 1:basic auth 2:digest auth
    unsigned int is_running:1;
    unsigned int need_reinit:1;
    unsigned int get_i_f:1;
    unsigned int is_start_cb:1;
    int is_active;
    
    

	thread_t thread_id;
    time_t last_keeplive_time;

    void *rtp_decode;

    onFramebufCallback frame_cb;
    struct sdp_payload *sdp;
    rtsp_session *v_sess;
    rtsp_session *a_sess;
    

    RtspSrcStatistics *v_rtsp_st;
    RtspSrcStatistics *a_rtsp_st;

    //frame_cache _frame;
    void *user_param;
    
	struct list_head list;
}rtsp_client;


typedef struct _rtsp_list{
    struct list_head list;
    mutex_t          lock;
}list_rtsp_client;


int rtsp_options_msg(rtsp_client *client);
int rtsp_describe_msg(rtsp_client *client);
int rtsp_describe_auth_msg(rtsp_client *client);
int rtsp_setup_msg(rtsp_client *client, int media_type);
int rtsp_play_msg(rtsp_client *client);
int rtsp_teardown_msg(rtsp_client *client);
int rtsp_pause_msg(rtsp_client *client);
int rtsp_get_parameter_msg(rtsp_client *client);

int md5sum_response(rtsp_client *client, char *cmd, char *uri, unsigned char *response);
int base64_response(rtsp_client *client, char *response);



#endif