typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

typedef unsigned int b32;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#include "util.c"
#include <stdlib.h>

_static_assert(sizeof(unsigned char) == 1, unexpected_data_type_size);
_static_assert(sizeof(unsigned short) == 2, unexpected_data_type_size);
_static_assert(sizeof(unsigned int) == 4, unexpected_data_type_size);
_static_assert(sizeof(unsigned long long) == 8, unexpected_data_type_size);
_static_assert(sizeof(signed char) == 1, unexpected_data_type_size);
_static_assert(sizeof(signed short) == 2, unexpected_data_type_size);
_static_assert(sizeof(signed int) == 4, unexpected_data_type_size);
_static_assert(sizeof(signed long long) == 8, unexpected_data_type_size);
_static_assert(sizeof(float) == 4, unexpected_data_type_size);
_static_assert(sizeof(double) == 8, unexpected_data_type_size);

int main(int argc, char **argv)
{
	log_error("%s %d", "test program ", __COUNTER__);
	log_warn("%s %d", "test program ", __COUNTER__);
	log_info("%s %d", "test program ", __COUNTER__);
	log_debug("%s %d", "test program ", __COUNTER__);
	log_lib("%s %d", "test program ", __COUNTER__);
	log_trace("%s %d", "test program ", __COUNTER__);

	/* TODO: test file i/o stuff */

	char *filename = argv[1];
	u64 file_size = get_file_size(filename);
	log_info("file: '%s' has size %llu", filename, get_file_size(filename));
	char *content_buffer = malloc(file_size+1);
	content_buffer[file_size] = '\0';
	_assert(read_file_into_buffer(filename, content_buffer, file_size));
	log_info("file: '%s' contents\n%s", filename, content_buffer); 

	_assert(create_file_read_write("out"));
	_assert(write_buffer_into_file_append("out", content_buffer, file_size));

	return 0;
}





