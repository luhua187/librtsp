#ifndef __SDP__h__
#define __SDP__h__

#include "list.h"
#include "rtsp_def.h"

typedef struct _rtsp_sdp{

	unsigned int addr;

	struct list_head media_list;

}rtsp_sdp;



int rtsp_uri_parse(rtsp_client *client, const char *uri);
int rtsp_msg_fl_parse(rtsp_client *client, const char *msg);
int rtsp_msg_parse(rtsp_client *client, const char *msg, int msg_type);






#endif