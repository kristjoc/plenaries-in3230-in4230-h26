# Final Exam Question Pool: HTTP/1.x, HTTP/2, HTTP/3 and QUIC

---

## Part A: Legacy HTTP (HTTP/1.0 and HTTP/1.1)

### Question 1
In HTTP/1.0, each request/response uses its own TCP connection. For a single small object on a brand-new connection, what is the main cost of this design?

- A) The server must keep state about the client between requests
- B) One RTT for the TCP handshake plus one RTT for the request and first response byte, so about 2 RTT before the transmission time
- C) The client must open exactly one connection for the whole page
- D) The response cannot be sent until the TLS handshake finishes

### Question 2
Which of the following is **NOT** a benefit of HTTP/1.1 persistent (keepalive) connections?

- A) No new TCP three-way handshake for each request
- B) Avoids repeating TCP slow-start for every request
- C) Saves one round trip of latency per reused request
- D) Removes head-of-line blocking between responses on the same connection

### Question 3
In HTTP/1.1 pipelining, the server can interleave the bytes of several responses on the same TCP connection, so a slow response never delays the ones behind it.

- A) True
- B) False

### Question 4
A client uses pipelining to send requests for objects A, B and C on one connection. A is large and slow to produce, while B and C are small and ready immediately. What happens, and why?

- A) B and C arrive first, because small objects always get priority
- B) A, B and C arrive in parallel, because the server multiplexes the responses
- C) B and C wait until A is fully returned, because responses must be sent in order and cannot be interleaved (head-of-line blocking)
- D) The client must close the connection and open a new one for B and C

---

## Part B: HTTP/2

### Question 5
Which statement about the HTTP/2 **binary framing layer** is correct?

- A) It changes HTTP semantics, such as the meaning of methods and headers
- B) It keeps HTTP semantics unchanged but encodes messages in a binary format, so an HTTP/1.x client cannot talk to an HTTP/2-only server
- C) It makes messages human-readable, so they are easier to debug
- D) It replaces TCP with a new transport protocol

### Question 6
Match the HTTP/2 terms to the right definitions. Which option is **correct**?

- A) *Frame* = a complete request or response; *Message* = the smallest unit of communication; *Stream* = a single header field
- B) *Stream* = a bidirectional flow of bytes within a connection; *Message* = a complete sequence of frames forming a request or response; *Frame* = the smallest unit of communication, whose header identifies the stream it belongs to
- C) *Stream* = the smallest unit of communication; *Frame* = a TCP connection; *Message* = a TCP segment
- D) *Message* = a bidirectional flow of bytes; *Stream* = the frame header; *Frame* = a complete response

### Question 7
How does HTTP/2 achieve **request and response multiplexing** over a single TCP connection?

- A) It opens many parallel TCP connections, one per resource
- B) It splits messages into independent frames, interleaves frames from different streams on one connection, and reassembles them at the other end using the stream identifier
- C) It sends all responses strictly in the order the requests were made
- D) It compresses all responses into a single frame

### Question 8
In HTTP/2, a client can express which resources matter more by giving each stream a weight and, optionally, a dependency on another stream. This lets the server decide how to share the connection between streams.

- A) True
- B) False

### Question 9
What is the principle behind HTTP/2 **header compression**?

- A) Headers are removed from all requests after the first one
- B) The server and client each maintain an indexed list of headers already seen, so repeated headers can be sent as short references instead of full text
- C) Headers are sent over UDP to reduce size
- D) Headers are encrypted so that no one else can read them

### Question 10
Because HTTP/2 multiplexes streams on one connection, it fully eliminates head-of-line blocking at every layer. A lost packet at the transport layer no longer delays any stream.

- A) True
- B) False

---

## Part C: HTTP/3 and QUIC

### Question 11
How does QUIC (used by HTTP/3) avoid the transport-level head-of-line blocking that remains in HTTP/2 over TCP?

- A) It runs over UDP in user space and provides multiplexed streams with per-stream flow control, so a lost packet blocks only the stream whose data was in that packet
- B) It sends every resource on a separate TCP connection
- C) It disables retransmission of lost packets
- D) It relies on the operating-system kernel to reorder packets across all streams

### Question 12
Which statement about QUIC's security, connection setup and mobility is correct?

- A) TLS runs as a separate layer above QUIC, so QUIC needs a full TCP handshake followed by a separate TLS handshake
- B) QUIC integrates TLS 1.3, needs 1 RTT on a first connection, and can send encrypted data with 0-RTT on a repeat connection. Because a connection is identified by a Connection ID rather than the IP/port 4-tuple, it can survive a change of network (for example cellular to Wi-Fi)
- C) QUIC leaves payloads unencrypted so that middleboxes can inspect and optimise traffic
- D) QUIC ties each connection to the client's IP address, so a network change always requires a new handshake

---
