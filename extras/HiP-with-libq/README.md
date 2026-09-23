# HiP over Ethernet with libq Integration

This project serves as an example of how to integrate a custom data structure (a
queue) into the HiP server to manage incoming packet states.

## What is it?

This codebase implements a basic, custom networking protocol that sends Protocol
Data Units (PDUs) over Ethernet using RAW sockets in Linux. It also integrates a
custom C-based queue data structure (`libq`) to temporarily hold incoming
packets while they are being processed by the server.

## Where things are

The project is divided into two main components:

- **`libq/`**: Contains the standalone queue library. 
  - `queue.c` and `queue.h`: A simple, doubly-linked list-based queue implementation with support for pushing, popping, and peeking from both the head and tail.
  
- **`HiP/`**: Contains the Hi Protocol implementation.
  - `src/main.c`: The entry point that sets up the RAW sockets, epoll event loops, and determines if the node runs as a client or server.
  - `src/utils.c`: Core networking utilities, packet handling logic, and MAC address discovery. This is where the queue is utilized.
  - `src/pdu.c`: Logic for constructing, serializing, and deserializing the Protocol Data Units.
  - `include/`: Header definitions for the protocol (Ethernet, HiP headers, etc.).
  - `script/topo_p2p.py`: A script for setting up virtual network topologies to test the protocol.

## How to build it

To build the project, navigate to the `HiP` directory and run `make all`. The `Makefile` is configured to compile the `libq` code alongside the HiP code.

```bash
cd HiP
make all
```

This will produce the compiled binary at `HiP/bin/hip`.

## How it works

1. **Raw Sockets:** The program bypasses standard TCP/UDP stacks by opening a raw `AF_PACKET` socket. This allows it to construct and parse raw Ethernet frames directly.
2. **Client-Server Interaction:** 
   - A client prepares a PDU with a custom HiP header and a message (e.g., "Hello") and broadcasts it over the network.
   - A server listens for incoming packets using an `epoll` loop.
3. **Queue Integration in Packet Handling:** 
   - When the server receives an incoming PDU, the packet is deserialized and immediately enqueued into the `pkt_queue` using `libq`.
   - The server then prepares an acknowledgment (greeting the client back) and
     transmits it as a unicast packet.
   - Once the ACK is successfully sent, the server dequeues the original packet and safely frees its memory (`destroy_pdu`).
   - This workflow showcases how a network application might buffer or track state for inflight packets before an operation is fully completed.

## Usage

- Compile all programmes with `make all` in the `HiP` directory. If you get an
  error, make sure to create the `build` directory first using `mkdir build`.
- Create the mininet topology using `sudo mn --custom script/topo_p2p.py --topo
  mytopo`
- In the mininet console, access node A and B using `xterm A B` *(Note: You
  should have used the -Y argument in your SSH command: `ssh -Y
  debian@ip_address_of_your_VM`)*
- From the xterm consoles, `cd` to the `bin` directory and run `./hip s` at node
  B and `./hip c hello` at node A.
