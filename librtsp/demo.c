#include <stdio.h>
#include <time.h>
#include "rtsp.h"

FILE *fp = NULL, *fp2 = NULL;
int flag = 0;

void OnRtspIpcErrorCallback(char *srcuuid, int errorcode)
{
	switch (errorcode)
	{
		case RTSP_RET_ERR_NOT_FOUND:
			printf(" %s send error\n", srcuuid);
			//todo
			break;
		case RTSP_RET_ERR_REVC_RESP:
			printf("%s timeout\n", srcuuid);
			//todo
			break;
		case RTSP_RET_ERR_AUTH_FAIL:
			printf("%s auth error\n", srcuuid);
			//todo
			break;
		default:
			break;
	}

    printf("ERROR ------- [%d]\n", errorcode);
}

void OnFrameCallback(const unsigned char* framebuf, int buflen, int frametype, void *user_param)
{
    char start[4] = {0, 0, 0, 1};

	switch(frametype)
	{
		case H264_FRAME_I:
            break;
		case H264_FRAME_PPS:
		case H264_FRAME_SPS:
            break;
		case H264_FRAME_P:
			break;
        case 9:
            return;
	}

    fwrite(start, 4, 1, fp);
    fwrite(framebuf, buflen, 1, fp);
}



void OnFrameCallback22(const unsigned char* framebuf, int buflen, int frametype, void *user_param)
{
    char start[4] = {0, 0, 0, 1};


	switch(frametype)
	{
		case H264_FRAME_I:
            break;
		case H264_FRAME_PPS:
            break;
		case H264_FRAME_SPS:
            break;
		case H264_FRAME_P:
			break;
        case 9:
            return;
	}

    fwrite(start, 4, 1, fp);
    fwrite(framebuf, buflen, 1, fp);
}


void OnRtspStatisticsCallback(char* outbuf, int buflen)
{
	RtspSrcStatistics *p = (RtspSrcStatistics *)outbuf;

	printf("srcuuid:[%s] frames:[%d] jitt:[%d]....\n",p->srcuuid, p->frames, p->jitter); 
}


int main()
{
	char *err_string;
	int ret = 0, running = 1;
	char user_cmd[64];
	int status;
    FILE *fd_log = NULL;
    time_t olddd, newww;

    fp = fopen("1.h264", "wb+");
    fp2 = fopen("2.h264", "wb+");
    fd_log = fopen("log.log", "at");

	ret = InitRtspIpc();
	if(ret < 0)
	{
		GetErrorCode(ret, &err_string);
		printf("%s", err_string);
	}

    SetLogOutput(fd_log, 2);
	SetErrorCallback(OnRtspIpcErrorCallback);
	SetStatisticsCallback(OnRtspStatisticsCallback, 5);
	SetNovideoImage("/data/wuluhua/librtsp/librtsp/novideo.h264");
	
#if 0
    ret = AddRtspSource("rtsp://admin:admin@192.168.1.133/main/av_stream", "0",  IPC_TYPE_UNKNOW);
	if(ret < 0)
	{
		GetErrorCode(ret, &err_string);
		printf("%s", err_string);
	}
    SetRtspSrcOfH264Output("0", OnFrameCallback, NULL);
#endif

#if 1 
   	//ret = AddRtspSource("rtsp://192.168.1.99", "1", IPC_TYPE_VHD);
   	ret = AddRtspSource("rtsp://192.168.1.222:8554/b.264", "1", IPC_TYPE_UNKNOW);
	if(ret < 0)
	{
		GetErrorCode(ret, &err_string);
		printf("%s", err_string);
	}
    SetRtspSrcOfH264Output("1", OnFrameCallback22, NULL);
#endif 
    

	SetRtspIpcRun();

    sleep(3);
    olddd = time(NULL);
	while(running)
	{
	    continue;

	}


	DestoryRtspIpc();

}
