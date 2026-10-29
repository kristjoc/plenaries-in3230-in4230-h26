/* ==========================================================================
 * RTP-over-HiP-over-Ethernet — main
 *
 * SKELETON FOR LIVE CODING DURING PLENARY 6
 *
 * USAGE:
 *   sudo ./rtp -l              // receiver: wait for a file transfer
 *   sudo ./rtp -c <filename>   // sender:   transfer file to broadcast
 * ========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "libeth/eth.h"
#include "libhip/hip.h"
#include "librtp/rtp.h"

#define MAX_EVENTS  10
#define MAX_BUF     1500

/* --------------------------------------------------------------------------
 * Sender loop
 * ------------------------------------------------------------------------*/
static void run_sender(struct eth_node *eth, uint8_t my_hip,
                       const char *filename)
{
	struct sender_state s;
	struct epoll_event  events[MAX_EVENTS];
	int                 efd, rc;

	if (sender_init(&s, filename) < 0)
		exit(EXIT_FAILURE);

	efd = epoll_create1(0);
	if (efd < 0) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	struct epoll_event ev = { .events = EPOLLIN, .data.fd = eth->raw_sock };
	if (epoll_ctl(efd, EPOLL_CTL_ADD, eth->raw_sock, &ev) < 0) {
		perror("epoll_ctl");
		exit(EXIT_FAILURE);
	}

	printf("<info> Sending file: %s\n\n", filename);

	for (;;) {
		/* Fill the window */
		int sent = 0;
		while (sender_send_next(eth, &s, my_hip, HIP_BROADCAST,
		                        ETH_BROADCAST) > 0)
			sent = 1;

		/* Done when window is empty and file is exhausted */
		if (!sent && s.window_size == 0)
			break;

		/* Wait for ACKs (500ms timeout for Go-Back-N retransmission) */
		rc = epoll_wait(efd, events, MAX_EVENTS, 500);
		if (rc < 0) {
			perror("epoll_wait");
			break;
		}

		if (rc == 0) {
			/* Timeout: retransmit unacknowledged packets */
			if (s.window_size > 0) {
				printf("[rtp] ACK timeout: triggering Go-Back-N from seq=%u\n", s.base);
				sender_go_back_n(eth, &s, my_hip, HIP_BROADCAST,
				                 ETH_BROADCAST);
			}
			continue;
		}

		for (int i = 0; i < rc; i++) {
			if (events[i].data.fd != eth->raw_sock)
				continue;

			uint8_t buf[MAX_BUF];
			ssize_t n = eth_recv(eth, buf, sizeof(buf));
			if (n <= 0)
				continue;

			/* ----------------------------------------------------------
			 * TODO 2B: Decapsulate ACK (Peel Ethernet -> HiP -> RTP)
			 *
			 * 1. Ethernet: eth_parse_hdr(buf, n, src_mac, dst_mac)
			 * 2. HiP:      hip_parse_hdr(buf + ETH_HDR_LEN, remaining, &hip)
			 *              Verify hip.type == HIP_TYPE_DATA || HIP_TYPE_ACK
			 * 3. RTP:      rtp_parse_hdr(rtp_buf, rtp_len, &rtp)
			 *              Verify rtp.type == RTP_TYPE_ACK
			 * 4. Dispatch:
			 *    - Duplicate ACK (rtp.seq_num == s.base - 1) -> sender_go_back_n(...)
			 *    - Normal ACK    (rtp.seq_num >= s.base)     -> sender_handle_ack(...)
			 * --------------------------------------------------------*/

			/* --- ETH decapsulation --- */
			uint8_t src_mac[6], dst_mac[6];
			if (eth_parse_hdr(buf, (size_t)n, src_mac, dst_mac) != 0)
				continue;

			/* --- HIP decapsulation --- */
			struct hip_hdr hip;
			if (hip_parse_hdr(buf + ETH_HDR_LEN,
			                  (size_t)n - ETH_HDR_LEN, &hip) != 0)
				continue;

			/* We only care about ACK frames coming back */
			if (hip.type != HIP_TYPE_DATA && hip.type != HIP_TYPE_ACK)
				continue;

			/* --- RTP decapsulation --- */
			const uint8_t *rtp_buf = buf + ETH_HDR_LEN + HIP_HDR_LEN;
			size_t         rtp_len = (size_t)n - ETH_HDR_LEN - HIP_HDR_LEN;

			struct rtp_hdr rtp;
			if (rtp_parse_hdr(rtp_buf, rtp_len, &rtp) != 0)
				continue;

			if (rtp.type != RTP_TYPE_ACK)
				continue;

			/* Duplicate ACK (last correctly received packet before base) → Go-Back-N */
			if (s.base > 0 && rtp.seq_num == s.base - 1) {
				sender_go_back_n(eth, &s, my_hip, HIP_BROADCAST,
				                 ETH_BROADCAST);
			} else if (rtp.seq_num >= s.base && rtp.seq_num < s.next_seq) {
				sender_handle_ack(&s, rtp.seq_num);
			}

		}
	}

	printf("\n<info> Transfer complete!\n");
	fclose(s.fp);
	close(efd);
}

/* --------------------------------------------------------------------------
 * Receiver loop
 * ------------------------------------------------------------------------*/
static void run_receiver(struct eth_node *eth, uint8_t my_hip)
{
	struct receiver_state r;
	struct epoll_event    events[MAX_EVENTS];
	int                   efd, rc;
	char                  outfile[64];
	int                   initialized = 0;

	efd = epoll_create1(0);
	if (efd < 0) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	struct epoll_event ev = {
		.events = EPOLLIN,
		.data.fd = eth->raw_sock
	};

	if (epoll_ctl(efd, EPOLL_CTL_ADD, eth->raw_sock, &ev) < 0) {
		perror("epoll_ctl");
		exit(EXIT_FAILURE);
	}

	printf("<info> Waiting for file transfer...\n\n");

	for (;;) {
		rc = epoll_wait(efd, events, MAX_EVENTS, -1);
		if (rc < 0) {
			perror("epoll_wait");
			break;
		}

		for (int i = 0; i < rc; i++) {
			if (events[i].data.fd != eth->raw_sock)
				continue;

			uint8_t buf[MAX_BUF];
			ssize_t n = eth_recv(eth, buf, sizeof(buf));
			if (n <= 0)
				continue;

			/* ----------------------------------------------------------
			 * TODO 2A => DONE: Decapsulate DATA packet
			 *
			 * 1. Ethernet: eth_parse_hdr(...)
			 * 2. HiP:      hip_parse_hdr(...) -> check hip.type == HIP_TYPE_DATA
			 * 3. RTP:      rtp_parse_hdr(...) -> check rtp.type == RTP_TYPE_DATA
			 * 4. On first packet: initialize receiver_init(&r, outfile)
			 * 5. Hand to RTP layer:
			 *    receiver_handle_data(eth, &r, rtp_buf, rtp_len,
			 *                         my_hip, src_mac, hip.src);
			 * --------------------------------------------------------*/

			/* --- ETH decapsulation --- */
			uint8_t src_mac[6], dst_mac[6];
			if (eth_parse_hdr(buf, (size_t)n, src_mac, dst_mac) != 0)
				continue;

			/* --- HIP decapsulation --- */
			struct hip_hdr hip;
			if (hip_parse_hdr(buf + ETH_HDR_LEN,
			                  (size_t)n - ETH_HDR_LEN, &hip) != 0)
				continue;

			if (hip.type != HIP_TYPE_DATA)
				continue;

			/* --- RTP decapsulation --- */
			const uint8_t *rtp_buf = buf + ETH_HDR_LEN + HIP_HDR_LEN;
			size_t         rtp_len = (size_t)n - ETH_HDR_LEN - HIP_HDR_LEN;

			struct rtp_hdr rtp;
			if (rtp_parse_hdr(rtp_buf, rtp_len, &rtp) != 0)
				continue;

			if (rtp.type != RTP_TYPE_DATA)
				continue;

			/* Open output file on the very first data packet */
			if (!initialized) {
				snprintf(outfile, sizeof(outfile),
				         "received_%u", my_hip);
				if (receiver_init(&r, outfile) < 0)
					exit(EXIT_FAILURE);
				printf("<info> Receiving → %s\n\n", outfile);
				initialized = 1;
			}

			receiver_handle_data(eth, &r,
			                     rtp_buf, rtp_len,
			                     my_hip, src_mac, hip.src);
		}
	}

	close(efd);
}

/* --------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	struct eth_node eth;
	uint8_t         my_hip;

	if (argc < 2) {
		fprintf(stderr,
		        "usage:\n"
		        "  %s -l              (receiver)\n"
		        "  %s -c <filename>   (sender)\n",
		        argv[0], argv[0]);
		return EXIT_FAILURE;
	}

	if (eth_setup(&eth) < 0)
		return EXIT_FAILURE;

	srand((unsigned)time(NULL));
	my_hip = (uint8_t)(rand() % 255 + 1);

	printf("<info> I am HiP node %u, MAC ", my_hip);
	eth_print_mac(eth.addr[0].sll_addr);
	printf("\n\n");

	if (strcmp(argv[1], "-l") == 0) {
		run_receiver(&eth, my_hip);

	} else if (strcmp(argv[1], "-c") == 0) {
		if (argc < 3) {
			fprintf(stderr, "error: -c requires a filename\n");
			eth_release(&eth);
			return EXIT_FAILURE;
		}
		run_sender(&eth, my_hip, argv[2]);

	} else {
		fprintf(stderr, "unknown option: %s\n", argv[1]);
		eth_release(&eth);
		return EXIT_FAILURE;
	}

	eth_release(&eth);
	return 0;
}
