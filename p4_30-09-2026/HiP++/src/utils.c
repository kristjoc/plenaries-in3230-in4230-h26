#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>		/* getifaddrs */
#include <linux/if_packet.h>    /* AF_PACKET  */
#include <arpa/inet.h>          /* htons      */
#include <sys/epoll.h>          /* epoll */
#include <sys/socket.h>		/* socket */
#include <stdint.h>
#include <time.h>
#include <errno.h>

#include "utils.h"
#include "ether.h"
#include "hip.h"
#include "pdu.h"


/*
 * Print MAC address in hex format
 */
void print_mac_addr(uint8_t *addr, size_t len)
{
        size_t i;

        for (i = 0; i < len - 1; i++) {
                printf("%02x:", addr[i]);
        }
        printf("%02x\n", addr[i]);
}


/*
 * This function stores struct sockaddr_ll addresses for all interfaces of the
 * node (except loopback interface)
 */
void get_mac_from_ifaces(struct ifs_data *ifs)
{
        struct ifaddrs *ifaces, *ifp;
        int i = 0;

        /* Enumerate interfaces: */
        /* Note in man getifaddrs that this function dynamically allocates
           memory. It becomes our responsability to free it! */
        if (getifaddrs(&ifaces)) {
                perror("getifaddrs");
                exit(-1);
        }

        /* Walk the list looking for ifaces interesting to us */
        for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
                /* We make sure that the ifa_addr member is actually set: */
                if (ifp->ifa_addr != NULL &&
                    ifp->ifa_addr->sa_family == AF_PACKET &&
                    strcmp("lo", ifp->ifa_name)) {
			/* Copy the address info into the array of our struct */
                        memcpy(&(ifs->addr[i]),
                               (struct sockaddr_ll*)ifp->ifa_addr,
                               sizeof(struct sockaddr_ll));
			int sll = ((struct sockaddr_ll*)ifp->ifa_addr)->sll_ifindex;
			ifs->pkt_queue[sll] = queue_create();
			i++;
		}
        }
        /* After the for loop, the address info of all interfaces are stored */
        /* Update the counter of the interfaces */
        ifs->ifn = i;

        /* Free the interface list */
        freeifaddrs(ifaces);
}


void init_ifs(struct ifs_data *ifs, int rsock)
{
	uint8_t rand_mip;

	/* Get some info about the local ifaces */
	get_mac_from_ifaces(ifs);

	/* We use one RAW socket per node */
	ifs->rsock = rsock;

	/* One MIP address per node; We name nodes and not interfaces like the
	 * Internet does. Read about RINA Network Architecture for more info
	 * about what's wrong with the current Internet.
	 */

	srand(time(0));
	rand_mip = (uint8_t)(rand() % 256);

	ifs->local_hip_addr = rand_mip;
}

/* Prepare RAW socket */
int create_raw_socket(void)
{
	int sd;
	short unsigned int protocol = ETH_P_HIP;

	/* Set up a raw AF_PACKET socket with ethertype filtering */
	sd = socket(AF_PACKET, SOCK_RAW, htons(protocol));
	if (sd == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	return sd;
}

int epoll_add_sock(int sd)
{
	struct epoll_event ev;

	/* Create epoll table */
	int epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	/* Add RAW socket to epoll table */
	ev.events = EPOLLIN;
	ev.data.fd = sd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sd, &ev) == -1) {
		perror("epoll_ctl: raw_sock");
		exit(EXIT_FAILURE);
	}

	return epollfd;
}

int send_hip_packet(struct ifs_data *ifs,
		    int i,
		    uint8_t *src_mac_addr,
		    uint8_t *dst_mac_addr,
		    uint8_t src_hip_addr,
		    uint8_t dst_hip_addr,
		    const char *sdu)
{
	struct pdu *pdu = alloc_pdu();
	uint8_t snd_buf[MAX_BUF_SIZE];

	if (NULL == pdu)
		return -ENOMEM;

	fill_pdu(pdu, src_mac_addr, dst_mac_addr, src_hip_addr, dst_hip_addr, sdu);

	size_t snd_len = hip_serialize_pdu(pdu, snd_buf);

	/* Send the serialized buffer via RAW socket */
	if (sendto(ifs->rsock, snd_buf, snd_len, 0,
		   (struct sockaddr *) &(ifs->addr[i]),
		   sizeof(struct sockaddr_ll)) <= 0) {
		perror("sendto()");
		close(ifs->rsock);
	}

	printf("Sending PDU with content (size %zu):\n", snd_len);
	print_pdu_content(pdu);

	destroy_pdu(pdu);
	return 0;
}

int handle_hip_packet(struct ifs_data *ifs, const char *app_mode)
{
	struct pdu *pdu = (struct pdu *)malloc(sizeof(struct pdu));
	if (NULL == pdu) {
		perror("malloc");
		return -ENOMEM;
	}

	struct sockaddr_ll src_add;
	socklen_t src_add_len = sizeof(struct sockaddr_ll);

	uint8_t rcv_buf[MAX_BUF_SIZE];

	/* Recv the serialized buffer via RAW socket */
	if (recvfrom(ifs->rsock,
		     rcv_buf, MAX_BUF_SIZE,
		     0,
		     (struct sockaddr *)&src_add,
		     &src_add_len) <= 0) {
		perror("recvfrom()");
		close(ifs->rsock);
	}

	printf("Printing something...\n");

	size_t rcv_len = hip_deserialize_pdu(pdu, rcv_buf);

	printf("Receiving PDU from iface with ifindex %d with content (size %zu) :\n",
	       src_add.sll_ifindex, rcv_len);
	print_pdu_content(pdu);

	if (strcmp(app_mode, "s") == 0) {

		// push the packet to the right queue of the incoming interface
		queue_tail_push(ifs->pkt_queue[src_add.sll_ifindex], pdu, 0);

		for (int i = 0; i < ifs->ifn; i++) {
			if (src_add.sll_ifindex == ifs->addr[i].sll_ifindex) {
				printf("PDU pushed to queue of iface with ifindex %d\n", src_add.sll_ifindex);
				/* Server must greet the client back via the same interface the
				   request arrived*/
				send_hip_packet(ifs, i, ifs->addr[i].sll_addr, pdu->ethhdr->src_mac,
						ifs->local_hip_addr, pdu->hiphdr->src,
						(const char *)pdu->sdu);
				break;
			}
		}

		struct pdu *popped_pdu = (struct pdu *)queue_head_pop(ifs->pkt_queue[src_add.sll_ifindex]);
		if (popped_pdu) {
			destroy_pdu(popped_pdu);
		}
	} else {
		destroy_pdu(pdu);
	}

	return 0;
}
