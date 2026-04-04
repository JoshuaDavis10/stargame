#include "game.h"

#include "../engine_code/util.c"
#include "../engine_code/jstring.h"
#include "../engine_code/math.c"

#include "../engine_code/profiler.c"

typedef struct {
	vector_2 position;
	vector_2 bounds; 
} camera;

#include "../engine_code/cpu_render.c"

#define JSTRING_MEMORY_SIZE 1024

#include "tilegame/tile_types.h"

#define TILEMAP_OFFSET_X 50
#define TILEMAP_OFFSET_Y 50
#define TILE_SIZE_COEFFICIENT 350

typedef struct {
	u32 blue_counter;
	u32 green_counter;
	u32 red_counter;
	i32 width;
	i32 height;
	tile tiles;
} tilemap;

typedef struct {
	char *filename;
	u64 size;
} file;

typedef struct {
	camera game_camera; 
	u64 last_time;
	u64 time_elapsed;
	f64 timer;
	b32 initialized;
	u8 *pixel_buffer;
	u16 pixel_buffer_width;
	u16 pixel_buffer_height;
	u64 tilemap_data_size;
	tilemap *tilemap_data;
	file level_file;
} game_state;


static tile *get_tile_from_index(game_state *state, i32 index) { return( (tile*)(&(state->tilemap_data->tiles)) + index); }
static void game_draw_tilemap(game_state *state);

static void *game_memory_allocate(u64 *used_memory, u64 size, void *game_memory, u64 game_memory_size)
{
	_assert((*used_memory) + size <= game_memory_size);
	void *result = (void*) ((char*)game_memory + (*used_memory));
	(*used_memory) += size;
	return result;
}

static void game_memory_free(u64 *used_memory, u64 size, void *game_memory, void *address)
{
	/* NOTE(josh): basically enforcing that we can only free off the end of the bump allocator */
	(*used_memory) -= size;
	void *expected_game_memory_address = (void*) ((char*)address - (*used_memory));
	log_debug("game_memory_free: addr: %p, size: %llu, used mem: %llu expected: %p, game memory: %p",
		address, size, *used_memory, expected_game_memory_address, game_memory);
	_assert(expected_game_memory_address == game_memory);
}

static void editor_draw_tilemap(game_state *state);

#ifdef __linux__
void game_update_and_render(
		void *game_memory,
		u64 game_memory_size,
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		input_state *input,
		char *level_filename) 
#endif

#ifdef _WIN32
__declspec(dllexport) void game_update_and_render(
		void *game_memory,
		u64 game_memory_size,
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		input_state *input,
		char *level_filename,
		u64 cpu_frequency) 
#endif
{
	start_profile();

	PROFILER_START_TIMING_BLOCK(setup);

	u64 used_memory = 0;
	void *jstring_memory = 0;
	game_state *state = (game_state*)game_memory_allocate(&used_memory, sizeof(game_state), game_memory, game_memory_size);
	_assert(state);

	if(state->initialized)
	{
		jstring_memory = (void*)game_memory_allocate(&used_memory, JSTRING_MEMORY_SIZE, game_memory, game_memory_size); 
		_assert(jstring_memory_activate(JSTRING_MEMORY_SIZE, jstring_memory));
		state->tilemap_data = 
			(void*)game_memory_allocate(
					&used_memory,
					state->tilemap_data_size,
					game_memory,
					game_memory_size);
		_assert(state->tilemap_data);
	}

	if(!state->initialized)
	{
		b32 init_success = true;

		jstring_memory = (void*)game_memory_allocate(&used_memory, JSTRING_MEMORY_SIZE, game_memory, game_memory_size); 
		_assert(jstring_memory_activate(JSTRING_MEMORY_SIZE, jstring_memory));

		/* state */
		state->initialized = true;

			/* time stuff */
		state->last_time = read_os_timer();
		state->timer = 0.0;

			/* camera */
		state->game_camera.position.x = 0.0f;
		state->game_camera.position.y = 0.0f;
		state->game_camera.bounds.x = 16.0f;
		state->game_camera.bounds.y = 9.0f;

			/* pixel buffer */
		state->pixel_buffer = pixel_buffer;
		state->pixel_buffer_width = pixel_buffer_width;
		state->pixel_buffer_height = pixel_buffer_height;

			/* load level */
		state->level_file.filename = level_filename;
		state->level_file.size = get_file_size(state->level_file.filename);
		state->tilemap_data_size = state->level_file.size;
		log_debug("level file size: %llu", state->level_file.size);

		if(state->level_file.size == 0) /* level doesn't exist */
		{
			log_error("level '%s' does not exist", state->level_file.filename);
			init_success = false;
		}
		else /* level exists */
		{
			state->tilemap_data = 
				(void*)game_memory_allocate(
						&used_memory,
						state->level_file.size,
						game_memory,
						game_memory_size);
			_assert(state->tilemap_data);

			if(!read_file_into_buffer(
				state->level_file.filename, 
				state->tilemap_data,
				state->level_file.size))
			{
				log_error("failed to read level file");
				init_success = false;
			}
		}

		if(init_success)
		{
			log_info("initialized editor state.");
		}
		else
		{
			log_error("failed to initialize editor state.");
			_assert(0);
		}
	}

	PROFILER_FINISH_TIMING_BLOCK(setup);

	/* update */
	PROFILER_START_TIMING_BLOCK(update);
	state->time_elapsed = read_os_timer() - state->last_time;
	state->timer += state->time_elapsed;
	state->last_time = read_os_timer();

	/* TODO: input handling */

	PROFILER_FINISH_TIMING_BLOCK(update);

	/* render */
	PROFILER_START_TIMING_BLOCK(render);
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		black); 

	game_draw_tilemap(state);

	PROFILER_FINISH_TIMING_BLOCK(render);

	if(state->timer > 2000000.0)
	{
		finish_and_print_profile(log_trace, cpu_frequency);
		state->timer = 0.0;
	}
}

static void game_draw_tilemap(game_state *state)
{

	draw_fill_rectangle_in_buffer(state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 100, 100, 100, 100, green);
	return;	
}
