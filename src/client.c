#include "../include/shared.h"

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
	
	struct addrinfo hints, *res, *walk;
	int status, sock;
	char ipstr[INET6_ADDRSTRLEN];
	
	// Setup address.
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	status = getaddrinfo(serverIp, port, &hints, &res);
	if(status != 0){
		printf("getaddrinfo err: %s.\n", gai_strerror(status));
		return 1;
	}
	
	// Create socket using given address info.
	sock = CreateSocket(res);
	
	// Connect using socket.
	printf("Connecting to %s:%s...\n", serverIp, port);
	if(connect(sock, res->ai_addr, res->ai_addrlen) != 0){
		perror("connect err");
		return 1;
	}else{
		AddrToChar(ipstr, res);
		printf("Connected to %s.\n", ipstr);
		
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
		
		close(sock);
	}
	
	printf("Exiting...\n");
	return 0;
}