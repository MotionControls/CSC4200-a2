#include "protocol.h"

int main(){
	// Seeding rand.
	srand((unsigned)time(NULL) ^ getpid());

	char temp = 0;
	//Packet packet = MakePacket((uint32_t)rand(), 0, &temp, sizeof(char), FLAG_FIN | FLAG_ACK);
	//LogPacket("", 0, packet);

	uint32_t* buffer = PacketSerialize(MakePacket((uint32_t)rand(), 0, &temp, sizeof(char), FLAG_FIN | FLAG_ACK));
	Packet newPacket = PacketDeserialize(buffer);
	
	LogPacket("", 1, newPacket);

	return 0;
}