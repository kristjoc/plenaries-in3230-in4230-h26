/* ============================================================================
 * HiP-over-Ethernet — INSTRUCTOR REFERENCE
 * ==========================================================================*/

/* ============================================================================
 * HiP-over-Ethernet — LIVE CODING SKELETON (Mininet edition)
 * ============================================================================
 *
 * Everything lives in this one file on purpose: no jumping between headers
 * and .c files during the session. Search for "TODO" to find every spot
 * you'll fill in live. Each TODO has the background info you need right
 * above it — you shouldn't need to remember anything, just read and reason
 * out loud.
 *
 * WIRE FORMAT (what actually goes out on the network, byte by byte):
 *
 *   Ethernet header (14 bytes)
 *   +--------------------+--------------------+------------+
 *   | dst MAC (6 bytes)  | src MAC (6 bytes)  | ethertype  |
 *   |                    |                    | (2 bytes)  |
 *   +--------------------+--------------------+------------+
 *
 *   HiP header (4 bytes)
 *   +---------+---------+---------+-------------------+
 *   | dst (1) | src (1) | len (1) | version(4)|type(4)|
 *   +---------+---------+---------+-------------------+
 *     len = length of the SDU (payload) in 4-byte words
 *
 *   SDU / payload (len * 4 bytes, zero-padded to a multiple of 4)
 *
 * Total on the wire = 14 (eth) + 4 (hip) + payload.
 *
 * TOPOLOGY ASSUMPTION (Mininet):
 *   Each node has exactly one loopback ("lo") and one virtual data
 *   interface. We never hardcode an interface name — we ask the kernel
 *   for all interfaces and pick the first non-loopback, AF_PACKET one.
 *   This is what makes the code portable across however Mininet names
 *   its veths (s1-eth0, h1-eth0, whatever the topology script picked).
 *
 * USAGE:
 *   sudo ./hip -s              // server: run forever, reply to everyone
 *   sudo ./hip -c <message>    // client: send once, then wait for the reply
 *
 * ==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#define ETH_HDR_LEN   (6 + 6 + 2)
#define HIP_HDR_LEN   4
#define MAX_SDU_LEN   (255 * 4)
#define MAX_BUF_SIZE  (ETH_HDR_LEN + HIP_HDR_LEN + MAX_SDU_LEN)
#define MAX_IF        3
#define MAX_EVENTS    10

#define ETH_P_HIP     0x3230
#define HIP_VERSION   4

#define HIP_TYPE_HELLO      0x4
#define HIP_TYPE_HELLO_ACK  0x5

#define HIP_BROADCAST_ADDR 0xff

static const uint8_t ETH_BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

enum node_mode {
	MODE_SERVER,
	MODE_CLIENT,
	MODE_UNKNOWN
};

struct hip_node {
	enum node_mode mode;
	struct sockaddr_ll addr[MAX_IF];
	int     if_count;
	int     raw_sock;
	uint8_t local_hip_addr;
};

struct hip_packet {
	uint8_t  eth_src_mac[6];
	uint8_t  eth_dst_mac[6];
	uint16_t ethertype;
	uint8_t  hip_src;
	uint8_t  hip_dst;
	uint8_t  version;
	uint8_t  type;
	uint8_t  sdu[MAX_SDU_LEN];
	size_t   sdu_len;
} __attribute__((packed));


void print_mac(const uint8_t *mac)
{
	printf("%02x:%02x:%02x:%02x:%02x:%02x",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int create_raw_socket(void)
{
	int sock;

	sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_HIP));

	if (sock == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	return sock;
}

void get_mac_from_ifaces(struct hip_node *node)
{
	struct ifaddrs *ifaces, *ifp;

	if (getifaddrs(&ifaces) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	node->if_count = 0;

	for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
		if (ifp->ifa_addr == NULL)
			continue;
		if (ifp->ifa_addr->sa_family != AF_PACKET)
			continue;
		if (strcmp(ifp->ifa_name, "lo") == 0)
			continue;
		if (node->if_count >= MAX_IF)
			break;

		printf("[iface] found %s\n", ifp->ifa_name);

		memcpy(&node->addr[node->if_count],
		       ifp->ifa_addr,
		       sizeof(struct sockaddr_ll));
		node->if_count++;
	}

	freeifaddrs(ifaces);

	if (node->if_count == 0) {
		fprintf(stderr, "no usable interface found\n");
		exit(EXIT_FAILURE);
	}
}

int epoll_add_sock(int sock)
{
	int epfd;
	struct epoll_event ev;

	epfd = epoll_create1(0);

	if (epfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	ev.events  = EPOLLIN;
	ev.data.fd = sock;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
		perror("epoll_ctl");
		exit(EXIT_FAILURE);
	}

	return epfd;
}

size_t build_eth_header(uint8_t *buf, const uint8_t *src_mac, const uint8_t *dst_mac)
{
	uint16_t ethertype = htons(ETH_P_HIP);

	memcpy(buf + 0, dst_mac, 6);
	memcpy(buf + 6, src_mac, 6);
	memcpy(buf + 12, &ethertype, 2);

	return ETH_HDR_LEN;
}

size_t build_hip_header(uint8_t *buf, uint8_t src_addr, uint8_t dst_addr,
			uint8_t type, size_t sdu_len_bytes)
{
	uint8_t len_words = (uint8_t)(sdu_len_bytes / 4);

	buf[0] = dst_addr;
	buf[1] = src_addr;
	buf[2] = len_words;
	buf[3] = (uint8_t)((HIP_VERSION << 4) | (type & 0x0f));

	return HIP_HDR_LEN;
}

int send_hip_packet(struct hip_node *node, const uint8_t *dst_mac,
		    uint8_t dst_hip_addr, uint8_t type, const char *message)
{
	uint8_t buf[MAX_BUF_SIZE];
	size_t offset = 0;
	size_t msg_len, padded_len;

	msg_len = strlen(message) + 1;
	padded_len = msg_len;
	if (padded_len % 4 != 0)
		padded_len += 4 - (padded_len % 4);

	if (ETH_HDR_LEN + HIP_HDR_LEN + padded_len > MAX_BUF_SIZE) {
		fprintf(stderr, "message too long\n");
		return -1;
	}

	offset += build_eth_header(buf + offset,
				   node->addr[0].sll_addr,
				   dst_mac);

	offset += build_hip_header(buf + offset,
				   node->local_hip_addr,
				   dst_hip_addr,
				   type,
				   padded_len);

	memset(buf + offset, 0, padded_len);
	memcpy(buf + offset, message, msg_len);
	offset += padded_len;

	if (sendto(node->raw_sock, buf, offset, 0,
		   (struct sockaddr *)&node->addr[0],
		   sizeof(struct sockaddr_ll)) != (ssize_t)offset) {
		perror("sendto");

		return -1;
	}

	printf("[send] %zu bytes to ", offset); print_mac(dst_mac);
	printf(" (HiP dst %u, type 0x%x)\n", dst_hip_addr, type);

	return 0;
}

int parse_hip_packet(const uint8_t *buf, size_t len, struct hip_packet *pkt)
{
	uint16_t net_ethertype;
	size_t offset;
	uint8_t hip_ver_type, len_words;

	if (len < ETH_HDR_LEN + HIP_HDR_LEN) {
		fprintf(stderr, "packet too short to be one of ours\n");
		return -1;
	}

	memcpy(pkt->eth_dst_mac, buf + 0, 6);
	memcpy(pkt->eth_src_mac, buf + 6, 6);

	memcpy(&net_ethertype, buf + 12, 2);
	pkt->ethertype = ntohs(net_ethertype);
	offset = ETH_HDR_LEN;

	if (pkt->ethertype != ETH_P_HIP) {
		printf("[recv] ignoring non-HiP frame (ethertype 0x%04x)\n", pkt->ethertype);
		return -1;
	}

	pkt->hip_dst = buf[offset + 0];
	pkt->hip_src = buf[offset + 1];
	len_words    = buf[offset + 2];
	hip_ver_type = buf[offset + 3];
	pkt->version = hip_ver_type >> 4;
	pkt->type    = hip_ver_type & 0x0f;
	offset += HIP_HDR_LEN;

	pkt->sdu_len = (size_t)len_words * 4;
	if (offset + pkt->sdu_len > len) {
		fprintf(stderr, "payload length field doesn't match packet size — "
			"corrupt or malicious packet, dropping\n");
		return -1;
	}

	memset(pkt->sdu, 0, sizeof(pkt->sdu));
	memcpy(pkt->sdu, buf + offset, pkt->sdu_len);

	return 0;
}

void print_hip_packet(const struct hip_packet *pkt)
{
	printf("====================================================\n");
	printf(" src MAC: "); print_mac(pkt->eth_src_mac); printf("\n");
	printf(" dst MAC: "); print_mac(pkt->eth_dst_mac); printf("\n");
	printf(" HiP src: %u   HiP dst: %u\n", pkt->hip_src, pkt->hip_dst);
	printf(" version: %u   type: 0x%x\n", pkt->version, pkt->type);
	printf(" SDU (%zu bytes): %s\n", pkt->sdu_len, pkt->sdu);
	printf("====================================================\n");
}

int handle_hip_packet(struct hip_node *node)
{
	uint8_t buf[MAX_BUF_SIZE];
	ssize_t n;
	struct hip_packet pkt;

	n = recvfrom(node->raw_sock, buf, sizeof(buf), 0, NULL, NULL);
	if (n <= 0) {
		perror("recvfrom");
		return -1;
	}

	if (parse_hip_packet(buf, (size_t)n, &pkt) != 0)
		return 0;

	printf("[recv] got %zd bytes:\n", n);
	print_hip_packet(&pkt);

	if (node->mode == MODE_SERVER  && pkt.type == HIP_TYPE_HELLO) {
		send_hip_packet(node, pkt.eth_src_mac, pkt.hip_src,
				HIP_TYPE_HELLO_ACK, (const char *)pkt.sdu);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct hip_node node;

	if (argc < 2 || (strcmp(argv[1], "-c") == 0 && argc < 3)) {
		fprintf(stderr, "usage:\n"
			"  %s -s              (server)\n"
			"  %s -c <message>    (client)\n",
			argv[0], argv[0]);
		return EXIT_FAILURE;
	}

	switch (argv[1][1]) {
	case 's':
		node.mode = MODE_SERVER;
		break;
	case 'c':
		node.mode = MODE_CLIENT;
		break;
	default:
		node.mode = MODE_UNKNOWN;
		break;
	}

	if (node.mode == MODE_UNKNOWN) {
		fprintf(stderr, "usage: %s -s | -c <message>\n", argv[0]);
		return EXIT_FAILURE;
	}

	node.raw_sock = create_raw_socket();

	get_mac_from_ifaces(&node);

	int epfd = epoll_add_sock(node.raw_sock);

	srand((unsigned)time(NULL));
	node.local_hip_addr = (uint8_t)(rand() % 256);

	printf("I am HiP node %u, MAC ", node.local_hip_addr);
	print_mac(node.addr[0].sll_addr);
	printf(" (%d interface(s) found)\n", node.if_count);

	if (node.mode == MODE_SERVER) {
		printf("<SERVER> waiting for HELLO packets...\n");
	} else {
		printf("<CLIENT> sending HELLO packet with message: %s\n", argv[2]);
		send_hip_packet(&node, ETH_BROADCAST, HIP_BROADCAST_ADDR,
				HIP_TYPE_HELLO, argv[2]);
	}

	struct epoll_event events[MAX_EVENTS];

	for (;;) {
		int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
		int i;

		if (n == -1) {
			perror("epoll_wait");
			break;
		}
		for (i = 0; i < n; i++) {
			if (events[i].data.fd == node.raw_sock)
				handle_hip_packet(&node);
		}
	}

	close(node.raw_sock);
	close(epfd);

	return 0;
}
