/* ============================================================================
 * HiP-over-Ethernet — LIVE CODING SKELETON
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
 * TOPOLOGY (Mininet):
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

/* --------------------------------------------------------------------------
 * Constants — the "protocol spec" for our toy HiP-over-Ethernet protocol.
 * ----------------------------------------------------------------------- */

#define ETH_HDR_LEN   14                 /* dst(6) + src(6) + ethertype(2) */
#define HIP_HDR_LEN   4                  /* dst(1) + src(1) + len(1) + ver/type(1) */
#define MAX_SDU_LEN   (255 * 4)          /* len field is 1 byte -> max 255 words */
#define MAX_BUF_SIZE  (ETH_HDR_LEN + HIP_HDR_LEN + MAX_SDU_LEN)
#define MAX_IF        3                  /* room for a few interfaces, just in case */
#define MAX_EVENTS    10

#define ETH_P_HIP     0x3230             /* our custom "ethertype" */
#define HIP_VERSION   4

#define HIP_TYPE_HELLO      0x4
#define HIP_TYPE_HELLO_ACK  0x5

#define HIP_BROADCAST_ADDR 0xff          /* HiP "address" meaning "everyone" */

static const uint8_t ETH_BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

enum node_mode {
	MODE_SERVER,
	MODE_CLIENT,
	MODE_UNKNOWN
};

/* --------------------------------------------------------------------------
 * Per-node state: node mode, our raw socket, our HiP address, and every
 * non-loopback link-layer interface we found. if_count tells you how many
 * of `addr[]` are actually populated.
 * ----------------------------------------------------------------------- */
struct hip_node {
	enum node_mode      mode;
	struct sockaddr_ll  addr[MAX_IF];
	int                 if_count;
	int                 raw_sock;
	uint8_t             local_hip_addr;
};

/* A parsed incoming packet, kept around so a server can build a reply
 * (it needs the sender's MAC + HiP address to answer back). */
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
};

void print_mac(const uint8_t *mac)
{
	printf("%02x:%02x:%02x:%02x:%02x:%02x",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* ==========================================================================
 * SECTION 1 — Open a raw socket
 * ==========================================================================*/
int create_raw_socket(void)
{
	int sock;

	/* TODO 1: create the socket.
	 *   - domain:   AF_PACKET   (link-layer access)
	 *   - type:     SOCK_RAW    (we build the whole frame ourselves)
	 *   - protocol: htons(ETH_P_HIP)
	 * socket() returns -1 on error — check it and perror("socket").
	 */
	sock = -1; /* <-- replace this line */

	if (sock == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	return sock;
}

/* ==========================================================================
 * SECTION 2 — Discover our own interfaces (no hardcoded iface names)
 *
 * getifaddrs() gives us a linked list describing every interface on the
 * node. We only want the link-layer (AF_PACKET) entries, and we skip "lo"
 * since you can't usefully send raw Ethernet frames out of loopback.
 * ==========================================================================*/
void get_mac_from_ifaces(struct hip_node *node)
{
	struct ifaddrs *ifaces, *ifp;

	if (getifaddrs(&ifaces) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	node->if_count = 0;

	/* TODO 2: walk the linked list `ifaces` with `ifp = ifp->ifa_next`.
	 * For each entry, keep it only if:
	 *   - ifp->ifa_addr != NULL
	 *   - ifp->ifa_addr->sa_family == AF_PACKET
	 *   - strcmp(ifp->ifa_name, "lo") != 0
	 *   - node->if_count < MAX_IF   (don't overflow our array)
	 *
	 * For each one that qualifies:
	 *   printf("[iface] found %s\n", ifp->ifa_name);
	 *   memcpy(&node->addr[node->if_count], ifp->ifa_addr, sizeof(struct sockaddr_ll));
	 *   node->if_count++;
	 */

	freeifaddrs(ifaces);

	if (node->if_count == 0) {
		fprintf(stderr, "no usable interface found\n");
		exit(EXIT_FAILURE);
	}
}

/* ==========================================================================
 * SECTION 3 — epoll: register our socket for readability events
 * ==========================================================================*/
int epoll_add_sock(int sock)
{
	int epfd;
	struct epoll_event ev;

	/* TODO 3a: create the epoll instance.
	 * From man epoll_create1:
	 * int epoll_create1(int flags);
	 * -1 on error -> perror("epoll_create1"), exit(EXIT_FAILURE).
	 */
	epfd = -1; /* <-- replace */

	if (epfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	/* TODO 3b: register `sock` for EPOLLIN (readable) events.
	 *   ev.events  = EPOLLIN;
	 *   ev.data.fd = sock;
	 *
	 * From man epoll_ctl:
	 * int epoll_ctl(int epfd, int op, int fd,
         *               struct epoll_event *_Nullable event);
	 *
	 * -1 on error -> perror("epoll_ctl"), exit(EXIT_FAILURE).
	 */

	return epfd;
}

/* ==========================================================================
 * SECTION 4 — Build the Ethernet header
 * ==========================================================================*/
size_t build_eth_header(uint8_t *buf, const uint8_t *src_mac, const uint8_t *dst_mac)
{
	/* TODO 4: fill in the 14 bytes of the Ethernet header at `buf` using
         * memcpy:
	 *
	 *   buf[0..5]   = dst_mac
	 *   buf[6..11]  = src_mac
	 *   buf[12..13] = ethertype, as a 16-bit value in NETWORK byte order
	 *
	 * Hint:
	 *   uint16_t ethertype = htons(ETH_P_HIP);
	 *   memcpy(buf + 12, &ethertype, 2);
	 */

	return ETH_HDR_LEN;
}

/* ==========================================================================
 * SECTION 5 — Build the HiP header
 *
 * Layout: byte0=dst, byte1=src, byte2=len(words), byte3=version(hi nibble)|type(lo nibble)
 * ==========================================================================*/
size_t build_hip_header(uint8_t *buf, uint8_t src_addr, uint8_t dst_addr,
			uint8_t type, size_t sdu_len_bytes)
{
	uint8_t len_words = (uint8_t)(sdu_len_bytes / 4);

	/* TODO 5: fill in the 4 bytes of the HiP header at `buf`:
	 *   buf[0] = dst_addr
	 *   buf[1] = src_addr
	 *   buf[2] = len_words
	 *   buf[3] = (HIP_VERSION << 4) | (type & 0x0f)
	 */

	return HIP_HDR_LEN;
}

/* ==========================================================================
 * SECTION 6 — Assemble and send a full packet
 *
 * We send out of node->addr[0]. For SOCK_RAW, sendto()'s destination
 * sockaddr_ll mainly picks the OUTGOING interface (via sll_ifindex); the
 * destination MAC on the wire is whatever we wrote into the frame itself.
 * ==========================================================================*/
int send_hip_packet(struct hip_node *node, const uint8_t *dst_mac,
		    uint8_t dst_hip_addr, uint8_t type, const char *message)
{
	uint8_t buf[MAX_BUF_SIZE];
	size_t offset = 0;
	size_t msg_len, padded_len;

	msg_len = strlen(message) + 1; /* +1 for the trailing '\0' */
	padded_len = msg_len;
	if (padded_len % 4 != 0)
		padded_len += 4 - (padded_len % 4);

	if (ETH_HDR_LEN + HiP_HDR_LEN + padded_len > MAX_BUF_SIZE) {
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

	/* TODO 6: send it.
	 *   sendto(node->raw_sock, buf, offset, 0,
	 *          (struct sockaddr *)&node->addr[0], sizeof(struct sockaddr_ll));
	 * Check the return value equals `offset`. On error, perror("sendto")
	 * and return -1.
	 */

	printf("[send] %zu bytes to ", offset); print_mac(dst_mac);
	printf(" (HiP dst %u, type 0x%x)\n", dst_hip_addr, type);

	return 0;
}

/* ==========================================================================
 * SECTION 7 — Receive and parse a packet
 * ==========================================================================*/
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

	/* TODO 7a: extract the ethertype (2 bytes at buf[12..13]) and convert
	 * it back to host byte order with ntohs(). Store it in
	 * `net_ethertype` first, then `pkt->ethertype = ntohs(net_ethertype);`
	 */
	pkt->ethertype = 0; /* <-- replace */
	offset = ETH_HDR_LEN;

	/* Sanity check: is this actually one of our packets, or something
	 * else that happened to arrive on this raw socket? */
	if (pkt->ethertype != ETH_P_HIP) {
		printf("[recv] ignoring non-HiP frame (ethertype 0x%04x)\n", pkt->ethertype);
		return -1;
	}

	/* TODO 7b: extract the HiP header fields from buf + offset:
	 *   pkt->hip_dst = buf[offset + 0]
	 *   pkt->hip_src = buf[offset + 1]
	 *   len_words    = buf[offset + 2]
	 *   hip_ver_type = buf[offset + 3]
	 *   pkt->version = hip_ver_type >> 4
	 *   pkt->type    = hip_ver_type & 0x0f
	 */
	pkt->hip_dst = pkt->hip_src = len_words = hip_ver_type = 0; /* <-- replace */
	pkt->version = 0;
	pkt->type = 0;
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

/* ==========================================================================
 * SECTION 8 — Handle one incoming packet (server replies, client just prints)
 * ==========================================================================*/
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
		return 0; /* not one of ours, or malformed — already logged */

	printf("[recv] got %zd bytes:\n", n);
	print_hip_packet(&pkt);

	if (node->mode == MODE_SERVER && pkt.type == HIP_TYPE_HELLO) {
		/* Reply straight back to whoever sent it */
		send_hip_packet(node, pkt.eth_src_mac, pkt.hip_src,
				HIP_TYPE_HELLO_ACK, (const char *)pkt.sdu);
	}

	return 0;
}

/*
 * ==========================================================================
 * SECTION 9 — Main program: parse args, init, then epoll loop
 * ==========================================================================
 */
int main(int argc, char *argv[])
{
	struct hip_node node;

	/* Parse command-line arguments:
	 *   -s           = server mode (wait forever for HiP pkts, reply to everyone)
	 *   -c <message> = client mode (send HELLO, wait for ACK)
	 */
	if (argc < 2 || (strcmp(argv[1], "-c") == 0 && argc < 3)) {
		fprintf(stderr, "usage:\n"
			"  %s -s              (server)\n"
			"  %s -c <message>    (client)\n",
			argv[0], argv[0]);
		return EXIT_FAILURE;
	}

	/* TODO 9a: figure out which mode we're in using a switch on argv[1][1]:
	 *   case 's': node.mode = MODE_SERVER; break;
	 *   case 'c': node.mode = MODE_CLIENT; break;
	 *   default:  node.mode = MODE_UNKNOWN; break;
	 * Then, if node.mode == MODE_UNKNOWN, print usage and return EXIT_FAILURE.
	 */

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

	if (node.mode == )

		/* Create the RAW socket using the create_raw_socket() wrapper function */
		node.raw_sock = create_raw_socket();

	/* Discover our own non-loopback interfaces and store their MAC
	 * addresses in the hip_node struct */
	get_mac_from_ifaces(&node);

	/* Register the raw socket with epoll to wait for incoming packets */
	int epfd = epoll_add_sock(node.raw_sock);

	/* Seed the random number generator and assign a random HiP address to
	 * this node */
	srand((unsigned)time(NULL));
	node.local_hip_addr = (uint8_t)(rand() % 256);

	/* Print out our running mode, HiP address, MAC address, and number of
	 * interfaces found */
	printf("I am HiP node %u, MAC ", node.local_hip_addr);
	print_mac(node.addr[0].sll_addr);
	printf(" (%d interface(s) found)\n", node.if_count);

	if (node.mode == MODE_SERVER) {
		printf("<SERVER> waiting for HELLO packets...\n");
	} else {
		printf("<CLIENT> sending HELLO packet with message: %s\n", argv[2]);
		/* Client: fire one HELLO, then fall into the same epoll loop
		 * below to wait for the ACK. */
		send_hip_packet(&node, ETH_BROADCAST, HIP_BROADCAST_ADDR,
				HIP_TYPE_HELLO, argv[2]);
	}

	/* TODO 9b: the epoll event loop.
	 * Server: runs forever, replying to every HELLO it sees.
	 * Client: falls into the same loop to wait for the ACK.
	 *
	 *   struct epoll_event events[MAX_EVENTS];
	 *   for (;;) {
	 *       int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
	 *       if (n == -1) { perror("epoll_wait"); break; }
	 *       for (int i = 0; i < n; i++) {
	 *           if (events[i].data.fd == node.raw_sock)
	 *               handle_hip_packet(&node);
	 *       }
	 *   }
	 */

	/* Cleanup: close the raw socket and epoll file descriptor before exiting */
	close(node.raw_sock);
	close(epfd);

	return 0;
}
