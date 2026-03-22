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
	struct sockaddr_storage* theirAddr = malloc(sizeof(struct sockaddr_storage));
	socklen_t theirSize = sizeof(struct sockaddr_storage);
	int sock;
	char ipstr[INET6_ADDRSTRLEN];
	
	sock = SetupServerSocket(NULL, port);
	
	printf("Listening...\n");
	while(1){
		// Get client ISN.
		printf("Shaking hands...\n");
		uint32_t* buffer;
		int numbytes = GetBuffer((struct sockaddr*)theirAddr, &theirSize, buffer, sock, HEADER_SIZE, -1);
		if(CheckRecv(numbytes, HEADER_SIZE)){
			close(sock);
			continue;
		}

		printf("Got buffer from %s.\n", inet_ntop(theirAddr->ss_family, &(((struct sockaddr_in*)theirAddr)->sin_addr), ipstr, sizeof(ipstr)));

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
			numbytes = SendBuffer(theirAddr, buffer, sock, HEADER_SIZE);
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
			
			numbytes = GetBuffer((struct sockaddr*)theirAddr, &theirSize, buffer, sock, HEADER_SIZE, -1);
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
		
		printf("Handshake complete.\n");

		// Close connection and continue listening.
		close(sock);
		printf("Listening...\n");
	}
	
	close(sock);
	printf("Exiting...\n");
	return 0;
}