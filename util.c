/*logging*/
#include <stdio.h>
#include <stdarg.h>

#define MAX_LOGGER_MESSAGE_SIZE 16384

#define LOGGER_ERROR_ENABLED 1
#define LOGGER_WARN_ENABLED 1
#define LOGGER_INFO_ENABLED 1
#define LOGGER_DEBUG_ENABLED 1
#define LOGGER_TRACE_ENABLED 1
#define LOGGER_LIBRARY_ENABLED 1

typedef enum {
	LOGGER_ERROR,
	LOGGER_WARN,
	LOGGER_INFO,
	LOGGER_DEBUG,
	LOGGER_TRACE,
	LOGGER_TYPES_COUNT
} logger_message_type;

#if LOGGER_ERROR_ENABLED
void LOG_ERROR(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
	/* NOTE: vsnprintf is not in C89 standard? */
    vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
	printf("\e[1;31m[ERROR]:\e[0;31m %s\e[0;37m\n", output);
}
#else
void LOG_ERROR(const char *message, ...) {
}
#endif

#if LOGGER_WARN_ENABLED
void LOG_WARN(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
	printf("\e[1;33m[WARN]:\e[0;33m %s\e[0;37m\n", output);
}
#else
void LOG_WARN(const char *message, ...) {
}
#endif

#if LOGGER_INFO_ENABLED
void LOG_INFO(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
	printf("\e[1;32m[INFO]:\e[0;32m %s\e[0;37m\n", output);
}
#else
void LOG_INFO(const char *message, ...) {
}
#endif

#if LOGGER_DEBUG_ENABLED
void LOG_DEBUG(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
	printf("\e[1;34m[DEBUG]:\e[0;34m %s\e[0;37m\n", output);
}
#else
void LOG_DEBUG(const char *message, ...) {
}
#endif

#if LOGGER_TRACE_ENABLED
void LOG_TRACE(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
	printf("\e[1;37m[TRACE]:\e[0;37m %s\n", output);
}
#else
void LOG_TRACE(const char *message, ...) {
}
#endif

#if LOGGER_LIBRARY_ENABLED
void LOG_LIB(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
	printf("\e[1;35m[LIB]:\e[0;35m %s\e[0;37m\n", output);
}
#else
void LOG_LIB(const char *message, ...) {
}
#endif

/* assertions */
#define _assert(expression)\
{							\
	if(!(expression))			\
	{						\
		printf("\e[1;31m[ASSERT]:\e[0;31mEXPRESSION: %s, FILE: %s, LINE: " \
				"%d\e[0;37m\n", \
				#expression, __FILE__, __LINE__); \
		__builtin_trap(); \
	} \
}

/* time stuff */
#include <sys/time.h> /* gettimeofday */
#include <unistd.h>

struct timeval timeval_get()
{
	struct timeval tv;

	i32 gettimeofday_result = gettimeofday(&tv, 0);
	_assert(gettimeofday_result == 0);

	return tv;
}

f64 get_time_ms()
{
	struct timeval tv = timeval_get();
	f64 ms = (tv.tv_sec * 1000.0) + ((f64)tv.tv_usec / 1000.0);	
	return ms;
}

struct timeval timeval_get_difference(
		struct timeval end, struct timeval start, b32 *success)
{
	struct timeval tv;
	time_t s;
	suseconds_t us;

	if(end.tv_sec == start.tv_sec)
	{
		if(end.tv_usec < start.tv_usec)
		{
			if(success)
			{
				*success = false;
			}
		}
	}

	s = end.tv_sec - start.tv_sec;
	/* NOTE: suseconds_t is signed, useconds_t is unsigned 
	 * timeval.tv_usec is of type suseconds_t 
	 */
	us = end.tv_usec - start.tv_usec;

	if(us < 0)
	{
		s--; /* NOTE: subtract from secs and carry over to usecs */
		us += 1000000;
	}

	tv.tv_sec = s;
	tv.tv_usec = us;
	return tv;
}

i8 timeval_sleep(struct timeval tv)
{
	if(sleep(tv.tv_sec) != 0)
	{
		LOG_ERROR("failed to sleep");
		_assert(0);
	}
	if(usleep(tv.tv_usec) == -1)
	{
		LOG_ERROR("failed to usleep");
		_assert(0);
	}
	return true;
}
