/* ==========================================================================
 * librtp — Reliable Transfer Protocol layer
 *
 * Wire format (6 bytes, sits right after the HiP header):
 *   +-------------------+---------------------+-----------------------+
 *   | ver(4) | type(4)  | seq_num (3 bytes BE) | payload_len (2 bytes) |
 *   +-------------------+---------------------+-----------------------+
 *     total RTP header = 6 bytes
 *
 * Go-Back-N sliding window with WINDOW_SIZE unacknowledged packets.
 * The receiver sends a cumulative ACK for every in-order chunk.
 * Out-of-order chunks are dropped and the last good seq is re-ACKed.
 * ========================================================================*/

#ifndef RTP_H
#define RTP_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Forward declaration — eth_node lives in libeth */
struct eth_node;

#define RTP_HDR_LEN   6              /* bytes on the wire                */
#define RTP_VERSION   1
#define WINDOW_SIZE   8
#define MAX_CHUNK     1024           /* max file bytes per RTP packet    */

/* Packet types */
#define RTP_TYPE_DATA 0x1
#define RTP_TYPE_ACK  0x2

/* Parsed RTP header */
struct rtp_hdr {
	uint8_t  version;
	uint8_t  type;
	uint32_t seq_num;      /* host byte order after parse    */
	uint16_t payload_len;  /* host byte order after parse    */
};

/* One slot in the sender's retransmission queue */
struct rtp_pkt {
	uint8_t  data[MAX_CHUNK];
	uint16_t len;
	uint32_t seq_num;
};

/* Sender state — all Go-Back-N bookkeeping */
struct sender_state {
	FILE          *fp;
	uint32_t       base;              /* oldest unacked sequence number  */
	uint32_t       next_seq;          /* next sequence number to use     */
	struct rtp_pkt window[WINDOW_SIZE];
	int            window_size;       /* number of packets currently queued */
};

/* Receiver state */
struct receiver_state {
	FILE    *fp;
	uint32_t expected_seq;
};

/* --- header build / parse --- */
size_t rtp_build_hdr(uint8_t *buf, uint8_t type,
                     uint32_t seq, uint16_t payload_len);
int    rtp_parse_hdr(const uint8_t *buf, size_t remaining,
                     struct rtp_hdr *out);

/* --- sender --- */
int  sender_init(struct sender_state *s, const char *filename);
/* Read next chunk from file, add to window, and send.
 * Returns 1 if sent, 0 if window full or EOF, -1 on error. */
int  sender_send_next(struct eth_node *eth,
                      struct sender_state *s,
                      uint8_t src_hip, uint8_t dst_hip,
                      const uint8_t *dst_mac);
/* Slide the window forward on a cumulative ACK. */
void sender_handle_ack(struct sender_state *s, uint32_t ack_seq);
/* Retransmit all packets in the window (Go-Back-N). */
void sender_go_back_n(struct eth_node *eth,
                      struct sender_state *s,
                      uint8_t src_hip, uint8_t dst_hip,
                      const uint8_t *dst_mac);

/* --- receiver --- */
int  receiver_init(struct receiver_state *r, const char *filename);
/* Handle one incoming DATA packet.  Sends ACK via eth.
 * buf points to the start of the RTP header; src_mac/src_hip are the
 * sender's addresses so we can direct the ACK back. */
void receiver_handle_data(struct eth_node *eth,
                          struct receiver_state *r,
                          const uint8_t *buf, size_t len,
                          uint8_t local_hip,
                          const uint8_t *src_mac, uint8_t src_hip);

#endif /* RTP_H */
