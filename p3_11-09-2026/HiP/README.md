## "HiP - Hi Protocol"

In this example, we're going to implement a simple greeting protocol called HiP
- Hi Protocol.

`topo_p2p.py` is the python script that generates the following mininet
topology..

    [NodeA]-ifAB---------ifBA-[NodeB]

NodeA will send a broadcast HiP packet via the RAW socket and NodeB will reply
via a unicast HiP packet.

### Before the session

- **Bring your laptop** — you'll need it to SSH into the VM and follow along.
  
- **SSH into the NREC VM**:

  ```
  ssh -Y -J <YourUioUsername>@login.uio.no debian@<VM IPv6>
  ```

  The `-Y` flag is required — it enables X11 forwarding, which is what lets
  `xterm` open later in the mininet console.

- **Clone the repository**:

  ```
  git clone https://github.com/kristjoc/plenaries-in3230-in4230-h26
  cd p3_11-09-2026/HiP/
  ```

Then, we can go straight into the hands-on part.

### Usage

- Compile all programmes with `make all` in the current directory.
- Create the mininet topology using `sudo mn --custom topo_p2p.py --topo mytopo`
- In the mininet console, launch node A and B xterms using `xterm A B`
  (this requires the `-Y` flag from the SSH command above)
- From the xterm consoles, run `./hip -s` at node B and `./hip -c HELLO` at node A

And this can be a skeleton for your MIP daemon!
