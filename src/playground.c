#include "protocol.h"

int main(){
	// Seeding rand.
	srand((unsigned)time(NULL) ^ getpid());

	/*
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
	*/

	FILE* file;
	file = fopen("test1.bin", "wb");
	char* buffer = "yeppers";
	fwrite(buffer, sizeof(char), strlen(buffer), file);
	fclose(file);

	printf("%s\n", buffer);
	
	file = fopen("test.jpg", "rb");
	fseek(file, 0L, SEEK_END);
	int size = ftell(file);
	
	fread(buffer, sizeof(char), size, file);
	printf("%s\n", buffer);

	fclose(file);
	
	return 0;
}