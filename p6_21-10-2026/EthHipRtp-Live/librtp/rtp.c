/* ==========================================================================
 * librtp — Reliable Transfer Protocol implementation
 *
 * SKELETON FOR LIVE CODING
 *
 * Encapsulation order when sending (top-down):
 *   RTP layer: read chunk -> build RTP header  -> SDU = RTP hdr + chunk
 *   HIP layer: take SDU  -> build HIP header   -> SDU = HIP hdr + prev SDU
 *   ETH layer: take SDU  -> build ETH header   -> frame = ETH hdr + prev SDU
 *
 * Decapsulation when receiving (bottom-up):
 *   ETH layer: strip ETH header, check ethertype
 *   HIP layer: strip HIP header, check length / type
 *   RTP layer: strip RTP header, process data or ACK
 * ========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "../libeth/eth.h"
#include "../libhip/hip.h"
#include "rtp.h"

/* --------------------------------------------------------------------------
 * Header build / parse (Pre-written boilerplate)
 *
 * Wire layout (6 bytes):
 *   byte 0    : (version << 4) | type
 *   bytes 1-3 : seq_num, big-endian, 3 bytes
 *   bytes 4-5 : payload_len, big-endian, 2 bytes
 * ------------------------------------------------------------------------*/

size_t rtp_build_hdr(uint8_t *buf, uint8_t type,
                     uint32_t seq, uint16_t payload_len)
{
	buf[0] = (uint8_t)((RTP_VERSION << 4) | (type & 0x0f));
	buf[1] = (seq >> 16) & 0xff;
	buf[2] = (seq >>  8) & 0xff;
	buf[3] =  seq        & 0xff;
	buf[4] = (payload_len >> 8) & 0xff;
	buf[5] =  payload_len       & 0xff;

	return RTP_HDR_LEN;
}

int rtp_parse_hdr(const uint8_t *buf, size_t remaining, struct rtp_hdr *out)
{
	if (remaining < RTP_HDR_LEN) {
		fprintf(stderr, "[rtp] header truncated\n");
		return -1;
	}

	out->version     = (buf[0] >> 4) & 0x0f;
	out->type        =  buf[0]       & 0x0f;
	out->seq_num     = ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8)
		| (uint32_t)buf[3];
	out->payload_len = ((uint16_t)buf[4] << 8) | (uint16_t)buf[5];

	return 0;
}

/* --------------------------------------------------------------------------
 * TODO 1 => DONE: Encapsulation (send_frame)
 *
 * Assemble full frame: [Ethernet (14B)] [HiP (4B)] [RTP (6B)] [Payload]
 *
 * Hint:
 * 1. Calculate sdu_len for HiP = RTP_HDR_LEN + payload_len.
 * 2. Ethernet header: eth_build_hdr(buf + off, eth->addr[0].sll_addr, dst_mac);
 * 3. HiP header:      hip_build_hdr(buf + off, src_hip, dst_hip, hip_type, sdu_len);
 * 4. RTP header:      rtp_build_hdr(buf + off, rtp_type, seq, payload_len);
 * 5. Copy payload:    memcpy(buf + off, payload, payload_len);
 * 6. Pad with 0s to:  ETH_HDR_LEN + HIP_HDR_LEN + hip_pad(sdu_len);
 * 7. Send:            eth_send(eth, buf, off);
 * ------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------
 * Internal: build and send one full frame (ETH + HIP + RTP + payload)
 * ------------------------------------------------------------------------*/
static int send_frame(struct eth_node *eth,
                      uint8_t src_hip, uint8_t dst_hip,
                      const uint8_t *dst_mac,
                      uint8_t rtp_type, uint32_t seq,
                      const uint8_t *payload, uint16_t payload_len)
{
	/* Total SDU seen by HIP = RTP header + payload */
	size_t sdu_len = RTP_HDR_LEN + payload_len;

	/* Full frame buffer */
	uint8_t buf[ETH_HDR_LEN + HIP_HDR_LEN + hip_pad(sdu_len)];
	size_t  off = 0;

	/* --- Ethernet header --- */
	off += eth_build_hdr(buf + off, eth->addr[0].sll_addr, dst_mac);

	/* --- HIP header --- */
	uint8_t hip_type = (rtp_type == RTP_TYPE_ACK) ? HIP_TYPE_ACK : HIP_TYPE_DATA;
	off += hip_build_hdr(buf + off, src_hip, dst_hip,
	                     hip_type, sdu_len);

	/* --- RTP header --- */
	off += rtp_build_hdr(buf + off, rtp_type, seq, payload_len);

	/* --- payload --- */
	if (payload_len > 0 && payload) {
		memcpy(buf + off, payload, payload_len);
		off += payload_len;
	}

	/* --- zero-pad to HIP alignment --- */
	size_t expected = ETH_HDR_LEN + HIP_HDR_LEN + hip_pad(sdu_len);
	while (off < expected)
		buf[off++] = 0;

	return eth_send(eth, buf, off);
}

/* --------------------------------------------------------------------------
 * Sender Initialization
 * ------------------------------------------------------------------------*/

int sender_init(struct sender_state *s, const char *filename)
{
	memset(s, 0, sizeof(*s));

	s->fp = fopen(filename, "rb");
	if (!s->fp) {
		perror("fopen");
		return -1;
	}

	return 0;
}

/* --------------------------------------------------------------------------
 * TODO 3: Send Next Packet (Sliding Window Producer)
 *
 * 1. Check if window is full: if (s->window_size >= WINDOW_SIZE) return 0;
 * 2. Read next chunk into &s->window[s->window_size] using fread().
 *    If 0 bytes read -> EOF -> return 0.
 * 3. Store len and seq_num in the slot:
 *    slot->len = bytes;
 *    slot->seq_num = s->next_seq;
 * 4. Call send_frame(...) with RTP_TYPE_DATA.
 * 5. Update state: s->next_seq++; s->window_size++;
 * 6. Return 1 (packet sent).
 * ------------------------------------------------------------------------*/
int sender_send_next(struct eth_node *eth,
                     struct sender_state *s,
                     uint8_t src_hip, uint8_t dst_hip,
                     const uint8_t *dst_mac)
{
	/* TODO 3: Implement sending the next packet from file */
	(void)eth; (void)s; (void)src_hip; (void)dst_hip; (void)dst_mac;

	return 0;
}

/* --------------------------------------------------------------------------
 * TODO 4: Handle Cumulative ACK (Slide the Window Forward)
 *
 * When an ACK arrives for ack_seq:
 * 1. Check if ack_seq is valid: ack_seq >= s->base && ack_seq < s->next_seq
 * 2. Calculate num_acked = ack_seq - s->base + 1;
 * 3. Shift remaining packets in the window:
 *    for (int i = 0; i < s->window_size - num_acked; i++)
 *        s->window[i] = s->window[i + num_acked];
 * 4. Update window_size and base:
 *    s->window_size -= num_acked;
 *    s->base = ack_seq + 1;
 * ------------------------------------------------------------------------*/
void sender_handle_ack(struct sender_state *s, uint32_t ack_seq)
{
	/* TODO 4: Slide window on cumulative ACK */
	(void)s; (void)ack_seq;
}

/* Retransmit every packet currently in the window (Go-Back-N) */
void sender_go_back_n(struct eth_node *eth,
                      struct sender_state *s,
                      uint8_t src_hip, uint8_t dst_hip,
                      const uint8_t *dst_mac)
{
	printf("[rtp] GO-BACK-N retransmitting %d packet(s) from seq=%u\n",
	       s->window_size, s->base);

	for (int i = 0; i < s->window_size; i++) {
		struct rtp_pkt *pkt = &s->window[i];

		send_frame(eth, src_hip, dst_hip, dst_mac,
		           RTP_TYPE_DATA, pkt->seq_num,
		           pkt->data, pkt->len);

		printf("[rtp] RETX seq=%u\n", pkt->seq_num);
	}
}

/* --------------------------------------------------------------------------
 * Receiver Initialization
 * ------------------------------------------------------------------------*/

int receiver_init(struct receiver_state *r, const char *filename)
{
	memset(r, 0, sizeof(*r));

	r->fp = fopen(filename, "wb");
	if (!r->fp) {
		perror("fopen");
		return -1;
	}

	return 0;
}

/* --------------------------------------------------------------------------
 * TODO 5: Receiver Data Handling & ACKing
 *
 * When a packet arrives (buf points to RTP header):
 * 1. Parse RTP header: rtp_parse_hdr(buf, len, &h).
 * 2. If h.seq_num == r->expected_seq (IN-ORDER):
 *    - Write chunk to file: fwrite(buf + RTP_HDR_LEN, 1, h.payload_len, r->fp);
 *    - Flush to disk:       fflush(r->fp);
 *    - Send ACK:            send_frame(eth, local_hip, src_hip, src_mac,
 *                                      RTP_TYPE_ACK, h.seq_num, NULL, 0);
 *    - Advance:             r->expected_seq++;
 * 3. Else (OUT-OF-ORDER):
 *    - Discard chunk and re-ACK last in-order packet (expected_seq - 1):
 *      if (r->expected_seq > 0)
 *          send_frame(eth, local_hip, src_hip, src_mac,
 *                     RTP_TYPE_ACK, r->expected_seq - 1, NULL, 0);
 * ------------------------------------------------------------------------*/
void receiver_handle_data(struct eth_node *eth,
                          struct receiver_state *r,
                          const uint8_t *buf, size_t len,
                          uint8_t local_hip,
                          const uint8_t *src_mac, uint8_t src_hip)
{
	/* TODO 5: Handle in-order vs out-of-order data and send ACK */
	(void)eth; (void)r; (void)buf; (void)len;
	(void)local_hip; (void)src_mac; (void)src_hip;
}
