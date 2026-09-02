# RAW sockets

This example focuses entirely on RAW sockets and the MIP-ARP protocol. Using a
simple three-node topology (A-B-C), we cover the main functions a host needs to
call to:

	- send a simple broadcast ARP request via a RAW socket
	- send an ARP reply in response to a BROADCAST ARP request
	- walk through all the interfaces of a host and store their MAC addreses
	- etc.

Instead of using `struct msghdr` to send data via a RAW socket, an alternative
approach involves copying a `struct` into a byte array (serialization), sending
it via a RAW socket, and then deserializing the data at the receiving end. One
can chose to use serialization/deserialization or encapsulating the information
into `struct msghdr` in order to send/receive MIP packets via RAW sockets. Both
options are valid and will be discussed in detail in the next
plenary session.  


## Usage

- Compile all programmes with `make all` in the current directory
- Create the mininet topology using `sudo -E mn --mac --custom topo_p2p.py --topo mytopo`
- In the mininet console, access node h1 and h2 using `xterm A B`
  (You should have used -Y argument in the ssh command: `ssh -Y debian@ip_address_of_your_VM`
- From the xterm consoles, run `./receiver` at node B and `./sender` at node A

  Practise and implement new features in the applications.

