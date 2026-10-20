/* ==========================================================================
 * libeth — Ethernet layer
 *
 * Wire format (14 bytes):
 *   +------------------+------------------+---------------+
 *   | dst MAC (6 bytes)| src MAC (6 bytes)| ethertype (2) |
 *   +------------------+------------------+---------------+
 * ========================================================================*/

#ifndef ETH_H
#define ETH_H

#include <stdint.h>
#include <stddef.h>
#include <linux/if_packet.h>

#define ETH_P_HIP     0x3230          /* Our custom ethertype            */
#define ETH_HDR_LEN   14              /* 6 + 6 + 2                       */
#define MAX_IFS       4               /* Max network interfaces tracked  */

static const uint8_t ETH_BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/* Interface bookkeeping (socket + MAC per interface) */
struct eth_node {
	struct sockaddr_ll addr[MAX_IFS];
	int    if_count;
	int    raw_sock;
};

/* --- socket / interface helpers --- */
int  eth_setup(struct eth_node *eth);        /* open raw socket + scan ifaces */
void eth_release(struct eth_node *eth);

/* --- frame build / parse --- */
/* Writes Ethernet header into buf[0..13], returns ETH_HDR_LEN */
size_t eth_build_hdr(uint8_t *buf,
                     const uint8_t *src_mac, const uint8_t *dst_mac);

/* Returns 0 on success, -1 if ethertype mismatch or buf too short */
int eth_parse_hdr(const uint8_t *buf, size_t len,
                  uint8_t *src_mac_out, uint8_t *dst_mac_out);

/* --- low-level send / recv --- */
/* Sends raw bytes (caller already built the full frame in buf) */
int eth_send(struct eth_node *eth, const uint8_t *buf, size_t len);

/* Reads one raw frame, returns byte count or -1 on error */
ssize_t eth_recv(struct eth_node *eth, uint8_t *buf, size_t cap);

/* --- helpers --- */
void eth_print_mac(const uint8_t *mac);

#endif /* ETH_H */
