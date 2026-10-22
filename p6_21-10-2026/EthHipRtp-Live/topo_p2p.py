""" Plenary IN3230/4230 - Point-to-Point topology

    A <----> B

    Usage: sudo -E mn --mac --custom topo_p2p.py --topo mytopo --link tc

"""

from mininet.topo import Topo

class MyTopo( Topo ):
    "Point to Point topology"

    def __init__( self ):
        "Create custom topo."

        # Initialize topology
        Topo.__init__( self )

        # Add hosts
        A = self.addHost( 'A' )
        B = self.addHost( 'B' )

        # Add link
        self.addLink(A, B, bw=10, delay='10ms', loss=0.0, use_tbf=False)

topos = { 'mytopo': ( lambda: MyTopo() ) }
