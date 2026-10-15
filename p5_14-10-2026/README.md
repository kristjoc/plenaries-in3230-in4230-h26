# Plenary session 5 - 14.10.2026 #

In the first part of the plenary session, we walked through the midterm exam questions and discussed each one along with the correct answers.

Later, we had a demonstration of how the mandatory assignment was graded, using
[this](https://github.com/kristjoc/plenaries-in3230-in4230-h26/blob/main/extras/oblig/oblig-mn-script-xtermless.py)
Mininet script.

Finally, in the last part of the plenary, we discussed BIRD, the BIRD Internet
Routing Daemon, which was originally developed as a school project at the
Faculty of Mathematics and Physics of Charles University in Prague.

## BIRD ##

The BIRD project aims to develop a fully functional dynamic IP routing daemon
primarily targeted on (but not limited to) Linux, FreeBSD and other UNIX-like
systems and distributed under the GNU General Public License. Read more about
BIRD [here](https://bird.network.cz/).  


Using
[this](https://github.com/kristjoc/plenaries-in3230-in4230-h26/blob/main/p5_14-10-2026/bird/init_mn.py)
mininet script, we will build a BIRD topology as shown in
[this](https://github.com/kristjoc/plenaries-in3230-in4230-h26/blob/main/p5_14-10-2026//bird/bird_topology.pdf)
figure.  

### Installation ###

To install BIRD on the NREC virtual machines, run `sudo apt install bird2`, which
will install BIRD version 2.17.5. The configuration files for r1, r2, and r3 are
written for this version. Remember to stop the BIRD service, as we will be using
the BIRD binary directly in the script.  

### Usage ###

To run the mininet script use the following command:

```
   $ sudo -E python3 init_mn.py
```

Inside the mininet CLI, launch xterms of the nodes with `xterm A B r1
r2 r3`. Inside the xterms, one can connect to the BIRD controller
using `birdc -s bird.ctl` in the r1, r2, or r3 directories and apply
Cisco-like commands (`show routes`, `show rip/ospf neighbors`, etc.).  

Use `wireshark`, `tshark` or `tcpdump` to capture HELLO and UPDATEs packets of
the routing protocols.  

When the routing protocol has converged, you should be able to ping from A to B
`ping 162.168.3.100`.  

Last but not least, check the BIRD source code from its
[GitLab](https://gitlab.nic.cz/labs/bird) repository and get some inspiration for
your MIP routing daemon.  
