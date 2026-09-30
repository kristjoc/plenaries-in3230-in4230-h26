#include <stdlib.h>		/* free */
#include <stdio.h> 		/* printf */
#include <unistd.h>             /* fgets */
#include <string.h>		/* memset */
#include <fcntl.h>
#include <linux/if_packet.h>	/* AF_PACKET */
#include <net/ethernet.h>	/* ETH_* */
#include <arpa/inet.h>		/* htons */
#include <sys/epoll.h>		/* epoll */

#include "utils.h"
#include "ether.h"
#include "hip.h"

int main(int argc, const char *argv[])
{
	struct ifs_data local_if;
	struct epoll_event events[MAX_EVENTS];
	int    raw_sock, efd, rc;

	if (argc < 2) {
		printf("Too few args: ./hip c/s 'hello'\n");
		exit(EXIT_FAILURE);
	}

	/* Set up a raw AF_PACKET socket without ethertype filtering */
	raw_sock = create_raw_socket();

	/* Initialize interface data */
	init_ifs(&local_if, raw_sock);

	/* Add socket to epoll table */
	efd = epoll_add_sock(raw_sock);

	/* Simply introduce yourself via stdout */
	printf("\n<info> Hi! I am node %u with MAC ", local_if.local_hip_addr);

	// print MAC addresses of all interfaces
	for (int i = 0; i < local_if.ifn; i++) {
		print_mac_addr(local_if.addr[i].sll_addr, 6);
		//print ifindex and name of the interface
		printf(" (ifindex: %d)", local_if.addr[i].sll_ifindex);
		printf("\n");
	}

	if (strcmp(argv[1], "c") == 0) {
		/* client mode */
		/* Send greeting to the server */
		uint8_t broadcast[] = ETH_DST_MAC;

		for (int i = 0; i < local_if.ifn; i++) {
			send_hip_packet(&local_if, i, local_if.addr[i].sll_addr, broadcast,
					local_if.local_hip_addr, HIP_DST_ADDR, argv[2]);
		}
	}

	while(1) {
		rc = epoll_wait(efd, events, MAX_EVENTS, -1);
		if (rc == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		} else if (events->data.fd == raw_sock) {
			rc = handle_hip_packet(&local_if, argv[1]);
			if (rc < 0) {
				perror("handle_hip_packet");
				exit(EXIT_FAILURE);
			}
		}
		break;
	}
	close(raw_sock);

	return 0;
}
