# RTP-over-HiP-over-Ethernet (Live-Coding)

Minimalistic, layered implementation of RTP (Reliable Transfer Protocol) running
over HiP (Hi Protocol) over raw Ethernet frames (`AF_PACKET`).

Structured for clear and simple live coding in three dedicated libraries:
- [`libeth/`](https://github.com/kristjoc/plenaries-in3230-in4230-h26/tree/main/p7_28-10-2026/EthHipRtp-Live/libeth): Ethernet header building/parsing, raw socket creation, interface enumeration.
- [`libhip/`](https://github.com/kristjoc/plenaries-in3230-in4230-h26/tree/main/p7_28-10-2026/EthHipRtp-Live/libhip): HiP 4-byte header formatting, SDU padding to 4-byte boundaries, header validation.
- [`librtp/`](https://github.com/kristjoc/plenaries-in3230-in4230-h26/tree/main/p7_28-10-2026/EthHipRtp-Live/libeth): Go-Back-N sliding window, ACK handling, out-of-order detection, file chunking.

---

## Wire Format

```
+------------------------------------+-----------------------+---------------------------+--------------------+
| Ethernet Header (14 bytes)         | HiP Header (4 bytes)  | RTP Header (6 bytes)      | Data Payload       |
| dst_mac(6) | src_mac(6) | type(2)  | dst(1)|src(1)|len(1)|V:T(1)| ver:type(1) | seq(3) | len(2)| (0-1024 B) |
+------------------------------------+-----------------------+---------------------------+--------------------+
```

Total packet size on wire = 14 (Ethernet) + 4 (HiP) + 6 (RTP) + payload (padded to 4-byte word boundary).

---

## Directory Structure

```
EthHipRtp/
├── libeth/
│   ├── eth.h         # Ethernet API and eth_node definition
│   └── eth.c         # Socket creation, interface discovery, send/recv
├── libhip/
│   ├── hip.h         # HiP header definitions and alignment helpers
│   └── hip.c         # HiP header serialize and deserialize
├── librtp/
│   ├── rtp.h         # Go-Back-N structures and sender/receiver API
│   └── rtp.c         # Encapsulation / decapsulation & transmission logic
├── main.c            # Epoll event loop and clean CLI
├── Makefile          # Simple C11 build rules
└── topo_p2p.py       # Mininet point-to-point topology
```

---

## Building

```bash
make all
```

---

## Running in Mininet

1. **Start the Mininet point-to-point topology:**
   ```bash
   sudo -E mn --mac --custom topo_p2p.py --topo mytopo --link tc
   ```

2. **Open terminals for nodes A and B:**
   ```text
   mininet> xterm A B
   ```

3. **Start the receiver on Node B:**
   ```bash
   sudo ./rtp -l
   ```

4. **Send a file from Node A:**
   ```bash
   dd if=/dev/random of=<filename> count=1 bs=1M
   sudo ./rtp -c <filename>
   ```
