#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <arpa/inet.h>

struct __attribute__((__packed__)) superblock_t {

	uint8_t fs_id[8];
	uint16_t block_size;
	uint32_t file_system_block_count;
	uint32_t fat_start_block;
	uint32_t fat_block_count;
	uint32_t root_dir_start_block;
	uint32_t root_dir_block_count;
};

struct __attribute__((__packed__)) dir_entry_timedate_t {

	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

struct __attribute__((__packed__)) dir_entry_t {

	uint8_t status;
	uint32_t starting_block;
	uint32_t block_count;
	uint32_t size;
	struct dir_entry_timedate_t create_time;
	struct dir_entry_timedate_t modify_time;
	uint8_t filename[31];
	uint8_t unused[6];
};

struct superblock_t getSuperBlockInfo(uint8_t* data);
void printSuperBlockInfo(struct superblock_t superb);
void getFatInfo(uint8_t* data, struct superblock_t superb);
void getFileInfo(uint8_t* data, struct superblock_t superb, char* subdir);

struct dir_entry_t findSubdir
(uint8_t* data, struct superblock_t superb, char* subdir);

uint8_t* getFileData
(uint8_t* data, struct superblock_t superb, struct dir_entry_t file);

int findOpenFat(uint8_t* data, struct superblock_t superb, int blocksNeeded);

int findFileSlot(uint8_t* data, struct superblock_t superb, char* subdir); 

void updateDirectory
(uint8_t* data, struct superblock_t superb, char* fileName, 
 int startByte, int fatByte, int fileSize, struct tm* info); 

void copyToFS
(uint8_t* data, struct superblock_t superb, uint8_t* fileData, int* fatBytes);

int* updateFat
(uint8_t* data, struct superblock_t superb, int fatByte, int fileBlockCount);

int* checkFat(uint8_t* data, struct superblock_t superb);

void moveFileData(uint8_t* data, struct superblock_t superb, int* blocks); 

int* checkFatFiles(uint8_t* data, struct superblock_t superb);

int checkInUse(uint8_t* data, struct superblock_t superb, int block); 

void diskinfo(int argc, char* argv[]);
void disklist(int argc, char* argv[]);
void diskget(int argc, char* argv[]);
void diskfix(int argc, char* argv[]);

int main(int argc, char* argv[]) {
#if defined(PART1)
	diskinfo(argc, argv);
#elif defined(PART2)
	disklist(argc, argv);
#elif defined(PART3)
	diskget(argc, argv);
#elif defined(PART4)
	diskput(argc, argv);
#elif defined(PART5)
	diskfix(argc, argv);
#else
#	error "PART[12345] must be defined"
#endif
	return 0;
}

//Part 1
//Prints info of supplied FAT image
void diskinfo(int argc, char* argv[]) {
	if(argc != 2) {
		printf("Error incorrect number of args\n");
		exit(-1);
	}

	FILE *fp;
	fp = open(argv[1], O_RDONLY);
	struct stat sb;
	uint8_t* data;
	struct superblock_t superb;

	if(fp < 0) {
		printf("Error reading file\n");
		exit(-1);
	}

	if(fstat(fp, &sb) < 0) {
		printf("fstat error\n");
		exit(-1);
	}

	data = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fp, 0);
	if(data == MAP_FAILED) {
		printf("Error mapping file\n");
		exit(-1);
	}
	
	superb = getSuperBlockInfo(data);
	printSuperBlockInfo(superb);
	getFatInfo(data, superb);
	munmap(data, sb.st_size);
	close(fp);
	
}

//Done?
//Part 2
//Prints File and directory information from FAT
void disklist(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Error incorrect number of arguments\n");
		exit(-1);
	}

	FILE* fp;
	fp = open(argv[1], O_RDONLY);
	struct stat sb;
	uint8_t* data;
	struct superblock_t superb;

	if(fp < 0) {
		printf("Error reading file\n");
		exit(-1);
	}

	if(fstat(fp, &sb) < 0) {
		printf("fstat error\n");
		exit(-1);
	}

	data = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fp, 0);
	if(data == MAP_FAILED) {
		printf("Error mapping file\n");
		exit(-1);
	}

	superb = getSuperBlockInfo(data);
	if(argc == 3) {
		getFileInfo(data, superb, argv[2]);
	}
	else {
		getFileInfo(data, superb, NULL);
	}
	munmap(data, sb.st_size);
	close(fp);
}

//Part 3
//Done? needs some testing
void diskget(int argc, char* argv[]) {
	if(argc < 4) {
		printf("Error incorrect number of arguments\n");
		exit(-1);
	}

	FILE *fp;
	fp = open(argv[1], O_RDONLY);
	struct stat sb;
	uint8_t* data;
	struct superblock_t superb;
	struct dir_entry_t file;
	FILE *new;
	uint8_t* fileData;

	if(fp < 0) {
		printf("Error reading file\n");
		exit(-1);
	}

	if(fstat(fp, &sb) < 0) {
		printf("fstat error\n");
		exit(-1);
	}

	data = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fp, 0);
	if(data == MAP_FAILED) {
		printf("Error mapping file\n");
		exit(-1);
	}
	
	superb = getSuperBlockInfo(data);
	file = findSubdir(data, superb, argv[2]);
	if(file.status == 0) {
		printf("File not found\n");
		exit(-1);
	}	
	//get file data
	fileData = getFileData(data, superb, file);
	//copy file over
	new = fopen(argv[3], "w");
	if(new == NULL) {
		printf("Error unable to create file: %s\n", argv[3]);
		exit(-1);
	}
	fprintf(new, "%s", fileData);
	//clean up
	fclose(new);
	munmap(data, sb.st_size);
	close(fp);
	free(fileData);
}

//Part 4
void diskput(int argc, char* argv[]) {
	if(argc < 4) {
		printf("Error incorrect number of arguments\n");
		exit(-1);
	}

	FILE* fp;
	fp = open(argv[1], O_RDWR);
	struct stat sb;
	struct stat sb2;
	uint8_t* data;
	struct superblock_t superb;
	FILE* new;
	uint8_t* fileData;
	int openFatByte;
	struct dir_entry_t file;
	int fileSlotByte;
	int fileSize;
	struct tm* info;
	time_t t = time(NULL);
	int numBlocks;
	int* blocks;

	if(fp < 0) {
		printf("Error reading file\n");
		exit(-1);
	}

	if(fstat(fp, &sb) < 0) {
		printf("fstat error\n");
		exit(-1);
	}

	data = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
	if(data == MAP_FAILED) {
		printf("Error mapping file\n");
		exit(-1);
	}

	superb = getSuperBlockInfo(data);
	//Open new file "foo.txt"
	new = open(argv[2], O_RDONLY);
	if(new < 0) {
		printf("Error unable to open file: %s\n", argv[2]);
		exit(-1);
	}
	if(fstat(new, &sb2) < 0) {
		printf("fstat error\n");
		exit(-1);
	}
	//Put contents of foo.txt into string or string array
	fileData = mmap(NULL, sb2.st_size, PROT_READ, MAP_SHARED, new, 0);
	if(data == MAP_FAILED) {
		printf("Error mapping file\n");
		exit(-1);
	}
	fileSize = sb2.st_size;
	if(fileSize % superb.block_size == 0) {
		numBlocks = fileSize / superb.block_size;
	}
	else {
		numBlocks = (fileSize / superb.block_size) + 1;
	}
	//Check for next free slot on FAT
	openFatByte = findOpenFat(data, superb, numBlocks);
	if(openFatByte == -1) {
		printf("Error not enough space to copy to %s\n", argv[1]);
		exit(-1);
	}	
	//Find directory/subdir
	//Find free slot in directory
	fileSlotByte = findFileSlot(data, superb, argv[3]);
	if(fileSlotByte == -1) {
		printf("Error not enough space to copy to %s\n", argv[1]);
		exit(-1);
	}
	info = localtime(&t);
	//Add file data to directory table
	updateDirectory
		(data, superb, argv[2], fileSlotByte, openFatByte, 
		 fileSize, info);
	//Update FAT
	blocks = updateFat(data, superb, openFatByte, numBlocks);
	//Copy file contents to block given by FAT	
	copyToFS(data, superb, fileData, blocks);
	munmap(data, sb.st_size);
	munmap(fileData, sb2.st_size);
	free(blocks);
	close(fp);
	close(new);
}

//Part 5
void diskfix(int argc, char* argv[]) {
	if(argc != 2) {
		printf("Error incorrect number of args\n");
		exit(-1);
	}

	FILE *fp;
	fp = open(argv[1], O_RDWR);
	struct stat sb;
	uint8_t* data;
	struct superblock_t superb;
	int* blocks;
	int* fsBlocks;
	if(fp < 0) {
		printf("Error reading file\n");
		exit(-1);
	}

	if(fstat(fp, &sb) < 0) {
		printf("fstat error\n");
		exit(-1);
	}

	data = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fp, 0);
	if(data == MAP_FAILED) {
		printf("Error mapping file\n");
		exit(-1);
	}
	
	superb = getSuperBlockInfo(data);
	//Loop through FAT
	//check allocated blocks for errors
	blocks = checkFat(data, superb);
	//Move all violating data to new location
	moveFileData(data, superb, blocks);
	//Check file entries in FAT
	fsBlocks = checkFatFiles(data, superb);
	//check root directory table for errors
	
	munmap(data, sb.st_size);
	free(blocks);
	free(fsBlocks);
	close(fp);
	
}

//Returns a superblock struct containing superblock info for input
struct superblock_t getSuperBlockInfo(uint8_t* data) {
	struct superblock_t result;

	//Get FS name
	for(int i = 0; i < 8; i++) {
		result.fs_id[i] = data[i];
	}

	//Get Block size
	result.block_size = (data[8] << 8)+data[9];

	
	//Get Block count
	result.file_system_block_count = (data[10] << 24)+(data[11] << 16)+
		(data[12] << 8)+data[13];
	
	//Get FAT Start block
	result.fat_start_block = (data[14] << 24)+(data[15] << 16)+
		(data[16] << 8)+data[17];

	//Get FAT Block count
	result.fat_block_count = (data[18] << 24)+(data[19] << 16)+
		(data[20] << 8)+data[21];

	//Get Rootdir Start block
	result.root_dir_start_block = (data[22] << 24)+(data[23] << 16)+
		(data[24] << 8)+data[25];

	//Get Rootdir Block count
	result.root_dir_block_count = (data[26] << 24)+(data[27] << 16)+
		(data[28] << 8)+data[29];
	return result;			
}

//Prints superblock informaiton
void printSuperBlockInfo(struct superblock_t superb) {
	printf("Super block information: \n");
	printf("Block size: %d\n", superb.block_size);
	printf("Block count: %d\n", superb.file_system_block_count);
	printf("FAT starts: %d\n", superb.fat_start_block);
	printf("FAT blocks: %d\n", superb.fat_block_count);
	printf("Root directory start: %d\n", superb.root_dir_start_block);
	printf("Root directory blocks: %d\n\n", superb.root_dir_block_count);
}

//Prints FAT information
void getFatInfo(uint8_t* data, struct superblock_t superb) {
	int free_blocks = 0;
	int reserved_blocks = 0;
	int allocated_blocks = 0;
	
	int startByte = superb.fat_start_block * superb.block_size;
	int endByte = (superb.fat_start_block + superb.fat_block_count)*
		superb.block_size;
	uint32_t value;

	//Loop through FAT and count block status
	for(int i = startByte; i < endByte; i = i+4) {
		value = (data[i] << 24)+(data[i+1] << 16)+
		       (data[i+2] << 8)+data[i+3];
		if(value == 0) {
			free_blocks++;
		}	
		else if(value == 1) {
			reserved_blocks++;
		}
		else {
			allocated_blocks++;
		}
	}
	printf("FAT information: \n");
	printf("Free Blocks: %d\n", free_blocks);
	printf("Reserved Blocks: %d\n", reserved_blocks);
	printf("Allocated Blocks: %d\n", allocated_blocks);	
}

//Prints file information for all files in a given subdirectory
void getFileInfo(uint8_t* data, struct superblock_t superb, char* subdir) {
	int startByte = superb.root_dir_start_block * superb.block_size;
	int endByte = (superb.root_dir_start_block +
			superb.root_dir_block_count) * superb.block_size;
	char filename[32];
	struct dir_entry_t file;
	char type = '\0';

	if(subdir != NULL) {
		//return file and use that to find subdir?
		file = findSubdir(data, superb, subdir);
		if(file.status == 0) {
			printf("Could not find subdir %s\n", subdir);
			exit(-1);
		}
		startByte = file.starting_block * superb.block_size;
		endByte = (file.starting_block + file.block_count) *
			superb.block_size;
	}	
	for(int i = startByte; i < endByte; i = i+64) {
		file.status = data[i];
		if(file.status != 0) {
			//printf("status: %d\n", file.status);
	
			file.starting_block = (data[i+1] << 24)+
				(data[i+2] << 16)+(data[i+3] << 8)+data[i+4];
			//printf("starts at: %d\n", file.starting_block);
		
			file.block_count = (data[i+5] << 24)+
				(data[i+6] << 16)+(data[i+7] << 8)+data[i+8];
			//printf("# blocks: %d\n", file.block_count);

			file.size = (data[i+9] << 24)+(data[i+10] << 16)+
				(data[i+11] << 8)+data[i+12];
			//printf("file size: %d\n", file.size);
			
			file.create_time.year = 
				(data[i+13] << 8)+(data[i+14]);
			file.create_time.month = data[i+15];
			file.create_time.day = data[i+16];
			file.create_time.hour = data[i+17];
			file.create_time.minute = data[i+18];
			file.create_time.second = data[i+19];

			//printf("Create: %d/%d/%d\n", file.create_time.year,
			//		file.create_time.month,
			//		file.create_time.day);

			file.modify_time.year = 
				(data[i+20] << 8)+(data[i+21]);
			file.modify_time.month = data[i+22];
			file.modify_time.day = data[i+23];
			file.modify_time.hour = data[i+24];
			file.modify_time.minute = data[i+25];
			file.modify_time.second = data[i+26];

			//printf("Modify: %d/%d/%d\n", file.modify_time.year,
			//		file.modify_time.month,
			//		file.modify_time.day);

			for(int j = 0; j < 31; j++) {
				filename[j] = data[i+27+j];
			}	
			
			//printf("Filename: %s\n", filename);
			
			if(file.status == 3) {
				type = 'F';
			}	
			else if(file.status == 5) {
				type = 'D';
			}
			printf("%c %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n",
					type, file.size, filename,
					file.modify_time.year,
					file.modify_time.month,
					file.modify_time.day,
					file.modify_time.hour,
					file.modify_time.minute,
					file.modify_time.second);
		}
	}

}
//How to do /subdir/subdir2?
//Done?
//needs more error handling
struct dir_entry_t findSubdir
(uint8_t* data, struct superblock_t superb, char* subdir) {
	int startByte = superb.root_dir_start_block * superb.block_size;
	int endByte = (superb.root_dir_start_block +
			superb.root_dir_block_count) * superb.block_size;
	
	int check = 0;
	struct dir_entry_t file;
	char filename[32];
	int counter = 1;
	int counter2 = 0;
	int counter3 = 0;
	char directories[1024][1024];
	char letter = subdir[counter];

	if(subdir == NULL) {
		file.status = 0;
		return file;
	}
	//Split directory path
	while(letter != '\0') {
		if(letter != '/') {
			directories[counter2][counter3] = letter;
			counter3++;
		}
		else {
			directories[counter2][counter3] = '\0';
			counter2++;
			counter3 = 0;
		}
		counter++;
		letter = subdir[counter];
	}
	counter2++;
	//Subdir string = "/"
	if(counter2 == 1 && counter == 1) {
		file.status = 3;
		file.starting_block = superb.root_dir_start_block;
		file.block_count = superb.root_dir_block_count;
		return file;
	}
	//Find subdir
	counter = 0;
	for(int i = startByte; i < endByte; i = i+64) {
		file.status = data[i];
		if(file.status != 0) {
			for(int j = 0; j < 31; j++) {
				filename[j] = data[i+27+j];
			}
			if(strcmp(filename, directories[counter]) == 0) {
				counter++;
				file.starting_block = (data[i+1] << 24)+
					(data[i+2] << 16)+(data[i+3] << 8)+
					data[i+4];
				file.block_count = (data[i+5] << 24)+
					(data[i+6] << 16)+(data[i+7] << 8)+
					data[i+8];
				startByte = file.starting_block *
					superb.block_size;
				endByte = (file.starting_block +
					file.block_count) * superb.block_size;
				i = startByte-64;
			}
		}
		if(counter >= counter2) {
			check = 1;
		}
	}
	//unable to find subdir
	if(check == 0) {
		file.status = 0;
		return file;
	}
	else {
		file.status = 5;
	}	
	return file;
}


//Gets data of given file from disk image
//returns in uint8_t array the contents of the file
uint8_t* getFileData
(uint8_t* data, struct superblock_t superb, struct dir_entry_t file) {
	int counter = 0;
	int buffersize = 1024;
	uint8_t* fileData = malloc(buffersize * sizeof(uint8_t));
	
	if(fileData == NULL) {
		printf("Error allocating memory\n");
		exit(-1);
	}
	int startByte = file.starting_block * superb.block_size;
	int endByte = (file.starting_block + file.block_count)*
		superb.block_size;

	
	if(file.status == 0) {
		return NULL;
	}

	for(int i = startByte; i < endByte; i++) {
		fileData[counter] = data[i];
		counter++;
		if(counter >= buffersize) {
			buffersize += 1024;
			fileData = realloc(fileData, buffersize);
			if(fileData == NULL) {
				printf("Error allocating memory\n");
			}
		}
	}
	//printf("%s\n", fileData);
	return fileData;
}

//Finds first open FAT block
int findOpenFat(uint8_t* data, struct superblock_t superb, int blocksNeeded) {
	int startByte = superb.fat_start_block * superb.block_size;
	int endByte = (superb.fat_start_block + superb.fat_block_count)*
		superb.block_size;

	uint32_t value;
	int result = -1;
	int counter = 0;
	int check = 0;
	int freeBlocks = 0;
	
	for(int i = startByte; i < endByte; i=i+4) {
		value = (data[i] << 24)+(data[i+1] << 16)+(data[i+2] << 8)+
			data[i+3];
		if(value == 0) {
			if(check == 0) {
				check = 1;
			}	
			freeBlocks++;
		}
		if(check == 0) {
			counter++;
		}
	}
	if(freeBlocks < blocksNeeded) {
		printf("Error not enough space to execute copy\n");
		exit(-1);
	}
	return counter;
}

//Finds next slot in given subdirectory
int findFileSlot (uint8_t* data, struct superblock_t superb, char* subdir) {
	int startByte;
	int endByte;
	char directories[1024][1024];
	int counter = 1;
	int counter2 = 0;
	int counter3 = 0;
	char letter = subdir[counter];
	char path[1024] = "";
	struct dir_entry_t file;

	//Split dir and find subdir location on image
	while(letter != '\0') {
		if(letter != '/') {
			directories[counter2][counter3] = letter;
			counter3++;
		}
		else {
			counter3 = 0;
			counter2++;
		}
		counter++;
		letter = subdir[counter];
	}
	for(int i = 0; i < counter2; i++) {
		strcat(path, "/");
		strcat(path, directories[i]);
	}
	//"/foo.txt"
	if(counter2 == 1 && counter3 != 1) {
		file = findSubdir(data, superb, "/");
	}	
	else {
		file = findSubdir(data, superb, path);
	}
	if(file.status == 0) {
		printf("Error subdir not found\n");
		exit(-1);
	}

	//Find empty slot in subdir
	startByte = file.starting_block * superb.block_size;
	endByte = (file.starting_block + file.block_count)*
		superb.block_size;
	
	for(int i = startByte; i < endByte; i = i+64) {
		file.status = data[i];
		if(file.status == 0) {
			return i;
		}
	}
	//No space found
	return -1;
}

//Copy file data to directory table
void updateDirectory
(uint8_t* data, struct superblock_t superb, char* fileName, 
 int startByte, int fatByte, int fileSize, struct tm* info) {
	int endByte = startByte + 64;
	int value;
	int startingBlock;
	//Set status to file
	value = 0x03;
	memcpy(data+startByte, &value, 1);
	//Set starting block
	value = htonl(fatByte);
	memcpy(data+startByte+1, &value, 4);	
	//Set number of blocks
	if(fileSize % superb.block_size == 0) {
		value = htonl(fileSize / superb.block_size);
	}
	else {
		value = htonl((fileSize / superb.block_size) + 1);
	}
	memcpy(data+startByte+5, &value, 4);
	//Set file size
	value = htonl(fileSize);
	memcpy(data+startByte+9, &value, 4);
	//Set create/modify time
	//Year
	value = htons(info->tm_year+1900);
	memcpy(data+startByte+13, &value, 2);
	memcpy(data+startByte+20, &value, 2);
	//Month
	value = info->tm_mon + 1;
	memcpy(data+startByte+15, &value, 1);
	memcpy(data+startByte+22, &value, 1);
	//Day
	value = info->tm_mday;
	memcpy(data+startByte+16, &value, 1);
	memcpy(data+startByte+23, &value, 1);
	//Hour
	value = info->tm_hour;
	memcpy(data+startByte+17, &value, 1);
	memcpy(data+startByte+24, &value, 1);
	//Minute
	value = info->tm_min;
	memcpy(data+startByte+18, &value, 1);
	memcpy(data+startByte+25, &value, 1);
	//Second	
	value = info->tm_sec;
	memcpy(data+startByte+19, &value, 1);
	memcpy(data+startByte+26, &value, 1);
	//Set filename
	int i = 0;
	while(fileName[i] != '\0' && i < 32) {
		value = fileName[i];
		memcpy(data+startByte+27+i, &value, 1);
		i++;
	}		
}

//Copy file data to block given by FAT
void copyToFS
(uint8_t* data, struct superblock_t superb, uint8_t* fileData, int* fatBytes) {
	int startByte;
	int counter = 0;
	int fileDataOffset = 0;
	while(fatBytes[counter] != NULL) {
		counter++;
	}
	//What to do with non continuous blocks?
	for(int i = 0; i < counter; i++) {
		startByte = (fatBytes[i] * superb.block_size);
		for(int j = 0; j < superb.block_size; j++) {
			memcpy(data+startByte+j, fileData+fileDataOffset, 1);
			fileDataOffset++;
		}	
	}	

}

//Updates FAT and returns the list of blocks updated
//Updates FAT with new file values as specified by 
//"fileBlockCount"
int* updateFat
(uint8_t* data, struct superblock_t superb, int fatByte, int fileBlockCount) {
	int startByte = (superb.fat_start_block * superb.block_size)+
		(fatByte * 4);
	int endByte = (superb.fat_start_block + superb.fat_block_count)*
		superb.block_size; 
	int value;
	int previousOffset = 4;
	int counter = 0;
	int currentBlock = fatByte;
	int* result = malloc(1024*sizeof(int));
	if(result == NULL) {
		printf("Error allocating memory\n");
		exit(-1);
	}
	result[0] = currentBlock;
	//Needs to know next fat entry to point to
	for(int i = startByte+4; i < endByte; i = i+4) {
		if(counter == fileBlockCount-1) {
			value = htonl(0xffffffff);
			memcpy(data+i-previousOffset, &value, 4);
			break;
		}
		currentBlock++;
		value = (data[i] << 24)+(data[i+1] << 16)+(data[i+2] << 8)+
			data[i+3];
		//What if data is spread apart? i.e. not continuous in FAT
		//update offset to indicate the gap between data
		if(value != 0) {
			previousOffset += 4;
			continue;
		}
		//Reset offset to point to previous block
		result[counter+1] = currentBlock;
		value = htonl(currentBlock);
		memcpy(data+i-previousOffset, &value, 4);
		previousOffset = 4;
		counter++;
	}
	//Return available block list
	return result;
}

//Checks FAT for violations of reserved block space
//Returns int array with pairs {violating block, next free block}
int* checkFat(uint8_t* data, struct superblock_t superb) {
	int startByte = superb.fat_start_block * superb.block_size;
	int endByte = startByte+
	       	(4 * (superb.fat_block_count+superb.fat_start_block));
	uint32_t value;
	uint32_t temp;
	int offset = 0;
	int counter = 0;
	int* result = malloc(1024*sizeof(int));
	if(result == NULL) {
		printf("Error allocating memory\n");
		exit(-1);
	}
	int newBlock;
	int currentBlock = superb.fat_start_block;
	//Check superblock allocation is reserved
	value = (data[startByte] << 24)+(data[startByte+1] << 16)+
		(data[startByte+2] << 18)+data[startByte+3];
	//Fix it if not
	if(value != 1) {
		value = htonl(0x01);
		memcpy(data+startByte, &value, 4); 
	}
	//Check remaining FAT
	startByte += superb.fat_start_block*4;
	for(int i = startByte; i < endByte; i = i+4) {
		value = (data[i] << 24)+(data[i+1] << 16)+(data[i+2] << 18)
			+data[i+3];
		if(value != 1) {
			//Find free space and remember blocks
			newBlock = findOpenFat(data, superb, 1);
			result[counter] = currentBlock;
			result[counter+1] = newBlock;
			counter += 2;
			//Adjust FAT
			temp = htonl(0x01);
			memcpy(data+i, &temp, 4);

			temp = htonl(value);
			offset = (superb.fat_start_block * superb.block_size)+ 
				(newBlock * 4);
			memcpy(data+offset, &temp, 4);
		}
		currentBlock++;
	}
	return result;

}

//Moves Data from 1 block in img file to another block
//Input array "blocks" must be in the form of pairs:
//ex. {source block, dest block,...}
void moveFileData(uint8_t* data, struct superblock_t superb, int* blocks) {
	uint8_t fileData[superb.block_size];
	int counter = 0;
	int fileOffset = 0;
	int startByte;
	int endByte;
	uint8_t value;
	while(blocks[counter] != NULL) {
		counter += 2;
	}
	//Copy data from blocks and paste it to new block
	for(int i = 0; i < counter; i = i+2) {
		//Copy Data to array
		startByte = blocks[i] * superb.block_size;
		endByte = startByte + superb.block_size;
		for(int j = startByte; j < endByte; j++) {
			value = 0;
			fileData[fileOffset] = data[j];
			memcpy(data+j, &value, 1);
			fileOffset++;
		}	
		//Paste data to new block
		startByte = blocks[i+1] * superb.block_size;
		for(int j = 0; j < superb.block_size; j++) {
			value = fileData[j];
			memcpy(data+startByte+j, &value, 1);
		}

		//Find filename
		char* filename = "placeholder.txt";
		printf("Block %4d indicated reserved in FAT but used by %s; %s relocated\n", blocks[i], filename, filename);

	}

}

//Unfinished
int* checkFatFiles(uint8_t* data, struct superblock_t superb) {
	int startByte = (superb.fat_start_block * superb.block_size)+
		(superb.root_dir_start_block*4);
	int endByte = startByte + superb.block_size;
	uint32_t value;
	int* result = malloc(1024*sizeof(int));
	if(result == NULL) {
		printf("Error allocating memory\n");
		exit(-1);
	}
	int counter = superb.root_dir_start_block;
	
	for(int i = startByte; i < endByte; i = i+4) {
		value = (data[i] << 24)+(data[i+1] << 16)+(data[i+2] << 18)
			+data[i+3];
		if(value != 0) {
			//check if actually in use by a file
			//Adjust to available if not in use
			if(checkInUse(data, superb, counter) == 0) {
				value = 0;
				memcpy(data+i, &value, 4);
				printf("Block %4d indicated allocated in FAT but not used by any files; fixed to available\n", counter);
			}
			//else check other conditions

		}
		counter++;
	}
	return result;
}

int checkInUse(uint8_t* data, struct superblock_t superb, int block) {
	int startByte = block * superb.block_size;
	int endByte = startByte + superb.block_size;
	uint8_t value;

	for(int i = startByte; i < endByte; i++) {
		value = data[i];
		if(value != 0) {
			return 1;
		}
	}
	return 0;
}
