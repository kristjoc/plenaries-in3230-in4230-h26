# Plenary Session 3 - 11.09.2026 #

## Midterm Exam

The first part of this plenary session focused on preparing for the upcoming
midterm exam. We started with a Mentimeter quiz to give everyone an idea of what
to expect on the midterm, followed by an overview of the exam's format and the
topics it will cover.
[Here](https://github.com/kristjoc/plenaries-in3230-in4230-h26/blob/main/extras/midterm/midterm-prep-C-questions.md)
you can find the questions of the quiz along with explanations.

### What to Expect
- **Topics Covered**: The midterm will cover everything related to C-programming
and the mandatory assignment/home exams. This includes pointers, structures,
socket programming, and all the knowledge required to complete your project
code.
  
- **Exam Format**: The exam will consist of autograded questions such as:
  - Multiple-choice questions
  - True/False questions
  - Matching questions
    
- **Code-focused**: The questions will focus specifically on the C code from
  your projects.

### What NOT to Expect
- **Logical Concepts**: Most of the logical and theoretical parts of the course
  such as Routing, Transport, Flow Control will **not** be on the midterm. These
  topics will be covered on the final written exam instead.

### Recommendations for Preparation
To best prepare for the midterm, it is highly recommended that you:
1. Go through the exercises from the Plenaries.
2. Work thoroughly through the mandatory assignment and home exams, ensuring you
   deeply understand the C code you've written.


## Live Coding: HiP (Hi Protocol) over Ethernet

The second part of the session consisted of a live coding demonstration where we
         built a simple greeting protocol called **HiP (Hi Protocol)** from
         scratch. You can find all the code for this in the
         [HiP](https://github.com/kristjoc/plenaries-in3230-in4230-h26/tree/main/p3_11-09-2026/HiP/)
         folder in the repository.

Key activities during the live coding:
- **Raw Sockets (`AF_PACKET`)**: We set up raw sockets to build and send custom Layer 2 Ethernet frames directly.
- **Interface Discovery**: We used `getifaddrs()` to iterate over available network interfaces, filtering for `AF_PACKET` and skipping the loopback interface (`lo`).
- **Custom Protocol Headers**: We demonstrated how to build a custom protocol by constructing an Ethernet header followed by the custom `HiP` header (Destination, Source, Length, Version, and Type).
- **Sending & Receiving**: We used `sendto()` to send the custom frames (e.g., a broadcast HELLO message from Node A) and `recvfrom()` within an `epoll` event loop to receive and parse them (e.g., Node B replying with a unicast ACK).
- **Mininet**: We ran the code in a point-to-point Mininet topology (`topo_p2p.py`), testing the client/server interaction between two nodes.

This live coding exercise can serve as a **skeleton for your mandatory
assignment**, providing the foundation for working with RAW sockets and custom
packet structures!

**NOTE:** The examples we code during plenaries are intentionally simplified and
may contain hardcoded elements. For your assignments, make sure to write clean C
code with proper functions and clear comments.

Thank you all for today! I look forward to seeing you in the next session on
Wednesday, 30.09.2026, at 12:15 in OJD Seminarrom Caml (3438).
