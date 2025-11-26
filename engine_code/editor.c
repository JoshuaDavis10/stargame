#include "game.h"

#include "linux_util.c"
#include "jstring.h"
#include "math.c"

#include "profiler.c"

typedef struct {
	vector_2 position;
	/* NOTE: -x and +x maximums + -y and +y maximums for camera coords 
	 * that will actually be drawn 
	 */
	vector_2 bounds; 
} camera;

#include "cpu_render.c"

#define JSTRING_MEMORY_SIZE 1024

enum {
	TILE_TYPE_BLUE,
	TILE_TYPE_RED,
	TILE_TYPE_GREEN,
	TILE_TYPE_COUNT
};

enum {
	UNIT_TYPE_NONE,
	UNIT_TYPE_BLUE,
	UNIT_TYPE_RED,
	UNIT_TYPE_GREEN,
	UNIT_TYPE_COUNT
};

typedef struct {
	i32 tile_type;
	i32 unit_type;
} tile;

typedef struct {
	i32 width;
	i32 height;
	tile *tiles;
} tilemap_data;

typedef struct {
	camera game_camera; /* NOTE: world space coords */
	u64 last_time;
	u64 time_elapsed;
	f64 timer;
	b32 initialized;
	tilemap_data tilemap;
} game_state;

static void *game_memory_allocate(u64 *used_memory, u64 size, void *game_memory, u64 game_memory_size)
{
	_assert((*used_memory) + size <= game_memory_size);
	void *result = (void*) ((char*)game_memory + (*used_memory));
	(*used_memory) += size;
	return result;
}

void game_update_and_render(
		void *game_memory,
		u64 game_memory_size,
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		x_input_state *input,
		char *level_filename) 
{
	start_profile();

	PROFILER_START_TIMING_BLOCK(memory_stuff);

	u64 used_memory = 0;

	void *jstring_memory = (void*)game_memory_allocate(&used_memory, JSTRING_MEMORY_SIZE, game_memory, game_memory_size); 

	_assert(jstring_memory_activate(JSTRING_MEMORY_SIZE, jstring_memory));

	game_state *state = (game_state*)game_memory_allocate(&used_memory, sizeof(game_state), game_memory, game_memory_size);

	PROFILER_FINISH_TIMING_BLOCK(memory_stuff);

	/* initialize */
	/* NOTE: game code expects platform layer to
	 * initially hand it zeroed out memory 
	 */
	if(!state->initialized)
	{
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

		/* TODO: open the level file that was specified 
		 * - IF the file doesn't exist yet, what do we do?
		 * - maybe we make a whole friggin UI
		 * - so we have a button menu, either to open or create a file
		 * is this gonna be a lot of work...
		 * we need a button, we need like drop down menus? we need to be able to get file data and open files from
		 * like a file explorer???
		 */

		LOG_INFO("initialized game state.");
	}

	/* update */
	PROFILER_START_TIMING_BLOCK(update);
	state->time_elapsed = read_os_timer() - state->last_time;	
	state->timer += state->time_elapsed;
	state->last_time = read_os_timer();

	/* TODO: edit tilemap based on input */
	/* TODO: some button to save to file */

	PROFILER_FINISH_TIMING_BLOCK(update);

	/* render */
	PROFILER_START_TIMING_BLOCK(render);
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		gray); 

	/* TODO: render level */

	PROFILER_FINISH_TIMING_BLOCK(render);

	if(state->timer > 2000000.0)
	{
		finish_and_print_profile(LOG_TRACE);
		state->timer = 0.0;
	}
}
