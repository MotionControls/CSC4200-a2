#include "protocol.h"

int main(){
	// Seeding rand.
	srand((unsigned)time(NULL) ^ getpid());

	char* path = "res/artofrally_1.jpg";

	uint8_t* buffer;
	size_t totalSize = GetFileContents(&buffer, path);
	char filename[strlen((char*)buffer)];
	strcpy(filename, (char*)buffer + FILESTR_SIZE);
	printf("%s\n", buffer);
	
	int packets = (int)ceil(1.0f * totalSize / (HEADER_SIZE + SPLITE_SIZE));
	int sent = 0;
	
	char* front = "downloads/";
	char downloadPath[strlen(filename) + strlen(front)];
	strcpy(downloadPath, front);
	strcat(downloadPath, filename);
	printf("Downloading to %s.\n", downloadPath);
	
	FILE* download = fopen(downloadPath, "wb");
	int filenameSize = FILESTR_SIZE + strlen(filename);
	//fwrite(buffer + filenameSize + 1, sizeof(uint8_t), totalSize - filenameSize, download);

	while(sent < packets){
		int realSize = ((sent * SPLITE_SIZE) + SPLITE_SIZE > (int)totalSize) ? totalSize - (sent * SPLITE_SIZE) : SPLITE_SIZE;

		uint8_t* tempBuffer = malloc(realSize);
		memcpy(tempBuffer, buffer +  + filenameSize + 1 + (sent*SPLITE_SIZE), realSize);
		fwrite(tempBuffer, sizeof(uint8_t), realSize, download);
		
		//for(int i = 0; i < realSize; i++) printf("%i\t%i $ %i $ %i\n", sent, *(tempBuffer + i), i, i + (sent * SPLITE_SIZE));

		free(tempBuffer);
		sent++;
	}

	fclose(download);
	
	return 0;
}