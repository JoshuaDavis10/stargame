#include <errno.h> //errno
#include <stdio.h>
#include <string.h> //strerror
#include <fcntl.h> //open
#include <unistd.h> //write

int main(int argc, char **argv) {

	if(argc != 2) {
		printf("error, usage: $ ./editor filename\n");	
		return -1;
	}

	errno = 0; //NOTE: set errno to zero before opening file
	int tilemap_file_descriptor = open(argv[1],
									O_RDWR | O_EXCL | O_CREAT);

	if(tilemap_file_descriptor == -1) {
		if(errno == EEXIST) {
			printf("error: file '%s' already exists\n", 
					argv[1]);
			return -1;
		}
		printf("error: %s\n", strerror(errno));
		return -1;
	}

	int data[2] = {0, 1};

	write(tilemap_file_descriptor, data, 2);

	//TODO: mmap the file so it is easy to manipulate?
	//that would make all writes "instant"?
	//issue: if we want an option to save progress or just leave it,
	//do we want to support undo/redo... 
	//^ depending on answers to those questions we might definitely not
	//want to use mmap

	//TODO: GUI to change tiles in map
	
}
