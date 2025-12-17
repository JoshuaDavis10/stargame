/* NOTE(josh): I think the engine only needs assertions/logging/timing/file IO working for now */
#include <windows.h>

/* TODO: assertions and logging */
#include <fileapi.h> /* for WriteFile */
#include <stdio.h> /* for vsnprintf */
#include <stdarg.h> /* for va arg stuff */
#include <string.h> /* for strlen */

#define MAX_LOGGER_MESSAGE_SIZE 32768

#define LOGGER_ERROR_ENABLED   1
#define LOGGER_WARN_ENABLED    1
#define LOGGER_INFO_ENABLED    1
#define LOGGER_DEBUG_ENABLED   1
#define LOGGER_TRACE_ENABLED   1
#define LOGGER_LIBRARY_ENABLED 1

enum {
	LOG_TYPE_ERROR,
	LOG_TYPE_WARN,
	LOG_TYPE_INFO,
	LOG_TYPE_DEBUG,
	LOG_TYPE_LIB,
	LOG_TYPE_TRACE,
	LOG_TYPE_COUNT
};

void log_message(i32 log_type, const char *message, ...)
{
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
	DWORD bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
	HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	switch(log_type)
	{
		case LOG_TYPE_ERROR:
		{
			SetConsoleTextAttribute(std_out_handle, 12);
			WriteFile(std_out_handle, "[ERROR]: ", strlen("[ERROR]: "), 0, 0);
			SetConsoleTextAttribute(std_out_handle, 4);
			WriteFile(std_out_handle, output, bytes_written, 0, 0);
			WriteFile(std_out_handle, "\n", 1, 0, 0);
			SetConsoleTextAttribute(std_out_handle, 7);
		} break;
		case LOG_TYPE_WARN:
		{
			SetConsoleTextAttribute(std_out_handle, 14);
			WriteFile(std_out_handle, "[WARN]:  ", strlen("[WARN]:  "), 0, 0);
			SetConsoleTextAttribute(std_out_handle, 6);
			WriteFile(std_out_handle, output, bytes_written, 0, 0);
			WriteFile(std_out_handle, "\n", 1, 0, 0);
			SetConsoleTextAttribute(std_out_handle, 7);
		} break;
		case LOG_TYPE_INFO:
		{
			SetConsoleTextAttribute(std_out_handle, 10);
			WriteFile(std_out_handle, "[INFO]:  ", strlen("[INFO]:  "), 0, 0);
			SetConsoleTextAttribute(std_out_handle, 2);
			WriteFile(std_out_handle, output, bytes_written, 0, 0);
			WriteFile(std_out_handle, "\n", 1, 0, 0);
			SetConsoleTextAttribute(std_out_handle, 7);
		} break;
		case LOG_TYPE_DEBUG:
		{
			SetConsoleTextAttribute(std_out_handle, 9);
			WriteFile(std_out_handle, "[DEBUG]: ", strlen("[DEBUG]: "), 0, 0);
			SetConsoleTextAttribute(std_out_handle, 1);
			WriteFile(std_out_handle, output, bytes_written, 0, 0);
			WriteFile(std_out_handle, "\n", 1, 0, 0);
			SetConsoleTextAttribute(std_out_handle, 7);
		} break;
		case LOG_TYPE_LIB:
		{
			SetConsoleTextAttribute(std_out_handle, 13);
			WriteFile(std_out_handle, "[LIB]:   ", strlen("[LIB]:   "), 0, 0);
			SetConsoleTextAttribute(std_out_handle, 5);
			WriteFile(std_out_handle, output, bytes_written, 0, 0);
			WriteFile(std_out_handle, "\n", 1, 0, 0);
			SetConsoleTextAttribute(std_out_handle, 7);
		} break;
		case LOG_TYPE_TRACE:
		{
			SetConsoleTextAttribute(std_out_handle, 15);
			WriteFile(std_out_handle, "[TRACE]: ", strlen("[TRACE]: "), 0, 0);
			SetConsoleTextAttribute(std_out_handle, 7);
			WriteFile(std_out_handle, output, bytes_written, 0, 0);
			WriteFile(std_out_handle, "\n", 1, 0, 0);
		} break;
		default:
		{
			/* TODO: _assert_log(0, "unexpected log_type: %d", log_type); */
		} break;
	}
}

#if LOGGER_ERROR_ENABLED
void log_error(const char *message, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, message);
	log_message(LOG_TYPE_ERROR, message, arg_ptr);
	va_end(arg_ptr);
}
#else
void log_error(const char *message, ...)
{
}
#endif

#if LOGGER_WARN_ENABLED
void log_warn(const char *message, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, message);
	log_message(LOG_TYPE_WARN, message, arg_ptr);
	va_end(arg_ptr);
}
#else
void log_warn(const char *message, ...)
{
}
#endif

#if LOGGER_INFO_ENABLED
void log_info(const char *message, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, message);
	log_message(LOG_TYPE_INFO, message, arg_ptr);
	va_end(arg_ptr);
}
#else
void log_info(const char *message, ...)
{
}
#endif

#if LOGGER_DEBUG_ENABLED
void log_debug(const char *message, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, message);
	log_message(LOG_TYPE_DEBUG, message, arg_ptr);
	va_end(arg_ptr);
}
#else
void log_debug(const char *message, ...)
{
}
#endif

#if LOGGER_LIBRARY_ENABLED
void log_lib(const char *message, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, message);
	log_message(LOG_TYPE_LIB, message, arg_ptr);
	va_end(arg_ptr);
}
#else
void log_lib(const char *message, ...)
{
}
#endif

#if LOGGER_TRACE_ENABLED
void log_trace(const char *message, ...)
{
	va_list arg_ptr;
	va_start(arg_ptr, message);
	log_message(LOG_TYPE_TRACE, message, arg_ptr);
	va_end(arg_ptr);
}
#else
void log_trace(const char *message, ...)
{
}
#endif


/* TODO: timing */

/* TODO: file I/O */

/* TODO: memory stuff */

/* TODO: dynamic arrays, NOTE should be the same as on linux */
