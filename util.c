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

#define _static_assert(expression, message) \
typedef char static_assertion_##message[(expression)?1:-1]

/* timing stuff */
#include <sys/time.h> 
#include <x86intrin.h>

#define MICROSECS_PER_SEC 1000000
#define MILLISECS_PER_SEC 1000
#define MILLISECONDS_FOR_CALIBRATION 100

static u64 read_os_timer()
{
	struct timeval time;
	gettimeofday(&time, 0);

	/* NOTE(josh): returns microseconds */
	return(MICROSECS_PER_SEC * time.tv_sec + time.tv_usec);
}

/* NOTE(josh): apparently this gets inlined automatically by most compilers 
 * but compiler might complain about the function not being used? 
 * TODO: update this comment if that occurs ^, then u can inline. just wanted to
 * see it happen for myself rather than just inlining like Casey did
 */
static u64 read_cpu_timer()
{
	return(__rdtsc());
}

static u64 read_cpu_frequency()
{
	u64 cpu_start = read_cpu_timer();
	u64 cpu_end = 0;
	u64 cpu_frequency = 0;
	u64 cpu_elapsed = 0;

	u64 os_start  = read_os_timer();
	u64 os_end = 0;
	u64 os_elapsed = 0;

	/* NOTE(josh): this will be #us in 100 ms */
	u64 os_wait_time = MICROSECS_PER_SEC * MILLISECONDS_FOR_CALIBRATION / MILLISECS_PER_SEC; 

	while(os_elapsed < os_wait_time)
	{
		os_end = read_os_timer();
		os_elapsed = os_end - os_start;
	}

	cpu_end = read_cpu_timer();
	cpu_elapsed = cpu_end - cpu_start;

	cpu_frequency = MICROSECS_PER_SEC * cpu_elapsed / os_elapsed;

	return(cpu_frequency);
}
