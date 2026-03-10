#include "shared.h"

int main(int argc, char** argv){
	// Check if required arguments exist, or user needs usage.
	if(argc <= 1 || strcmp(argv[1], "--help") == 0){
		printf("USAGE: appclient [address] [?port]\n");
		printf("\taddress: Address to connect to.\n\t?port: Port to connect to. Defaults to 8008.\n");
		return 0;
	}
	
	printf("Starting client...\n");
	
	struct addrinfo hints, *res, *walk;
	int status, sock;
	char ipstr[INET6_ADDRSTRLEN];
	char* port = (argc > 2) ? argv[2] : USED_PORT;
	
	// Setup address.
	// https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
	memset(&hints, 0, sizeof(hints));	// Clear memory.
	hints.ai_family = AF_UNSPEC;		// Use IPv4 or IPv6.
	hints.ai_socktype = SOCK_STREAM;	// Use TCP sockets.
	
	status = getaddrinfo(argv[1], port, &hints, &res);
	if(status != 0){
		printf("getaddrinfo err: %s.\n", gai_strerror(status));
		return 1;
	}
	
	// Create socket using given address info.
	sock = CreateSocket(res);
	
	// Connect using socket.
	printf("Connecting to %s:%s...\n", argv[1], port);
	if(connect(sock, res->ai_addr, res->ai_addrlen) != 0){
		perror("connect err");
		return 1;
	}else{
		// Convert connected address to char*.
		void* addr;
		struct sockaddr* check = (struct sockaddr*)res->ai_addr;
		if(check->sa_family == AF_INET){
			addr = &(((struct sockaddr_in*)check)->sin_addr);
		}else{
			addr = &(((struct sockaddr_in6*)check)->sin6_addr);
		}
		inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
		
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