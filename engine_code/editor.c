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
	vector_2 bounds; 
} camera;

#include "cpu_render.c"

#define JSTRING_MEMORY_SIZE 1024

#include "tile_types.h"

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
	b32 tilemap_saved;
	u8 *pixel_buffer;
	u16 pixel_buffer_width;
	u16 pixel_buffer_height;
	u64 tilemap_data_size;
	tilemap *tilemap_data; 
	file level_file;
} game_state;

enum {
	UNIT_COUNTER_BLUE,
	UNIT_COUNTER_GREEN,
	UNIT_COUNTER_RED,
};

static tile *get_tile_from_index(game_state *state, i32 index);

static void editor_change_unit_at_mouse_location(game_state *state, x_input_state *input, i32 unit_type);
static void editor_change_tile_at_mouse_location(game_state *state, x_input_state *input, i32 tile_type);
static void editor_change_tilemap_width(game_state *state, b32 increment);
static void editor_change_tilemap_height(game_state *state, b32 increment);
static void editor_change_tilemap_unit_counter(game_state *state, b32 increment, i32 unit_counter_type);

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
			log_info("IGNORE STAT ERROR FROM get_file_size! Creating new level file '%s'...", state->level_file.filename);
			_assert(create_file_read_write(state->level_file.filename));
			state->tilemap_data = 
				(void*)game_memory_allocate(
						&used_memory,
						sizeof(i32) * 5 + sizeof(tile), /* NOTE(josh): allocate 1 tile worth of memory */
						game_memory,
						game_memory_size);
			_assert(state->tilemap_data);
			state->tilemap_data->width = 1;
			state->tilemap_data->height = 1;
			state->tilemap_data->blue_counter = 1;
			state->tilemap_data->green_counter = 1;
			state->tilemap_data->red_counter = 1;
			state->tilemap_data->tiles.unit_type = UNIT_TYPE_RED;
			state->tilemap_data->tiles.tile_type = TILE_TYPE_GREEN;

			write_buffer_into_file_truncate(
				state->level_file.filename, 
				state->tilemap_data, 
				state->tilemap_data->width * state->tilemap_data->height * sizeof(tile) + 5 * sizeof(i32)); 

			state->tilemap_saved = true;
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

	/* TODO: what keypresses change unit counters ? */
	if(input->left_control == INPUT_BUTTON_STATE_DOWN)
	{
		if(input->letters[1] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_unit_at_mouse_location(state, input, UNIT_TYPE_BLUE);
		}
		if(input->letters[6] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_unit_at_mouse_location(state, input, UNIT_TYPE_GREEN);
		}
		if(input->letters[17] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_unit_at_mouse_location(state, input, UNIT_TYPE_RED);
		}

		if(input->letters[22] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_width(state, true);
			game_memory_free(&used_memory, state->tilemap_data_size, game_memory, state->tilemap_data);
			u64 allocation_size = sizeof(tile) * (state->tilemap_data->width) * (state->tilemap_data->height) + (5 * sizeof(i32));
			state->tilemap_data_size = allocation_size;
			state->tilemap_data = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
		}
		if(input->letters[7] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_height(state, true);
			game_memory_free(&used_memory, state->tilemap_data_size, game_memory, state->tilemap_data);
			u64 allocation_size = sizeof(tile) * (state->tilemap_data->width) * (state->tilemap_data->height) + (5 * sizeof(i32));
			state->tilemap_data_size = allocation_size;
			state->tilemap_data = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
		}
	}
	else if(input->left_shift == INPUT_BUTTON_STATE_DOWN)
	{
		if(input->letters[22] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_width(state, false);
			game_memory_free(&used_memory, state->tilemap_data_size, game_memory, state->tilemap_data);
			u64 allocation_size = sizeof(tile) * (state->tilemap_data->width) * (state->tilemap_data->height) + (5 * sizeof(i32));
			state->tilemap_data_size = allocation_size;
			state->tilemap_data = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
		}
		if(input->letters[7] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_height(state, false);
			game_memory_free(&used_memory, state->tilemap_data_size, game_memory, state->tilemap_data);
			u64 allocation_size = sizeof(tile) * (state->tilemap_data->width) * (state->tilemap_data->height) + (5 * sizeof(i32));
			state->tilemap_data_size = allocation_size;
			state->tilemap_data = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
		}
	}
	else
	{
		if(input->letters[1] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tile_at_mouse_location(state, input, TILE_TYPE_BLUE);
		}
		if(input->letters[6] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tile_at_mouse_location(state, input, TILE_TYPE_GREEN);
		}
		if(input->letters[17] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tile_at_mouse_location(state, input, TILE_TYPE_RED);
		}
		if(input->letters[23] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_unit_at_mouse_location(state, input, UNIT_TYPE_NONE);
		}
	}

	if(input->spacebar == INPUT_BUTTON_STATE_PRESSED)
	{
		write_buffer_into_file_truncate(
			state->level_file.filename, 
			state->tilemap_data, 
			state->tilemap_data->width * state->tilemap_data->height * sizeof(tile) + 5 * sizeof(i32)); 

		state->tilemap_saved = true;
		log_info("saved tilemap to '%s'", state->level_file.filename);
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
	/* TODO: draw unit type counters */
	/* TODO: display 'saved' if most recent changes have been saved and 'recent changes not saved' or smn if not */

	PROFILER_FINISH_TIMING_BLOCK(render);

	if(state->timer > 2000000.0)
	{
		finish_and_print_profile(log_trace);
		state->timer = 0.0;
	}
}

static void editor_draw_tilemap(game_state *state)
{
	i32 map_width = state->tilemap_data->width;
	i32 map_height = state->tilemap_data->height;

	i32 x = 0; 
	i32 y = 0; 
	tile *t = 0;
	for(x = 0; x < map_width; x++)
	{
		for(y = 0; y < map_height; y++)
		{
			i32 tile_index = y * map_width + x;
			t = get_tile_from_index(state, tile_index);

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

static tile *get_tile_from_index(game_state *state, i32 index)
{
	return( (tile*)(&(state->tilemap_data->tiles)) + index);
}

static i32 get_tile_index_from_mouse_coords(game_state *state, x_input_state *input)
{
	i32 x = input->mouse_x;
	i32 y = input->mouse_y;

	i32 tile_x = (x - TILE_SIZE) / TILE_SIZE;
	i32 tile_y = (y - TILE_SIZE) / TILE_SIZE;

	if(tile_x >= state->tilemap_data->width)
	{
		return(-1);
	}
	if(tile_y >= state->tilemap_data->height)
	{
		return(-1);
	}
	if(x < TILE_SIZE)
	{
		return(-1);
	}
	if(y < TILE_SIZE)
	{
		return(-1);
	}

	i32 tile_index = tile_y * state->tilemap_data->width + tile_x;
	return(tile_index);
}

static void editor_change_unit_at_mouse_location(game_state *state, x_input_state *input, i32 unit_type)
{
	i32 tile_index = get_tile_index_from_mouse_coords(state, input);
	if(tile_index != -1)
	{
		tile *t = get_tile_from_index(state, tile_index);
		t->unit_type = unit_type;
		state->tilemap_saved = false;
	}
}

static void editor_change_tile_at_mouse_location(game_state *state, x_input_state *input, i32 tile_type)
{
	i32 tile_index = get_tile_index_from_mouse_coords(state, input);
	if(tile_index != -1)
	{
		tile *t = get_tile_from_index(state, tile_index);
		t->tile_type = tile_type;
		state->tilemap_saved = false;
	}
}

static void editor_change_tilemap_width(game_state *state, b32 increment)
{
	/* TODO: move tiles around so that each tile that is still in the map remains the same */

	if(increment)
	{
		if(state->tilemap_data->width< MAX_TILEMAP_HEIGHT)
		{
			state->tilemap_data->width++;
			state->tilemap_saved = false;
		}
		return;
	}
	if(state->tilemap_data->width> 1)
	{
		state->tilemap_data->width--;
		state->tilemap_saved = false;
	}
}

static void editor_change_tilemap_height(game_state *state, b32 increment)
{
	/* NOTE(josh): no need to rearrange tiles like we do in editor_change_tilemap_width, since this one simply adds
	* or removes the last row */
	/* TODO: ^ even so maybe set the removed tiles to all be blue and empty when height decreased ? */
	if(increment)
	{
		if(state->tilemap_data->height < MAX_TILEMAP_HEIGHT)
		{
			state->tilemap_data->height++;
			state->tilemap_saved = false;
		}
		return;
	}
	if(state->tilemap_data->height > 1)
	{
		state->tilemap_data->height--;
		state->tilemap_saved = false;
	}
}

static void editor_change_tilemap_unit_counter(game_state *state, b32 increment, i32 unit_counter_type)
{
}
