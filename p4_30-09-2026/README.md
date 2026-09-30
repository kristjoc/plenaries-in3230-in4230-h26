# Plenary Session 4 - 30.09.2026 #

## Live Coding: HiP over Ethernet — Multi-Interface Extension

This plenary session was a hands-on live coding session where we extended the
**HiP (Hi Protocol)** implementation from the previous session to support
**multiple network interfaces** and a **three-node topology**. You can find all
the code in the
[HiP](https://github.com/kristjoc/plenaries-in3230-in4230-h26/tree/main/p4_30-09-2026/HiP++/)
folder in the repository.

### What Changed from Last Time

Previously, HiP only handled a single network interface and a simple
point-to-point topology between two nodes (A ↔ B). Today we made it fully
**multi-interface-aware**, so a node can discover, send, and receive on
**all** its network interfaces independently.

Key changes made during the session:

- **Three-node topology (`topo_p2p.py`)**: We extended the Mininet topology
  from a simple A ↔ B link to a three-node linear chain **A ↔ B ↔ C**,
  where node B has two interfaces, one facing A and one facing C.

      [NodeA]ifAB----ifBA[NodeB]ifBC----ifCB[NodeC]

- **EtherType filtering (`ether.h`)**: We changed the raw socket's EtherType
  from the catch-all `0xFFFF` to the specific value `0x3230` assigned to HiP,
  so the socket only picks up HiP frames and ignores unrelated traffic.

- **Per-interface packet queues (`utils.h`, `utils.c`)**: The single
  `pkt_queue` pointer was replaced with an array `pkt_queue[MAX_IF]`, giving
  each network interface its own independent queue. Each queue is initialised
  during interface discovery, keyed by the interface's `sll_ifindex`.

- **Interface-aware send (`send_hip_packet()`)**: The function now takes an
  explicit index `int i` so packets are sent out on the correct
  interface (`ifs->addr[i]`) instead of always defaulting to `addr[0]`.

- **Interface-pinned receive & reply (`handle_hip_packet()`)**: `recvfrom()`
  now captures the source `sockaddr_ll` to identify which interface a packet
  arrived on. The server pushes the PDU into the right per-interface queue and
  replies back through that **same interface** — a key step towards proper
  forwarding behaviour, needed for the Home Exam 1.

- **Multi-interface client (`main.c`)**: In client mode, the node now loops
  over all its interfaces and sends a greeting packet out of **each one**.
  On startup, it also prints the MAC address and `ifindex` of every interface.

### How to Run

1. Compile the code with `make all` in the `HiP/` directory.
2. Install the binary with `make install` (this also copies it to `script/`).
3. Create the Mininet topology:
   ```
   sudo mn --mac --custom script/topo_p2p.py --topo mytopo
   ```
4. Open terminals for all three nodes:
   ```
   xterm A B C
   ```
   (Remember to use `ssh -Y` when connecting to your VM.)
5. In each xterm, `cd` into the `bin/` directory and run:
   - Node B (middle, server): `./hip s`
   - Node C (server): `./hip s`
   - Node A (client): `./hip c HELLO`

   Node A will send a HiP greeting out of all its interfaces. Node B will
   receive it, push it to the right queue, and reply back via the same
   interface the request arrived on.

**NOTE:** The examples we code during plenaries are intentionally simplified and
may contain hardcoded elements. For your assignments, make sure to write clean C
code with proper functions and clear comments.

Thank you all for today! I look forward to seeing you in the next session on
Wednesday, 07.10.2026, at 12:15 in OJD Seminarrom Caml (3438).
