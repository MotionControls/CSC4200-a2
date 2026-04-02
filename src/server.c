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
	
	int sock = SetupServerSocket("localhost", port);
	
	uint32_t selfIsn = (uint32_t)rand();
	uint32_t theirIsn;
	uint32_t* buffer;
	while(1){
		struct timeval opt = {0,0};
		if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(opt)) == -1){
			perror("setsockopt err");
			return errno;
		}
		
		struct sockaddr_storage* theirAddr = malloc(sizeof(struct sockaddr_storage));
		socklen_t theirSize = sizeof(*theirAddr);
		
		// SYN+ACK packets.
		buffer = malloc(PACKET_SIZE);
		int numbytes = GetBuffer(theirAddr, &theirSize, buffer, sock);
		if(CheckSend(numbytes, HEADER_SIZE)) continue;

		Packet synPacket = PacketDeserialize(buffer);
		if(synPacket.flags != FLAG_SYN){
			printf("GetBuffer err: SYN flags incorrect.\n");
			continue;
		}

		theirIsn = synPacket.seq;
		LogPacket(logPath, 1, synPacket);
		printf("Recieved SYN.\nSending ACK...\n");

		char* dummy = "dummy";
		Packet synackPacket = MakePacket(selfIsn, synPacket.seq + 1, dummy, strlen(dummy), FLAG_ACK | FLAG_SYN);
		buffer = malloc(HEADER_SIZE + synackPacket.length);
		PacketSerialize(buffer, synackPacket);

		bool transFail = true;
		int tries = -1;
		while(transFail){
			transFail = false;
			tries++;
			if(tries >= MAX_RETRIES){
				printf("Attempted %i times.\n", tries);
				continue;
			}
			
			opt = (struct timeval){0,0};
			if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(opt)) == -1){
				perror("setsockopt err");
				return errno;
			}

			numbytes = SendBuffer((struct sockaddr*)theirAddr, buffer, sock, HEADER_SIZE + synackPacket.length);
			if(CheckSend(numbytes, HEADER_SIZE + synackPacket.length)) continue;
			LogPacket(logPath, 0, synackPacket);

			opt = (struct timeval){TIMEOUT_SEC, TIMEOUT_SEC};
			if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(opt)) == -1){
				perror("setsockopt err");
				return errno;
			}

			buffer = malloc(PACKET_SIZE);
			numbytes = GetBuffer(theirAddr, &theirSize, buffer, sock);
			if(CheckRecv(numbytes, HEADER_SIZE)) continue;
			
			Packet ackPacket = PacketDeserialize(buffer);
			if(ackPacket.flags != FLAG_ACK){
				printf("Getbuffer err: ACK flags incorrect.");
				continue;
			}
			LogPacket(logPath, 1, ackPacket);
		};
		if(tries >= MAX_RETRIES) continue;

		printf("Handshake complete.\n");

		// Data packets.
		uint32_t exSeq = synackPacket.seq + 1;
		char nameBuffer[200];
		int packets = 0;
		PayloadComp* compHead = malloc(sizeof(PayloadComp));
		PayloadComp* compCur = compHead;
		do{
			buffer = malloc(PACKET_SIZE);
			numbytes = GetBuffer(theirAddr, &theirSize, buffer, sock);
			if(CheckRecv(numbytes, HEADER_SIZE)) continue;

			Packet packet = PacketDeserialize(buffer);
			if(packet.seq != exSeq){
				/*	Ask for resend.	*/
			}

			uint32_t* payload = malloc(packet.length);
			memcpy(payload, packet.payload, packet.length);

			if(packets == 0){
				// Get filename.
				int nameSize = strlen((char*)payload);
				strcpy(nameBuffer, (char*)payload);
				printf("%s\n", nameBuffer);

				// Store payload.
				compCur->payload = payload + nameSize;
				compCur->size = packet.length - nameSize;
			}else{
				PayloadComp* cur = malloc(sizeof(*compCur));
				cur->payload = payload;
				cur->size = packet.length;
				compCur->next = cur;
			}

			// Send ACK.
			packet = MakePacket(0, exSeq, 0, 0, FLAG_ACK);
			buffer = malloc(HEADER_SIZE);
			PacketSerialize(buffer, packet);
			numbytes = SendBuffer((struct sockaddr*)theirAddr, buffer, sock, HEADER_SIZE);
			if(CheckRecv(numbytes, HEADER_SIZE)) continue;
			
			exSeq++;
			packets++;
		}while(1);
	}

	printf("Exiting...\n");
	return 0;
}