#ifndef SHARED_H
#define SHARED_H

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define USED_PORT	"8008"
#define BUFFER_SIZE	100
#define HEADER_SIZE	sizeof(uint32_t)*4
#define TIMEOUT		10

void WalkAddrInfo(struct addrinfo* res){
	char ipstr[INET6_ADDRSTRLEN];
	for(struct addrinfo* walk = res; walk != NULL; walk = walk->ai_next){
		void *addr;
		char *ipver;
		struct sockaddr_in *ipv4;
		struct sockaddr_in6 *ipv6;
		
		if(walk->ai_family == AF_INET){
			ipv4 = (struct sockaddr_in*)walk->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}else{
			ipv6 = (struct sockaddr_in6*)walk->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}
		
		inet_ntop(walk->ai_family, addr, ipstr, sizeof(ipstr));		
		printf("Using %s IP: %s.\n", ipver, ipstr);
	}
}

int CreateSocket(struct addrinfo* res){
	printf("Creating socket...\n");
	int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(sock == -1){
		perror("socket err");
		exit(1);
	}
	
	return sock;
}

/*	GetBuffer(size, buffer, sock, startTime, ?expectedSize);
	Returns number of bytes recieved.
*/
int GetBuffer(int size, void* buffer, int sock, time_t startTime, int expectedSize){
	printf("Getting buffer...\n");
	
	// recv() only blocks until there is data to read,
	// so if not all the data is present then we should ask for more.
	int numbytes = 0;
	do{
		numbytes += recv(sock, buffer + numbytes, size, 0);
		printf("\t%i / %i | %i\n", numbytes, size, expectedSize);
	}while(numbytes < size &&
		   (expectedSize <= -1 || numbytes < expectedSize) &&
		   time(NULL) - startTime <= TIMEOUT);
	
	printf("\tGot %i bytes.\n", numbytes);
	return numbytes;
}

/*	CheckSend(numbytes, size);
	Returns true if error, otherwise false.
*/
bool CheckSend(int numbytes, int size){
	if(numbytes < size){
		perror("send err");
		printf("Sent %u bytes.\n", numbytes);
		return 1;
	}
	
	return 0;
}

/*	CheckRecv(numbytes, size, startTime);
	Returns true if error, otherwise false.
*/
bool CheckRecv(int numbytes, int size, time_t startTime){
	if(numbytes < size){
		perror("recv err");
		printf("Received %u bytes.\n", numbytes);
		return true;
	}
	
	if(time(NULL) - startTime > TIMEOUT){
		printf("recv err: Timeout.\nReceived %i bytes.\n", numbytes);
		return true;
	}
	
	return false;
}

uint32_t* CreatePacket(int version, int type, int length, float payload){
	uint32_t* buffer = (uint32_t*)malloc((sizeof(uint32_t)*4) + length);

	uint32_t v = htonl(version);
	uint32_t t = htonl(type);
	uint32_t l = htonl(length);
	uint32_t p = htonl(payload);
	memcpy(&buffer[0], &v, sizeof(uint32_t));
	memcpy(&buffer[1], &t, sizeof(uint32_t));
	memcpy(&buffer[2], &l, sizeof(uint32_t));
	memcpy(&buffer[3], &p, sizeof(uint32_t));

	return buffer;
}

#endif