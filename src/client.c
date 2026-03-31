#include "../include/protocol.h"

int main(int argc, char** argv){
	if(argc < 9) return 1;
	
	// Get args.
	char* port, *logPath, *serverIp, *filePath;
	for(int i = 1; i < argc; i += 2){
		if(strcmp(argv[i], "-p") == 0){
			port = argv[i+1];
		}else if(strcmp(argv[i], "-l") == 0){
			logPath = argv[i+1];
		}else if(strcmp(argv[i], "-s") == 0){
			serverIp = argv[i+1];
		}else if(strcmp(argv[i], "-f") == 0){
			filePath = argv[i+1];
		}
	}

	if(port == NULL || serverIp == NULL || filePath == NULL || logPath == NULL){
		printf("Setup err: Missing args.\n");
		return 1;
	}
	
	printf("Starting client...\n\tport = %s\n\tIP = %s\n\tlog = %s\n\tfile = %s\n", port, serverIp, logPath, filePath);
	
	// Seed rand.
	srand((unsigned)time(NULL) ^ getpid());

	struct addrinfo* theirAddr = malloc(sizeof(struct addrinfo));
	
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int code = getaddrinfo(serverIp, port, &hints, &theirAddr);
	if(code != 0){
		printf("getaddrinfo err: %s\n", gai_strerror(code));
		return 1;
	}

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1){
		perror("socket err");
		return errno;
	}

	char foundAddr[INET_ADDRSTRLEN];
	printf("Socket setup for %s.\n", inet_ntop(theirAddr->ai_family, &(((struct sockaddr_in*)(theirAddr->ai_addr))->sin_addr), foundAddr, INET_ADDRSTRLEN));
	
	// SYN+ACK packets.
	struct timeval opt = {TIMEOUT_SEC, TIMEOUT_SEC};
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(opt)) == -1){
		perror("setsockopt err");
		return errno;
	}

	uint32_t selfIsn = (uint32_t)rand();
	uint32_t curSeq;

	Packet synackPacket;
	char* dummy = "dummy";
	uint32_t* buffer;
	bool transFail;
	int tries = -1;
	do{
		transFail = false;
		tries++;
		if(tries >= MAX_RETRIES){
			printf("Attempted %i times.\n", tries);
			close(sock);
			return 1;
		}
		
		printf("Sending SYN...\n");
		Packet synPacket = MakePacket(selfIsn, 0, dummy, strlen(dummy), FLAG_SYN);
		buffer = malloc(HEADER_SIZE + synPacket.length);
		PacketSerialize(buffer, synPacket);
		int numbytes = SendBuffer((struct sockaddr*)theirAddr->ai_addr, buffer, sock, HEADER_SIZE + synPacket.length);
		if(CheckSend(numbytes, HEADER_SIZE + synPacket.length)){transFail = true; continue;}
		LogPacket(logPath, 0, synPacket);

		buffer = malloc(HEADER_SIZE);
		socklen_t theirSize = sizeof(*theirAddr);
		numbytes = GetBuffer((struct sockaddr*)theirAddr->ai_addr, &theirSize, buffer, sock);
		if(CheckRecv(numbytes, HEADER_SIZE)){transFail = true; continue;}
		printf("Recieved ACK.\n");
		
		synackPacket = PacketDeserialize(buffer);
		LogPacket(logPath, 1, synackPacket);
		if(synackPacket.flags != (FLAG_SYN | FLAG_ACK) || synackPacket.ack != selfIsn+1){
			printf("GetBuffer err: ACK flags or ACK num incorrect.\n");
			transFail = true;
			continue;
		}

		curSeq = synackPacket.seq;
	}while(transFail);

	printf("Sending ACK...\n");
	Packet ackPacket = MakePacket(selfIsn + 1, curSeq++, dummy, strlen(dummy), FLAG_ACK);
	buffer = malloc(HEADER_SIZE + ackPacket.length);
	PacketSerialize(buffer, ackPacket);
	int numbytes = SendBuffer((struct sockaddr*)theirAddr->ai_addr, buffer, sock, HEADER_SIZE + ackPacket.length);
	if(CheckSend(numbytes, HEADER_SIZE + ackPacket.length)) return errno;
	LogPacket(logPath, 0, ackPacket);

	printf("Handshake complete.\nSending file...\n");

	// Data packets.
	uint8_t* fileBuffer;
	size_t fileSize = GetFileContents(fileBuffer, filePath);
	if((fileSize + HEADER_SIZE) > PACKET_SIZE){
		printf("\tFile too big. Splitting...\n");
		int packets = (HEADER_SIZE + SPLITE_SIZE) / fileSize;
		tries = -1;
		do{
			tries++;

			uint8_t* split = malloc(SPLITE_SIZE);
			memcpy(split, fileBuffer + (packets*SPLITE_SIZE), SPLITE_SIZE);
			
			Packet packet = MakePacket(selfIsn + 1, curSeq + SPLITE_SIZE, split, SPLITE_SIZE, 0);
			buffer = malloc(HEADER_SIZE + SPLITE_SIZE);
			PacketSerialize(buffer, packet);
			numbytes = SendBuffer((struct sockaddr*)theirAddr->ai_addr, buffer, sock, PACKET_SIZE + SPLITE_SIZE);
			if(CheckSend(numbytes, HEADER_SIZE + SPLITE_SIZE)){
				if(tries >= MAX_RETRIES)	return errno;
				else						continue;
			}
			LogPacket(logPath, 0, packet);

			packets--;
		}while(packets > 0);
	}else{
		tries = -1;
		bool sent = false;
		Packet packet;
		do{
			tries++;
			
			packet = MakePacket(selfIsn + 1, curSeq + fileSize, fileBuffer, fileSize, 0);
			buffer = malloc(HEADER_SIZE + fileSize);
			PacketSerialize(buffer, packet);
			numbytes = SendBuffer((struct sockaddr*)theirAddr->ai_addr, buffer, sock, PACKET_SIZE + fileSize);
			if(CheckSend(numbytes, HEADER_SIZE + fileSize)){
				if(tries >= MAX_RETRIES)	return errno;
				else						continue;
			}
			sent = true;
		}while(!sent);
		LogPacket(logPath, 0, packet);
	}

	close(sock);
	printf("Exiting...\n");
	return 0;
}