#include <errno.h>

/* assertions and logging */
#include <stdio.h>
#include <stdarg.h>

/* TODO: some buffer here instead of stack allocating for every log */
#define MAX_LOGGER_MESSAGE_SIZE 32768

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

#define _assert_message(expression, message)\
{							\
	if(!(expression))			\
	{						\
		printf("\e[1;31m[ASSERT]:\e[0;31mEXPRESSION: %s, MESSAGE: %s, FILE: %s, LINE: " \
				"%d\e[0;37m\n", \
				#expression, message, __FILE__, __LINE__); \
		__builtin_trap(); \
	} \
}

/* NOTE(josh): unfortunately, this won't be able to print the stringified expression, so
 * we will only be able to log a message, file, and line number. you'll just have to go look
 * at the _assert_log call to see what expression was passed */
void _assert_log(b32 expression, const char *message, ...)
{
	if(!(expression))
	{
		char output[MAX_LOGGER_MESSAGE_SIZE];
		va_list arg_ptr;
		va_start(arg_ptr, message);
    	vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
		va_end(arg_ptr);
		/* XXX(josh): __FILE__ and __LINE__ will just show up as this file and this function's lines. i.e. that's not useful information
		 * so take it out */
		printf("\e[1;31m[ASSERT]: \e[0;31m%s, FILE: %s, LINE: %d\e[0;37m\n", output, __FILE__, __LINE__);
		__builtin_trap();
	}
}

#define _static_assert(expression, message) \
typedef char static_assertion_##message[(expression)?1:-1]

#define LOGGER_ERROR_ENABLED 1
#define LOGGER_WARN_ENABLED 1 
#define LOGGER_INFO_ENABLED 1 
#define LOGGER_DEBUG_ENABLED 1 
#define LOGGER_TRACE_ENABLED 1 
#define LOGGER_LIBRARY_ENABLED 1

#define LOGGER_LOG_TO_FILE 0
#if LOGGER_LOG_TO_FILE

#define LOGGER_LOG_FILE_NAME "output.log"
#include <fcntl.h> /* for open(2) */

static b32 logging_initialized = false;
static i32 logfile_fd = 0;

#endif

typedef enum {
	LOGGER_ERROR,
	LOGGER_WARN,
	LOGGER_INFO,
	LOGGER_DEBUG,
	LOGGER_TRACE,
	LOGGER_TYPES_COUNT
} logger_message_type;

#if LOGGER_ERROR_ENABLED
void log_error(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
	/* NOTE: vsnprintf is not in C89 standard? */
    i32 bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
#if LOGGER_LOG_TO_FILE
	{
		if(!logging_initialized)
		{
			/* NOTE(josh) 00600 is read/write permissions */
			logfile_fd = open(LOGGER_LOG_FILE_NAME, O_CREAT | O_TRUNC | O_WRONLY, 00600);
			logging_initialized = true;
			_assert_message(logfile_fd, "failed to open logfile for writing.");
		}
		dprintf(logfile_fd, "[ERROR]: %s\n", output);
	}
#else
	printf("\e[1;31m[ERROR]:\e[0;31m %s\e[0;37m\n", output);
#endif
	if(bytes_written >= MAX_LOGGER_MESSAGE_SIZE)
	{
		/* NOTE(josh): let user know that the message was truncated since it hit max length */
		_assert_log(0, "log_error had to truncate the message it printed since the message was longer than"
			" the MAX_LOGGER_MESSAGE_SIZE (message length: %d, max message size: %d)", 
			bytes_written, MAX_LOGGER_MESSAGE_SIZE);
	}
}
#else
void log_error(const char *message, ...) {
}
#endif

#if LOGGER_WARN_ENABLED
void log_warn(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    i32 bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
#if LOGGER_LOG_TO_FILE
	{
		if(!logging_initialized)
		{
			/* NOTE(josh) 00600 is read/write permissions */
			logfile_fd = open(LOGGER_LOG_FILE_NAME, O_CREAT | O_TRUNC | O_WRONLY, 00600);
			logging_initialized = true;
			_assert_message(logfile_fd, "failed to open logfile for writing.");
		}
		dprintf(logfile_fd, "[WARN]:  %s\n", output);
	}
#else
	printf("\e[1;33m[WARN]: \e[0;33m %s\e[0;37m\n", output);
#endif
	if(bytes_written >= MAX_LOGGER_MESSAGE_SIZE)
	{
		/* NOTE(josh): let user know that the message was truncated since it hit max length */
		_assert_log(0, "log_warn had to truncate the message it printed since the message was longer than"
			" the MAX_LOGGER_MESSAGE_SIZE (message length: %d, max message size: %d)", 
			bytes_written, MAX_LOGGER_MESSAGE_SIZE);
	}
}
#else
void log_warn(const char *message, ...) {
}
#endif

#if LOGGER_INFO_ENABLED
void log_info(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    i32 bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
#if LOGGER_LOG_TO_FILE
	{
		if(!logging_initialized)
		{
			/* NOTE(josh) 00600 is read/write permissions */
			logfile_fd = open(LOGGER_LOG_FILE_NAME, O_CREAT | O_TRUNC | O_WRONLY, 00600);
			logging_initialized = true;
			_assert_message(logfile_fd, "failed to open logfile for writing.");
		}
		dprintf(logfile_fd, "[INFO]:  %s\n", output);
	}
#else
	printf("\e[1;32m[INFO]: \e[0;32m %s\e[0;37m\n", output);
#endif
	if(bytes_written >= MAX_LOGGER_MESSAGE_SIZE)
	{
		/* NOTE(josh): let user know that the message was truncated since it hit max length */
		_assert_log(0, "log_info had to truncate the message it printed since the message was longer than"
			" the MAX_LOGGER_MESSAGE_SIZE (message length: %d, max message size: %d)", 
			bytes_written, MAX_LOGGER_MESSAGE_SIZE);
	}
}
#else
void log_info(const char *message, ...) {
}
#endif

#if LOGGER_DEBUG_ENABLED
void log_debug(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    i32 bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
#if LOGGER_LOG_TO_FILE
	{
		if(!logging_initialized)
		{
			/* NOTE(josh) 00600 is read/write permissions */
			logfile_fd = open(LOGGER_LOG_FILE_NAME, O_CREAT | O_TRUNC | O_WRONLY, 00600);
			logging_initialized = true;
			_assert_message(logfile_fd, "failed to open logfile for writing.");
		}
		dprintf(logfile_fd, "[DEBUG]: %s\n", output);
	}
#else
	printf("\e[1;34m[DEBUG]:\e[0;34m %s\e[0;37m\n", output);
#endif
	if(bytes_written >= MAX_LOGGER_MESSAGE_SIZE)
	{
		/* NOTE(josh): let user know that the message was truncated since it hit max length */
		_assert_log(0, "log_debug had to truncate the message it printed since the message was longer than"
			" the MAX_LOGGER_MESSAGE_SIZE (message length: %d, max message size: %d)", 
			bytes_written, MAX_LOGGER_MESSAGE_SIZE);
	}
}
#else
void log_debug(const char *message, ...) {
}
#endif

#if LOGGER_TRACE_ENABLED
void log_trace(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    i32 bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
#if LOGGER_LOG_TO_FILE
	{
		if(!logging_initialized)
		{
			/* NOTE(josh) 00600 is read/write permissions */
			logfile_fd = open(LOGGER_LOG_FILE_NAME, O_CREAT | O_TRUNC | O_WRONLY, 00600);
			logging_initialized = true;
			_assert_message(logfile_fd, "failed to open logfile for writing.");
		}
		dprintf(logfile_fd, "[TRACE]: %s\n", output);
	}
#else
	printf("\e[1;37m[TRACE]:\e[0;37m %s\n", output);
#endif
	if(bytes_written >= MAX_LOGGER_MESSAGE_SIZE)
	{
		/* NOTE(josh): let user know that the message was truncated since it hit max length */
		_assert_log(0, "log_trace had to truncate the message it printed since the message was longer than"
			" the MAX_LOGGER_MESSAGE_SIZE (message length: %d, max message size: %d)", 
			bytes_written, MAX_LOGGER_MESSAGE_SIZE);
	}
}
#else
void log_trace(const char *message, ...) {
}
#endif

#if LOGGER_LIBRARY_ENABLED
void log_lib(const char *message, ...) {
	char output[MAX_LOGGER_MESSAGE_SIZE];
	va_list arg_ptr;
	va_start(arg_ptr, message);
    i32 bytes_written = vsnprintf(output, MAX_LOGGER_MESSAGE_SIZE, message, arg_ptr);
	va_end(arg_ptr);
#if LOGGER_LOG_TO_FILE
	{
		if(!logging_initialized)
		{
			/* NOTE(josh) 00600 is read/write permissions */
			logfile_fd = open(LOGGER_LOG_FILE_NAME, O_CREAT | O_TRUNC | O_WRONLY, 00600);
			logging_initialized = true;
			_assert_message(logfile_fd, "failed to open logfile for writing.");
		}
		dprintf(logfile_fd, "[LIB]:   %s\n", output);
	}
#else
	printf("\e[1;35m[LIB]:  \e[0;35m %s\e[0;37m\n", output);
#endif
	if(bytes_written == MAX_LOGGER_MESSAGE_SIZE)
	{
		/* NOTE(josh): let user know that the message was truncated since it hit max length */
		_assert_log(0, "log_lib had to truncate the message it printed since the message was longer than"
			" the MAX_LOGGER_MESSAGE_SIZE (message length: %d, max message size: %d)", 
			bytes_written, MAX_LOGGER_MESSAGE_SIZE);
	}
}
#else
void log_lib(const char *message, ...) {
}
#endif

/* timing stuff */
#include <sys/time.h> 
#include <x86intrin.h>

#define MICROSECS_PER_SEC 1000000
#define MILLISECS_PER_SEC 1000
#define MILLISECONDS_FOR_CALIBRATION 100

u64 read_os_timer()
{
	struct timeval time;
	gettimeofday(&time, 0);

	/* NOTE(josh): returns microseconds */
	return(MICROSECS_PER_SEC * time.tv_sec + time.tv_usec);
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

/* file I/O */
#include <fcntl.h> /* for open(2) */
#include <unistd.h> /* for read(2) write(2) close(2) */
#include <sys/stat.h> /* for stat(2) */
#include <sys/mman.h> /* for mmap(2) */

u64 get_file_size(const char *filename)
{
	struct stat st;
	if(stat(filename, &st) == -1)
	{
		log_error("get_file_size: failed to stat '%s' (errno: %d)", filename, errno);
		return(0);
	}

	return(st.st_size);	
}

b32 create_file_read_write(const char *filename)
{
	i32 fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 00600);
	if(fd == -1)
	{
		log_error("create_file_read_write: failed to open '%s' (errno: %d)", filename, errno);
		return(false);
	}
	if(close(fd) == -1)
	{
		log_error("create_file_read_write: failed to close '%s' (errno: %d)", filename, errno);
		return(false);
	}
	return(true);
}

b32 read_file_into_buffer(const char *filename, void *buffer, u64 buffer_size)
{
	i32 fd = open(filename, O_RDONLY);
	if(fd == -1)
	{
		log_error("read_file_into_buffer: failed to open '%s' (errno: %d)", filename, errno);
		return(false);
	}

	i64 bytes_read = read(fd, buffer, buffer_size);	
	if(bytes_read == -1)
	{
		log_error("read_file_into_buffer: failed to read from '%s' (errno: %d)", filename, errno);
		return(false);
	}

	_assert((u64)bytes_read <= buffer_size);	

	if(close(fd) == -1)
	{
		log_error("read_file_into_buffer: failed to close '%s' (errno: %d)", filename, errno);
		return(false);
	}

	return(true);
}

void* read_file_mmapped(const char *filename)
{
	i32 fd = open(filename, O_RDONLY);
	u64 file_size;
	void *result;

	if(fd == -1)
	{
		log_error("read_file_mmapped: failed to open '%s' (errno: %d)", filename, errno);
		return(0);
	}

	file_size = get_file_size(filename);

	if(file_size == 0)
	{
		log_error("read_file_mmapped: file size for '%s' was 0 (errno: %d)", filename, errno);
		return(0);
	}

	result = mmap(0, file_size, PROT_READ, MAP_SHARED, fd, 0);

	if(result == MAP_FAILED || result == 0)
	{
		log_error("read_file_mmapped: failed to mmap(2) '%s' (errno: %d)", filename, errno);
		return(0);
	}

	if(close(fd) == -1)
	{
		log_error("read_file_mmapped: failed to close '%s' (errno: %d)", filename, errno);
		return(0);
	}

	return(result);
}

b32 write_buffer_into_file_truncate(const char *filename, void *buffer, u64 buffer_size)
{
	/* NOTE(josh): this will clear the file's original contents */
	i32 fd = open(filename, O_WRONLY | O_TRUNC);
	if(fd == -1)
	{
		log_error("write_buffer_into_file_truncate: failed to open '%s' (errno: %d)", filename, errno);
		return(false);
	}

	i64 bytes_written = write(fd, buffer, buffer_size);
	if(bytes_written == -1)
	{
		log_error("write_buffer_into_file_truncate: failed to write to '%s' (errno: %d)", filename, errno);
		return(false);
	}

	if(close(fd) == -1)
	{
		log_error("write_buffer_into_file_truncate: failed to close '%s' (errno: %d)", filename, errno);
		return(false);
	}

	return(true);
}

b32 write_buffer_into_file_append(const char *filename, void *buffer, u64 buffer_size)
{
	/* NOTE(josh): this will clear the file's original contents */
	i32 fd = open(filename, O_WRONLY | O_APPEND);
	if(fd == -1)
	{
		log_error("write_buffer_into_file_append: failed to open '%s' (errno: %d)", filename, errno);
		return(false);
	}

	i64 bytes_written = write(fd, buffer, buffer_size);
	if(bytes_written == -1)
	{
		log_error("write_buffer_into_file_append: failed to write to '%s' (errno: %d)", filename, errno);
		return(false);
	}

	if(close(fd) == -1)
	{
		log_error("write_buffer_into_file_append: failed to close '%s' (errno: %d)", filename, errno);
		return(false);
	}

	return(true);
}

/* memory stuff */
	/* TODO: write own version of memset or look into platform-specific alternatives ? 
	 * (i just want to avoid using c standard library where possible */
#include <string.h> 
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

/* NOTE(josh): binary trees might be nice as well? */
