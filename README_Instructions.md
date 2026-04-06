# Program 2 – TCP-Like Reliable Protocol over UDP
CSC4200

**Individual Assignment — Do not collaborate**

**Reference:** https://beej.us/guide/bgnet/html/split-wide/

---

## Scenario

In PA1 you built a binary protocol on top of TCP, which gave you reliability and ordering for free.

TCP handles retransmission, in-order delivery, and connection management in the kernel. You never had to think about what happens when a packet is lost, duplicated, or arrives out of order.

This assignment removes that safety net.

You will implement a **TCP-like reliable delivery layer on top of UDP** (`SOCK_DGRAM`). UDP gives you nothing: no guaranteed delivery, no ordering, no duplicate detection. Every mechanism that made your PA1 communication reliable must now be built by you, explicitly, in C.

This is how protocols like QUIC, real-time game networking, and VoIP work. They need control over timing and retransmission behavior that TCP cannot provide, so they build reliability themselves on top of UDP.

Your protocol will implement:

- A **three-way handshake** to establish connection state (SYN → SYN|ACK → ACK)
- **Sequence numbers** to track byte position and detect loss
- **Acknowledgment numbers** to confirm received data
- **Retransmission timeouts** to recover from lost packets
- **FIN / FIN-ACK teardown** to close the connection cleanly

The protocol is split into three sprints of increasing complexity.

---

## Learning Objectives

By completing this assignment, you will:

- Understand why TCP's reliability features exist by implementing them yourself
- Construct and parse a custom binary packet header over UDP
- Implement a three-way handshake with random initial sequence numbers
- Build a stop-and-wait reliable transfer with timeout and retransmission
- Implement connection teardown with FIN and FIN-ACK
- Apply `htonl()` / `ntohl()` correctly in a new protocol context
- Read and write log files in a defined format

---

## Packet Format

The payload of every UDP datagram must begin with this **16-byte header**. All fields are in **network byte order** (big-endian).

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Sequence Number  (32 bits)                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                  Acknowledgment Number (32 bits)              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   Not Used (29 bits)                    |A|S|F|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Payload Length (32 bits)                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Payload (variable)                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Field Definitions

| Field | Size | Description |
|---|---|---|
| Sequence Number | 4 bytes | Byte offset of the first byte in this packet's payload. For SYN packets, this is the randomly chosen Initial Sequence Number (ISN). |
| Acknowledgment Number | 4 bytes | Valid only when ACK flag is set. Contains the **next** sequence number the sender expects to receive. |
| Flags | 4 bytes | Only the low 3 bits are used. All other bits must be zero. |
| Payload Length | 4 bytes | Number of payload bytes following the header. |

### Flag Bits

| Bit | Name | Meaning |
|---|---|---|
| 0 (F) | FIN | No more data from sender. Initiates teardown. |
| 1 (S) | SYN | Synchronize sequence numbers. Used during handshake only. |
| 2 (A) | ACK | Acknowledgment Number field is valid. |

---

## Protocol State Machine

```
Client                                    Server
  |                                          |
  |--- SYN  (seq=ISN_c) ------------------> |   Step 1
  |                                          |
  | <-- SYN|ACK (seq=ISN_s, ack=ISN_c+1) --|   Step 2
  |                                          |
  |--- ACK  (seq=ISN_c+1, ack=ISN_s+1) ---> |   Step 3  ← Handshake complete
  |                                          |
  |--- DATA (seq=ISN_c+1, len=N) ----------> |   Step 4
  | <-- ACK (ack=ISN_c+1+N) --------------- |   Step 5
  |          ... more packets ...            |
  |                                          |
  |--- FIN  (seq=last_seq) ---------------> |   Step 6
  | <-- FIN|ACK (ack=last_seq+1) ---------- |   Step 7  ← Connection closed
```

---

## Log Format

Every packet you send or receive must be logged in this exact format:

```
[YYYY-MM-DD-HH-MM-SS] SEND|RECV  SEQ=<n> ACK=<n> [ACK] [SYN] [FIN] [LEN=<n>]
```

- Print `SEND` for packets you transmit, `RECV` for packets you receive.
- Print flag tokens (`ACK`, `SYN`, `FIN`) only when the corresponding bit is set.
- Print `LEN=<n>` only when `payload_len > 0`.

Example log output:
```
[2026-03-15-14-22-01] SEND SEQ=482910234 ACK=0 SYN
[2026-03-15-14-22-01] RECV SEQ=903471122 ACK=482910235 ACK SYN
[2026-03-15-14-22-01] SEND SEQ=482910235 ACK=903471123 ACK
[2026-03-15-14-22-01] SEND SEQ=482910235 ACK=903471123 ACK LEN=512
[2026-03-15-14-22-01] RECV SEQ=903471123 ACK=482910747 ACK
[2026-03-15-14-22-01] SEND SEQ=482910747 ACK=903471123 FIN
[2026-03-15-14-22-01] RECV SEQ=903471123 ACK=482910748 ACK FIN
```

---

## Files You Are Given

```
pa2/
├── include/
│   └── protocol.h      ← Struct, constants, and function prototypes (read carefully)
├── src/
│   ├── protocol.c      ← YOUR WORK: implement the 4 helper functions
│   ├── server.c        ← YOUR WORK: implement the server logic
│   └── client.c        ← YOUR WORK: implement the client logic
└── Makefile            ← Build system (do not modify)
```

Read `protocol.h` completely before touching any `.c` file. Every function you need to implement is declared there with detailed step-by-step instructions.

---

## Build Instructions

```bash
# On GCP VM or any Linux system:
make          # builds ./server and ./client
make clean    # removes binaries and generated files
```

Requires: `gcc`, standard POSIX headers. Works on Ubuntu / Debian / Raspberry Pi.

---

## Running the Programs

### Server

```bash
./server -p <PORT> -s <LOGFILE>

# Example:
./server -p 5000 -s server.log
```

- `PORT` — UDP port to bind to (must be > 1024 for non-root)
- `LOGFILE` — path where packet logs are written
- The server **must run indefinitely**. It must not exit after a client disconnects.

### Client

```bash
./client -s <SERVER-IP> -p <PORT> -l <LOGFILE> -f <FILE>

# Example (local test):
./client -s 127.0.0.1 -p 5000 -l client.log -f photo.jpg

# Example (GCP VM to VM):
./client -s 10.128.0.5 -p 5000 -l client.log -f photo.jpg
```

- `SERVER-IP` — IPv4 address of the server VM
- `PORT` — must match what the server was started with
- `LOGFILE` — path where packet logs are written
- `FILE` — local file to transfer to the server

The server saves the received file as `received_<original-filename>` in its working directory.

---

# Sprint 1 – Three-Way Handshake (30 Points)

## Before You Start

Read this entire sprint before writing a single line of code.

Sprint 1 is about connection setup. You are implementing the TCP three-way handshake on top of UDP. Unlike TCP, the kernel gives you nothing — you are doing this entirely in userspace.

If your handshake is broken, data transfer will never work correctly. Do not move to Sprint 2 until Sprint 1 is solid.

---

## Goal

Establish a shared connection state between client and server.

After the handshake completes, both sides must know:
- Their own Initial Sequence Number (ISN)
- The other side's ISN

Sequence numbers are initialized to random 32-bit values, not 0. This is how real TCP prevents old packets from a previous connection from being confused with new ones.

---

## What You Must Do

### In `protocol.c` first

Implement all four functions: `packet_serialize`, `packet_deserialize`, `make_packet`, `log_packet`, `timestamp`.

These are used by both client and server. Nothing works until these are correct.

Test them in isolation before touching `server.c` or `client.c`. Print a packet, serialize it, deserialize it, and confirm the values round-trip correctly.

### Client (`client.c`)

1. Create a UDP socket with `socket(AF_INET, SOCK_DGRAM, 0)`.
2. Set `SO_RCVTIMEO` to `{TIMEOUT_SEC, TIMEOUT_USEC}`.
3. Generate a random ISN: seed with `srand((unsigned)time(NULL) ^ getpid())`, then call `rand()`.
4. Send a SYN packet (flags = `FLAG_SYN`, seq = ISN, ack = 0).
5. Wait for SYN|ACK. Validate: `FLAG_SYN | FLAG_ACK` both set, `ack_num == client_isn + 1`.
6. If timeout or invalid, retransmit SYN (up to `MAX_RETRIES`).
7. Send ACK (flags = `FLAG_ACK`, seq = client_isn + 1, ack = server_isn + 1).
8. Print "Handshake complete."

### Server (`server.c`)

1. Create a UDP socket, bind to port.
2. Wait for SYN (blocking `recvfrom` — no timeout yet).
3. Validate `FLAG_SYN` is set. Ignore other packets.
4. Generate your own random ISN.
5. Send SYN|ACK (seq = server_isn, ack = client_isn + 1).
6. Set `SO_RCVTIMEO`. Wait for ACK.
7. If timeout, retransmit SYN|ACK.
8. On valid ACK: print "Handshake complete", log the event.

---

## Technical Notes

- `rand()` returns values up to `RAND_MAX`. Cast to `uint32_t`.
- Retransmitting the SYN|ACK is necessary because UDP is unreliable — the SYN|ACK may have been lost.
- `recvfrom()` fills in the client's address on the server side. Save it — you need it to send replies.
- After the handshake, both sides track their own sequence number separately.

---

## Sprint 1 Deliverables

Submit a screen capture showing:

1. Server starts, binds, and prints "Listening on port X."
2. Client sends SYN.
3. Server prints SYN received with the client's ISN.
4. Server sends SYN|ACK.
5. Client prints "Handshake complete."
6. Server prints "Handshake complete."
7. Both log files show the three packets.
8. Server continues running after the client exits.

Be prepared to explain:

- Why ISNs are random, not 0.
- What happens if the SYN|ACK is lost.
- Why `recv()` on a UDP socket returns 0 bytes in some cases but means something different from TCP.
- Why the server uses `recvfrom()` instead of `recv()`.

---

# Sprint 2 – Reliable Data Transfer (35 Points)

## Before You Start

Read this entire sprint carefully.

Sprint 2 assumes your handshake works perfectly. Fix Sprint 1 completely before starting here.

You are implementing stop-and-wait reliable transfer. This is the simplest form of reliability: send one packet, wait for an ACK, then send the next. It is not efficient, but it is correct and easy to reason about.

---

## Goal

Transfer a file reliably from client to server, handling packet loss through timeout and retransmission.

---

## What You Must Do

### Client

After the handshake, `seq = client_isn + 1`.

**First packet — embed the filename:**

The first data packet has a special payload format:
```
"FILENAME:<basename>\0<file data...>"
```
Example: if you are sending `photo.jpg`, the payload starts with `FILENAME:photo.jpg\0` followed by as many file bytes as fit in `MAX_PAYLOAD`.

This lets the server know what to name the output file.

**All subsequent packets:**

Raw file bytes, up to `MAX_PAYLOAD` each.

**For each packet:**

```
loop up to MAX_RETRIES:
    sendto() the packet
    recvfrom() with SO_RCVTIMEO already set
    if timeout → retransmit
    if ACK received and ack_num == seq + payload_len → success
        advance seq += payload_len
        break
if still no ACK → exit with error
```

### Server

Track `expected_seq = client_isn + 1`.

**For each packet received:**

- If `seq_num != expected_seq`:
  - Send duplicate ACK (`ack = expected_seq`) — do not advance.
- If `seq_num == expected_seq` and payload present:
  - Write payload bytes to the output file.
  - Advance `expected_seq += payload_len`.
  - Send ACK (`ack = expected_seq`).

**First packet — extract filename:**

Look for the `"FILENAME:"` prefix. Extract the name up to the first `\0`. Open `received_<name>` for writing. Write any bytes that follow the `\0` in the same packet.

---

## Technical Notes

- Sequence numbers are byte-based, not packet-based — just like real TCP.
- `payload_len` in the ACK packet must be 0 (ACKs carry no data).
- `SO_RCVTIMEO` was set during socket creation — it applies to every `recvfrom()` call.
- Do not reset the timeout between packets; it is already set.
- `fwrite()` returns the number of items written. Check it.
- `fflush()` your log file after every write so logs appear immediately.

---

## Sprint 2 Deliverables

Submit a screen capture showing:

1. Client sends a file (use something at least 10 KB).
2. Server prints "Receiving file → received_<name>".
3. Transfer completes. Client prints bytes sent.
4. Server saves the file correctly (verify with `diff` or `md5sum`).
5. Both log files show data packets and ACKs with correct sequence numbers.
6. Demonstrate retransmission: kill the network briefly with `sudo iptables -A INPUT -p udp --dport <PORT> -j DROP`, then restore it.

Verify file integrity:
```bash
md5sum original.jpg
md5sum received_original.jpg
# hashes must match
```

Be prepared to explain:

- Why sequence numbers are byte-based.
- What a duplicate ACK means and when the server sends one.
- What happens if the client retries more than `MAX_RETRIES` times.
- Why you must loop on `recvfrom()` — what if you get an out-of-order packet while waiting for a specific ACK?

---

# Sprint 3 – Connection Teardown (35 Points)

## Before You Start

Read this sprint carefully. Sprint 3 is a live demo.

Sprint 3 assumes Sprints 1 and 2 work completely. If your transfer is not reliable, fix it before this sprint.

---

## Goal

Close the connection cleanly using FIN / FIN-ACK, and verify the complete system end-to-end.

---

## What You Must Do

### Client

After all file data is sent:

1. Send a FIN packet (flags = `FLAG_FIN`, seq = current seq, no payload).
2. Wait for FIN|ACK from the server (both `FLAG_FIN` and `FLAG_ACK` set).
3. If timeout, retransmit FIN.
4. On valid FIN|ACK: print "Connection closed cleanly." and exit.

### Server

When a FIN packet arrives:

1. Log: `":Interaction with <client IP> completed."`
2. Send FIN|ACK (flags = `FLAG_FIN | FLAG_ACK`, ack = fin_seq + 1).
3. Close the output file.
4. Remove the socket timeout.
5. Print "Waiting for next client..."
6. Return to the top of the outer loop.

---

## Technical Notes

- A FIN packet has no payload. `payload_len = 0`.
- The server's FIN|ACK ack_num should be `pkt.seq_num + 1`.
- After teardown, the server must accept a completely new connection — including a new handshake with fresh ISNs.
- The `received_<filename>` file must be complete and correct at teardown time.

---

## Sprint 3 Live Demo Checklist

You must demonstrate live in class:

- [ ] Server starts cleanly on GCP VM (or local)
- [ ] Client connects from a second VM (or `127.0.0.1`)
- [ ] Three-way handshake visible in logs
- [ ] File transfer completes
- [ ] `md5sum` confirms file integrity
- [ ] FIN / FIN-ACK teardown visible in logs
- [ ] Server accepts a second client immediately after
- [ ] Wireshark shows 16-byte header + payload on each data packet

Be prepared to explain live:

- What would happen if the FIN-ACK is lost.
- Why the server must remove the socket timeout after teardown.
- How out-of-order packets are handled at the server.
- What `seq + payload_len` represents and why the ACK uses that value.

---

## Submission Requirements

Submit these files to GitHub:

```
server.c       server implementation
client.c       client implementation
protocol.c     packet serialization, logging, helpers
protocol.h     header (provided — do not modify)
Makefile       build system (provided — do not modify)
README.md      describe your implementation (see below)
```

Your `README.md` must include:

1. Your name and the assignment name.
2. How to compile (`make`) and run both programs.
3. A description of your three-way handshake implementation.
4. A description of your retransmission logic.
5. A description of your teardown logic.
6. Any known limitations or bugs.
7. Sample log output from a successful run.

---

## Grading Rubric (Total: 100 Points)

Points are awarded only if you can clearly and correctly explain your code.
**If you cannot explain your implementation, points will not be awarded — even if the code works.**

| Sprint | Component | Points |
|---|---|---|
| Sprint 1 | Three-Way Handshake | 30 |
| Sprint 2 | Reliable Data Transfer | 35 |
| Sprint 3 | Connection Teardown (Live Demo) | 35 |
| | **Total** | **100** |

### Sprint 1 — Three-Way Handshake (30 Points)

| Category | Points |
|---|---|
| Correct SYN / SYN-ACK / ACK exchange | 10 |
| Random ISNs, correct ack_num arithmetic | 10 |
| Retransmission on timeout, error handling | 10 |

### Sprint 2 — Reliable Data Transfer (35 Points)

| Category | Points |
|---|---|
| Correct sequence number advancement | 10 |
| Timeout and retransmission logic | 10 |
| Out-of-order detection and duplicate ACK | 10 |
| File received correctly (md5sum match) | 5 |

### Sprint 3 — Connection Teardown / Live Demo (35 Points)

| Category | Points |
|---|---|
| Correct FIN / FIN-ACK exchange | 10 |
| Complete end-to-end system working | 10 |
| Wireshark demonstration | 5 |
| Technical explanation (live) | 10 |

---

## Advice

**Do not skip protocol.c.** Every other file depends on `packet_serialize()` and `packet_deserialize()`. Write them first, test them with a simple print loop before opening any socket.

**Do not send raw structs.** The compiler may add padding between fields. Two different compilers may lay out the same struct differently. Always serialize field-by-field using `memcpy()` + `htonl()`.

**Always loop on recvfrom.** A single call may time out, return a stale packet, or return a packet from a different source. Check return values and validate packet fields before acting on them.

**Test locally first.** Run both programs on `127.0.0.1` before deploying to two GCP VMs. This eliminates network variables while you debug protocol logic.

**Use Wireshark.** Install it on your GCP VM with `sudo apt install wireshark tshark`. Capture with:
```bash
sudo tshark -i eth0 -f "udp port 5000" -w capture.pcap
```
Open `capture.pcap` in Wireshark to inspect your byte layout.
