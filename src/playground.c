#include "protocol.h"

int main(){
	// Seeding rand.
	srand((unsigned)time(NULL) ^ getpid());

	char* path = "res/artofrally_1.jpg";

	/*
	FILE* file;
	file = fopen(path, "rb");
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	printf("Reading %li bytes.\n", size);

	char temp[strlen(path)];
	strcpy(temp, path);
	char* base = basename(temp);
	
	char* front = "FILENAME:";
	char* final = malloc(strlen(front) + strlen(base));
	strcpy(final, front);
	strcat(final, base);
	int totalSize = size + strlen(final);
	uint8_t* buffer = malloc(totalSize);

	printf("Reading %s...\n", base);

	size_t got = fread(buffer + strlen(final), sizeof(uint8_t), size, file);
	printf("\tRead %li of %li bytes.\n", got, size);
	if(ferror(file) || got != size){
		perror("fread err");
		return errno;
	}
	*/

	uint8_t* buffer = malloc(PACKET_SIZE);
	size_t totalSize = GetFileContents(buffer, path);

	int packets = (int)ceil(1.0f * totalSize / (HEADER_SIZE + SPLITE_SIZE));
	int sent = 0;
	
	while(sent < packets){
		int realSize = ((sent * SPLITE_SIZE) + SPLITE_SIZE > totalSize) ? totalSize - (sent * SPLITE_SIZE) : SPLITE_SIZE;
		
		uint8_t* tempBuffer = malloc(realSize);
		memcpy(tempBuffer, buffer + (sent*SPLITE_SIZE), realSize);
		
		for(int i = 0; i < realSize; i++) printf("%i\t%i $ %i $ %i\n", sent, *(tempBuffer + i), i, i + (sent * SPLITE_SIZE));

		sent++;
	}
	
	return 0;
}