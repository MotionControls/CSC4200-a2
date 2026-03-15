#include "shared.h"

int main(){
	// Seeding rand.
	srand((unsigned)time(NULL) ^ getpid());

	char temp = 0;
	Packet packet = MakePacket((uint32_t)rand(), 0, &temp, sizeof(char), FLAG_FIN | FLAG_ACK);
	LogPacket("", 0, packet);

	uint32_t* buffer = PacketSerialize(packet);
	Packet newPacket = PacketDeserialize(buffer);
	
	sleep(1);
	LogPacket("", 1, newPacket);

	return 0;
}