#include "../include/protocol.h"

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

	// Seed rand.
	srand((unsigned)time(NULL) ^ getpid());
	
	uint32_t lastIsn;
	struct sockaddr_in* myAddr = malloc(sizeof(struct sockaddr_in));
	struct sockaddr_in* theirAddr = malloc(sizeof(struct sockaddr_in));
	socklen_t mySize;
	int sock;
	char ipstr[INET6_ADDRSTRLEN];
	
	/*
	// Setup address.
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;	// Specify UDP.
	hints.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(NULL, port, &hints, &res);
	if(status != 0){
		printf("getaddrinfo err: %s.\n", gai_strerror(status));
		return 1;
	}
	*/
	
	// Create socket using given address info.
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	myAddr->sin_family = AF_INET;
	myAddr->sin_addr.s_addr = INADDR_ANY;
	myAddr->sin_port = htons(atoi(port));
	mySize = sizeof(struct sockaddr_in);
	
	// Tell socket to reuse given port, even if "in use."
	int reuse = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &reuse, sizeof(int)) < 0){
		perror("setsockopt err");
		return errno;
	}
	
	// Bind socket.
	printf("Binding socket...\n");
	if(bind(sock, (struct sockaddr*)myAddr, mySize) != 0){
		perror("bind err");
		return errno;
	}

	AddrToChar(ipstr, myAddr);
	printf("Hosting as %s:%s.\n", ipstr, port);
	
	/*
	// Listen for up to 10 connections.
	printf("Listening...\n");
	if(listen(sock, 10) != 0){
		perror("listen err");
		return errno;
	}
	*/
	
	printf("Listening...\n");
	while(1){
		/*
		// Get connection socket.
		sock = accept(sock, (struct sockaddr*)theirAddr, &theirSize);
		if(sock == -1){
			perror("accept err");
			close(sock);
			continue;
		}
		*/
		
		// Convert connected address to char*.
		/*
		void* addr;
		struct sockaddr* check = (struct sockaddr*)&theirAddr;
		if(check->sa_family == AF_INET){
			addr = &(((struct sockaddr_in*)check)->sin_addr);
		}else{
			addr = &(((struct sockaddr_in6*)check)->sin6_addr);
		}
		inet_ntop(theirAddr.ss_family, addr, ipstr, sizeof(ipstr));
		*/
		//AddrToChar(ipstr, theirAddr);
		//printf("Connected to %s.\n", ipstr);
		
		/*
		// Get packet.
		uint32_t packet[4];
		time_t startTime = time(NULL);
		int numbytes = GetBuffer(HEADER_SIZE, packet, sock, startTime, -1);
		if(CheckRecv(numbytes, HEADER_SIZE, startTime) || ntohl(packet[0]) != 17 || ntohl(packet[2]) != sizeof(ntohl(packet[3]))){
			close(sock);
			continue;
		}
		printf("Got packet:\n\tVersion: %i\n\tType: %i\n\tLength: %i\n\tPayload: %f\n", ntohl(packet[0]), ntohl(packet[1]), ntohl(packet[2]), (float)ntohl(packet[3]));
		
		// Resend packet.
		numbytes = send(sock, packet, HEADER_SIZE, 0);
		if(CheckSend(numbytes, HEADER_SIZE)){
			close(sock);
			continue;
		}
		*/

		// Get client ISN.
		printf("Shaking hands...\n");
		uint32_t* buffer;
		int numbytes = GetBuffer((struct sockaddr*)theirAddr, buffer, sock, HEADER_SIZE, -1);
		if(CheckRecv(numbytes, HEADER_SIZE)){
			close(sock);
			continue;
		}

		Packet fromPacket = PacketDeserialize(buffer);
		if(!(fromPacket.flags & FLAG_SYN >> 1)){
			printf("GetBuffer err: FLAG_SYN not set.");
			close(sock);
			continue;
		}
		uint32_t theirIsn = fromPacket.seq;

		bool doTransmit;
		int opt = 1;
		do{
			doTransmit = false;

			// Send server ISN.
			lastIsn = (uint32_t)rand();
			uint32_t dummy = 0;
			buffer = PacketSerialize(MakePacket(lastIsn, theirIsn + 1, &dummy, sizeof(uint32_t), FLAG_ACK | FLAG_SYN));
			numbytes = SendBuffer((struct sockaddr*)theirAddr, buffer, sock, HEADER_SIZE);
			if(CheckSend(numbytes, HEADER_SIZE)){
				close(sock);
				break;
			}

			// Wait for ACK.
			if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(opt)) < 0){
				perror("setsockopt err");
				close(sock);
				break;
			}
			
			numbytes = GetBuffer((struct sockaddr*)theirAddr, buffer, sock, HEADER_SIZE, -1);
			if(CheckRecv(numbytes, HEADER_SIZE)){
				// Retransmit if timed-out.
				if(errno == ETIMEDOUT){
					printf("Retransmitting...\n");

					doTransmit = true;
					continue;
				}
				
				close(sock);
				break;
			}
		}while(doTransmit);
		
		opt = 0;
		if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(opt)) < 0){
			perror("setsockopt err");
			close(sock);
			continue;
		}

		// Get ACK.
		numbytes = GetBuffer((struct sockaddr*)theirAddr, buffer, sock, HEADER_SIZE, -1);
		if(CheckRecv(numbytes, HEADER_SIZE)){
			close(sock);
			continue;
		}

		fromPacket = PacketDeserialize(buffer);
		if(!(fromPacket.flags & FLAG_ACK >> 1) || fromPacket.seq != theirIsn + 1 || fromPacket.ack != lastIsn + 1){
			printf("GetBuffer err: FLAG_SYN not set, incorrect SEQ, or incorrect ACK.");
			close(sock);
			continue;
		}

		printf("Handshake complete.\n");

		// Close connection and continue listening.
		close(sock);
		printf("Listening...\n");
	}
	
	close(sock);
	printf("Exiting...\n");
	return 0;
}