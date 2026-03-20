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

	uint32_t myIsn;
	struct sockaddr_in* theirAddr = malloc(sizeof(struct sockaddr_in));
	int sock;
	char ipstr[INET6_ADDRSTRLEN];
	
	/*
	// Setup address.
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	status = getaddrinfo(serverIp, port, &hints, &res);
	if(status != 0){
		printf("getaddrinfo err: %s.\n", gai_strerror(status));
		return 1;
	}
	*/
	
	// Create socket using given address info.
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	theirAddr->sin_family = AF_INET;
	if(inet_aton(serverIp, &(theirAddr->sin_addr)) == 0){
		perror("inet_aton err");
		close(sock);
		return errno;
	}

	// Set timeout.
	struct timeval timeOpt = {TIMEOUT_SEC, TIMEOUT_USEC};
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeOpt, sizeof(struct timeval)) < 0){
		perror("setsockopt err");
		close(sock);
		return errno;
	}

	// Set ISN.
	myIsn = (uint32_t)rand();
	
	// Connect using socket.
	/*
	printf("Connecting to %s:%s...\n", serverIp, port);
	if(connect(sock, (struct sockaddr*)theirAddr, theirSize) != 0){
		perror("connect err");
		return errno;
	}else{
	*/
	AddrToChar(ipstr, theirAddr);
	printf("Connected to %s.\n", ipstr);
	
	/*
	// Assemble packet.
	float payload = 42.0f;
	printf("Sending %f.\n", payload);
	int paylen = sizeof(float);
	uint32_t* packet = CreatePacket(17, 2, paylen, payload);
	
	// Send packet.
	int numbytes = send(sock, packet, HEADER_SIZE, 0);
	if(CheckSend(numbytes, HEADER_SIZE)){
		close(sock);
		return 1;
	}
	
	// Get response packet.
	time_t startTime = time(NULL);
	numbytes = GetBuffer(HEADER_SIZE, packet, sock, startTime, -1);
	if(CheckRecv(numbytes, HEADER_SIZE, startTime)){
		close(sock);
		return 1;
	}
	printf("Got packet:\n\tVersion: %i\n\tType: %i\n\tLength: %i\n\tPayload: %f\n", ntohl(packet[0]), ntohl(packet[1]), ntohl(packet[2]), (float)ntohl(packet[3]));

	if((float)ntohl(packet[3]) == payload) printf("Server returned matching float.\n");
	*/

	printf("Shaking hands...\n");
	bool doTransmit;
	int tries = 0;
	uint32_t theirIsn;
	uint32_t dummy = 0;
	do{
		if(tries == MAX_RETRIES){
			printf("err: Max retries reached.\n");
			close(sock);
			return 1;
		}
		
		doTransmit = false;
	
		// Send ISN.
		uint32_t* buffer = PacketSerialize(MakePacket(myIsn, 0, &dummy, sizeof(uint32_t), FLAG_SYN));
		int numbytes = SendBuffer(theirAddr, buffer, sock, HEADER_SIZE);
		if(CheckSend(numbytes, HEADER_SIZE)){
			close(sock);
			return 1;
		}

		// Get server ISN.
		numbytes = GetBuffer((struct sockaddr*)theirAddr, buffer, sock, HEADER_SIZE, -1);
		if(CheckRecv(numbytes, HEADER_SIZE)){
			printf("Retransmitting...\n");
			tries++;
			doTransmit = true;
			continue;
		}

		Packet fromPacket = PacketDeserialize(buffer);
		if(!(fromPacket.flags & FLAG_SYN >> 1) || !(fromPacket.flags & FLAG_ACK >> 2) || fromPacket.ack != myIsn + 1){
			printf("GetBuffer err: FLAG_SYN, FLAG_ACK not set, or incorrect ACK.\nRetransmitting...\n");
			tries++;
			doTransmit = true;
			continue;
		}
		theirIsn = fromPacket.seq;
	}while(doTransmit);

	// Send ACK.
	uint32_t* buffer = PacketSerialize(MakePacket(myIsn + 1, theirIsn + 1, &dummy, sizeof(uint32_t), FLAG_ACK));
	int numbytes = SendBuffer(theirAddr, buffer, sock, HEADER_SIZE);
	if(CheckSend(numbytes, HEADER_SIZE)){
		close(sock);
		return 1;
	}
	printf("Handshake complete.\n");

	close(sock);
	//}
	
	printf("Exiting...\n");
	return 0;
}