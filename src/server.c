#include "../include/shared.h"

int main(int argc, char** argv){
	if(argc < 5) return 1;
	
	// Get args.
	char* port, *logPath;
	for(int i = 1; i < argc; i += 2){
		if(strcmp(argv[i], "-p") == 0){
			port = argv[i+1];
		}else if(strcmp(argv[i], "-s") == 0){
			logPath = argv[i+1];
		}
	}

	if(port == NULL || logPath == NULL){
		printf("Setup err: Missing args.\n");
		return 1;
	}
	
	printf("Starting server...\n\tport = %s\n\tlog = %s\n", port, logPath);
	
	struct addrinfo hints, *res, *walk;
	struct sockaddr_storage theirAddr;
	socklen_t theirSize;
	int status, sock, newSock;
	char ipstr[INET6_ADDRSTRLEN];
	
	// Setup address.
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(NULL, port, &hints, &res);
	if(status != 0){
		printf("getaddrinfo err: %s.\n", gai_strerror(status));
		return 1;
	}
	
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

	AddrToChar(ipstr, res);
	printf("Hosting as %s:%s.\n", ipstr, port);
	
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