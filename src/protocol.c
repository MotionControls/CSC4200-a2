#include "protocol.h"

Packet MakePacket(uint32_t seq, uint32_t ack, void* payload, uint32_t length, uint32_t flags){
	Packet packet;
	packet.seq = seq;
	packet.ack = ack;
	packet.flags = flags;
	packet.length = length;
	packet.payload = payload;

	return packet;
}

/*	PacketSerialize(packet);
	Morphs packet into byte array.
packet 	; Packet.
*/
uint32_t* PacketSerialize(Packet packet){
	uint32_t* buffer = (uint32_t*)malloc(packet.length + sizeof(uint32_t)*4);

	uint32_t seq = htonl(packet.seq);
	uint32_t ack = htonl(packet.ack);
	uint32_t flags = htonl(packet.flags);
	uint32_t length = htonl(packet.length);

	memcpy(&buffer[0], &seq, sizeof(uint32_t));
	memcpy(&buffer[1], &ack, sizeof(uint32_t));
	memcpy(&buffer[2], &flags, sizeof(uint32_t));
	memcpy(&buffer[3], &length, sizeof(uint32_t));
	memset(&buffer[4], 1, packet.length);			// We don't care about this for sprint 1.

	return buffer;
}

/*	PacketDeserialize(buffer)
	Morphs buffer into packet struct.
buffer	;	Byte buffer.
*/
Packet PacketDeserialize(uint32_t* buffer){
	uint32_t seq = ntohl(buffer[0]);
	uint32_t ack = ntohl(buffer[1]);
	uint32_t flags = ntohl(buffer[2]);
	uint32_t length = ntohl(buffer[3]);

	void* payload = malloc(length);
	memcpy(&payload, &buffer[4], length);

	return MakePacket(seq, ack, payload, length, flags);
}

/*	LogPacket(log, packet)
	Logs a packet in the following format:
		[YYYY-MM-DD-HH-MM-SS] SEND|RECV  SEQ=<n> ACK=<n> [ACK] [SYN] [FIN] [LEN=<n>]
	Returns 1 on success, 0 otherwise.
log 	;	Logfile path.
recv	;	Assumes RECV if >0, SEND otherwise.
packet  ;	Packet.
*/
int LogPacket(char* log, int recv, Packet packet){
	// We're just gonna print for the moment.
	printf("%s %s SEQ=%u ACK=%u %u %u %u LEN=%u\n",
		Timestamp(),
		(recv) ? "RECV" : "SEND",
		packet.seq,
		packet.ack,
		(packet.flags & FLAG_ACK) >> 2,	// If unshifted, number shows up as >1 when set.
		(packet.flags & FLAG_SYN) >> 1,
		packet.flags & FLAG_FIN,
		packet.length);
	return 1;
}

/*	Timestamp()
	Returns a the current time in the format:
		YYYY-MM-DD-HH-MM-SS
*/
char* Timestamp(){
	char* buffer = malloc(50);
	time_t now = time(NULL);
	strftime(buffer, 50, "%Y-%m-%d-%H-%M-%S", localtime(&now));
	return buffer;
}

void AddrToChar(char* ipstr, struct sockaddr_in* info){
	void* addr;
	addr = &(info->sin_addr);
	inet_ntop(info->sin_family, addr, ipstr, sizeof(ipstr));
}

int GetBuffer(struct sockaddr* addr, void* buffer, int sock, int size, int expectedSize){
	printf("\tGetting buffer...\n");

	socklen_t fromlen = sizeof(*addr);
	int numbytes = 0;
	do{
		int got = recvfrom(sock, buffer + numbytes, size, 0, addr, &fromlen);
		if(got == -1){
			perror("recvfrom err");
			return -1;
		}

		numbytes += got;
		printf("\t%i / %i | %i\n", numbytes, size, expectedSize);
	}while(numbytes < size && (expectedSize <= -1 || numbytes < expectedSize));
	
	printf("\tGot %i bytes.\n", numbytes);
	return numbytes;
}

int SendBuffer(struct sockaddr_in* info, void* buffer, int sock, int size){
	char printAddr[25];
	printf("\tSending buffer to %s...\n", inet_ntop(info->sin_family, &(info->sin_addr.s_addr), printAddr, 25));

	socklen_t tolen = sizeof(struct sockaddr);
	int numbytes = 0;
	do{
		int sent = sendto(sock, buffer + numbytes, size, 0, (struct sockaddr*)info, tolen);
		if(sent == -1){
			perror("send err");
			return -1;
		}

		numbytes += sent;
		printf("\t%i / %i\n", numbytes, size);
	}while(numbytes < size);

	printf("\tSent %i bytes.\n", numbytes);
	return numbytes;
}

bool CheckRecv(int numbytes, int size){
	if(numbytes < size){
		perror("recvfrom err");
		printf("\tReceived %u bytes.\n", numbytes);
		return true;
	}
	
	return false;
}

bool CheckSend(int numbytes, int size){
	if(numbytes < size){
		perror("send err");
		printf("\tSent %u bytes.\n", numbytes);
		return true;
	}
	
	return false;
}