/*
Your server must:

- Receive exactly 12 bytes for the header.
- Loop on `recv()` until all 12 bytes are received.
- Use `ntohl()` to interpret each field.
- Print:
  - Version
  - Message Type
  - Message Length
- Validate that `Version == 17`.
- If valid, receive the payload based on `Message Length`.
- Send `"Hello"` back to the client.
*/

#include "shared.h"

int main(int argc, char** argv){
	// Check if user needs usage.
	if(argc > 1 && strcmp(argv[1], "--help") == 0){
		printf("USAGE: appserver [?address] [?port]\n");
		printf("\t?address: Address to host from. Defaults to first available machine address.\n\t?port: Port to host from. Defaults to 8008.\n");
		return 0;
	}
	
	printf("Starting server...\n");
	
	struct addrinfo hints, *res, *walk;
	struct sockaddr_storage theirAddr;
	socklen_t theirSize;
	int status, sock, newSock;
	char ipstr[INET6_ADDRSTRLEN];
	char* port = (argc > 2) ? argv[2] : USED_PORT;
	
	// Setup address.
	// https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	// If we don't specify an address, use first available when binding.
	if(argc > 1) hints.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(
		(argc > 1) ? argv[1] : NULL,	// NULL if address has not been specified.
		port,
		&hints, &res);
	if(status != 0){
		printf("getaddrinfo err: %s.\n", gai_strerror(status));
		return 1;
	}
	
	// Show available addresses.
	// Only shows given address if specified.
	WalkAddrInfo(res);
	
	// Create socket using given address info.
	sock = CreateSocket(res);
	
	// Tell socket to reuse port, even if "in use."
	int reuse = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1){
		perror("setsockopt err");
		return 1;
	}
	
	// Bind socket.
	printf("Binding socket...\n");
	if(bind(sock, res->ai_addr, res->ai_addrlen) != 0){
		perror("bind err");
		return 1;
	}
	
	// Listen for up to 10 connections.
	printf("Listening...\n");
	if(listen(sock, 10) != 0){
		perror("listen err");
		return 1;
	}
	
	while(1){
		theirSize = sizeof(theirAddr);
		
		// Get connection socket.
		newSock = accept(sock, (struct sockaddr*)&theirAddr, &theirSize);
		if(newSock == -1){
			perror("accept err");
			continue;
		}
		
		// Convert connected address to char*.
		void* addr;
		struct sockaddr* check = (struct sockaddr*)&theirAddr;
		if(check->sa_family == AF_INET){
			addr = &(((struct sockaddr_in*)check)->sin_addr);
		}else{
			addr = &(((struct sockaddr_in6*)check)->sin6_addr);
		}
		
		inet_ntop(theirAddr.ss_family, addr, ipstr, sizeof(ipstr));
		printf("Connected to %s.\n", ipstr);
		
		// Get packet.
		uint32_t packet[4];
		time_t startTime = time(NULL);
		int numbytes = GetBuffer(HEADER_SIZE, packet, newSock, startTime, -1);
		if(CheckRecv(numbytes, HEADER_SIZE, startTime) || ntohl(packet[0]) != 17 || ntohl(packet[2]) != sizeof(ntohl(packet[3]))){
			close(newSock);
			continue;
		}
		printf("Got packet:\n\tVersion: %i\n\tType: %i\n\tLength: %i\n\tPayload: %f\n", ntohl(packet[0]), ntohl(packet[1]), ntohl(packet[2]), (float)ntohl(packet[3]));
		
		// Resend packet.
		numbytes = send(newSock, packet, HEADER_SIZE, 0);
		if(CheckSend(numbytes, HEADER_SIZE)){
			close(newSock);
			continue;
		}
		
		// Close connection and continue listening.
		close(newSock);
		printf("Listening...\n");
	}
	
	close(sock);
	printf("Exiting...\n");
	return 0;
}