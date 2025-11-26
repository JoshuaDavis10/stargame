#ifdef __linux__

#include "linux_util.c"

#endif

#ifdef _WIN32

#include "win32_util.c"

#endif

/* math */
i32 absolute_value_i32(i32 x);
i64 absolute_value_i64(i64 x);
f32 absolute_value_f32(f32 x);
f64 absolute_value_f64(f64 x);


i32 absolute_value_i32(i32 x)
{
	/* NOTE(josh): shift right with sign fill 
	 * '>>' will do that in C if it's a signed number 
	 */
	i32 y = x >> 31;
	return( (x ^ y) - y );
}

i64 absolute_value_i64(i64 x)
{
	/* NOTE(josh): shift right with sign fill 
	 * '>>' will do that in C if it's a signed number 
	 */
	i64 y = x >> 63;
	return( (x ^ y) - y );
}

/* TODO: better ways to do this obv. apparently, fabs() does a union
 * and some bit stuff that I don't understand
 */
f32 absolute_value_f32(f32 x)
{
	if(x < 0.0f)
	{
		return(-x);
	}
	return x;
}

/* TODO: better ways to do this obv. apparently, fabs() does a union
 * and some bit stuff that I don't understand
 */
f64 absolute_value_f64(f64 x)
{
	if(x < 0.0)
	{
		return(-x);
	}
	return x;
}
