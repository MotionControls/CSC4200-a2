# Assignment 2 - TCP-Like Reliable Protocol over UDP

## Building
Run `make` to compile both programs.

## Server Usage
`./server -p [PORT] -s [LOG_PATH]`
    `PORT` - The port to host the server on. Must be >1024 for non-root usage.
    `LOG_PATH` - Path to log file. Will overwrite the file if it already exists.
All files are downloaded to `./downloads/` under the same name they are uploaded as.

## Client Usage
`./client -s [IP] -p [PORT] -l [LOG_PATH] -f [FILE_PATH]`
    `IP` - The ip the server is hosted under.
    `PORT` - The port the server is hosted under.
    `LOG_PATH` - Path to log file. Will overwrite file if it already exists.
    `FILE_PATH` - Path to file to transfer.

## Implementation Details
1. Server binds to UDP socket using first available IP, which is printed to external file called `addr`.
2. Client creates UDP socket.
3. Client sends SYN to socket.
4. Server identifies SYN and sends ACK.
5. Client identifies server ACK and sends additional ACK.
6. Server identifies ACK and readies file receiving.
7. If any of steps 3 to 5 fails, due to interruption or timeout, client resends SYN.
    a. Up to 5 times.
8. Client prepares file. If file is too big, split into chunks.
9. Client sends file packet.
10. Server gets file and appends the file data to the appropriate file in `./downloads/`.
11. Server sends ACK.
12. Client identifies ACK and prepares next file packet.
13. Repeat steps 9 to 12 for each packet, or if interruption or timeout.
    a. Up to 5 times, on failure.
14. Client sends FIN.
15. Server identifies FIN and sends ACK.
16. Server closes connection and prepares for next SYN.
17. Client identifies ACK and exits cleanly.

## Client Log Example
```
2026-04-06-14-08-41 SEND SEQ=2018904075 ACK=0 0 1 0 LEN=0
2026-04-06-14-08-41 RECV SEQ=368478028 ACK=2018904076 1 1 0 LEN=0
2026-04-06-14-08-41 SEND SEQ=368478028 ACK=2018904076 1 0 0 LEN=0
2026-04-06-14-08-41 SEND SEQ=2018904076 ACK=0 0 0 0 LEN=53248
2026-04-06-14-08-41 RECV SEQ=0 ACK=2018957324 1 0 0 LEN=0
2026-04-06-14-08-41 SEND SEQ=2018957324 ACK=0 0 0 0 LEN=53248
2026-04-06-14-08-41 RECV SEQ=0 ACK=2019010572 1 0 0 LEN=0
2026-04-06-14-08-41 SEND SEQ=2019010572 ACK=0 0 0 0 LEN=53248
2026-04-06-14-08-41 RECV SEQ=0 ACK=2019063820 1 0 0 LEN=0
2026-04-06-14-08-41 SEND SEQ=2019063820 ACK=0 0 0 0 LEN=53248
2026-04-06-14-08-41 RECV SEQ=0 ACK=2019117068 1 0 0 LEN=0
2026-04-06-14-08-41 SEND SEQ=2019117068 ACK=0 0 0 0 LEN=53248
2026-04-06-14-08-41 RECV SEQ=0 ACK=2019170316 1 0 0 LEN=0
2026-04-06-14-08-41 SEND SEQ=2019170316 ACK=0 0 0 0 LEN=49003
2026-04-06-14-08-41 RECV SEQ=0 ACK=2019219319 1 0 0 LEN=0
2026-04-06-14-08-41 SEND SEQ=2019219319 ACK=0 0 0 1 LEN=0
2026-04-06-14-08-41 RECV SEQ=0 ACK=2019219320 1 0 1 LEN=0
```