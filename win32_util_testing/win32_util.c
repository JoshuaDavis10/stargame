#include <windows.h>

/* assertions and logging */
#include <fileapi.h> /* for WriteFile */
#include <stdio.h> /* for vsnprintf, snprintf */
#include <stdarg.h> /* for vararg stuff */
#include <string.h> /* for strlen */

#define MAX_LOGGER_MESSAGE_SIZE 32768

#define _assert(expression)																			\
{																									\
	if(!(expression))																				\
	{																								\
		char output[MAX_LOGGER_MESSAGE_SIZE];														\
		DWORD bytes_written =																		\
			snprintf(output, MAX_LOGGER_MESSAGE_SIZE, "EXPRESSION: %s, FILE: %s, LINE: %d",			\
					  #expression, __FILE__, __LINE__);												\
		HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);									\
																									\
		SetConsoleTextAttribute(std_out_handle, 12);												\
		WriteFile(std_out_handle, "[ASSERT]:", strlen("[ASSERT]:"), 0, 0);							\
		SetConsoleTextAttribute(std_out_handle, 4);													\
		WriteFile(std_out_handle, output, bytes_written, 0, 0);										\
		WriteFile(std_out_handle, "\n", 1, 0, 0);													\
		SetConsoleTextAttribute(std_out_handle, 7);													\
																									\
		__builtin_trap();																			\
	}																								\
}

#define _assert_message(expression, message)														\
{																									\
	if(!(expression))																				\
	{																								\
		char output[MAX_LOGGER_MESSAGE_SIZE];														\
		DWORD bytes_written =																		\
			snprintf(output, MAX_LOGGER_MESSAGE_SIZE,												\
					"EXPRESSION: %s, MESSAGE: %s, FILE: %s, LINE: %d",								\
					#expression, message, __FILE__, __LINE__);										\
		HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);									\
																									\
		SetConsoleTextAttribute(std_out_handle, 12);												\
		WriteFile(std_out_handle, "[ASSERT]:", strlen("[ASSERT]:"), 0, 0);							\
		SetConsoleTextAttribute(std_out_handle, 4);													\
		WriteFile(std_out_handle, output, bytes_written, 0, 0);										\
		WriteFile(std_out_handle, "\n", 1, 0, 0);													\
		SetConsoleTextAttribute(std_out_handle, 7);													\
																									\
		__builtin_trap();																			\
	}																								\
}

void _assert_log(b32 expression, const char *message, ...)
{
	if(!(expression))
	{
		char output[MAX_LOGGER_MESSAGE_SIZE];
		va_list arg_ptr;
		va_start(arg_ptr, message);
		DWORD bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
		va_end(arg_ptr);

		HANDLE std_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);

		SetConsoleTextAttribute(std_out_handle, 12);
		WriteFile(std_out_handle, "[ASSERT]:", strlen("[ASSERT]:"), 0, 0);
		SetConsoleTextAttribute(std_out_handle, 4);
		WriteFile(std_out_handle, output, bytes_written, 0, 0);
		WriteFile(std_out_handle, "\n", 1, 0, 0);
		SetConsoleTextAttribute(std_out_handle, 7);

		__builtin_trap();
	}
}

#define _static_assert(expression, message) \
typedef char static_assertion_##message[(expression)?1:-1]

/* TODO: logging to files */

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

void log_message(i32 log_type, const char *message, va_list args) 
{
	char output[MAX_LOGGER_MESSAGE_SIZE];
	DWORD bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, args);
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
			_assert_log(0, "unexpected log_type: %d", log_type);
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

/* timing stuff */
#include <x86intrin.h>

#define MICROSECS_PER_SEC 1000000
#define MILLISECS_PER_SEC 1000
#define MILLISECONDS_FOR_CALIBRATION 100

u64 read_os_frequency()
{
	LARGE_INTEGER performance_frequency;
	_assert(QueryPerformanceFrequency(&performance_frequency));

	return(performance_frequency.QuadPart);
}

u64 read_os_timer()
{
	LARGE_INTEGER performance_counter;
	_assert(QueryPerformanceCounter(&performance_counter));

	return(performance_counter.QuadPart);
}

u64 read_cpu_timer()
{
	return(__rdtsc());
}

u64 read_cpu_frequency()
{
	u64 cpu_start = read_cpu_timer();
	u64 cpu_end = 0;
	u64 cpu_frequency = 0;
	u64 cpu_elapsed = 0;

	u64 os_start  = read_os_timer();
	u64 os_end = 0;
	u64 os_frequency = read_os_frequency();
	u64 os_elapsed = 0;

	/* NOTE(josh): this will be #us in 100 ms */
	u64 os_wait_time = os_frequency * MILLISECONDS_FOR_CALIBRATION / MILLISECS_PER_SEC; 

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

/* file I/O */
u64 get_file_size(const char *filename)
{
	u64 file_size = 0;

	/* XXX */
	/*
HANDLE CreateFileA(
  [in]           LPCSTR                lpFileName,
  [in]           DWORD                 dwDesiredAccess,
  [in]           DWORD                 dwShareMode,
  [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  [in]           DWORD                 dwCreationDisposition,
  [in]           DWORD                 dwFlagsAndAttributes,
  [in, optional] HANDLE                hTemplateFile
);

	file_size = GetFileSize(
	DWORD GetFileSize(
	  [in]            HANDLE  hFile,
	  [out, optional] LPDWORD lpFileSizeHigh
	);
	*/
	return(file_size);
}

b32 create_file_read_write(const char *filename)
{
	/* XXX: */
	return(true);
}

b32 read_file_into_buffer(const char *filename, void *buffer, u64 buffer_size)
{
	/* XXX: */
	return(true);
}

/* TODO: read_file_mmapped, not necessary for engine atm */

b32 write_buffer_into_file_truncate(const char *filename, void *buffer, u64 buffer_size)
{
	/* XXX: */
	return(true);
}

b32 write_buffer_into_file_append(const char *filename, void *buffer, u64 buffer_size)
{
	/* XXX: */
	return(true);
}

/* memory stuff */
	/* TODO: write own version of memset or look into platform-specific alternatives ? 
	 * (i just want to avoid using c standard library where possible */
b32 zero_memory(void *memory, u64 size)
{
	if(memset(memory, 0, size))
	{
		return(true);
	}
	return(false);
}

/* dynamic arrays */
/* NOTE(josh): not rlly sure how to name stuff, but I guess width is "stride", size is how much is actually allocated,
 * and length is how much is actually in it? */
#define DYNAMIC_ARRAY_HEADER_SIZE 3 * sizeof(u64)
#define DYNAMIC_ARRAY_SIZE_INDEX 0
#define DYNAMIC_ARRAY_LENGTH_INDEX 1
#define DYNAMIC_ARRAY_WIDTH_INDEX 2
void *dynamic_array_create(u64 width, u64 size)
{
	u64 *header = (u64*)malloc(width * size + DYNAMIC_ARRAY_HEADER_SIZE);
	header[DYNAMIC_ARRAY_SIZE_INDEX]   = size;
	header[DYNAMIC_ARRAY_LENGTH_INDEX] = 0;
	header[DYNAMIC_ARRAY_WIDTH_INDEX]  = width;
	void *array = (void *) ((char *)header + DYNAMIC_ARRAY_HEADER_SIZE);
	return(array);
}

void dynamic_array_destroy(void *array)
{
	void *header = ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	free(header);
}

void *dynamic_array_expand(void *array)
{
	u64 *header = (u64 *) ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	u64 array_size = header[DYNAMIC_ARRAY_SIZE_INDEX];
	_assert(array_size != 0);
	header[DYNAMIC_ARRAY_SIZE_INDEX] = 2 * array_size;

	header = 
		(u64*)realloc(header, header[DYNAMIC_ARRAY_WIDTH_INDEX] * header[DYNAMIC_ARRAY_SIZE_INDEX] + DYNAMIC_ARRAY_HEADER_SIZE);
	array = (void *) ((char *)header + DYNAMIC_ARRAY_HEADER_SIZE);
	return(array);
}

/* TODO: test this macro lol */
#define dynamic_array_access(array, index, type) (*((type *)_dynamic_array_access(array, index)))
void *_dynamic_array_access(void *array, u64 index)
{
	u64 *header = (u64 *) ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	_assert(header[DYNAMIC_ARRAY_LENGTH_INDEX] > index);

	void *result = (void *) ((char *)array + (header[DYNAMIC_ARRAY_WIDTH_INDEX] * index));
	return(result);
}

void *dynamic_array_add(void *array, void *data)
{
	u64 *header = (u64 *) ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	if(header[DYNAMIC_ARRAY_LENGTH_INDEX] >= header[DYNAMIC_ARRAY_SIZE_INDEX])
	{
		array = dynamic_array_expand(array);
		header = (u64 *) ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	}
	void *add_address = (void *) ((char *)array + header[DYNAMIC_ARRAY_WIDTH_INDEX] * header[DYNAMIC_ARRAY_LENGTH_INDEX]);
	memcpy(add_address, data, header[DYNAMIC_ARRAY_WIDTH_INDEX]);
	header[DYNAMIC_ARRAY_LENGTH_INDEX] += 1;
	return(array);
}

void *dynamic_array_insert(void *array, void *data, u64 index)
{
	u64 *header = (u64 *) ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	_assert(index <= header[DYNAMIC_ARRAY_LENGTH_INDEX]);
	if(header[DYNAMIC_ARRAY_LENGTH_INDEX] >= header[DYNAMIC_ARRAY_SIZE_INDEX])
	{
		array = dynamic_array_expand(array);
		header = (u64 *) ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	}

	/* move */
	i32 char_index = header[DYNAMIC_ARRAY_WIDTH_INDEX] * (header[DYNAMIC_ARRAY_LENGTH_INDEX] - index);
	void *move_from = (void *) ((char *)array + header[DYNAMIC_ARRAY_WIDTH_INDEX] * index);
	void *move_to   = (void *) ((char *)array + header[DYNAMIC_ARRAY_WIDTH_INDEX] * (index + 1));
	memmove(move_to, move_from, header[DYNAMIC_ARRAY_WIDTH_INDEX] * (header[DYNAMIC_ARRAY_LENGTH_INDEX] - index));

	/* insert */
	memcpy(move_from, data, header[DYNAMIC_ARRAY_WIDTH_INDEX]);
	header[DYNAMIC_ARRAY_LENGTH_INDEX] += 1;
	return(array);
}

u64 dynamic_array_length(void *array)
{
	u64 *header = (u64 *) ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	return(header[DYNAMIC_ARRAY_LENGTH_INDEX]);
}

u64 dynamic_array_memory_allocated_count(void *array)
{
	u64 *header = (u64 *) ((char *)array - DYNAMIC_ARRAY_HEADER_SIZE);
	return(header[DYNAMIC_ARRAY_WIDTH_INDEX] * header[DYNAMIC_ARRAY_SIZE_INDEX]);
}

/* TODO: dynamic_array_remove */
