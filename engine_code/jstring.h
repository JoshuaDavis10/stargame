#ifndef JSTRING_H
#define JSTRING_H

static void (*jstring_log)(const char*, ...);

/* NOTE: this assertion sets off the address sanitizer */
#define JSTRING_ASSERT(expression, msg) \
if(!(expression)) \
{ \
	jstring_log("JSTRING ASSERTION: %s. file: %s. line: %d", \
			msg, __FILE__, __LINE__); \
	__builtin_trap(); \
}

/* NOTE: this is how you do a static assert without having to include C 
 * standard headers and stuff so this is how we doin' it 
 */
#define JSTRING_STATIC_ASSERT(expression, message) \
typedef char static_assertion_##message[(expression)?1:-1]

JSTRING_STATIC_ASSERT(sizeof(signed char) == 1, 
		asserting_signed_char_size_is_one_byte);
JSTRING_STATIC_ASSERT(sizeof(signed short) == 2, 
		asserting_signed_short_size_is_two_bytes);
JSTRING_STATIC_ASSERT(sizeof(signed int) == 4, 
		asserting_signed_int_size_is_four_bytes);
JSTRING_STATIC_ASSERT(sizeof(signed long long) == 8, 
	asserting_signed_long_long_size_is_eight_bytes);
JSTRING_STATIC_ASSERT(sizeof(unsigned char) == 1, 
		asserting_unsigned_char_size_is_one_byte);
JSTRING_STATIC_ASSERT(sizeof(unsigned short) == 2, 
		asserting_unsigned_short_size_is_two_bytes);
JSTRING_STATIC_ASSERT(sizeof(unsigned int) == 4, 
		asserting_unsigned_int_size_is_four_bytes);
JSTRING_STATIC_ASSERT(sizeof(unsigned long long) == 8, 
		asserting_unsigned_long_long_size_is_eight_bytes);
JSTRING_STATIC_ASSERT(sizeof(float) == 4, 
		asserting_float_size_is_four_bytes);
JSTRING_STATIC_ASSERT(sizeof(double) == 8, 
		asserting_double_size_is_eight_bytes);

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;
typedef unsigned char u8;
typedef unsigned int b32; /* for "boolean" stuff, this is more so I can 
						   * remember what stuff is is used for booleans
						   */
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

#define true 1
#define false 0 

static void jstring_log_stub(const char* msg, ...) { }
static void (*jstring_log)(const char*, ...) = jstring_log_stub;
    
typedef struct {
	u64 size;

	/* current offset past address that we have allocated to */
	u64 offset; 
	void *address;
	b32 activated;
} jstring_memory;

typedef struct {
	u32 length;
	u32 capacity;
	char *data;
} jstring;

/* NOTE: should forward declare functions with descriptions somewhere
 * around here once the library is pretty well fleshed out
 */

static jstring_memory jstring_temporary_memory_info = {0};

/* NOTE: basically user is just saying, "hey you get to use this memory, 
 * I will not touch it" 
 */
static b32 jstring_memory_activate(u64 size, void *address)
{
	if(jstring_temporary_memory_info.activated)
	{
		jstring_log("jstring_memory_activate: jstring memory has already" 
					" been activated! returning 0.");
		return false;
	}
	jstring_temporary_memory_info.size = size;
	jstring_temporary_memory_info.offset = 0;
	jstring_temporary_memory_info.address = address;
	jstring_temporary_memory_info.activated = true;

	return true;
}

static b32 jstring_memory_reset(u64 size, void *address)
{
	if(jstring_temporary_memory_info.activated == false)
	{
		jstring_log(
			"jstring_memory_reset: jstring memory has not been"
			" activated. unable to reset memory. returning 0.");
		return false;
	}
	if(size == 0 && address == 0)
	{
		jstring_temporary_memory_info.offset = 0;
	}
	else
	{
		jstring_temporary_memory_info.size = size;
		jstring_temporary_memory_info.offset = 0;
		jstring_temporary_memory_info.address = address;
	}

	jstring_log(
		"jstring_memory_reset: jstring memory reset "
		" - %u bytes @%p",
		jstring_temporary_memory_info.size,
		jstring_temporary_memory_info.address);
	return true;
}

static b32 jstring_load_logging_function(void (*func)(const char*, ...))
{
	if(func == 0)
	{
		/* TODO: is there any way to log this failure lol */
		/* probably just document that this is the only failure case */
		return false;
	}
	jstring_log = func;
	return true;
}

static char *jstring_temporary_memory_allocate_string(u64 size)
{
	if(!jstring_temporary_memory_info.activated)
	{
		jstring_log(
			"jstring_temporary_memory_allocate_string: "
			"temporary memory buffer not activated. returning 0");
		return 0;
	}
	if((jstring_temporary_memory_info.offset + size) > 
		jstring_temporary_memory_info.size)
	{
		jstring_log(
			"jstring_temporary_memory_allocate_string: "
			"allocating %u bytes would go past temporary memory buffer:"
			"\nmemory size: %u, offset: %u",
			size, 
			jstring_temporary_memory_info.size,
			jstring_temporary_memory_info.offset);

		return 0;
	}
	char *address = jstring_temporary_memory_info.address +
		jstring_temporary_memory_info.offset;
	jstring_temporary_memory_info.offset += size;
	return address;
}

static jstring jstring_create_temporary(
		const char *chars, 
		u64 length)
{
	jstring result_jstring;
	result_jstring.length = 0;
	result_jstring.capacity = 0;
	result_jstring.data = 0;
	if(chars == 0)
	{
		jstring_log("jstring_create_temporary: got passed null string"
				" for data: %p. returning zeroed out jstring struct.",
				chars);
		return result_jstring;
	}

	result_jstring.length = length;
	result_jstring.capacity = 2 * (length + 1);

	char *jstring_temporary_memory_insertion_address = 
		jstring_temporary_memory_allocate_string(
				result_jstring.capacity);

	u32 index;
	for(index = 0; index < length; index++)
	{
		if(chars[index] == '\0')
		{
			jstring_log("jstring_create_temporary: "
					"hit null terminator sooner than expected. "
					"returning zeroed out jstring struct.");
			result_jstring.length = 0;
			result_jstring.capacity = 0;
			result_jstring.data = 0;
			return result_jstring;
		}
		jstring_temporary_memory_insertion_address[index] = chars[index];
	}
	if(chars[length] != '\0')
	{
		jstring_log("jstring_create_temporary: "
			"null terminator not found "
			"at end of passed string, instead found: '%c'\n"
			"expected chars[length] == '\0'\n"
			"returning zeroed out jstring struct",
			chars[length]);
		result_jstring.length = 0;
		result_jstring.capacity = 0;
		result_jstring.data = 0;
		return result_jstring;
	}
	jstring_temporary_memory_insertion_address[length] = '\0';

	result_jstring.data = jstring_temporary_memory_insertion_address;

	return result_jstring;
}

static b32 jstring_create_format_string(const char *chars, ...)
{
	/* TODO: signed numbers, unsigned numbers, strings should be the
	 * only format symbols you really need to recognize for now
	 * eventually floats 
	 */
	return true;
}

static u32 jstring_length(const char *string)
{
	const char *tmp = string;
	u32 length = 0;
	while(*tmp != '\0')
	{
		tmp++;
		length++;
		JSTRING_ASSERT(length < 4096, "really long string");
	}
	return length;
}

static i32 jstring_compare_raw(const char *first, const char *second)
{
	u32 index = 0;
	while((first[index] != '\0') && (second[index] != '\0'))
	{
		if(first[index] < second[index])
		{
			return -1;
		}
		if(first[index] > second[index])
		{
			return 1;
		}
		index++;
	}
	if(first[index] < second[index])
	{
		return -1;
	}
	if(first[index] > second[index])
	{
		return 1;
	}
	return 0;
}

static i32 jstring_compare_jstring(jstring first, jstring second)
{
	if(first.length < second.length)
	{
		return -1;
	}
	if(first.length > second.length)
	{
		return 1;
	}

	char *first_data = first.data;
	char *second_data = second.data;

	u32 index;
	for(index = 0; index < first.length; index++)
	{
		if(first_data[index] < second_data[index])
		{
			return -1;
		}
		if(first_data[index] > second_data[index])
		{
			return 1;
		}
	}

	return 0;
}

static i32 jstring_compare_jstring_and_raw(
		jstring first, 
		const char *second)
{
	if(first.length < jstring_length(second))
	{
		return -1;
	}
	if(first.length > jstring_length(second))
	{
		return 1;
	}

	u32 index;
	for(index = 0; index < first.length; index++)
	{
		if(first.data[index] < second[index])
		{
			return -1;
		}
		if(first.data[index] > second[index])
		{
			return 1;
		}
	}
	return 0;
}

static b32 jstring_equals_jstring_and_raw(
		jstring first,
		const char *second)
{
	if(jstring_compare_jstring_and_raw(first, second) == 0)
	{
		return true;
	}
	return false;
}

static b32 jstring_equals_raw(const char *first, const char *second)
{
	if(jstring_compare_raw(first, second) == 0)
	{
		return true;
	}
	return false;
}

static b32 jstring_equals_jstring(jstring first, jstring second)
{
	if(jstring_compare_jstring(first, second) == 0)
	{
		return true;
	}
	return false;
}

static jstring jstring_create_substring_temporary(
		jstring string, u32 start, u32 end)
{
	jstring result_jstring;
	result_jstring.length = end-start+1;
	result_jstring.capacity = 2 * (result_jstring.length + 1);
	char *tmp = 
		jstring_temporary_memory_allocate_string(
				result_jstring.capacity);
	u32 index;
	for(index = 0; index <= end-start; index++)
	{
		tmp[index] = string.data[index + start];
	}
	tmp[index] = '\0';
	result_jstring.data = tmp;

	return result_jstring;
}

static b32 jstring_concatenate_jstring(
		jstring *to, jstring from)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}
	if(to->length + from.length < to->capacity)
	{
		/* concatenate in place */
		char *tmp = to->data + to->length;
		u32 index;
		for(index = 0; index <= from.length; index++)
		{
			tmp[index] = from.data[index];
		}
		to->length += from.length;
	}
	else
	{
		/* concatenate by creating new jstring */
		if(jstring_temporary_memory_info.size < 
			jstring_temporary_memory_info.offset + 
			((to->length + from.length + 1) * 2))
		{
			JSTRING_ASSERT(0, " ");
		}
		char *tmp = 
			jstring_temporary_memory_allocate_string(
				((to->length + from.length + 1) * 2));

		u32 index;
		for(index = 0; index < to->length; index++)
		{
			tmp[index] = to->data[index];
		}
		to->data = tmp;
		tmp += to->length;
		for(index = 0; index <= from.length; index++)
		{
			tmp[index] = from.data[index];
		}
		to->length += from.length;
		to->capacity = 2 * (to->length + 1);
	}
	return true;
}

static b32 jstring_concatenate_raw(
		jstring *to, const char *from)
{
	u32 from_length = jstring_length(from);

	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
	}
	if(to->length + from_length < to->capacity)
	{
		/* concatenate in place */
		char *tmp = to->data + to->length;
		u32 index;
		for(index = 0; index <= from_length; index++)
		{
			tmp[index] = from[index];
		}
		to->length += from_length;
	}
	else
	{
		/* concatenate by creating new jstring */
		char *tmp = 
			jstring_temporary_memory_allocate_string(
				((to->length + from_length + 1) * 2));

		u32 index;
		for(index = 0; index < to->length; index++)
		{
			tmp[index] = to->data[index];
		}
		to->data = tmp;
		tmp += to->length;
		for(index = 0; index <= from_length; index++)
		{
			tmp[index] = from[index];
		}
		to->length += from_length;
		to->capacity = 2 * (to->length + 1);
	}
	return true;
}

static b32 jstring_begins_with(jstring string, const char *chars)
{
	u32 chars_length = jstring_length(chars);
	if(chars_length > string.length)
	{
		return false;
	}
	u32 index = 0;
	while(chars[index] == string.data[index])
	{
		index++;
		if(index == chars_length)
		{
			return true;
		}
	}

	return false;
}

static b32 jstring_ends_with(jstring string, const char *chars)
{
	u32 chars_length = jstring_length(chars);
	if(chars_length > string.length)
	{
		return false;
	}
	u32 index = 0;
	while(chars[index] == 
			string.data[string.length - chars_length + index])
	{
		index++;
		if(index == chars_length)
		{
			return true;
		}
	}

	return false;
}

/* TODO: be able to take signed int (i32) */
static jstring jstring_create_integer(u32 number)
{
	u32 digits = 1;
	u32 divisor = 10;
	u32 index;

	jstring return_string;
	while(number / divisor != 0)
	{
		digits++;
		divisor *= 10;
	}
	divisor /= 10;

	char *tmp =
		jstring_temporary_memory_allocate_string(
			((digits + 1) * 2));

	for(index = 0; index < digits; index++)
	{
		tmp[index] = (number / divisor) + 48;
		number %= divisor;
		divisor /= 10;
	}
	tmp[digits] = '\0';
	return_string.data = tmp;
	return_string.length = digits;
	return_string.capacity = 2 * (return_string.length + 1);
	return return_string;
}

static i32 jstring_index_of_raw(jstring string, const char *chars)
{
	u32 chars_length = jstring_length(chars);
	u32 index;
	for(index = 0; index <= string.length - chars_length; index++)
	{
		u32 inner_index;
		for(inner_index = 0; inner_index < chars_length; inner_index++)
		{
			if(string.data[inner_index + index] != chars[inner_index])
			{
				break;
			}
			if(inner_index == chars_length - 1)
			{
				return index;
			}
		}
	}
	return -1;
}

static i32 jstring_index_of_jstring(jstring string, jstring check)
{
	u32 index;
	for(index = 0; index <= string.length - check.length; index++)
	{
		u32 inner_index;
		for(inner_index = 0; inner_index < check.length; inner_index++)
		{
			if(string.data[inner_index + index] != 
					check.data[inner_index])
			{
				break;
			}
			if(inner_index == check.length - 1)
			{
				return index;
			}
		}
	}
	return -1;
}

static i32 jstring_last_index_of_raw(jstring string, const char *chars)
{
	u32 chars_length = jstring_length(chars);
	u32 index;
	i32 last = -1;
	for(index = 0; index <= string.length - chars_length; index++)
	{
		u32 inner_index;
		for(inner_index = 0; inner_index < chars_length; inner_index++)
		{
			if(string.data[inner_index + index] != chars[inner_index])
			{
				break;
			}
			if(inner_index == chars_length - 1)
			{
				jstring_log("%d", last);
				last = index;
			}
		}
	}
	return last;
}

static i32 jstring_last_index_of_jstring(jstring string, jstring check)
{
	u32 index;
	i32 last = -1;
	for(index = 0; index <= string.length - check.length; index++)
	{
		u32 inner_index;
		for(inner_index = 0; inner_index < check.length; inner_index++)
		{
			if(string.data[inner_index + index] != 
					check.data[inner_index])
			{
				break;
			}
			if(inner_index == check.length - 1)
			{
				last = index;
			}
		}
	}
	return last;
}

static void jstring_to_upper_in_place(jstring *string)
{
	u32 index;
	for(index = 0; index < string->length; index++)
	{
		if((string->data[index] < 123) && (string->data[index] > 96))
		{
			string->data[index] = string->data[index] - 32;
		}
	}
}

static void jstring_to_lower_in_place(jstring *string)
{
	u32 index;
	for(index = 0; index < string->length; index++)
	{
		if((string->data[index] < 91) && (string->data[index] > 64))
		{
			string->data[index] = string->data[index] + 32;
		}
	}
}

static jstring jstring_to_upper_jstring(jstring string)
{
	u32 index;
	jstring return_jstring;
	char *tmp = jstring_temporary_memory_info.address +
		jstring_temporary_memory_info.offset;

	return_jstring.length = string.length;
	return_jstring.capacity = (return_jstring.length + 1) * 2;

	for(index = 0; index <= string.length; index++)
	{
		if((string.data[index] < 123) && (string.data[index] > 96))
		{
			tmp[index] = string.data[index] - 32;
		}
		else
		{
			tmp[index] = string.data[index];
		}
	}

	jstring_temporary_memory_info.offset += return_jstring.capacity;

	return return_jstring;
}

static jstring jstring_to_lower_jstring(jstring string)
{
	u32 index;
	jstring return_jstring;
	char *tmp = jstring_temporary_memory_info.address +
		jstring_temporary_memory_info.offset;

	return_jstring.length = string.length;
	return_jstring.capacity = (return_jstring.length + 1) * 2;

	for(index = 0; index <= string.length; index++)
	{
		if((string.data[index] < 91) && (string.data[index] > 64))
		{
			tmp[index] = string.data[index] + 32;
		}
		else
		{
			tmp[index] = string.data[index];
		}
	}

	jstring_temporary_memory_info.offset += return_jstring.capacity;

	return return_jstring;
}

/* NOTE: convenience function for insert/remove/replace/trim 
 * type functions. really shouldn't be used by "users" of the jstring
 * library, but anything goes lol I want "them" (me) to have full control
 * if needed. I mean users have direct access to the jstring memory since
 * they allocate it themselves. dangerous but useful. they could write
 * this function themselves
 */
static b32 copy_temporary_memory_chars(
		char *address, 
		i64 to_offset,
		i32 num_chars)
{
	i32 index;
	if(to_offset > 0)
	{
		/* copy it backwards so that you don't overwrite the data
		 * that you're copying. hope that makes sense
		 */
		for(index = num_chars - 1; index >= 0; index--)
		{
			(address + to_offset)[index] = address[index];
		}
	}
	else
	{
		for(index = 0; index < num_chars; index++)
		{
			(address + to_offset)[index] = address[index];
		}
	}
	
	return true;
}

static b32 jstring_insert_chars_at(
		jstring *to, const char *chars, u32 index)
{
	if(index > to->length)
	{
		/* NOTE: this might technically be caught by the next if 
		 * statement ? perchance
		 */
		JSTRING_ASSERT(0, " ");
		return false;
	}
	u32 chars_length = jstring_length(chars);

	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	/* don't need to create a whole new jstring */
	if((to->capacity - to->length) > chars_length)
	{
		copy_temporary_memory_chars(
			to->data + index, chars_length, to->length - index + 1); 
		u32 loop_index;
		for(loop_index = 0; loop_index < chars_length; loop_index++)
		{
			to->data[index + loop_index] = chars[loop_index];
		}
		to->length += chars_length;
	}
	/* need to create a whole new jstring */
	else
	{
		u32 new_length = to->length + chars_length;
		char *tmp = 
			jstring_temporary_memory_allocate_string(
				(new_length + 1) * 2);

		u32 loop_index;
		for(loop_index = 0; loop_index < index; loop_index++)
		{
			tmp[loop_index] = to->data[loop_index];
		}
		for(loop_index = 0; loop_index < chars_length; loop_index++)
		{
			tmp[index + loop_index] = chars[loop_index];
		}
		for(
			loop_index = 0; 
			loop_index < (to->length - index + 1);
			loop_index++)
		{
			tmp[index + chars_length + loop_index] = 
				to->data[index + loop_index];
		}

		to->length = new_length;
		to->capacity = (to->length + 1) * 2;
		to->data = tmp;
	}

	return true;
}

static b32 jstring_insert_jstring_at(jstring *to, jstring from, u32 index)
{
	if(index > to->length)
	{
		/* NOTE: this might technically be caught by the next if statement
		 * ? perchance
		 */
		JSTRING_ASSERT(0, " ");
		return false;
	}

	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	/* don't need to create a whole new jstring */
	if((to->capacity - to->length) > from.length)
	{
		copy_temporary_memory_chars(
			to->data + index, from.length, to->length - index + 1); 
		u32 loop_index;
		for(loop_index = 0; loop_index < from.length; loop_index++)
		{
			to->data[index + loop_index] = from.data[loop_index];
		}
		to->length += from.length;
	}
	/* need to create a whole new jstring */
	else
	{
		u32 new_length = to->length + from.length;
		char *tmp = 
			jstring_temporary_memory_allocate_string(
				(new_length + 1) * 2);
		
		u32 loop_index;
		for(loop_index = 0; loop_index < index; loop_index++)
		{
			tmp[loop_index] = to->data[loop_index];
		}
		for(loop_index = 0; loop_index < from.length; loop_index++)
		{
			tmp[index + loop_index] = from.data[loop_index];
		}
		for(
			loop_index = 0; 
			loop_index < (to->length - index + 1);
			loop_index++)
		{
			tmp[index + from.length + loop_index] = 
				to->data[index + loop_index];
		}

		to->length = new_length;
		to->capacity = (to->length + 1) * 2;
		to->data = tmp;
	}

	return true;
}

static b32 jstring_remove_at(
		jstring *string, u32 index, u32 remove_length)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	if(index + remove_length > string->length)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	i32 temp = (i32)remove_length;
	if(!copy_temporary_memory_chars(
		(string->data + index + remove_length), 
		(-temp),
		(string->length - index - remove_length + 1)))
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	string->length -= remove_length;

	return true;
}

static b32 jstring_remove_chars(jstring *string, const char *chars)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	i32 chars_index = jstring_index_of_raw(*string, chars);

	if(chars_index < 0)
	{
		jstring_log("jstring_remove_chars: '%s' not in string @%p",
				chars, string->data);
		return false;
	}

	if(!jstring_remove_at(string, chars_index, jstring_length(chars)))
	{
		return false;
	}

	return true;
}

static b32 jstring_remove_chars_all(jstring *string, const char *chars)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	i32 chars_index = jstring_index_of_raw(*string, chars);


	if(chars_index < 0)
	{
		jstring_log("jstring_remove_chars: '%s' not in string @%p",
				chars, string->data);
		return false;
	}

	while(chars_index > 0)
	{
		if(!jstring_remove_at(string, chars_index, jstring_length(chars)))
		{
			return false;
		}
		chars_index = jstring_index_of_raw(*string, chars);
	}

	return true;
}

static b32 jstring_remove_jstring(jstring *string, jstring remove_string)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	i32 chars_index = jstring_index_of_jstring(*string, remove_string);

	if(chars_index < 0)
	{
		jstring_log("jstring_remove_chars: '%s' not in string @%p",
				remove_string.data, string->data);
		return false;
	}

	if(!jstring_remove_at(string, chars_index, remove_string.length))
	{
		return false;
	}

	return true;
}

static b32 jstring_remove_jstring_all(
		jstring *string, 
		jstring remove_string)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	i32 chars_index = jstring_index_of_jstring(*string, remove_string);

	if(chars_index < 0)
	{
		jstring_log("jstring_remove_chars: '%s' not in string @%p",
				remove_string.data, string->data);
		return false;
	}

	while(chars_index > 0)
	{
		if(!jstring_remove_at(string, chars_index, remove_string.length))
		{
			return false;
		}
		chars_index = jstring_index_of_jstring(*string, remove_string);
	}

	return true;
}

/* NOTE: faster way to do this would just be to paste the replacement
 * string over the old stuff rather than calling these 2 already 
 * existing functions ?
 */
static b32 jstring_replace_at_raw(
		jstring *string, 
		u32 index, 
		u32 remove_length,
		const char *chars)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	if(!jstring_remove_at(string, index, remove_length))
	{
		return false;
	}
	if(!jstring_insert_chars_at(string, chars, index))
	{
		return false;
	}
	return true;
}

/* NOTE: faster way to do this would just be to paste the replacement
 * string over the old stuff rather than calling these 2 already 
 * existing functions ?
 */
static b32 jstring_replace_at_jstring(
		jstring *string, 
		u32 index, 
		u32 remove_length,
		jstring replace_string)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	if(!jstring_remove_at(string, index, remove_length))
	{
		return false;
	}
	if(!jstring_insert_jstring_at(string, replace_string, index))
	{
		return false;
	}
	return true;
}

/* TODO: implement these if you ever feel like they'd be really nice
 * to have I guess
 *
 *		jstring_replace_jstring_with_raw
 *		jstring_replace_raw_with_jstring
 *		jstring_replace_raw_with_raw
 *		jstring_replace_jstring_with_jstring
 *
 *		also maybe (?):
 *		jstring_replace_jstring_with_raw_all
 *		jstring_replace_raw_with_jstring_all
 *		jstring_replace_raw_with_raw_all
 *		jstring_replace_jstring_with_jstring_all
 */

static b32 jstring_copy_to_buffer(
		char *buffer, 
		u32 buffer_size, 
		jstring string)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
		return false;
	}

	if(string.length > buffer_size)
	{
		jstring_log(
				"jstring_copy_to_buffer: cannot copy jstring to buffer."
				"\nbuffer has size %u, and string has length %u",
				buffer_size, string.length);
		return false;
	}
	u32 index;
	for(index = 0; index <= string.length; index++)
	{
		buffer[index] = string.data[index];
	}
	return true;
}

static jstring jstring_copy_to_jstring(jstring string)
{
	if(!jstring_temporary_memory_info.activated)
	{
		JSTRING_ASSERT(0, " ");
	}

	jstring return_string;
	return_string.capacity = 0;
	return_string.length = 0;
	return_string.data = 0;

	return_string.length = string.length;
	return_string.capacity = (return_string.length + 1) * 2;
	return_string.data = 
			jstring_temporary_memory_allocate_string(
				(return_string.length + 1) * 2);

	u32 index;
	for(index = 0; index <= string.length; index++)
	{
		return_string.data[index] = string.data[index];
	}

	return return_string;
}

static jstring jstring_join_jstrings(
		jstring *strings, 
		u32 strings_count,
		const char *separator)
{
	jstring result_string;
	u32 index;
	u32 separator_length = jstring_length(separator);

	result_string.length = 0;

	/* figure out length of new string and get memory for it */
	for(index = 0; index < strings_count; index++)
	{
		result_string.length += strings[index].length;
		if(index != strings_count - 1)
		{
			result_string.length += separator_length;
		}
	}

	result_string.capacity = (result_string.length + 1) * 2;

	result_string.data = 
		jstring_temporary_memory_allocate_string(result_string.capacity);

	/* fill out the data for the new string */
	char *tmp = result_string.data;
	for(index = 0; index < strings_count; index++)
	{
		if(!jstring_copy_to_buffer(
				tmp, result_string.capacity, strings[index]))
		{
			JSTRING_ASSERT(0, " ");
		}

		tmp += strings[index].length;

		if(index != strings_count - 1)
		{
			u32 inner_index;
			for(
				inner_index = 0; 
				inner_index < separator_length; 
				inner_index++)
			{
				tmp[inner_index] = separator[inner_index];
			}

			tmp += separator_length;
		}
	}
	result_string.data[result_string.length] = '\0';

	return result_string;
}

/* NOTE: idk if this is a useful way to have this function but I'll
 * use it and see if it's more/less cumbersome than strtok
 */
/* TODO: maybe also have a version of the function that is "destructive"
 * and does not prefer the original string, because creating the temp
 * string in this version of the function kinda takes up a good bit
 * of the string memory
 */
static b32 jstring_split_jstring(
		jstring string,
		jstring *result_strings_list, 
		u32 result_strings_list_size,
		const char *separator)
{
	jstring temp = jstring_copy_to_jstring(string);
	u32 separator_length = jstring_length(separator);

	i32 separator_index = 
		jstring_index_of_raw(temp, separator);
	u32 result_strings_list_index = 0;
	while(separator_index != -1)
	{
		if(result_strings_list_index >= result_strings_list_size)
		{
			JSTRING_ASSERT(0, " ");
			return false;
		}

		result_strings_list[result_strings_list_index] =
			jstring_create_substring_temporary(
				temp, 0, separator_index - 1);

		if(!jstring_remove_at(
			&temp, 0, separator_index + separator_length))
		{
			return false;
		}

		separator_index = 
			jstring_index_of_raw(temp, separator);

		result_strings_list_index++;

	}
	jstring_log("DEBUG: temp -> %s\nlength: %u, capacity: %u", 
				temp.data, temp.length, temp.capacity);

	result_strings_list[result_strings_list_index] =
		jstring_create_substring_temporary(
			temp, 0, temp.length - 1);

	return true;
}

/* NOTE: convenience function for trimming functions */
static b32 char_is_whitespace(char c)
{
	switch(c)
	{
		case 9:  /* tab */
		case 10: /* line feed (newline) */
		case 11: /* vertical tab (?) */
		case 12: /* form feed */
		case 13: /* carriage return */
		case 32: /* space */
		{
			return true;
		} break;
		default:
		{
			return false;
		}
	}
}

/* NOTE: these all do the trimming in place */
static b32 jstring_trim_left(jstring *string)
{
	u32 index = 0;
	while(char_is_whitespace(string->data[index]))
	{
		index++;
	}
	if(!jstring_remove_at(string, 0, index))
	{
		return false;
	}
	return true;
}
static b32 jstring_trim_right(jstring *string)
{
	/* start at end */
	u32 index = string->length - 1;
	while(char_is_whitespace(string->data[index]))
	{
		index--;
	}
	if(!jstring_remove_at(string, index + 1, string->length - index - 1))
	{
		return false;
	}
	return true;
}
static b32 jstring_trim(jstring *string)
{
	if(!jstring_trim_right(string))
	{
		return false;
	}
	if(!jstring_trim_left(string))
	{
		return false;
	}
	return true;
}

#endif
