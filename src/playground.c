#include "protocol.h"

int main(){
	// Seeding rand.
	srand((unsigned)time(NULL) ^ getpid());

	char* payload1 = "Hello World!";
	Packet packet1 = MakePacket(42, 69, (void*)payload1, strlen(payload1), 0);
	uint32_t* buffer = malloc(packet1.length + sizeof(Packet));
	PacketSerialize(buffer, packet1);
	LogPacket("", 0, packet1);
	
	char* test = malloc(packet1.length);
	memcpy(test, packet1.payload, packet1.length);
	test[packet1.length] = '\0';
	printf("%s\n", test);

	Packet packet2 = PacketDeserialize(buffer);
	LogPacket("", 1, packet2);
	test = malloc(packet2.length);
	memcpy(test, packet2.payload, packet2.length);
	printf("%s\n", test);

	return 0;
}