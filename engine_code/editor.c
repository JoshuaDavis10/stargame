#include "game.h"

#include "util.c"
#include "jstring.h"
#include "math.c"

#include "profiler.c"

typedef struct {
	vector_2 position;
	vector_2 bounds; 
} camera;

#include "cpu_render.c"

#define JSTRING_MEMORY_SIZE 1024

#include "tile_types.h"

#define TILEMAP_OFFSET_X 50
#define TILEMAP_OFFSET_Y 50
#define TILE_SIZE_COEFFICIENT 400

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
	i32 tile_size;
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

static i32 max(i32 x, i32 y);

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

			state->tile_size = TILE_SIZE_COEFFICIENT / max_i32(state->tilemap_data->width, state->tilemap_data->height);

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
			state->tile_size = TILE_SIZE_COEFFICIENT / max_i32(state->tilemap_data->width, state->tilemap_data->height);
			state->tilemap_saved = true;
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
			game_memory_free(&used_memory, state->tilemap_data_size, game_memory, state->tilemap_data);
			u64 allocation_size = 
				sizeof(tile) * (state->tilemap_data->width + 1) * (state->tilemap_data->height) + (5 * sizeof(i32));
			state->tilemap_data_size = allocation_size;
			state->tilemap_data = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
			editor_change_tilemap_width(state, true);
		}
		if(input->letters[7] == INPUT_BUTTON_STATE_PRESSED)
		{
			game_memory_free(&used_memory, state->tilemap_data_size, game_memory, state->tilemap_data);
			u64 allocation_size = 
				sizeof(tile) * (state->tilemap_data->width) * (state->tilemap_data->height + 1) + (5 * sizeof(i32));
			state->tilemap_data_size = allocation_size;
			state->tilemap_data = game_memory_allocate(&used_memory, allocation_size, game_memory, game_memory_size);
			editor_change_tilemap_height(state, true);
		}
		if(input->numbers[1] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_unit_counter(state, true, UNIT_COUNTER_BLUE);
		}
		if(input->numbers[2] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_unit_counter(state, true, UNIT_COUNTER_GREEN);
		}
		if(input->numbers[3] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_unit_counter(state, true, UNIT_COUNTER_RED);
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
		if(input->numbers[1] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_unit_counter(state, false, UNIT_COUNTER_BLUE);
		}
		if(input->numbers[2] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_unit_counter(state, false, UNIT_COUNTER_GREEN);
		}
		if(input->numbers[3] == INPUT_BUTTON_STATE_PRESSED)
		{
			editor_change_tilemap_unit_counter(state, false, UNIT_COUNTER_RED);
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

	/* XXX: bro, we really need a jstring_create_formatted() function or smn this is hell */
	jstring blue_counter_text = jstring_create_temporary("BLUE COUNT ", jstring_length("BLUE COUNT "));
	jstring green_counter_text = jstring_create_temporary("GREEN COUNT ", jstring_length("GREEN COUNT "));
	jstring red_counter_text = jstring_create_temporary("RED COUNT ", jstring_length("RED COUNT "));

	jstring blue_counter_number = jstring_create_integer(state->tilemap_data->blue_counter);
	jstring green_counter_number = jstring_create_integer(state->tilemap_data->green_counter);
	jstring red_counter_number = jstring_create_integer(state->tilemap_data->red_counter);

	_assert_log(jstring_concatenate_jstring(&blue_counter_text, blue_counter_number), 
		"failed to concatenate '%s' with '%s'", blue_counter_text.data, blue_counter_number.data);
	_assert_log(jstring_concatenate_jstring(&green_counter_text, green_counter_number), 
		"failed to concatenate '%s' with '%s'", green_counter_text.data, green_counter_number.data);
	_assert_log(jstring_concatenate_jstring(&red_counter_text, red_counter_number), 
		"failed to concatenate '%s' with '%s'", red_counter_text.data, red_counter_number.data);

	PROFILER_FINISH_TIMING_BLOCK(update);

	/* render */
	PROFILER_START_TIMING_BLOCK(render);
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		gray); 

	editor_draw_tilemap(state);

	/* TODO: hardcoded numbers for position. prob readjust this/create macros, after you've made tile width's adjust to
	 * # of tiles */
	draw_text_in_buffer(
		state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 
		state->pixel_buffer_width - 120, 20, 1, blue_counter_text, white);
	draw_text_in_buffer(
		state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 
		state->pixel_buffer_width - 120, 40, 1, green_counter_text, white);
	draw_text_in_buffer(
		state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 
		state->pixel_buffer_width - 120, 60, 1, red_counter_text, white);

	jstring saved_string = jstring_create_temporary("ALL CHANGES SAVED", jstring_length("ALL CHANGES SAVED"));
	jstring not_saved_string = 
		jstring_create_temporary(
			"NOT ALL CHANGES HAVE BEEN SAVED", 
			jstring_length("NOT ALL CHANGES HAVE BEEN SAVED"));

	if(state->tilemap_saved)
	{
		draw_text_in_buffer(
			state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 
			20, state->pixel_buffer_height - 20, 1, saved_string, white);
	}
	else
	{
		draw_text_in_buffer(
			state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 
			20, state->pixel_buffer_height - 20, 1, not_saved_string, white);
	}

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

			rgba_color color;
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
				state->tile_size * x + TILEMAP_OFFSET_X, state->tile_size * y + TILEMAP_OFFSET_Y,
				state->tile_size, state->tile_size, color);

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
					state->tile_size * x + (TILEMAP_OFFSET_X + state->tile_size / 5), state->tile_size * y + 
						(TILEMAP_OFFSET_Y + state->tile_size / 5),
					(state->tile_size / 5) * 3, (state->tile_size / 5) * 3, color);
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

	i32 tile_x = (x - TILEMAP_OFFSET_X) / state->tile_size;
	i32 tile_y = (y - TILEMAP_OFFSET_Y) / state->tile_size;

	if(tile_x >= state->tilemap_data->width)
	{
		return(-1);
	}
	if(tile_y >= state->tilemap_data->height)
	{
		return(-1);
	}
	if(x < TILEMAP_OFFSET_X)
	{
		return(-1);
	}
	if(y < TILEMAP_OFFSET_Y)
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
	if(increment)
	{
		if(state->tilemap_data->width< MAX_TILEMAP_HEIGHT)
		{
			state->tilemap_data->width++;
			state->tilemap_saved = false;

			i32 old_width = state->tilemap_data->width - 1;

			i32 tile_index = state->tilemap_data->width * state->tilemap_data->height - 1;
			for( ; tile_index >= 0; tile_index--)
			{
				i32 shift_amount = tile_index / old_width;
				tile *t = get_tile_from_index(state, tile_index);
				tile *t_shifted = get_tile_from_index(state, tile_index + shift_amount);
				t_shifted->unit_type = t->unit_type;
				t_shifted->tile_type = t->tile_type;
				if(shift_amount != 0)
				{
					t->tile_type = TILE_TYPE_BLUE;
					t->unit_type = UNIT_TYPE_NONE;
				}
			}
		}
		state->tile_size = TILE_SIZE_COEFFICIENT / max_i32(state->tilemap_data->width, state->tilemap_data->height);
		return;
	}
	if(state->tilemap_data->width> 1)
	{
		state->tilemap_data->width--;
		state->tilemap_saved = false;

		i32 old_width = state->tilemap_data->width + 1;

		i32 tile_index = 0;
		for( ; tile_index < old_width * state->tilemap_data->height; tile_index++)
		{
			i32 shift_amount = tile_index / old_width;
			log_debug("tile: %d, shift amount: %d", tile_index, shift_amount);
			tile *t = get_tile_from_index(state, tile_index);
			tile *t_shifted = get_tile_from_index(state, tile_index - shift_amount);
			t_shifted->unit_type = t->unit_type;
			t_shifted->tile_type = t->tile_type;
			if(shift_amount != 0)
			{
				t->tile_type = TILE_TYPE_BLUE;
				t->unit_type = UNIT_TYPE_NONE;
			}
		}
	}
	state->tile_size = TILE_SIZE_COEFFICIENT / max_i32(state->tilemap_data->width, state->tilemap_data->height);
}

static void editor_change_tilemap_height(game_state *state, b32 increment)
{
	if(increment)
	{
		if(state->tilemap_data->height < MAX_TILEMAP_HEIGHT)
		{
			state->tilemap_data->height++;
			state->tilemap_saved = false;
			
			/* first tile of last row */
			i32 tile_index = state->tilemap_data->width * (state->tilemap_data->height - 1);
			for( ; tile_index < state->tilemap_data->width * state->tilemap_data->height; tile_index++)
			{
				tile *t = get_tile_from_index(state, tile_index);
				t->unit_type = UNIT_TYPE_NONE;
				t->tile_type = TILE_TYPE_BLUE;
			}
		}
		state->tile_size = TILE_SIZE_COEFFICIENT / max_i32(state->tilemap_data->width, state->tilemap_data->height);
		return;
	}
	if(state->tilemap_data->height > 1)
	{
		state->tilemap_data->height--;
		state->tilemap_saved = false;
	}
	state->tile_size = TILE_SIZE_COEFFICIENT / max_i32(state->tilemap_data->width, state->tilemap_data->height);
}

static void editor_change_tilemap_unit_counter(game_state *state, b32 increment, i32 unit_counter_type)
{
	if(increment)
	{
		switch(unit_counter_type)
		{
			case UNIT_COUNTER_BLUE:
			{
				state->tilemap_data->blue_counter++;
				state->tilemap_saved = false;
				return;
			} break;
			case UNIT_COUNTER_GREEN:
			{
				state->tilemap_data->green_counter++;
				state->tilemap_saved = false;
				return;
			} break;
			case UNIT_COUNTER_RED:
			{
				state->tilemap_data->red_counter++;
				state->tilemap_saved = false;
				return;
			} break;
			default:
			{
				_assert_log(0, "unexpected unit_counter_type: %d", unit_counter_type);
			} break;
		}
	}
	switch(unit_counter_type)
	{
		case UNIT_COUNTER_BLUE:
		{
			if(state->tilemap_data->blue_counter > 0)
			{
				state->tilemap_data->blue_counter--;
				state->tilemap_saved = false;
			}
			return;
		} break;
		case UNIT_COUNTER_GREEN:
		{
			if(state->tilemap_data->green_counter > 0)
			{
				state->tilemap_data->green_counter--;
				state->tilemap_saved = false;
			}
			return;
		} break;
		case UNIT_COUNTER_RED:
		{
			if(state->tilemap_data->red_counter > 0)
			{
				state->tilemap_data->red_counter--;
				state->tilemap_saved = false;
			}
			return;
		} break;
		default:
		{
			_assert_log(0, "unexpected unit_counter_type: %d", unit_counter_type);
		} break;
	}
}
