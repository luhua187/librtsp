#include <math.h>
#include "rtsp_os.h"
#include "rtp.h"
#include <math.h>
#include "rtp-payload.h"
#include "rtsp_def.h"
#include "log.h"
#include "get_frame.h"
#include "comm_func.h"

unsigned int u(unsigned int BitCount, unsigned char* buf, unsigned int *nStartBit)
{
	unsigned int dwRet = 0;
    unsigned int i;

	for (i = 0; i < BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[*nStartBit / 8] & (0x80 >> (*nStartBit % 8)))
		{
			dwRet += 1;
		}

		*nStartBit = *nStartBit + 1;
	}

	return dwRet;
}

unsigned int Ue(unsigned char *pBuff, unsigned int nLen, unsigned int *nStartBit)
{
	unsigned int nZeroNum = 0;
    unsigned int dwRet = 0, i;
    
	while (*nStartBit < nLen * 8)
	{
		if (pBuff[*nStartBit / 8] & (0x80 >> (*nStartBit % 8)))
			break;

		nZeroNum++;
		*nStartBit = *nStartBit + 1;
	}
	*nStartBit = *nStartBit + 1;
    
	
	for (i = 0; i < nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[*nStartBit / 8] & (0x80 >> (*nStartBit % 8)))
		{
			dwRet += 1;
		}

		*nStartBit = *nStartBit + 1;
	}

	return (1 << nZeroNum) - 1 + dwRet;
}

int Se(unsigned char *pBuff, unsigned int nLen, unsigned int *nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	int nValue = (int)ceil((double)UeVal / 2);
	if (0 == UeVal % 2)
		nValue = -nValue;

	return nValue;
}

void h264_sps_parser(const unsigned char* Buf, unsigned int spsLen, int *width, int *height)
{
	unsigned int StartBit = 0;
    int profile, level;
    int forbidden_zero_bit;
	int nal_ref_idc;
    int nal_unit_type;
    int profile_idc;
    unsigned char rbsp_byte[8192];
	int NumBytesInRbsp = 0;
    int i, j;
    int level_idc ;
    int seq_parameter_set_id ;

	int chroma_format_idc;
	int separate_colour_plane_flag;

    int ChromaArrayType;
    int pic_order_cnt_type ;

    int pic_width_in_mbs_minus1 ;
	int pic_height_in_map_units_minus1 ;
	int frame_mbs_only_flag;
    	int frame_crop_left_offset = 0;
	int frame_crop_right_offset = 0;
	int frame_crop_top_offset = 0;
	int frame_crop_bottom_offset = 0;
	int frame_cropping_flag;
    int vui_parameters_present_flag;

    int SubWidthC = 0, SubHeightC = 0;
    int CropUnitX, CropUnitY;

    int iWidth, iHeight;

    unsigned char *spsBuf = (unsigned char *)malloc(spsLen + 1);
    memcpy(spsBuf, Buf, spsLen);
    spsBuf[spsLen] = 0;

	forbidden_zero_bit = u(1, spsBuf, &StartBit);
	nal_ref_idc = u(2, spsBuf, &StartBit);

	if (0 == nal_ref_idc)
	{
		profile = 0;
		return;
	}

	nal_unit_type = u(5, spsBuf, &StartBit);
	if (7 != nal_unit_type)
		return;

	profile_idc = u(8, spsBuf, &StartBit);
	profile = profile_idc;

	if (66 != profile && 77 != profile && 100 != profile)
	{
		profile = 0;
		return;
	}

	

	for (i = 2; i < spsLen; i++)
	{
		if (i + 2 < spsLen && 0x00 == spsBuf[i] && 0x00 == spsBuf[i + 1] && 0x03 == spsBuf[i + 2])
		{
			rbsp_byte[NumBytesInRbsp++] = spsBuf[i];
			rbsp_byte[NumBytesInRbsp++] = spsBuf[i + 1];
			i += 2;
		}
		else
		{
			rbsp_byte[NumBytesInRbsp++] = spsBuf[i];
		}
	}

	StartBit = 0;

	/*int constraint_set0_flag =*/ u(1, rbsp_byte, &StartBit);
	/*int constraint_set1_flag =*/ u(1, rbsp_byte, &StartBit);
	/*int constraint_set2_flag =*/ u(1, rbsp_byte, &StartBit);
	/*int constraint_set3_flag =*/ u(1, rbsp_byte, &StartBit);
	/*int reserved_zero_4bits =*/  u(4, rbsp_byte, &StartBit);

	level_idc = u(8, rbsp_byte, &StartBit);
	level = level_idc;

	seq_parameter_set_id = Ue(rbsp_byte, spsLen, &StartBit);

	chroma_format_idc = 1;
	separate_colour_plane_flag = 0;

	if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128)
	{
        int seq_scaling_matrix_present_flag;
		int seq_scaling_list_present_flag[12];
    
		chroma_format_idc = Ue(rbsp_byte, spsLen, &StartBit);

		if (3 == chroma_format_idc)
			separate_colour_plane_flag = u(1, rbsp_byte, &StartBit);

		/*int bit_depth_luma_minus8 =*/ Ue(rbsp_byte, spsLen, &StartBit);
		/*int bit_depth_chroma_minus8 =*/ Ue(rbsp_byte, spsLen, &StartBit);
		/*int qpprime_y_zero_transform_bypass_flag =*/ u(1, rbsp_byte, &StartBit);

		seq_scaling_matrix_present_flag = u(1, rbsp_byte, &StartBit);
		
		if (seq_scaling_matrix_present_flag)
		{
			for (i = 0; i < (3 != chroma_format_idc ? 8 : 12); i++)
			{
				seq_scaling_list_present_flag[i] = u(1, rbsp_byte, &StartBit);
				if (seq_scaling_list_present_flag[i])
				{
					if (i < 6)
					{
						int lastScale = 8;
						int nextScale = 8;
						for (j = 0; j < 16; j++)
						{
							if (0 != nextScale)
							{
								int delta_scale = Se(rbsp_byte, spsLen, &StartBit);
								nextScale = (lastScale + delta_scale + 256) % 256;
							}
							lastScale = (0 == nextScale) ? lastScale : nextScale;
						}
					}
					else
					{
						int lastScale = 8;
						int nextScale = 8;
						for (j = 0; j < 64; j++)
						{
							if (0 != nextScale)
							{
								int delta_scale = Se(rbsp_byte, spsLen, &StartBit);
								nextScale = (lastScale + delta_scale + 256) % 256;
							}
							lastScale = (0 == nextScale) ? lastScale : nextScale;
						}
					}
				}
			}
		}
	}

	if (0 == separate_colour_plane_flag)
		ChromaArrayType = chroma_format_idc;
	else
		ChromaArrayType = 0;

	/*int log2_max_frame_num_minus4 =*/ Ue(rbsp_byte, spsLen, &StartBit);

	pic_order_cnt_type = Ue(rbsp_byte, spsLen, &StartBit);

	if (0 == pic_order_cnt_type)
	{
		/*int log2_max_pic_order_cnt_lsb_minus4 =*/ Ue(rbsp_byte, spsLen, &StartBit);
	}
	else if (1 == pic_order_cnt_type)
	{
	    int num_ref_frames_in_pic_order_cnt_cycle;
        int* offset_for_ref_frame;
		/*int delta_pic_order_always_zero_flag =*/ u(1, rbsp_byte, &StartBit);
		/*int offset_for_non_ref_pic =*/ Se(rbsp_byte, spsLen, &StartBit);
		/*int offset_for_top_to_bottom_field =*/ Se(rbsp_byte, spsLen, &StartBit);

		num_ref_frames_in_pic_order_cnt_cycle = Ue(rbsp_byte, spsLen, &StartBit);

		offset_for_ref_frame = (int *)malloc(num_ref_frames_in_pic_order_cnt_cycle);
		for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
			offset_for_ref_frame[i] = Se(rbsp_byte, spsLen, &StartBit);
		free(offset_for_ref_frame);
	}

	/*int max_num_ref_frames =*/ Ue(rbsp_byte, spsLen, &StartBit);
	/*int gaps_in_frame_num_value_allowed_flag =*/ u(1, rbsp_byte, &StartBit);

	pic_width_in_mbs_minus1 = Ue(rbsp_byte, spsLen, &StartBit);
	pic_height_in_map_units_minus1 = Ue(rbsp_byte, spsLen, &StartBit);

	frame_mbs_only_flag = u(1, rbsp_byte, &StartBit);
	if (!frame_mbs_only_flag)
	{
		/*int mb_adaptive_frame_field_flag =*/ u(1, rbsp_byte, &StartBit);
	}
	/*int direct_8x8_inference_flag =*/ u(1, rbsp_byte, &StartBit);

	frame_crop_left_offset = 0;
	frame_crop_right_offset = 0;
	frame_crop_top_offset = 0;
	frame_crop_bottom_offset = 0;
	frame_cropping_flag = u(1, rbsp_byte, &StartBit);
	if (frame_cropping_flag)
	{
		frame_crop_left_offset = Ue(rbsp_byte, spsLen, &StartBit);
		frame_crop_right_offset = Ue(rbsp_byte, spsLen, &StartBit);
		frame_crop_top_offset = Ue(rbsp_byte, spsLen, &StartBit);
		frame_crop_bottom_offset = Ue(rbsp_byte, spsLen, &StartBit);
	}

	vui_parameters_present_flag = u(1, rbsp_byte, &StartBit);
	if (vui_parameters_present_flag)
	{
		// vui_parameters();
	}

	SubWidthC = 0;
	SubHeightC = 0;

	if (0 == separate_colour_plane_flag)
	{
		if (1 == chroma_format_idc)
		{
			SubWidthC = 2;
			SubHeightC = 2;
		}
		else if (2 == chroma_format_idc)
		{
			SubWidthC = 2;
			SubHeightC = 1;
		}
		else if (3 == chroma_format_idc)
		{
			SubWidthC = 1;
			SubHeightC = 1;
		}
	}

	if (0 == ChromaArrayType)
	{
		CropUnitX = 1;
		CropUnitY = 2 - frame_mbs_only_flag;
	}
	else
	{
		CropUnitX = SubWidthC;
		CropUnitY = SubHeightC * (2 - frame_mbs_only_flag);
	}

	iWidth = (pic_width_in_mbs_minus1 + 1) * 16 - CropUnitX*(frame_crop_left_offset + frame_crop_right_offset);
	iHeight = (pic_height_in_map_units_minus1 + 1) * 16 - CropUnitY*(frame_crop_top_offset + frame_crop_bottom_offset);


	if (0 != iWidth && 0 != iHeight)
	{
		*width = iWidth;
		*height = iHeight;
	}

    free(spsBuf);
}

inline unsigned int srt_to_int(const unsigned char * ptr)
{
	return (((unsigned int)ptr[0]) << 24) | (((unsigned int)ptr[1]) << 16) | (((unsigned int)ptr[2]) << 8) | ptr[3];
}

void rtp_receiver_h264(void* param, const void *packet, int bytes, unsigned int timestamp, int flags)
{
    rtsp_client *client = (rtsp_client *)param;
    const unsigned char * p = (const unsigned char *)packet;
    int nalu_type = (int)(p[0] & 0x1F);
    int ret = 0;

    if(client->frame_cb == NULL)
        return;

    switch(nalu_type)
    {
        case NALU_TYPE_SLICE:
            break;
	    case NALU_TYPE_DPA:
            break;
	    case NALU_TYPE_DPB:
            break;
        case NALU_TYPE_SEI:
            break;
	    case NALU_TYPE_SPS:
            if(client->v_rtsp_st->width==0 || client->v_rtsp_st->height==0)
                h264_sps_parser(p, bytes, &client->v_rtsp_st->width, &client->v_rtsp_st->height);
            client->is_start_cb = 1;
            break;
	    case NALU_TYPE_PPS:
            break;
        case NALU_TYPE_IDR:
            client->v_rtsp_st->iframes++;
            client->is_start_cb = 1;
            break;
    }
    
    client->v_rtsp_st->frames++;

    ret = rtp_slice_packet(client, packet, bytes, nalu_type);

    if(client->is_start_cb && ret) 
        client->frame_cb(p,  bytes, nalu_type, client->user_param);
    
}


int create_rtp_decoder(rtsp_client *client)
{
    struct rtp_payload_t my_handler;
    my_handler.alloc  = NULL;
    my_handler.free   = NULL;
    my_handler.packet = rtp_receiver_h264;

    client->rtp_decode = rtp_payload_decode_create(97, "H264", &my_handler, client);

    return client->rtp_decode?0:-1;
}


void add_rtp_packet(rtsp_client *client, char *packet, int packet_len)
{
    unsigned char *p = packet;
    unsigned int v = srt_to_int(p);

    client->v_sess->is_frame_end = RTP_M(v);
    
    rtp_payload_decode_input(client->rtp_decode, packet, packet_len);
}

inline void get_rtp_seq(char *packet, int *seq)
{
    unsigned char *p = packet;
    unsigned int v = srt_to_int(p);

    *seq        = RTP_SEQ(v);
}

inline int get_seq_distance(int old_seq, int new_seq)
{
    return (int)(new_seq + 65536 - old_seq) % 65536;
}


int rtp_packet_lost_check(rtsp_client *client, char *packet, int media_type)
{
    int new_seq;
    int sub = 0;
    rtsp_session *sess = NULL;

    if(media_type == MEDIA_TYPE_VEDIO)
        sess = client->v_sess;
    else
        sess = client->a_sess;    

    get_rtp_seq(packet, &new_seq);

    if(sess->last_seq == -1)
    {
        sess->last_seq = new_seq;
        return 0;
    }

    sub = get_seq_distance(sess->last_seq, new_seq);        

    if(sub == 1)
    {
        sess->last_seq = new_seq;
    }
    else if(sub <= 0)
    {
        return 0;
    }
    else
    {
        
        if(media_type == MEDIA_TYPE_VEDIO)
        {
            client->v_rtsp_st->pktlosts = sub-1;
            //get_ipc_i_frame(client);
            sess->last_seq = new_seq;
        }
        else
        {
            
        }
    }       


    return 0;
}

int rtp_calc_jitter(rtsp_client *client, char *packet, int media_type)
{
    rtsp_session *sess = NULL;
    int new_seq = 0;
    int fac = 90;
	unsigned int sts, rts;
    int diff;
    
    
	if(media_type == MEDIA_TYPE_VEDIO)
        sess = client->v_sess;
    else
        sess = client->a_sess;

    if(sess->time_zero == 0)
        sess->time_zero = get_ticks();

    get_rtp_seq(packet, &new_seq);

    if(sess->last_seq+1 != new_seq)
        return 0;

    
	sts = srt_to_int(packet+4);
	rts = (get_ticks() - sess->time_zero)*fac;
	if (0 != sess->last_sts && 0xFFFFFFFF != sess->time_zero)
	{
		diff = (int)((rts - sess->last_rts) - (sts - sess->last_sts));
		sess->jitter += (((double)abs(diff) - sess->jitter) / 16.0);
	}
	sess->last_sts = sts;
	sess->last_rts = sts;

    return (int)(sess->jitter / fac + 0.5);
}


int rtp_slice_packet(rtsp_client *client, const void *packet, int bytes, int packet_type)
{
    unsigned char start_code[3] = {0, 0, 1};
    rtsp_session *sess = client->v_sess;
    char *p = sess->rtp_slice_buf;

    if(packet_type > NALU_TYPE_IDR)
        return 1;

    if(sess->is_frame_end == 0)
    {    
        if(sess->slice_len > 0)
        {            
            memcpy(p + sess->slice_len, start_code, 3);
            sess->slice_len += 3;
            memcpy(p + sess->slice_len, packet, bytes);
            sess->slice_len += bytes;
        }
        else
        {
            memcpy(p + sess->slice_len, packet, bytes);
            sess->slice_len += bytes;
        }

        return 0;
    }
    else
    {
        if(sess->slice_len == 0)
            return 1;
        else
        {
            int type = (int)(*p & 0x1F);

            memcpy(p + sess->slice_len, start_code, 3);
            sess->slice_len += 3;
            memcpy(p + sess->slice_len, packet, bytes);
            sess->slice_len += bytes;

            if(client->is_start_cb && client->frame_cb) 
                client->frame_cb(p,  sess->slice_len, type, client->user_param);

            sess->slice_len = 0;
                 
            return 0;
        } 
    }

    return 1;
}
