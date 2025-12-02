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

#include "tile_types.h"

typedef struct {
	camera game_camera; /* NOTE: world space coords */
	u64 last_time;
	u64 time_elapsed;
	f64 timer;
	b32 initialized;
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

		u64 level_file_size = get_file_size(level_filename);

		if(level_file_size == 0)
		{
			init_success = false;
		}

		/* TODO: if new file, open a blank editor, with options to
		 * set width/height 
		 */

		state->tilemap_memory = 
			(void*)game_memory_allocate(
					&used_memory,
					level_file_size,
					game_memory,
					game_memory_size);
		_assert(state->tilemap_memory);
		LOG_DEBUG("tilemap mem: %p", state->tilemap_memory);

		if(!read_file_into_buffer(
				level_filename, 
				state->tilemap_memory,
				level_file_size))
		{
			init_success = false;
		}

		if(init_success)
		{
			LOG_INFO("initialized editor state."); /* NOTE(josh): I mean do we just call this a game state still? */
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

	/* TODO: edit tilemap based on input */
	/* TODO: some button to save to file, or maybe just a key combo */

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
			LOG_DEBUG("result: %p", result);
			LOG_TRACE("tilemap_access: no crash");
			*result = (void*)((i32*)state->tilemap_memory + 3);
		} break;
		case TILEMAP_ACCESS_HEIGHT:
		{
			LOG_TRACE("tilemap_access: no crash");
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

	LOG_TRACE("width: %d, height: %d", map_width, map_height);

	
	i32 x = 0; 
	i32 y = 0; 
	tile *t = 0;
	void *tile_ptr = 0;
	for(x = 0; x < map_width; x++)
	{
		for(y = 0; y < map_height; y++)
		{
			LOG_DEBUG("index: %d", y * map_width + x);
			_assert(
				tilemap_access(
					state, 
					&tile_ptr, 
					y * map_width + x,
					TILEMAP_ACCESS_TILE_PTR));
			
			t = (tile*)tile_ptr;

			struct_rgba_color color;
			LOG_DEBUG("tile type: %d", t->tile_type);
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
			LOG_DEBUG("x: %d, y: %d", x, y);
			draw_nofill_rectangle_in_buffer(
				state->pixel_buffer,
				state->pixel_buffer_width,
				state->pixel_buffer_height,
				100 * x + 100, 100 * y + 100,
				100, 100, color);

			switch(t->unit_type)
			{
				case UNIT_TYPE_BLUE:
				{
					draw_nofill_rectangle_in_buffer(
						state->pixel_buffer,
						state->pixel_buffer_width,
						state->pixel_buffer_height,
						100 * x + 120, 100 * y + 120,
						60, 60, blue);
				} break;
				case UNIT_TYPE_GREEN:
				{
					draw_nofill_rectangle_in_buffer(
						state->pixel_buffer,
						state->pixel_buffer_width,
						state->pixel_buffer_height,
						100 * x + 120, 100 * y + 120,
						60, 60, green);
				} break;
				case UNIT_TYPE_RED:
				{
					draw_nofill_rectangle_in_buffer(
						state->pixel_buffer,
						state->pixel_buffer_width,
						state->pixel_buffer_height,
						100 * x + 120, 100 * y + 120,
						60, 60, red);
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
		}
	}
}
