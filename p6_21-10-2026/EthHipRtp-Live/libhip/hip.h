/* ==========================================================================
 * libhip — HiP (Hi Protocol) layer
 *
 * Wire format (4 bytes, sits right after the Ethernet header):
 *   +---------+---------+---------+-------------------+
 *   | dst (1) | src (1) | len (1) | version(4)|type(4)|
 *   +---------+---------+---------+-------------------+
 *     len = size of the SDU (payload) in 4-byte words
 *
 * The SDU is whatever the upper layer hands down — for RTP-over-HiP that
 * is the RTP header + file chunk, zero-padded to a multiple of 4 bytes.
 * ========================================================================*/

#ifndef HIP_H
#define HIP_H

#include <stdint.h>
#include <stddef.h>

#define HIP_HDR_LEN   4
#define HIP_VERSION   4

/* HiP packet types */
#define HIP_TYPE_DATA 0x1             /* carries an upper-layer SDU      */
#define HIP_TYPE_ACK  0x2             /* carries an upper-layer ACK      */

#define HIP_BROADCAST 0xff

/* Parsed representation of a HiP header (not the wire layout) */
struct hip_hdr {
	uint8_t dst;
	uint8_t src;
	uint8_t len;      /* SDU length in 4-byte words */
	uint8_t version;
	uint8_t type;
};

/* --- header build / parse --- */

/* Writes 4-byte HiP header into buf.  sdu_len_bytes is the upper-layer
 * payload size (will be rounded up to next multiple of 4 internally).
 * Returns HIP_HDR_LEN. */
size_t hip_build_hdr(uint8_t *buf, uint8_t src, uint8_t dst, uint8_t type,
		     size_t sdu_len_bytes);

/* Parses a HiP header from buf (which must start right after the Ethernet
 * header).  Returns 0 on success, -1 on malformed packet. */
int hip_parse_hdr(const uint8_t *buf, size_t remaining, struct hip_hdr *out);

/* --- helper --- */
void hip_print_hdr(const struct hip_hdr *h);

/* Pad sdu_len up to the next multiple of 4 (HiP alignment rule). */
static inline size_t hip_pad(size_t sdu_len)
{
	if (sdu_len % 4 == 0)
		return sdu_len;
	return sdu_len + (4 - sdu_len % 4);
}

#endif /* HIP_H */
