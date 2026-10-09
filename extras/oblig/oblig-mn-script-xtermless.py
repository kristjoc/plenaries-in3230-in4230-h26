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
#    sudo -E mn --mac --custom oblig-mn-script-xtermless.py --topo oblig --link tc
#
# 2. Second, inside the mininet console run 'init_oblig'
#
# 3. Third, inside the mininet console run 'EOF' to gracefully kill the mininet console
#    or 'init_oblig' to run another test.

class Oblig(Topo):
    "Simple topology for Oblig."

    def __init__(self):
        Topo.__init__(self)

        # Create 3 hosts, A, B, and C
        A = self.addHost('A')
        B = self.addHost('B')
        C = self.addHost('C')

        # Create p2p links between A - B and B - C
        self.addLink(A, B, bw=10, delay='10ms', loss=0.0, use_tbf=False)
        self.addLink(B, C, bw=10, delay='10ms', loss=0.0, use_tbf=False)

terms = []
log_files = []


def openProc(node, title, cmd, outdir):
    "Run cmd on node headless, logging stdout+stderr to outdir/title.log"

    safe_title = title.replace(" ", "_").replace("/", "-")
    logpath = os.path.join(outdir, "%s.log" % safe_title)
    logfile = open(logpath, "w")
    log_files.append(logfile)

    logfile.write("# cmd: %s\n" % cmd)
    logfile.flush()

    # 1. stdbuf -o0 -e0 forces line-buffered stdout/stderr like xterm
    # 2. Pass ['sh', '-c', ...] without shell=True to avoid breaking mnexec
    buffered_cmd = "stdbuf -o0 -e0 %s" % cmd
    return node.popen(["sh", "-c", buffered_cmd], stdout=logfile, stderr=logfile)


def init_oblig(self, line):
    "init is an example command to extend the Mininet CLI"

    net = self.mn
    A = net.get('A')
    B = net.get('B')
    C = net.get('C')

    # Clean up any leftover UNIX domain sockets from previous runs
    os.system("rm -f usockA usockB usockC")

    # Create a unique output directory for this test run
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
    terms.append(openProc(A, "Client_A_to_B", "./ping_client usockA \"Hello A=>B\" 20", outdir))
    time.sleep(3)

    terms.append(openProc(C, "Client_C_to_B", "./ping_client usockC \"Hello C=>B\" 20", outdir))
    time.sleep(3)

    # Expected ping timeout
    terms.append(openProc(A, "Client_A_to_C_timeout", "./ping_client usockA \"Hello A=>C\" 30", outdir))
    time.sleep(3)

    # Cached ping
    terms.append(openProc(A, "Client_A_to_B_cached", "./ping_client usockA \"Hello again A=>B\" 20", outdir))
    time.sleep(1)

    # Kill ping_server on Host B
    B.cmd("pkill -9 -f ping_server")

    # Run ping_server on Host C
    terms.append(openProc(C, "Server_C", "./ping_server usockC", outdir))
    time.sleep(1)

    # Run ping_client from B to C
    terms.append(openProc(B, "Client_B_to_C", "./ping_client usockB \"Hello B=>C\" 30", outdir))
    time.sleep(3)

    print("*** Test sequence finished. Check logs in %s/" % outdir)


# Mininet Callbacks
CLI.do_init_oblig = init_oblig

orig_EOF = CLI.do_EOF

def do_EOF(self, line):
    # Kill background daemons and child processes
    for host in self.mn.hosts:
        host.cmd("pkill -9 -f mipd")
        host.cmd("pkill -9 -f ping_server")
        host.cmd("pkill -9 -f ping_client")

    for t in terms:
        try:
            t.terminate()
        except OSError:
            pass

    for f in log_files:
        try:
            f.close()
        except OSError:
            pass

    os.system("rm -f usockA usockB usockC")
    return orig_EOF(self, line)

CLI.do_EOF = do_EOF

topos = {
    'oblig': (lambda: Oblig()),
}
