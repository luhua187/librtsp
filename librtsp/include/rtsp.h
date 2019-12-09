#ifndef __RTSP__H__
#define __RTSP__H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

enum{

	RTSP_RET_SUCCESS  = 0,


	RTSP_RET_ERR_PARAM = -10,
	RTSP_RET_ERR_DUP_UUID = -11,
	RTSP_RET_ERR_URI = -12,
	RTSP_RET_ERR_RTSP_MSG = -13,
	RTSP_RET_ERR_REVC_RESP = -14,
	RTSP_RET_ERR_AUTH_FAIL = -15,
    RTSP_RET_ERR_NOT_FOUND = -16,
    RTSP_RET_ERR_REVC_TIMEOUT = -17,
    RTSP_RET_ERR_CONNECT = -18,
    RTSP_RET_ERR_NO_RTP  = -19,


    
};


enum{
    H264_FRAME_P = 0,
    H264_FRAME_I = 5,
    H264_FRAME_SPS = 7,
    H264_FRAME_PPS = 8,
};


enum ipc_type{
    IPC_TYPE_UNKNOW = 0,
    IPC_TYPE_VHD  = 1,
    
};


typedef struct _RtspSrcStatistics{
    char srcuuid[32];
	time_t secondFrom;  
	time_t duration;    
	int bytes;       
	int frames;      
	int iframes;     
	int ptktotal;    
	int pktlosts;    
	int jitter;      
	int width;       
	int height;      
}RtspSrcStatistics;


typedef void (*onErrorCallback)(char *srcuuid, int error_num);
typedef void (*onFramebufCallback)(const unsigned char *frame, int len, int frame_type, void *user_param);
typedef void (*onStatisticsCallback)(char *buf, int buf_len);


int  GetErrorCode(const int ncode, char** info);

int GetIframeOfH264Output(const char* srcuuid);

int GetRtspSourceStatus(const char* srcuuid, int* status);


int InitRtspIpc();

int SetRtspIpcRun();

int AddRtspSource(const char* rtspurl, const char* srcuuid, enum ipc_type type);

int SetRtspSrcOfH264Output(const char* srcuuid, onFramebufCallback func, void *user_param);

int  SetNovideoImage(const char* h264fname);

int SetLogOutput(FILE* fp, int loglevel);

int SetErrorCallback(onErrorCallback func);

int SetStatisticsCallback(onStatisticsCallback func, int seconds);

int DelRtspSource(const char* srcuuid);

void DestoryRtspIpc();



#ifdef __cplusplus
}
#endif
#endif
