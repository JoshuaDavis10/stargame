#include "game.h"

#include "linux_util.c"
#include "jstring.h"
#include "math.c"

#include "profiler.c"

/* TODO: this needs to be some constant * the width of tiles or smn so that it scales with map size 
 * (inversely ofc) */
#define TILE_SIZE 50

typedef struct {
	vector_2 position;
	/* NOTE: -x and +x maximums + -y and +y maximums for camera coords 
	 * that will actually be drawn 
	 */
	vector_2 bounds; 
} camera;

#include "cpu_render.c"

#define JSTRING_MEMORY_SIZE 1024

#include "tile_types.h"

typedef struct {
	camera game_camera; /* NOTE: world space coords */
	u64 last_time;
	u64 time_elapsed;
	f64 timer;
	b32 initialized;
	b32 tilemap_saved;
	u8 *pixel_buffer;
	u16 pixel_buffer_width;
	u16 pixel_buffer_height;
	/* NOTE(josh): accessor functions for width/height
	 * since this is kinda a memory blob
	 */
	void *tilemap_memory; 
} game_state;

typedef struct {
	i32 fd;
	u64 size;
} level_file;

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
	_assert(expected_game_memory_address == game_memory);
}

enum {
	TILEMAP_ACCESS_BLUE_COUNT,
	TILEMAP_ACCESS_GREEN_COUNT,
	TILEMAP_ACCESS_RED_COUNT,
	TILEMAP_ACCESS_WIDTH,
	TILEMAP_ACCESS_HEIGHT,
	TILEMAP_ACCESS_TILE_PTR,
	TILEMAP_ACCESS_TILE_TYPE,
	TILEMAP_ACCESS_UNIT_TYPE,
	TILEMAP_ACCESS_COUNT
};

static b32 tilemap_access(
		game_state *state, 
		void **result, 
		i32 index, 
		i32 access_type);

static void editor_draw_tilemap(game_state *state);

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
	u64 level_file_size = 0;
	if(!state->initialized)
	{
		b32 init_success = true;

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

		/* TODO: if new file, create a new file, just have width and height be 0
		 */

		level_file_size = get_file_size(level_filename);

		if(level_file_size == 0)
		{
			init_success = false;
		}

		state->tilemap_memory = 
			(void*)game_memory_allocate(
					&used_memory,
					level_file_size,
					game_memory,
					game_memory_size);
		_assert(state->tilemap_memory);

		if(!read_file_into_buffer(
				level_filename, 
				state->tilemap_memory,
				level_file_size))
		{
			init_success = false;
		}

		if(init_success)
		{
			/* NOTE(josh): I mean do we just call this a game state still? */
			LOG_INFO("initialized editor state.");
		}
		else
		{
			LOG_ERROR("failed to initialize editor state.");
			_assert(0);
		}
	}

	/* update */
	PROFILER_START_TIMING_BLOCK(update);
	state->time_elapsed = read_os_timer() - state->last_time;	
	state->timer += state->time_elapsed;
	state->last_time = read_os_timer();

	/* TODO: break all this repeat code into a function PLEASE FOR THE LOVE OF GOD */
	if(input->left_control == INPUT_BUTTON_STATE_DOWN)
	{
		if(input->letters[1] == INPUT_BUTTON_STATE_PRESSED)
		{
			i32 tile_index = -1;	
			i32 x = input->mouse_x;
			i32 y = input->mouse_y;

			x = (x - TILE_SIZE) / TILE_SIZE;
			y = (y - TILE_SIZE) / TILE_SIZE;

			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 tilemap_width  = *((i32*)tilemap_width_ptr);
			i32 tilemap_height = *((i32*)tilemap_height_ptr);

			if(y >= 0 && x >= 0 && x < tilemap_width && y < tilemap_height)
			{
				tile_index = y * tilemap_width + x;
			}

			if(tile_index >= 0)
			{
				LOG_DEBUG("setting tile %d to blue unit", tile_index);
				void *tile_ptr = 0;
				tile *t = 0;
				_assert(tilemap_access(state, &tile_ptr, tile_index, TILEMAP_ACCESS_TILE_PTR));
				t = (tile *)tile_ptr;
				t->unit_type = UNIT_TYPE_BLUE;
			}
			LOG_DEBUG("(%d, %d) tile: %d (%d, %d)", input->mouse_x, input->mouse_y, tile_index, x, y);
			state->tilemap_saved = false;
		}
		if(input->letters[6] == INPUT_BUTTON_STATE_PRESSED)
		{
			i32 tile_index = -1;	
			i32 x = input->mouse_x;
			i32 y = input->mouse_y;

			x = (x - TILE_SIZE) / TILE_SIZE;
			y = (y - TILE_SIZE) / TILE_SIZE;

			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 tilemap_width  = *((i32*)tilemap_width_ptr);
			i32 tilemap_height = *((i32*)tilemap_height_ptr);

			if(y >= 0 && x >= 0 && x < tilemap_width && y < tilemap_height)
			{
				tile_index = y * tilemap_width + x;
			}

			if(tile_index >= 0)
			{
				LOG_DEBUG("setting tile %d to green unit", tile_index);
				void *tile_ptr = 0;
				tile *t = 0;
				_assert(tilemap_access(state, &tile_ptr, tile_index, TILEMAP_ACCESS_TILE_PTR));
				t = (tile *)tile_ptr;
				t->unit_type = UNIT_TYPE_GREEN;
			}
			LOG_DEBUG("(%d, %d) tile: %d (%d, %d)", input->mouse_x, input->mouse_y, tile_index, x, y);
			state->tilemap_saved = false;
		}
		if(input->letters[17] == INPUT_BUTTON_STATE_PRESSED)
		{
			i32 tile_index = -1;	
			i32 x = input->mouse_x;
			i32 y = input->mouse_y;

			x = (x - TILE_SIZE) / TILE_SIZE;
			y = (y - TILE_SIZE) / TILE_SIZE;

			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 tilemap_width  = *((i32*)tilemap_width_ptr);
			i32 tilemap_height = *((i32*)tilemap_height_ptr);

			if(y >= 0 && x >= 0 && x < tilemap_width && y < tilemap_height)
			{
				tile_index = y * tilemap_width + x;
			}

			if(tile_index >= 0)
			{
				LOG_DEBUG("setting tile %d to red unit", tile_index);
				void *tile_ptr = 0;
				tile *t = 0;
				_assert(tilemap_access(state, &tile_ptr, tile_index, TILEMAP_ACCESS_TILE_PTR));
				t = (tile *)tile_ptr;
				t->unit_type = UNIT_TYPE_RED;
			}
			LOG_DEBUG("(%d, %d) tile: %d (%d, %d)", input->mouse_x, input->mouse_y, tile_index, x, y);
			state->tilemap_saved = false;
		}

		if(input->letters[22] == INPUT_BUTTON_STATE_PRESSED)
		{
			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 *tilemap_width  = (i32*)tilemap_width_ptr;
			i32 *tilemap_height = (i32*)tilemap_height_ptr;

			if((*tilemap_width) > 1)
			{
				*(tilemap_width) -= 1;
			}

			/* TODO: move tiles around so that each tile that is still in the map remains the same */

			game_memory_free(&used_memory, level_file_size, game_memory, state->tilemap_memory);
			u64 allocation_size = sizeof(tile) * (*tilemap_width) * (*tilemap_height) + (5 * sizeof(i32));
			state->tilemap_memory = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
		}
		if(input->letters[7] == INPUT_BUTTON_STATE_PRESSED)
		{
			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 *tilemap_width  = (i32*)tilemap_width_ptr;
			i32 *tilemap_height = (i32*)tilemap_height_ptr;

			if((*tilemap_height) > 1)
			{
				*(tilemap_height) -= 1;
			}

			/* TODO: move tiles around so that each tile that is still in the map remains the same */

			game_memory_free(&used_memory, level_file_size, game_memory, state->tilemap_memory);
			u64 allocation_size = sizeof(tile) * (*tilemap_width) * (*tilemap_height) + (5 * sizeof(i32));
			state->tilemap_memory = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
		}
	}
	else if(input->left_shift == INPUT_BUTTON_STATE_DOWN)
	{
		if(input->letters[22] == INPUT_BUTTON_STATE_PRESSED)
		{
			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 *tilemap_width  = (i32*)tilemap_width_ptr;
			i32 *tilemap_height = (i32*)tilemap_height_ptr;

			*(tilemap_width) += 1;

			/* TODO: move tiles around so that each tile that is still in the map remains the same */

			game_memory_free(&used_memory, level_file_size, game_memory, state->tilemap_memory);
			u64 allocation_size = sizeof(tile) * (*tilemap_width) * (*tilemap_height) + (5 * sizeof(i32));
			state->tilemap_memory = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
		}
		if(input->letters[7] == INPUT_BUTTON_STATE_PRESSED)
		{
			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 *tilemap_width  = (i32*)tilemap_width_ptr;
			i32 *tilemap_height = (i32*)tilemap_height_ptr;

			*(tilemap_height) += 1;

			/* TODO: move tiles around so that each tile that is still in the map remains the same */

			game_memory_free(&used_memory, level_file_size, game_memory, state->tilemap_memory);
			u64 allocation_size = sizeof(tile) * (*tilemap_width) * (*tilemap_height) + (5 * sizeof(i32));
			state->tilemap_memory = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
		}
	}
	else
	{
		if(input->letters[1] == INPUT_BUTTON_STATE_PRESSED)
		{
			i32 tile_index = -1;	
			i32 x = input->mouse_x;
			i32 y = input->mouse_y;

			x = (x - TILE_SIZE) / TILE_SIZE;
			y = (y - TILE_SIZE) / TILE_SIZE;

			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 tilemap_width  = *((i32*)tilemap_width_ptr);
			i32 tilemap_height = *((i32*)tilemap_height_ptr);

			if(y >= 0 && x >= 0 && x < tilemap_width && y < tilemap_height)
			{
				tile_index = y * tilemap_width + x;
			}

			if(tile_index >= 0)
			{
				LOG_DEBUG("setting tile %d to blue", tile_index);
				void *tile_ptr = 0;
				tile *t = 0;
				_assert(tilemap_access(state, &tile_ptr, tile_index, TILEMAP_ACCESS_TILE_PTR));
				t = (tile *)tile_ptr;
				t->tile_type = TILE_TYPE_BLUE;
			}
			LOG_DEBUG("(%d, %d) tile: %d (%d, %d)", input->mouse_x, input->mouse_y, tile_index, x, y);
			state->tilemap_saved = false;
		}
		if(input->letters[6] == INPUT_BUTTON_STATE_PRESSED)
		{
			i32 tile_index = -1;	
			i32 x = input->mouse_x;
			i32 y = input->mouse_y;

			x = (x - TILE_SIZE) / TILE_SIZE;
			y = (y - TILE_SIZE) / TILE_SIZE;

			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 tilemap_width  = *((i32*)tilemap_width_ptr);
			i32 tilemap_height = *((i32*)tilemap_height_ptr);

			if(y >= 0 && x >= 0 && x < tilemap_width && y < tilemap_height)
			{
				tile_index = y * tilemap_width + x;
			}

			if(tile_index >= 0)
			{
				LOG_DEBUG("setting tile %d to green", tile_index);
				void *tile_ptr = 0;
				tile *t = 0;
				_assert(tilemap_access(state, &tile_ptr, tile_index, TILEMAP_ACCESS_TILE_PTR));
				t = (tile *)tile_ptr;
				t->tile_type = TILE_TYPE_GREEN;
			}
			LOG_DEBUG("(%d, %d) tile: %d (%d, %d)", input->mouse_x, input->mouse_y, tile_index, x, y);
			state->tilemap_saved = false;
		}
		if(input->letters[17] == INPUT_BUTTON_STATE_PRESSED)
		{
			i32 tile_index = -1;	
			i32 x = input->mouse_x;
			i32 y = input->mouse_y;

			x = (x - TILE_SIZE) / TILE_SIZE;
			y = (y - TILE_SIZE) / TILE_SIZE;

			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 tilemap_width  = *((i32*)tilemap_width_ptr);
			i32 tilemap_height = *((i32*)tilemap_height_ptr);

			if(y >= 0 && x >= 0 && x < tilemap_width && y < tilemap_height)
			{
				tile_index = y * tilemap_width + x;
			}

			if(tile_index >= 0)
			{
				LOG_DEBUG("setting tile %d to red", tile_index);
				void *tile_ptr = 0;
				tile *t = 0;
				_assert(tilemap_access(state, &tile_ptr, tile_index, TILEMAP_ACCESS_TILE_PTR));
				t = (tile *)tile_ptr;
				t->tile_type = TILE_TYPE_RED;
			}
			LOG_DEBUG("(%d, %d) tile: %d (%d, %d)", input->mouse_x, input->mouse_y, tile_index, x, y);
			state->tilemap_saved = false;
		}
		if(input->letters[23] == INPUT_BUTTON_STATE_PRESSED)
		{
			i32 tile_index = -1;	
			i32 x = input->mouse_x;
			i32 y = input->mouse_y;

			x = (x - TILE_SIZE) / TILE_SIZE;
			y = (y - TILE_SIZE) / TILE_SIZE;

			void *tilemap_width_ptr = 0;
			void *tilemap_height_ptr = 0;

			_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
			_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

			_assert(tilemap_width_ptr);
			_assert(tilemap_height_ptr);

			i32 tilemap_width  = *((i32*)tilemap_width_ptr);
			i32 tilemap_height = *((i32*)tilemap_height_ptr);

			if(y >= 0 && x >= 0 && x < tilemap_width && y < tilemap_height)
			{
				tile_index = y * tilemap_width + x;
			}

			if(tile_index >= 0)
			{
				LOG_DEBUG("setting tile %d to none", tile_index);
				void *tile_ptr = 0;
				tile *t = 0;
				_assert(tilemap_access(state, &tile_ptr, tile_index, TILEMAP_ACCESS_TILE_PTR));
				t = (tile *)tile_ptr;
				t->unit_type = UNIT_TYPE_NONE;
			}
			LOG_DEBUG("(%d, %d) tile: %d (%d, %d)", input->mouse_x, input->mouse_y, tile_index, x, y);
			state->tilemap_saved = false;
		}
	}

	/* TODO: increment blue, green, red count with SHIFT + B/G/R and display the values as text */

	if(input->spacebar == INPUT_BUTTON_STATE_PRESSED)
	{
		void *tilemap_width_ptr = 0;
		void *tilemap_height_ptr = 0;

		_assert(tilemap_access(state, &tilemap_width_ptr,  0, TILEMAP_ACCESS_WIDTH));
		_assert(tilemap_access(state, &tilemap_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

		_assert(tilemap_width_ptr);
		_assert(tilemap_height_ptr);

		i32 tilemap_width  = *((i32*)tilemap_width_ptr);
		i32 tilemap_height = *((i32*)tilemap_height_ptr);

		write_buffer_into_file_truncate(level_filename, state->tilemap_memory, tilemap_width * tilemap_height * sizeof(tile) + 5 * sizeof(i32)); 
		state->tilemap_saved = true;
		LOG_INFO("saved tilemap to '%s'", level_filename);
	}

	PROFILER_FINISH_TIMING_BLOCK(update);

	/* render */
	PROFILER_START_TIMING_BLOCK(render);
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		gray); 

	editor_draw_tilemap(state);

	PROFILER_FINISH_TIMING_BLOCK(render);

	if(state->timer > 2000000.0)
	{
		finish_and_print_profile(LOG_TRACE);
		state->timer = 0.0;
	}
}

static b32 tilemap_access(
		game_state *state, 
		void **result, 
		i32 index, 
		i32 access_type)
{
	*result = 0;
	i32 width  = *((i32*)state->tilemap_memory + 3);
	i32 height = *((i32*)state->tilemap_memory + 4);
	if(index >= width * height)
	{
		LOG_ERROR("index is out of bounds of tilemap data (%d)", index);
		return(false);
	}

	switch(access_type)
	{
		case TILEMAP_ACCESS_BLUE_COUNT:
		{
			*result = (state->tilemap_memory);
		} break;
		case TILEMAP_ACCESS_GREEN_COUNT:
		{
			*result = (void*)((u32*)state->tilemap_memory + 1);
		} break;
		case TILEMAP_ACCESS_RED_COUNT:
		{
			*result = (void*)((u32*)state->tilemap_memory + 2);
		} break;
		case TILEMAP_ACCESS_WIDTH:
		{
			*result = (void*)((i32*)state->tilemap_memory + 3);
		} break;
		case TILEMAP_ACCESS_HEIGHT:
		{
			*result = (void*)((i32*)state->tilemap_memory + 4);
		} break;
		case TILEMAP_ACCESS_TILE_PTR:
		{
			*result = (void*)( (tile*) ((i32*)state->tilemap_memory + 5) + index );

		} break;
		case TILEMAP_ACCESS_TILE_TYPE:
		{
			tile *t = (tile*)((i32*)state->tilemap_memory + 5);
			*result = &(t[index].tile_type);
		} break;
		case TILEMAP_ACCESS_UNIT_TYPE:
		{
			tile *t = (tile*)((i32*)state->tilemap_memory + 5);
			*result = &(t[index].unit_type);
		} break;
		default:
		{
			_assert(0);
		} break;
	}

	return(true);
}

static void editor_draw_tilemap(game_state *state)
{
	void *map_width_ptr = 0; 
	void *map_height_ptr = 0; 
	i32 map_width = 0;
	i32 map_height = 0;

	_assert(
		tilemap_access(state, &map_width_ptr, 0, TILEMAP_ACCESS_WIDTH));
	_assert(
		tilemap_access(state, &map_height_ptr, 0, TILEMAP_ACCESS_HEIGHT));

	map_width = *((i32*)map_width_ptr);
	map_height = *((i32*)map_height_ptr);

	i32 x = 0; 
	i32 y = 0; 
	tile *t = 0;
	void *tile_ptr = 0;
	for(x = 0; x < map_width; x++)
	{
		for(y = 0; y < map_height; y++)
		{
			_assert(
				tilemap_access(
					state, 
					&tile_ptr, 
					y * map_width + x,
					TILEMAP_ACCESS_TILE_PTR));
			
			t = (tile*)tile_ptr;

			struct_rgba_color color;
			switch(t->tile_type)
			{
				case TILE_TYPE_BLUE:
				{
					color.r = 0;
					color.g = 0;
					color.b = 255;
					color.a = 0;
				} break;
				case TILE_TYPE_GREEN:
				{
					color.r = 0;
					color.g = 255;
					color.b = 0;
					color.a = 0;
				} break;
				case TILE_TYPE_RED:
				{
					color.r = 255;
					color.g = 0;
					color.b = 0;
					color.a = 0;
				} break;
				default:
				{
					_assert(0);
				} break;
			}
			draw_nofill_rectangle_in_buffer(
				state->pixel_buffer,
				state->pixel_buffer_width,
				state->pixel_buffer_height,
				TILE_SIZE * x + TILE_SIZE, TILE_SIZE * y + TILE_SIZE,
				TILE_SIZE, TILE_SIZE, color);

			color.r = 0;
			color.g = 0;
			color.b = 0;
			color.a = 0;

			switch(t->unit_type)
			{
				case UNIT_TYPE_BLUE:
				{
					color.b = 255;
				} break;
				case UNIT_TYPE_GREEN:
				{
					color.g = 255;
				} break;
				case UNIT_TYPE_RED:
				{
					color.r = 255;
				} break;
				case UNIT_TYPE_NONE:
				{
					/* do nothing */
				} break;
				default:
				{
					_assert(0);
				} break;
			}
			if(t->unit_type != UNIT_TYPE_NONE)
			{
				draw_nofill_rectangle_in_buffer(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					TILE_SIZE * x + (TILE_SIZE + TILE_SIZE / 5), TILE_SIZE * y + (TILE_SIZE + TILE_SIZE / 5),
					(TILE_SIZE / 5) * 3, (TILE_SIZE / 5) * 3, color);
			}
		}
	}
}
