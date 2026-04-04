#include "game.h"

#include "../engine_code/util.c"
#include "../engine_code/profiler.c"
#include "../engine_code/math.c"

#include "../engine_code/jstring.h"

typedef struct {
	vector_2 position;
	/* NOTE: -x and +x maximums + -y and +y maximums for camera coords 
	 * that will actually be drawn 
	 */
	vector_2 bounds; 
} camera;

typedef struct {
	camera cam;
	f64 last_time;
	f64 timer;
	b32 initialized;
} game_state;

#include "../engine_code/cpu_render.c"
#include "../engine_code/particle.c"

static void *game_memory_allocate(u64 *used_memory, u64 size, void *game_memory, u64 game_memory_size)
{
	_assert((*used_memory) + size <= game_memory_size);
	void *result = (void*) ((char*)game_memory + (*used_memory));
	(*used_memory) += size;
	return result;
}

static game_state *global_game_state = 0;

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

	/* get pointer to game state */
	u64 used_memory = 0;
	global_game_state = 
		game_memory_allocate(
			&used_memory, 
			sizeof(game_state), 
			game_memory, 
			game_memory_size);

	_assert(global_game_state);

	/* initialize game state */
	PROFILER_START_TIMING_BLOCK(initialization);
	if(!(global_game_state->initialized))
	{
		global_game_state->last_time = read_os_timer();
		global_game_state->timer = 0.0;
		global_game_state->initialized = true;
		global_game_state->cam.position.x = 0.0f;
		global_game_state->cam.position.y = 0.0f;
		global_game_state->cam.bounds.x = 8.0f;
		global_game_state->cam.bounds.y = 4.5f;
	}
	PROFILER_FINISH_TIMING_BLOCK(initialization);

	f64 time = read_os_timer();
	f64 elapsed_time = time - global_game_state->last_time;
	global_game_state->last_time = time;
	global_game_state->timer += elapsed_time;

	PROFILER_START_TIMING_BANDWIDTH(game_update_and_render, sizeof(rgba_color) * pixel_buffer_width * pixel_buffer_height);

	draw_background_in_buffer(pixel_buffer, pixel_buffer_width, pixel_buffer_height, gray);	

	PROFILER_FINISH_TIMING_BLOCK(game_update_and_render);

	/* every 2 seconds print profile data */
	if(global_game_state->timer > 2000000.0)
	{
		finish_and_print_profile(log_trace, cpu_frequency);
		global_game_state->timer = 0.0;
	}
}

PROFILER_END;
