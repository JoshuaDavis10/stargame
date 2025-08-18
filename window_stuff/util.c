#include <stdio.h>
#include <stdarg.h>

#define MAX_LOGGER_MESSAGE_SIZE 16384

/*logging*/

#define LOGGER_ERROR_ENABLED 1
#define LOGGER_WARN_ENABLED 1
#define LOGGER_INFO_ENABLED 1
#define LOGGER_DEBUG_ENABLED 1
#define LOGGER_TRACE_ENABLED 1

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
	printf("\e[0;31m[ERROR]: %s\e[0m\n", output);
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
	printf("\e[0;33m[WARN]: %s\e[0m\n", output);
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
	printf("\e[0;32m[INFO]: %s\e[0m\n", output);
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
	printf("\e[0;34m[DEBUG]: %s\e[0m\n", output);
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
	printf("[TRACE]: %s\n", output);
}
#else
void LOG_TRACE(const char *message, ...) {
}
#endif

/* assertions */

#define _assert(expression)\
{							\
	if(!(expression))			\
	{						\
		printf("\e[0;31m[ASSERT]: EXPRESSION: %s, FILE: %s, LINE: " \
				"%d\e[0m\n", \
				#expression, __FILE__, __LINE__); \
		__builtin_trap(); \
	} \
}
