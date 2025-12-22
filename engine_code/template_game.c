#include "game.h"

#include "util.c"
#include "profiler.c"
#include "math.c"

#include "jstring.h"

typedef struct {
	vector_2 position;
	/* NOTE: -x and +x maximums + -y and +y maximums for camera coords 
	 * that will actually be drawn 
	 */
	vector_2 bounds; 
} camera;

#include "cpu_render.c"

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
		char *level_filename) 

#endif
{
	/*
	start_profile();

	PROFILER_START_TIMING_BLOCK(game_update_and_render);
	*/

	draw_background_in_buffer(pixel_buffer, pixel_buffer_width, pixel_buffer_height, red);	

	/*
	PROFILER_FINISH_TIMING_BLOCK(game_update_and_render);
	finish_and_print_profile(log_trace);
	*/
}
