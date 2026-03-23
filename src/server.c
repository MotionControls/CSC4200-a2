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

	struct sockaddr_storage* theirAddr;
	socklen_t theirSize = sizeof(*theirAddr);
	uint32_t buffer;
	int numbytes = recvfrom(sock, &buffer, sizeof(buffer), 0, (struct sockaddr*)theirAddr, &theirSize);
	if(numbytes == -1){
		perror("recvfrom err");
		return errno;
	}

	printf("%u\n", buffer);

	close(sock);
	printf("Exiting...\n");
	return 0;
}