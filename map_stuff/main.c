#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char **argv) {

	if(argc != 2) {
		printf("error, usage: $ ./map filename\n");	
		return -1;
	}

	int tilemap_file_descriptor;
	struct stat filestat;
	const char *filename = argv[1];

	tilemap_file_descriptor = open(filename, O_RDONLY);
	if(tilemap_file_descriptor == -1) {
		printf("error: open\n");
		return -1;
	}

	//get file size, struct stat has lots of info about files, but
	//we just need st_size (which is of type off_t, which is a signed
	//integer type, so casting it to a size_t could be lossy since
	//size_t is unsigned)
	if(stat(filename, &filestat) == -1) {
		return -1; //TODO: figure out if the return value of your program
				   //is at all meaningful. it's just something I've sort
				   //of accepted without thinking about
		//TODO: logging
	}

	size_t filesize = (size_t)filestat.st_size; //NOTE: potentially lossy
												//conversion
	
	char *tilemap_memory = (char *)mmap(
			0, filesize, PROT_READ, MAP_SHARED, 
			tilemap_file_descriptor, 0); 

	if(!tilemap_memory) {
		//TODO: logging
		return -1;
	}

	for(size_t i = 0; i < filesize; i++) {
		putc(tilemap_memory[i], stdout);
	}


	//TODO: create X window
	//TODO: software rendering (CPU rendering) in X window

	return 0;
}
