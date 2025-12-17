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

int main()
{
	log_error("test program");
	log_warn("test program");
	log_info("test program");
	log_debug("test program");
	log_lib("test program");
	log_trace("test program");

	return 0;
}
