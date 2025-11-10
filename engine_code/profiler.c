#ifndef PROFILER
#define PROFILER 0
#endif

#if PROFILER

#define PROFILER_UNIT_COUNT 4096
#define PROFILER_END \
_static_assert(__COUNTER__ < PROFILER_UNIT_COUNT, counter_went_past_profiler_unit_count);

typedef struct {
	const char *name;
	u64 tsc_elapsed_inclusive;
	u64 tsc_elapsed_exclusive;
	u64 bytes_processed;
	u32 hits;
} profiler_unit;

typedef struct {
	profiler_unit units[PROFILER_UNIT_COUNT];
	u64 tsc_start;
	u64 tsc_end;
} profiler;

typedef struct {
	u64 tsc_start;
	u64 unit_tsc_inclusive;
	u32 unit_index;
	u32 parent_unit_index;
} profiler_block;

static profiler global_profiler = {0};
static u32 global_parent_unit_index = 0;

#define CONCAT(a, b) a##b

#define PROFILER_START_TIMING_BLOCK(block_name) PROFILER_START_TIMING_BANDWIDTH(block_name, 0)

#define PROFILER_START_TIMING_BANDWIDTH(block_name, bytes) \
profiler_block CONCAT(profiler_block_, block_name); \
{ \
	/* NOTE(josh): __COUNTER__ is not in C standard but seems to work with gcc -std=c89 fine */ \
	CONCAT(profiler_block_, block_name).unit_index = __COUNTER__ + 1; \
\
	u32 temp_unit_index = CONCAT(profiler_block_, block_name).unit_index; \
\
	CONCAT(profiler_block_, block_name).unit_tsc_inclusive = \
		global_profiler.units[temp_unit_index].tsc_elapsed_inclusive; \
	CONCAT(profiler_block_, block_name).parent_unit_index = global_parent_unit_index; \
\
	global_profiler.units[temp_unit_index].bytes_processed += bytes; \
\
	global_parent_unit_index = temp_unit_index; \
	CONCAT(profiler_block_, block_name).tsc_start = read_cpu_timer(); \
}

#define PROFILER_FINISH_TIMING_BLOCK(block_name) \
{ \
	u64 temp_tsc_end = read_cpu_timer(); \
	u64 temp_tsc_start  = CONCAT(profiler_block_, block_name).tsc_start; \
	u64 temp_unit_index = CONCAT(profiler_block_, block_name).unit_index; \
	u64 temp_parent_unit_index = CONCAT(profiler_block_, block_name).parent_unit_index; \
\
	global_profiler.units[temp_unit_index].name = #block_name; \
	global_profiler.units[temp_unit_index].hits++; \
	global_profiler.units[temp_unit_index].tsc_elapsed_exclusive += (temp_tsc_end - temp_tsc_start); \
	global_profiler.units[temp_unit_index].tsc_elapsed_inclusive = \
		CONCAT(profiler_block_, block_name).unit_tsc_inclusive + (temp_tsc_end - temp_tsc_start); \
\
	global_profiler.units[temp_parent_unit_index].tsc_elapsed_exclusive \
		-= (temp_tsc_end - temp_tsc_start); \
\
	global_parent_unit_index = CONCAT(profiler_block_, block_name).parent_unit_index; \
}

static void start_profile()
{
	global_profiler.tsc_start = read_cpu_timer();
}

/* NOTE(josh): pass whatever logging function you want to use for it */
static void finish_and_print_profile(void (*logger)(const char *, ...))
{
	global_profiler.tsc_end = read_cpu_timer();
	if(!(logger))
	{
		return;
	}

	u64 total_elapsed = global_profiler.tsc_end - global_profiler.tsc_start;

	u64 cpu_frequency = read_cpu_frequency();
	/* NOTE(josh): avoiding a division by 0 */
	_assert(cpu_frequency);
	logger("PROFILE: Total time: %0.4lfms (CPU freq: %llu | total TSC: %llu)", 
		1000.0 * (f64)total_elapsed / (f64)cpu_frequency, cpu_frequency, total_elapsed);

	u32 unit_index = 1;
	u64 elapsed_exclusive;
	u64 elapsed_inclusive;
	u64 bytes_processed;
	while(unit_index < PROFILER_UNIT_COUNT)
	{
		elapsed_exclusive = global_profiler.units[unit_index].tsc_elapsed_exclusive;
		elapsed_inclusive = global_profiler.units[unit_index].tsc_elapsed_inclusive;

		if(elapsed_exclusive)
		{
			f64 percentage = 
				((f64)elapsed_exclusive / (f64)total_elapsed) * 100.0;
			logger("  %s[%llu]: %llu (%.2lf%%)", 
				global_profiler.units[unit_index].name,
				global_profiler.units[unit_index].hits,
				elapsed_exclusive,
				percentage);
			if(elapsed_exclusive != elapsed_inclusive)
			{
				f64 percentage_with_children = 
					((f64)elapsed_inclusive / 
					(f64)total_elapsed) * 100.0;
				logger("    (%.2lf%% with children)", percentage_with_children);
			}
		}

		bytes_processed = global_profiler.units[unit_index].bytes_processed;
		if(bytes_processed)
		{
			f64 megabyte = 1024.0 * 1024.0;
			f64 gigabyte = megabyte * 1024.0;

			f64 seconds = (f64)elapsed_inclusive / (f64)cpu_frequency;
			f64 bytes_per_second = (f64)bytes_processed / seconds;
			f64 megabytes = bytes_processed / megabyte;
			f64 gigabytes_per_second = bytes_per_second / gigabyte;
			logger("    %.6lfmb at %.6lfgb/s", megabytes, gigabytes_per_second); 
		}

		unit_index++;
	}
}


#else

#define PROFILER_START_TIMING_BLOCK(name) 
#define PROFILER_FINISH_TIMING_BLOCK(name)
#define PROFILER_END

typedef struct {
	u64 tsc_start;
	u64 tsc_end;
} profiler;

static profiler global_profiler = {0};

static void start_profile()
{
	global_profiler.tsc_start = read_cpu_timer();
}

/* NOTE(josh): pass whatever logging function you want to use for it */
static void finish_and_print_profile(void (*logger)(const char *, ...))
{
	global_profiler.tsc_end = read_cpu_timer();
	/* NOTE(josh): commenting this out bc read_cpu_frequency causes a 100ms lag every time
	 * the profiler prints
	 */
	/*
	if(!(logger))
	{
		return;
	}

	u64 total_elapsed = global_profiler.tsc_end - global_profiler.tsc_start;

	u64 cpu_frequency = read_cpu_frequency();
	_assert(cpu_frequency);
	logger("PROFILE: Total time: %0.4lfms (CPU freq: %llu | total TSC: %llu)", 
		1000.0 * (f64)total_elapsed / (f64)cpu_frequency, cpu_frequency, total_elapsed);
		*/
}

#endif

/* NOTE(josh): put _static_assert(__COUNTER__ < PROFILER_UNIT_COUNT); at end of any program that 
 * uses profiler
 */
