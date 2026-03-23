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
	//int sock = SetupClientSocket(theirAddr, serverIp, port);
	
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int code = getaddrinfo(serverIp, port, &hints, &theirAddr);
	if(code != 0){
		printf("getaddrinfo err: %s\n", gai_strerror(code));
		exit(1);
	}

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1){
		perror("socket err");
		exit(errno);
	}

	char foundAddr[INET_ADDRSTRLEN];
	printf("Socket setup for %s.\n", inet_ntop(theirAddr->ai_family, &(((struct sockaddr_in*)(theirAddr->ai_addr))->sin_addr), foundAddr, INET_ADDRSTRLEN));
	
	uint32_t buffer = 42;
	int numbytes = sendto(sock, &buffer, sizeof(buffer), 0, theirAddr->ai_addr, theirAddr->ai_addrlen);
	if(numbytes == -1){
		perror("sendto err");
		return errno;
	}

	close(sock);
	printf("Exiting...\n");
	return 0;
}