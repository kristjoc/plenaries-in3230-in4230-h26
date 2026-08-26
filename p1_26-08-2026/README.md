# Plenary Session 1 - 26.08.2026 #


During this first Plenary session we discussed about the following topics:

* Course prerequisites
* NREC cloud
* Minimal Interconnection Protocol (MIP)

## Course Prerequisites ##

In the first part of the session, we discussed some of the prerequisites for the
course, which can be summarized as follows:

* An introductory course on Operating Systems and Networks
* Basic C programming

The prerequisites mentioned above are not mandatory. However, if a student lacks
these skills, it is highly recommended to attend the plenary sessions where we
will cover the necessary concepts required to complete the assignments.

We then proceeded with an interactive question-and-answer session to enable
students to effectively self-assess the skills needed for the assignments. You
can review the Mentimeter results
[here](https://github.com/kristjoc/plenaries-in3230-in4230-h26/blob/main/p1_26-08-2025/MentiResults_in3230-in4230-h25.pdf).

## NREC Cloud ##

Next, we focused on NREC, a cloud infrastructure platform delivered in
collaboration between the universities of Oslo and Bergen. We provided a
step-by-step demonstration of how to launch a virtual machine in the cloud and
access it via SSH. You can follow along with the detailed tutorial provided
[here](https://www.uio.no/studier/emner/matnat/ifi/IN3230/h26/obligatorisk-oppgave/running-your-vm-on-nrec.html).

Unfortunately, some students who have already submitted the form are not yet
part of the project. I have contacted NREC, and they have assured me that they
will add the requested usernames by Friday.

## Minimal Interconnection Protocol (MIP) ##

The final part of the session introduced MIP, a minimal network layer protocol
that we will implement to construct our simple network stack for the course
assignments. We covered a high-level overview of MIP and walked through its
specifications.

For more details about MIP, please refer to
[this](https://www.uio.no/studier/emner/matnat/ifi/IN3230/h26/obligatorisk-oppgave/ispec-mip-2026.txt)
RFC-like document.

## Next plenary ##

If today's plenary felt confusing due to all the new information and concepts,
don't worry. In our next session, we will continue from where we left off with
the MIP introduction. Through an illustrative example, we will explore all the
steps involved in the ping-pong process, including the application request via
the UNIX interface, the MIP-ARP request and reply, and the subsequent ping and
pong packets.

Additionally, we plan to continue the session with an introduction to UNIX and
RAW sockets, and code a simple implementation of MIP-ARP using RAW sockets that
helps three nodes (A --- B --- C) get to know each other. MIP-ARP is a crucial
part of the MIP daemon in the Oblig.

Meanwhile, consider to check <https://beej.us/guide/bgnet/>, which is a good
resource about socket programming in C.

Thank you all for today! I look forward to seeing you in the next session on
Wednesday, 02.09.2025, at 12:15 in OJD Seminarrom Caml (3438).
