#include <stdint.h>
#include "sockutil.h"
#include "sys/pollfd.h"
#include "sys/thread.h"
#include "rtp-profile.h"
#include "rtp-payload.h"
#include "rtp.h"
#include "time64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(OS_WINDOWS)
#define strcasecmp _stricmp
#endif

struct rtp_context_t
{
	FILE *fp;
	FILE *frtp;

	char encoding[64];
	u_short port[2];
	socket_t socket[2];
	struct sockaddr_storage ss;
	socklen_t len;

	char rtp_buffer[64 * 1024];
	char rtcp_buffer[32 * 1024];

	void* payload;
	void* rtp;
};

static int rtp_read(struct rtp_context_t* ctx, socket_t s)
{
	int r;
	uint8_t size[2];
	static int i, n = 0;
	socklen_t len;
	struct sockaddr_storage ss;
	len = sizeof(ss);

	r = recvfrom(s, ctx->rtp_buffer, sizeof(ctx->rtp_buffer), 0, (struct sockaddr*)&ss, &len);
	if (r < 12)
		return -1;
	assert(AF_INET == ss.ss_family);
	assert(((struct sockaddr_in*)&ss)->sin_port == htons(ctx->port[0]));
	assert(0 == memcmp(&((struct sockaddr_in*)&ss)->sin_addr, &((struct sockaddr_in*)&ctx->ss)->sin_addr, 4));

	n += r;
	if(0 == i++ % 100)
		printf("packet: %d, seq: %u, size: %d/%d\n", i, ((uint8_t)ctx->rtp_buffer[2] << 8) | (uint8_t)ctx->rtp_buffer[3], r, n);
	
	size[0] = r >> 8;
	size[1] = r >> 0;
	fwrite(size, 1, sizeof(size), ctx->frtp);
	fwrite(ctx->rtp_buffer, 1, r, ctx->frtp);

	rtp_payload_decode_input(ctx->payload, ctx->rtp_buffer, r);
	rtp_onreceived(ctx->rtp, ctx->rtp_buffer, r);
	return r;
}

static int rtcp_read(struct rtp_context_t* ctx, socket_t s)
{
	int r;
	socklen_t len;
	struct sockaddr_storage ss;
	len = sizeof(ss);
	r = recvfrom(s, ctx->rtcp_buffer, sizeof(ctx->rtcp_buffer), 0, (struct sockaddr*)&ss, &len);
	if (r < 12)
		return -1;
	assert(AF_INET == ss.ss_family);
	assert(((struct sockaddr_in*)&ss)->sin_port == htons(ctx->port[1]));
	assert(0 == memcmp(&((struct sockaddr_in*)&ss)->sin_addr, &((struct sockaddr_in*)&ctx->ss)->sin_addr, 4));

	r = rtp_onreceived_rtcp(ctx->rtp, ctx->rtcp_buffer, r);
	fflush(ctx->fp);
	return r;
}

static int rtp_receiver(struct rtp_context_t* ctx, socket_t rtp[2], int timeout)
{
	int i, r;
	int interval;
	time64_t clock;
	struct pollfd fds[2];

	for (i = 0; i < 2; i++)
	{
		fds[i].fd = rtp[i];
		fds[i].events = POLLIN;
		fds[i].revents = 0;
	}

	clock = time64_now();
	while (1)
	{
		// RTCP report
		interval = rtp_rtcp_interval(ctx->rtp);
		if (clock + interval < time64_now())
		{
			r = rtp_rtcp_report(ctx->rtp, ctx->rtcp_buffer, sizeof(ctx->rtcp_buffer));
			r = socket_send_all_by_time(rtp[1], ctx->rtcp_buffer, r, 0, timeout);
			clock = time64_now();
		}

		r = poll(fds, 2, timeout);
		while (-1 == r && EINTR == errno)
			r = poll(fds, 2, timeout);

		if (0 == r)
		{
			continue; // timeout
		}
		else if (r < 0)
		{
			return r; // error
		}
		else
		{
			if (0 != fds[0].revents)
			{
				rtp_read(ctx, rtp[0]);
				fds[0].revents = 0;
			}

			if (0 != fds[1].revents)
			{
				rtcp_read(ctx, rtp[1]);
				fds[1].revents = 0;
			}
		}
	}
	return r;
}

static void rtp_packet(void* param, const void *packet, int bytes, uint32_t time, int flags)
{
	const uint8_t start_code[] = { 0, 0, 0, 1 };
	struct rtp_context_t* ctx;
	ctx = (struct rtp_context_t*)param;
	if (0 == strcmp("H264", ctx->encoding) || 0 == strcmp("H265", ctx->encoding))
	{
		fwrite(start_code, 1, 4, ctx->fp);
	}
	else if (0 == strcasecmp("mpeg4-generic", ctx->encoding))
	{
		uint8_t adts[7];
		int len = bytes + 7;
		uint8_t profile = 2;
		uint8_t sampling_frequency_index = 4;
		uint8_t channel_configuration = 2;
		adts[0] = 0xFF; /* 12-syncword */
		adts[1] = 0xF0 /* 12-syncword */ | (0 << 3)/*1-ID*/ | (0x00 << 2) /*2-layer*/ | 0x01 /*1-protection_absent*/;
		adts[2] = ((profile - 1) << 6) | ((sampling_frequency_index & 0x0F) << 2) | ((channel_configuration >> 2) & 0x01);
		adts[3] = ((channel_configuration & 0x03) << 6) | ((len >> 11) & 0x03); /*0-original_copy*/ /*0-home*/ /*0-copyright_identification_bit*/ /*0-copyright_identification_start*/
		adts[4] = (uint8_t)(len >> 3);
		adts[5] = ((len & 0x07) << 5) | 0x1F;
		adts[6] = 0xFC | ((len / 1024) & 0x03);
		fwrite(adts, 1, sizeof(adts), ctx->fp);
	}
	else if (0 == strcmp("MP4A-LATM", ctx->encoding))
	{
		// add ADTS header
	}
	fwrite(packet, 1, bytes, ctx->fp);
	(void)time;
	(void)flags;

	if (0 == strcmp("H264", ctx->encoding))
	{
		uint8_t type = *(uint8_t*)packet & 0x1f;
		if (0 < type && type <= 5)
		{
			// VCL frame
		}
	}
	else if (0 == strcmp("H265", ctx->encoding))
	{
		uint8_t type = (*(uint8_t*)packet >> 1) & 0x3f;
		if (type <= 32)
		{
			// VCL frame
		}
	}
}

static void rtp_on_rtcp(void* param, const struct rtcp_msg_t* msg)
{
	struct rtp_context_t* ctx;
	ctx = (struct rtp_context_t*)param;
	if (RTCP_MSG_BYE == msg->type)
	{
		printf("finished\n");
	}
}

static int STDCALL rtp_worker(void* param)
{
	struct rtp_context_t* ctx;
	ctx = (struct rtp_context_t*)param;

	rtp_receiver(ctx, ctx->socket, 2000);

	rtp_destroy(ctx->rtp);
	rtp_payload_decode_destroy(ctx->payload);
	fclose(ctx->frtp);
	fclose(ctx->fp);
	free(ctx);
	return 0;
}

void rtp_receiver_test(socket_t rtp[2], const char* peer, int peerport[2], int payload, const char* encoding)
{
	size_t n;
	pthread_t t;
	struct rtp_context_t* ctx;
	struct rtp_event_t evthandler;
	struct rtp_payload_t handler;
	const struct rtp_profile_t* profile;

	ctx = malloc(sizeof(*ctx));
	snprintf(ctx->rtp_buffer, sizeof(ctx->rtp_buffer), "%s.%d.%d.%s", peer, peerport[0], payload, encoding);
	snprintf(ctx->rtcp_buffer, sizeof(ctx->rtcp_buffer), "%s.%d.%d.%s.rtp", peer, peerport[0], payload, encoding);
	ctx->fp = fopen(ctx->rtp_buffer, "wb");
	ctx->frtp = fopen(ctx->rtcp_buffer, "wb");

	socket_getrecvbuf(rtp[0], &n);
	socket_setrecvbuf(rtp[0], 256*1024);
	socket_getrecvbuf(rtp[0], &n);

	profile = rtp_profile_find(payload);
	
	handler.alloc = NULL;
	handler.free = NULL;
	handler.packet = rtp_packet;
	ctx->payload = rtp_payload_decode_create(payload, encoding, &handler, ctx);
	if (NULL == ctx->payload)
		return; // ignore
	
	evthandler.on_rtcp = rtp_on_rtcp;
	ctx->rtp = rtp_create(&evthandler, &ctx, (uint32_t)(intptr_t)&ctx, profile ? profile->frequency : 9000, 2*1024*1024);

	assert(0 == socket_addr_from(&ctx->ss, &ctx->len, peer, (u_short)peerport[0]));
	//assert(0 == socket_addr_setport((struct sockaddr*)&ss, len, (u_short)peerport[0]));
	//assert(0 == connect(rtp[0], (struct sockaddr*)&ss, len));
	assert(0 == socket_addr_setport((struct sockaddr*)&ctx->ss, ctx->len, (u_short)peerport[1]));
	//assert(0 == connect(rtp[1], (struct sockaddr*)&ss, len));

	snprintf(ctx->encoding, sizeof(ctx->encoding), "%s", encoding);
	ctx->socket[0] = rtp[0];
	ctx->socket[1] = rtp[1];
	ctx->port[0] = (u_short)peerport[0];
	ctx->port[1] = (u_short)peerport[1];
	if (0 == thread_create(&t, rtp_worker, ctx))
		thread_detach(t);
}

static struct rtp_context_t* ctx;
void* rtp_receiver_tcp_test(uint8_t interleave1, uint8_t interleave2, int payload, const char* encoding)
{
	//struct rtp_context_t* ctx;
	struct rtp_event_t evthandler;
	struct rtp_payload_t handler;
	const struct rtp_profile_t* profile;

	ctx = malloc(sizeof(*ctx));
	snprintf(ctx->rtp_buffer, sizeof(ctx->rtp_buffer), "tcp.%d.%s", payload, encoding);
	snprintf(ctx->rtcp_buffer, sizeof(ctx->rtcp_buffer), "tcp.%d.%s.rtp", payload, encoding);
	ctx->fp = fopen(ctx->rtp_buffer, "wb");

	profile = rtp_profile_find(payload);

	handler.alloc = NULL;
	handler.free = NULL;
	handler.packet = rtp_packet;
	ctx->payload = rtp_payload_decode_create(payload, encoding, &handler, ctx);
	if (NULL == ctx->payload)
		return NULL; // ignore

	evthandler.on_rtcp = rtp_on_rtcp;
	ctx->rtp = rtp_create(&evthandler, &ctx, (uint32_t)(intptr_t)&ctx, profile ? profile->frequency : 9000, 2 * 1024 * 1024);

	snprintf(ctx->encoding, sizeof(ctx->encoding), "%s", encoding);
	return ctx;
}

void rtp_receiver_tcp_input(uint8_t channel, const void* data, uint16_t bytes)
{
	if (0 == channel)
	{
		rtp_payload_decode_input(ctx->payload, data, bytes);
		rtp_onreceived(ctx->rtp, data, bytes);
	}
	else
	{
		rtp_onreceived_rtcp(ctx->rtp, data, bytes);
	}
}
