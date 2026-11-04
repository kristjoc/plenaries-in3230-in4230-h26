/* ==========================================================================
 * libeth — Ethernet layer implementation
 * ========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include "eth.h"

/* Open a raw AF_PACKET socket filtered to ETH_P_HIP, then enumerate
 * all non-loopback AF_PACKET interfaces and store their sockaddr_ll. */
int eth_setup(struct eth_node *eth)
{
	struct ifaddrs *ifaces, *ifp;

	eth->raw_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_HIP));
	if (eth->raw_sock == -1) {
		perror("socket");
		return -1;
	}

	if (getifaddrs(&ifaces) == -1) {
		perror("getifaddrs");
		return -1;
	}

	eth->if_count = 0;
	for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
		if (!ifp->ifa_addr)
			continue;
		if (ifp->ifa_addr->sa_family != AF_PACKET)
			continue;
		if (strcmp(ifp->ifa_name, "lo") == 0)
			continue;
		if (eth->if_count >= MAX_IFS)
			break;

		printf("[eth] iface %s\n", ifp->ifa_name);
		memcpy(&eth->addr[eth->if_count], ifp->ifa_addr,
		       sizeof(struct sockaddr_ll));
		eth->if_count++;
	}

	freeifaddrs(ifaces);

	if (eth->if_count == 0) {
		fprintf(stderr, "[eth] no usable interface\n");
		return -1;
	}

	return 0;
}

void eth_release(struct eth_node *eth)
{
	close(eth->raw_sock);
}

/* Build a 14-byte Ethernet header at buf.  Returns ETH_HDR_LEN. */
size_t eth_build_hdr(uint8_t *buf,
                     const uint8_t *src_mac, const uint8_t *dst_mac)
{
	uint16_t ethertype = htons(ETH_P_HIP);

	memcpy(buf + 0, dst_mac, 6);
	memcpy(buf + 6, src_mac, 6);
	memcpy(buf + 12, &ethertype, 2);

	return ETH_HDR_LEN;
}

/* Parse a received frame's Ethernet header.
 * Returns 0 if it's ours, -1 if too short or wrong ethertype. */
int eth_parse_hdr(const uint8_t *buf, size_t len,
                  uint8_t *src_mac_out, uint8_t *dst_mac_out)
{
	uint16_t ethertype;

	if (len < ETH_HDR_LEN) {
		fprintf(stderr, "[eth] frame too short (%zu)\n", len);
		return -1;
	}

	memcpy(dst_mac_out, buf + 0, 6);
	memcpy(src_mac_out, buf + 6, 6);
	memcpy(&ethertype, buf + 12, 2);

	if (ntohs(ethertype) != ETH_P_HIP) {
		/* Not our protocol — silently drop */
		return -1;
	}

	return 0;
}

int eth_send(struct eth_node *eth, const uint8_t *buf, size_t len)
{
	ssize_t sent = sendto(eth->raw_sock, buf, len, 0,
	                      (struct sockaddr *)&eth->addr[0],
	                      sizeof(struct sockaddr_ll));
	if (sent != (ssize_t)len) {
		perror("sendto");
		return -1;
	}

	return 0;
}

ssize_t eth_recv(struct eth_node *eth, uint8_t *buf, size_t cap)
{
	ssize_t n = recvfrom(eth->raw_sock, buf, cap, 0, NULL, NULL);
	if (n < 0)
		perror("recvfrom");

	return n;
}

void eth_print_mac(const uint8_t *mac)
{
	printf("%02x:%02x:%02x:%02x:%02x:%02x",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
