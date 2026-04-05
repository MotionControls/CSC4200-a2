#include "protocol.h"

int main(){
	// Seeding rand.
	srand((unsigned)time(NULL) ^ getpid());

	FILE* check1 = fopen("res/artofrally_1.jpg", "rb");
	fseek(check1, 0L, SEEK_END);
	size_t size1 = ftell(check1);
	rewind(check1);
	uint8_t buffer1[size1];
	fread(buffer1, sizeof(uint8_t), size1, check1);
	fclose(check1);

	FILE* check2 = fopen("downloads/artofrally_1.jpg", "rb");
	fseek(check2, 0L, SEEK_END);
	size_t size2 = ftell(check2);
	rewind(check2);
	uint8_t buffer2[size2];
	fread(buffer2, sizeof(uint8_t), size2, check2);
	fclose(check2);

	if(size1 != size2) printf("File sizes differ:\t%li & %li\n", size1, size2);

	for(int i = 0; i < size1; i++){
		if(buffer1[i] != buffer2[i]) printf("Byte %i differs:\t%u & %u\n", i, buffer1[i], buffer2[i]);
	}
	
	return 0;
}