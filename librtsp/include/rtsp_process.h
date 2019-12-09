#ifndef __RTSP_PROCESS__H__
#define __RTSP_PROCESS__H__

#include "rtsp_def.h"


extern list_rtsp_client* get_globle_list();

#if defined (WIN32) || defined(_WIN32)
DWORD WINAPI thread_handle_rtsp(void *param);
#else
void * thread_handle_rtsp(void *param);
#endif


void free_client_node(rtsp_client *client);
void reinit_client_node(rtsp_client *client);
void reinit_rtp_statisc(rtsp_client *client);


#endif