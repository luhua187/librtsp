#ifndef _rtp_util_h_
#define _rtp_util_h_

#include "rtp-header.h"
#include "rtcp-header.h"

#ifdef __cplusplus
extern "C" {
#endif

uint16_t rtp_read_uint16(const uint8_t* ptr);
uint32_t rtp_read_uint32(const uint8_t* ptr);
void rtp_write_uint16(uint8_t* ptr, uint16_t val);
void rtp_write_uint32(uint8_t* ptr, uint32_t val);

// The Internet Protocol defines big-endian as the standard network byte order
#define nbo_r16 rtp_read_uint16
#define nbo_r32 rtp_read_uint32
#define nbo_w16 rtp_write_uint16
#define nbo_w32 rtp_write_uint32

// uint16_t rtp_read_uint16_2(const uint8_t* ptr)
// {
// 	return rtp_read_uint16(ptr);
// }
// 
// uint32_t rtp_read_uint32_2(const uint8_t* ptr)
// {
// 	return rtp_read_uint32(ptr);
// }
// 
// void rtp_write_uint16_2(uint8_t* ptr, uint16_t val)
// {
// 	rtp_write_uint16(ptr, val);
// }
// 
// void rtp_write_uint32_2(uint8_t* ptr, uint32_t val)
// {
// 	rtp_write_uint32(ptr, val);
// }

void nbo_write_rtp_header(uint8_t *ptr, const rtp_header_t *header);
void nbo_write_rtcp_header(uint8_t *ptr, const rtcp_header_t *header);

#ifdef __cplusplus
}
#endif

#endif /* !_rtp_util_h_ */
