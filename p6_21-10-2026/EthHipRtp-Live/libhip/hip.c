/* ==========================================================================
 * libhip — HiP (Hi Protocol) layer implementation
 * ========================================================================*/

#include <stdio.h>
#include <string.h>

#include "hip.h"

/* Build a 4-byte HiP header at buf.
 *
 * Layout (network byte order, MSB first):
 *   byte 0: dst
 *   byte 1: src
 *   byte 2: len  (sdu_len_bytes rounded up to 4-byte words)
 *   byte 3: (version << 4) | type
 *
 * Returns HIP_HDR_LEN. */
size_t hip_build_hdr(uint8_t *buf, uint8_t src, uint8_t dst, uint8_t type,
		     size_t sdu_len_bytes)
{
	buf[0] = dst;
	buf[1] = src;
	size_t words = hip_pad(sdu_len_bytes) / 4;
	buf[2] = (uint8_t)(words > 255 ? 255 : words);
	buf[3] = (uint8_t)((HIP_VERSION << 4) | (type & 0x0f));

	return HIP_HDR_LEN;
}

/* Parse the 4-byte HiP header from buf.
 * buf must point to the first byte *after* the Ethernet header.
 * remaining is the number of bytes available from buf onward.
 * Returns 0 on success, -1 on error. */
int hip_parse_hdr(const uint8_t *buf, size_t remaining, struct hip_hdr *out)
{
	uint8_t ver_type;

	if (remaining < HIP_HDR_LEN) {
		fprintf(stderr, "[hip] header truncated\n");
		return -1;
	}

	out->dst     = buf[0];
	out->src     = buf[1];
	out->len     = buf[2];
	ver_type     = buf[3];
	out->version = ver_type >> 4;
	out->type    = ver_type & 0x0f;

	/* Validate the claimed SDU length */
	if (HIP_HDR_LEN + (size_t)out->len * 4 > remaining) {
		fprintf(stderr, "[hip] SDU length overflows frame\n");
		return -1;
	}

	return 0;
}

void hip_print_hdr(const struct hip_hdr *h)
{
	printf("[hip] src=%u dst=%u len=%u words version=%u type=0x%x\n",
	       h->src, h->dst, h->len, h->version, h->type);
}
