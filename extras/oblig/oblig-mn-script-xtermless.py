#!/usr/bin/env python

""" xtermless mininet script to test IN3230/IN4230 H26 Oblig assignment"""

from mininet.topo import Topo

from mininet.cli import CLI
import os
import signal
import time
from datetime import datetime


# Usage example:
#
# 1. First, run the following command to start mininet with this script:
#    sudo -E mn --mac --custom oblig-mn-script.py --topo oblig --link tc
#
# 2. Second, inside the mininet console run 'init_oblig'
#
# 3. Third, inside the mininet console run 'EOF' to gracefully kill the mininet console
#    or 'init_oblig' to run another test.

class Oblig(Topo):
    "Simple topology for Oblig."

    def __init__(self):
        "Set up our custom topo."

        # Initialize topology
        Topo.__init__(self)

        # Create 3 hosts, A, B, and C..
        A = self.addHost('A')
        B = self.addHost('B')
        C = self.addHost('C')

        # Create p2p links between A - B and B - C.
        self.addLink(A, B, bw=10, delay='10ms', loss=0.0, use_tbf=False)
        self.addLink(B, C, bw=10, delay='10ms', loss=0.0, use_tbf=False)

terms = []


def openProc(node, title, cmd, outdir):
    "Run cmd on node headless, logging stdout+stderr to outdir/title.log"

    safe_title = title.replace(" ", "_").replace("/", "-")
    logpath = os.path.join(outdir, "%s.log" % safe_title)
    logfile = open(logpath, "w")
    logfile.write("# cmd: %s\n" % cmd)
    logfile.flush()
    return node.popen(cmd, shell=True, stdout=logfile, stderr=logfile)


def init_oblig(self, line):
    "init is an example command to extend the Mininet CLI"

    net = self.mn
    A = net.get('A')
    B = net.get('B')
    C = net.get('C')

    # Create a unique output directory for this test run (never overwrites)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    outdir = os.path.join("outputs", "test_%s" % timestamp)
    os.makedirs(outdir, exist_ok=True)
    print("*** Logging output to %s/" % outdir)


    # Start MIP daemons
    terms.append(openProc(A, "Host_A", "./mipd -d usockA 10", outdir))
    time.sleep(1)

    terms.append(openProc(B, "Host_B", "./mipd -d usockB 20", outdir))
    time.sleep(1)

    terms.append(openProc(C, "Host_C", "./mipd -d usockC 30", outdir))
    time.sleep(1)

    # Run ping_server on Host B
    terms.append(openProc(B, "Server_B", "./ping_server usockB", outdir))
    time.sleep(1)

    # Run ping_clients on Hosts A and C
    # Since this is the first try, the RTT should be ~40ms
    terms.append(openProc(A, "Client_A_to_B", "./ping_client usockA \"Hello IN3230\" 20", outdir))
    time.sleep(3)

    terms.append(openProc(C, "Client_C_to_B", "./ping_client usockC \"Hello IN4230\" 20", outdir))
    time.sleep(3)

    # This MUST output 'ping timeout' since A is not able to reach C.
    terms.append(openProc(A, "Client_A_to_C_timeout", "./ping_client usockA \"Hello IN4230\" 30", outdir))
    time.sleep(3)

    # This time the RTT should be smaller, ~20ms, since MIP-ARP cache
    # is being utilized.
    terms.append(openProc(A, "Client_A_to_B_cached", "./ping_client usockA \"Hello again IN4230\" 20", outdir))


# Mininet Callbacks
# Inside mininet console run 'init_oblig'

CLI.do_init_oblig = init_oblig


# Inside mininet console run 'EOF' to gracefully kill the mininet console
orig_EOF = CLI.do_EOF


# Kill mininet console
def do_EOF(self, line):
    for t in terms:
        os.kill(t.pid, signal.SIGKILL)
    return orig_EOF(self, line)


CLI.do_EOF = do_EOF


# Topologies
topos = {
    'oblig': (lambda: Oblig()),
}
