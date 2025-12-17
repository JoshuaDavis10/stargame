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

int main()
{
	/* TODO: test file i/o stuff */
	return 0;
}





