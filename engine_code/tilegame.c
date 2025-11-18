#include "game.h"

#include "linux_util.c"
#include "jstring.h"
#include "math.c"

#include "profiler.c"

#define MAX_VERTICES 512
#define JSTRING_MEMORY_SIZE 2048

#define TILEMAP_WIDTH 8
#define TILEMAP_HEIGHT 8 

#define FONTSIZE 2 

static const char *unit_type_red_label = "RED";
static const char *unit_type_green_label = "GREEN";
static const char *unit_type_blue_label = "BLUE";
static const char *tile_type_basic_label = "BASIC";
static const char *tile_type_red_label = "RED";
static const char *tile_type_green_label = "GREEN";

enum {
	TILE_TYPE_BASIC,
	TILE_TYPE_RED,
	TILE_TYPE_GREEN,
	TILE_TYPE_COUNT
};

enum {
	UNIT_TYPE_NONE,
	UNIT_TYPE_RED,
	UNIT_TYPE_GREEN,
	UNIT_TYPE_BLUE,
	UNIT_TYPE_COUNT
};

typedef struct {
	i32 tile_type;
	i32 unit_type;
} tile;

enum {
	TARGET_TYPE_ADJACENT_STRAIGHT,
	TARGET_TYPE_ADJACENT_DIAGONAL,
	TARGET_TYPE_ADJACENT_ALL,
	TARGET_TYPE_ROW,
	TARGET_TYPE_COLUMN,
	TARGET_TYPE_DIAGONAL,
	TARGET_TYPE_NONE,
	TARGET_TYPE_COUNT
};

enum {
	STATE_PLAYING,
	STATE_WON,
	STATE_COUNT
};

typedef struct {
	i32 target_type;
	i32 tile_type_shift_targets;
	i32 unit_type_shift_targets;
} tile_behavior;

/* TODO(josh): this basically describes 
 * the steps of a move so that the game can
 * render each step at various timesteps
 * after the move has been made
 * this should get generated immediately when
 * the unit is placed, be stored in game state
 * and then the game is in like "step" mode or smn
 * and an if statement checks for that, and does the
 * next step
 */
enum {
	MOVE_STEP_PLACE_UNIT,
	MOVE_STEP_PLACE_CHANGE_TILE,
	MOVE_STEP_PLACE_CHANGE_TARGET,
	MOVE_STEP_PLACE_CHANGE_TARGET_UNIT,
	MOVE_STEP_COUNT
};
typedef struct {
	i32 current_step; /* NOTE(josh): so we know which step of the process we are on */
	i32 placed_unit_type;
	i32 placed_tile_type;
	i32 placed_tile_index;
	/* NOTE(josh): ^ that should be all the necessary info to carry out the move */
} move_steps;

/* TODO: this should be a table of some sort so it can be indexed by tile_type enum */
static const tile_behavior tile_type_basic_behavior = {TARGET_TYPE_ADJACENT_STRAIGHT, TILE_TYPE_BASIC, UNIT_TYPE_RED};
static const tile_behavior tile_type_red_behavior = {TARGET_TYPE_ADJACENT_STRAIGHT, TILE_TYPE_RED, UNIT_TYPE_BLUE};
static const tile_behavior tile_type_green_behavior = {TARGET_TYPE_ADJACENT_STRAIGHT, TILE_TYPE_RED, UNIT_TYPE_NONE};

typedef enum {
	MESH_TYPE_TILE_BASIC,
	MESH_TYPE_TILE_RED,
	MESH_TYPE_TILE_GREEN,
	MESH_TYPE_UNIT_RED,
	MESH_TYPE_UNIT_GREEN,
	MESH_TYPE_UNIT_BLUE,
	MESH_TYPE_COUNT
} mesh_type;

typedef struct {
	vector_2 position;
	/* NOTE: -x and +x maximums + -y and +y maximums for camera coords 
	 * that will actually be drawn 
	 */
	vector_2 bounds; 
} camera;

#include "cpu_render.c"

/* TODO: rotate mesh function */
#include <math.h> /* cosf, sinf, sqrt TODO write your own versions */

typedef struct {
	void *jstring_memory;
	render_mesh *mesh_data;
	vector_2 *vertex_position_data;
	vector_4 *vertex_color_data;
	u32 vertex_count;
	tile *tiles;
	f32 tile_stride;
	f32 tilemap_offset_x;
	f32 tilemap_offset_y;
	u32 blue_count;
	u32 red_count;
	u32 green_count;
} game_memory;

typedef struct {
	camera game_camera; /* NOTE: world space coords */
	u64 last_time;
	u64 time_elapsed;
	f64 timer;
	b32 initialized;
	game_memory memory;
	u8 *pixel_buffer;
	u16 pixel_buffer_width;
	u16 pixel_buffer_height;
	i32 state;
} game_state;

static void *game_memory_allocate(u64 *used_memory, u64 size, void *game_memory, u64 game_memory_size)
{
	_assert((*used_memory) + size <= game_memory_size);
	void *result = (void*) ((char*)game_memory + (*used_memory));
	(*used_memory) += size;
	return result;
}

static void screen_to_world(i32 x_in, i32 y_in, f32 *x_out, f32 *y_out, game_state *state)
{
	(*x_out) = ((f32)x_in * state->game_camera.bounds.x / (f32)state->pixel_buffer_width) -
			   (state->game_camera.bounds.x/2.0f) - state->game_camera.position.x;
	(*y_out) = ((f32)y_in * state->game_camera.bounds.y / (f32)state->pixel_buffer_height) -
			   (state->game_camera.bounds.y/2.0f) - state->game_camera.position.y;
}

static void world_to_screen(f32 x_in, f32 y_in, i32 *x_out, i32 *y_out, game_state *state)
{
	x_in -= state->game_camera.position.x;
	y_in -= state->game_camera.position.y;

	(*x_out) = (i32)((x_in * (state->pixel_buffer_width/state->game_camera.bounds.x)) + (state->pixel_buffer_width/2.0f));
	(*y_out) = (i32)((y_in * (state->pixel_buffer_height/state->game_camera.bounds.y)) + (state->pixel_buffer_height/2.0f));
}

static b32 game_initialize_meshes(game_state *state);

static b32 game_initialize_tilemap(game_state *state)
{
	/* TODO: read from a file */
	u32 index = 0;
	for( ; index < (TILEMAP_WIDTH * TILEMAP_HEIGHT); index++)
	{
		state->memory.tiles[index].tile_type = TILE_TYPE_BASIC;
		state->memory.tiles[index].unit_type = UNIT_TYPE_NONE;
	}
	state->memory.tiles[3].tile_type = TILE_TYPE_RED;
	state->memory.tiles[3].unit_type = UNIT_TYPE_BLUE;
	state->memory.tiles[2].tile_type = TILE_TYPE_GREEN;
	state->memory.tiles[4].tile_type = TILE_TYPE_GREEN;
	state->memory.tiles[20].tile_type = TILE_TYPE_GREEN;
	state->memory.tiles[30].tile_type = TILE_TYPE_RED;
	state->memory.tiles[30].unit_type = UNIT_TYPE_BLUE;
	state->memory.tiles[22].tile_type = TILE_TYPE_GREEN;
	state->memory.tiles[38].tile_type = TILE_TYPE_GREEN;

	state->memory.tile_stride = 1.0f;
	state->memory.tilemap_offset_x = -3.5f;
	state->memory.tilemap_offset_y = -3.5f;

	state->memory.blue_count = 5;
	state->memory.green_count = 2;
	state->memory.red_count = 0;

	return(true);
}

static void game_draw_mesh(vector_2 position, mesh_type type, game_state *state)
{
	draw_mesh(state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 
		   state->memory.mesh_data[type], position, state->game_camera);
}

static void game_draw_tilemap(game_state *state)
{
	u32 index = 0;
	vector_2 pos;
	u32 x;
	u32 y;
	for( ; index < (TILEMAP_WIDTH * TILEMAP_HEIGHT); index++)
	{
		x = index % TILEMAP_WIDTH;
		y = index / TILEMAP_HEIGHT;

		pos.x = (x * state->memory.tile_stride) + state->memory.tilemap_offset_x;
		pos.y = (y * state->memory.tile_stride) + state->memory.tilemap_offset_y;

		switch(state->memory.tiles[index].tile_type)
		{
			case TILE_TYPE_BASIC:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_BASIC, state);

				if(state->memory.tiles[index].unit_type == UNIT_TYPE_NONE)
				{
				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary(tile_type_basic_label, jstring_length(tile_type_basic_label));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					FONTSIZE, text, black);
				}
			} break;
			case TILE_TYPE_RED:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_RED, state);

				if(state->memory.tiles[index].unit_type == UNIT_TYPE_NONE)
				{
				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary(tile_type_red_label, jstring_length(tile_type_red_label));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					FONTSIZE, text, black);
				}
			} break;
			case TILE_TYPE_GREEN:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_GREEN, state);

				if(state->memory.tiles[index].unit_type == UNIT_TYPE_NONE)
				{
				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary(tile_type_green_label, jstring_length(tile_type_green_label));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					FONTSIZE, text, black);
				}
			} break;
			default:
			{
				_assert(0);
			} break;
		}
		switch(state->memory.tiles[index].unit_type)
		{
			case UNIT_TYPE_RED:
			{
				game_draw_mesh(pos, MESH_TYPE_UNIT_RED, state);
				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary(unit_type_red_label, jstring_length(unit_type_red_label));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					FONTSIZE, text, black);
			} break;
			case UNIT_TYPE_BLUE:
			{
				game_draw_mesh(pos, MESH_TYPE_UNIT_BLUE, state);
				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary(unit_type_blue_label, jstring_length(unit_type_blue_label));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					FONTSIZE, text, white);
			} break;
			case UNIT_TYPE_GREEN:
			{
				game_draw_mesh(pos, MESH_TYPE_UNIT_GREEN, state);
				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary(unit_type_green_label, jstring_length(unit_type_green_label));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					FONTSIZE, text, black);
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

static i32 tile_from_world_coords(f32 x, f32 y, game_state *state)
{
	x -= state->memory.tilemap_offset_x;
	y -= state->memory.tilemap_offset_y;
	x += state->memory.tile_stride / 2.0f; /* NOTE(josh): accounts for the fact that our tiles are not drawn with origin at top left
											* of square, but rather at center of square
											*/
	y += state->memory.tile_stride / 2.0f;

	if(x < 0 || y < 0)
	{
		return -1;
	}

	i32 tile_x = (i32)(x / state->memory.tile_stride);
	i32 tile_y = (i32)(y / state->memory.tile_stride);

	if(tile_x >= TILEMAP_WIDTH || tile_y >= TILEMAP_HEIGHT)
	{
		/* world coords do not match a tile */
		return -1;
	}
	return(tile_y * TILEMAP_WIDTH + tile_x);
}

static void game_update_tilemap(i32 tile_index, i32 unit_type, game_state *state);
static void game_update_tilemap_from_tile_behavior(i32 tile_index, tile_behavior behavior, game_state *state);

void game_update_and_render(
		void *game_memory,
		u64 game_memory_size,
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		x_input_state *input) 
{
	start_profile();

	PROFILER_START_TIMING_BLOCK(memory_stuff);
	/* memory stuff */	
	u64 used_memory = 0;

		/* game state */
	game_state *state = (game_state*)game_memory_allocate(&used_memory, sizeof(game_state), game_memory, game_memory_size); 

		/* set up jstring stuff */
	state->memory.jstring_memory = game_memory_allocate(&used_memory, JSTRING_MEMORY_SIZE, game_memory, game_memory_size);
	if(!jstring_load_logging_function(LOG_LIB))
	{
		_assert(0);
	}

	if(!jstring_memory_activate(JSTRING_MEMORY_SIZE, state->memory.jstring_memory))
	{
		_assert(0);
	}
		/* mesh data */
	state->memory.mesh_data = 
		(render_mesh*)game_memory_allocate(&used_memory, sizeof(render_mesh) * MESH_TYPE_COUNT, game_memory, game_memory_size);
	state->memory.vertex_position_data = 
		(vector_2*)game_memory_allocate(&used_memory, sizeof(vector_2) * MAX_VERTICES, game_memory, game_memory_size); 
	state->memory.vertex_color_data = 
		(vector_4*)game_memory_allocate(&used_memory, sizeof(vector_4) * MAX_VERTICES, game_memory, game_memory_size); 

	if(state->state == STATE_WON)
	{
		draw_background_in_buffer_asm(pixel_buffer, pixel_buffer_width, pixel_buffer_height, gray);	
		vector_2 pos_blue = {7.0f, -3.0f};
		vector_2 pos_green = {-4.0f, 2.0f};
		vector_2 pos_red = {3.0f, 3.0f};
		game_draw_mesh(pos_red, MESH_TYPE_UNIT_RED, state);
		game_draw_mesh(pos_green, MESH_TYPE_UNIT_GREEN, state);
		game_draw_mesh(pos_blue, MESH_TYPE_UNIT_BLUE, state);
		jstring win_string = jstring_create_temporary("YOU WIN", jstring_length("YOU WIN"));
		jstring instruction_string = jstring_create_temporary("PRESS SPACEBAR", jstring_length("PRESS SPACEBAR"));
		jstring instruction_string2 = jstring_create_temporary("TO RESTART", jstring_length("TO RESTART"));
		draw_text_in_buffer_centered(
			pixel_buffer,
			pixel_buffer_width,
			pixel_buffer_height,
			pixel_buffer_width/2, pixel_buffer_height/2,
			8, win_string, white);
		draw_text_in_buffer_centered(
			pixel_buffer,
			pixel_buffer_width,
			pixel_buffer_height,
			pixel_buffer_width/2, pixel_buffer_height/2 + 8 * 10,
			8, instruction_string, white);
		draw_text_in_buffer_centered(
			pixel_buffer,
			pixel_buffer_width,
			pixel_buffer_height,
			pixel_buffer_width/2, pixel_buffer_height/2 + 16 * 10,
			8, instruction_string2, white);

		if(input->spacebar == INPUT_BUTTON_STATE_PRESSED)
		{
			state->state = STATE_PLAYING;
			game_initialize_tilemap(state);
		}
	
		return;
	}

		/* tilemap data */
	state->memory.tiles = 
		(tile*)game_memory_allocate(&used_memory, (sizeof(tile) * TILEMAP_WIDTH * TILEMAP_HEIGHT), game_memory, game_memory_size);

	PROFILER_FINISH_TIMING_BLOCK(memory_stuff);

	/* initialize */
	/* NOTE: game code expects platform layer to
	 * initially hand it zeroed out memory 
	 */
	if(!state->initialized)
	{
		LOG_DEBUG("game memory addr: %p", game_memory);
		LOG_DEBUG("->game state mem: %p", state);
		LOG_DEBUG("->jstring mem:    %p", state->memory.jstring_memory);
		LOG_DEBUG("->mesh data mem:  %p", state->memory.mesh_data);
		LOG_DEBUG("->position mem:   %p", state->memory.vertex_position_data);
		LOG_DEBUG("->color mem:      %p", state->memory.vertex_color_data);
		LOG_DEBUG("game memory size: %u", game_memory_size);
		LOG_DEBUG("used memory     : %u", used_memory);

		/* state */
		state->initialized = true;

			/* pixel buffer stuff */
		state->pixel_buffer = pixel_buffer;
		state->pixel_buffer_width = pixel_buffer_width;
		state->pixel_buffer_height = pixel_buffer_height;

			/* time stuff */
		state->last_time = read_os_timer();
		state->timer = 0.0;

			/* camera */
		state->game_camera.position.x = 0.0f;
		state->game_camera.position.y = 0.0f;
		state->game_camera.bounds.x = 16.0f;
		state->game_camera.bounds.y = 9.0f;


		b32 init_success = true;

		if(!game_initialize_meshes(state))
		{
			LOG_ERROR("failed to initialize game state! mesh data initialization failed.");
			init_success = false;
		}

		if(!game_initialize_tilemap(state))
		{
			LOG_ERROR("failed to initialize game state! tilemap initialization failed.");
			init_success = false;
		}
		if(init_success)
		{
			LOG_INFO("initialized game state.");
		}
	}

	/* update */
	PROFILER_START_TIMING_BLOCK(update);
	state->time_elapsed = read_os_timer() - state->last_time;	
	state->timer += state->time_elapsed;
	state->last_time = read_os_timer();

	const char *str = "UNITS AVAILABLE TO PLACE";
	const char *str2 = "ELIMINATE ALL RED TILES AND UNITS";
	jstring ui_string = 
		jstring_create_temporary(str, jstring_length(str));
	jstring ui_string2 = 
		jstring_create_temporary(str2, jstring_length(str2));

	jstring blue_count_string = 
		jstring_create_temporary("BLUE ", jstring_length("BLUE "));
	jstring green_count_string = 
		jstring_create_temporary("GREEN ", jstring_length("GREEN "));
	jstring red_count_string = 
		jstring_create_temporary("RED ", jstring_length("RED "));

	jstring blue_count = jstring_create_integer(state->memory.blue_count);
	jstring green_count = jstring_create_integer(state->memory.green_count);
	jstring red_count = jstring_create_integer(state->memory.red_count);

	_assert(jstring_concatenate_jstring(&blue_count_string, blue_count));
	_assert(jstring_concatenate_jstring(&green_count_string, green_count));
	_assert(jstring_concatenate_jstring(&red_count_string, red_count));

	/* TODO: when we click, how to determine which unit to put on the tile ? */
	if(input->letters[1] == INPUT_BUTTON_STATE_PRESSED)
	{
		i32 mouse_x = input->mouse_x;
		i32 mouse_y = input->mouse_y;
		f32 world_x;
		f32 world_y;
		screen_to_world(mouse_x, mouse_y, &world_x, &world_y, state);

		i32 tile_index = tile_from_world_coords(world_x, world_y, state);
		if(tile_index != -1)
		{
			if(state->memory.blue_count > 0)
			{
				if(state->memory.tiles[tile_index].unit_type == UNIT_TYPE_NONE)
				{
					state->memory.blue_count--;
				}
				game_update_tilemap(tile_index, UNIT_TYPE_BLUE, state);
			}
		}
	}
	else if(input->letters[6] == INPUT_BUTTON_STATE_PRESSED)
	{
		i32 mouse_x = input->mouse_x;
		i32 mouse_y = input->mouse_y;
		f32 world_x;
		f32 world_y;
		screen_to_world(mouse_x, mouse_y, &world_x, &world_y, state);

		i32 tile_index = tile_from_world_coords(world_x, world_y, state);
		if(tile_index != -1)
		{
			if(state->memory.green_count > 0)
			{
				if(state->memory.tiles[tile_index].unit_type == UNIT_TYPE_NONE)
				{
					state->memory.green_count--;
				}
				game_update_tilemap(tile_index, UNIT_TYPE_GREEN, state);
			}
		}
	}
	else if(input->letters[17] == INPUT_BUTTON_STATE_PRESSED)
	{
		i32 mouse_x = input->mouse_x;
		i32 mouse_y = input->mouse_y;
		f32 world_x;
		f32 world_y;
		screen_to_world(mouse_x, mouse_y, &world_x, &world_y, state);

		i32 tile_index = tile_from_world_coords(world_x, world_y, state);
		if(tile_index != -1)
		{
			if(state->memory.red_count > 0)
			{
				if(state->memory.tiles[tile_index].unit_type == UNIT_TYPE_NONE)
				{
					state->memory.red_count--;
				}
				game_update_tilemap(tile_index, UNIT_TYPE_RED, state);
			}
		}
	}
	else if(input->spacebar == INPUT_BUTTON_STATE_PRESSED)
	{
		game_initialize_tilemap(state);
	}

	PROFILER_FINISH_TIMING_BLOCK(update);

	/* render */
	PROFILER_START_TIMING_BLOCK(render);

	draw_background_in_buffer_asm(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		gray); 

	game_draw_tilemap(state);

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 40, 
		2,
		ui_string,
		white);
	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 10, 
		2,
		ui_string2,
		white);

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 70, 
		2,
		blue_count_string,
		cyan);

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 100, 
		2,
		green_count_string,
		yellow);

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 130, 
		2,
		red_count_string,
		orange);

	PROFILER_FINISH_TIMING_BLOCK(render);
	if(state->timer > 2000000.0)
	{
		/* TODO: read cpu_frequency at startup so that this doesn't take 100ms */
		finish_and_print_profile(LOG_TRACE);
		state->timer = 0.0;
	}
}

static b32 game_initialize_meshes(game_state *state)
{
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions = state->memory.vertex_position_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	/* TODO: index-based rendering, then only need 4 vertices */
	/* TODO: read meshes from files */
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].vertex_count = 6;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[0].x = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[0].y = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[1].x =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[1].y = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[2].x = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[2].y =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[3].x = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[3].y =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[4].x =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[4].y = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[5].x =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions[5].y =-0.5f;

	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].colors[0] = white4;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].colors[1] = white4;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].colors[2] = white4;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].colors[3] = white4;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].colors[4] = white4;
	state->memory.mesh_data[MESH_TYPE_TILE_BASIC].colors[5] = black4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_BASIC].vertex_count;

	state->memory.mesh_data[MESH_TYPE_TILE_RED].positions = 
		state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[0] = red4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[1] = red4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[2] = red4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[3] = red4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[4] = red4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[5] = black4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_RED].vertex_count;

	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].positions = 
		state->memory.mesh_data[MESH_TYPE_TILE_BASIC].positions;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[0] = green4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[1] = green4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[2] = green4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[3] = green4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[4] = green4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[5] = black4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_GREEN].vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions = state->memory.vertex_position_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].vertex_count = 6;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[0].x = 0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[0].y = 0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[1].x =-0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[1].y = 0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[2].x = 0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[2].y =-0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[3].x = 0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[3].y =-0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[4].x =-0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[4].y = 0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[5].x =-0.4f;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[5].y =-0.4f;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[0] = white4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[1] = red4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[2] = red4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[3] = red4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[4] = red4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[5] = red4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_UNIT_RED].vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].positions = 
		state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[0] = white4;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[1] = blue4;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[2] = blue4;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[3] = blue4;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[4] = blue4;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[5] = blue4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].positions = 
		state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[0] = white4;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[1] = green4;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[2] = green4;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[3] = green4;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[4] = green4;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[5] = green4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].vertex_count;

	LOG_TRACE("game initialize meshes finished (vertex count: %u)", state->memory.vertex_count);
	_assert(state->memory.vertex_count <= MAX_VERTICES);
	return(true);
}

/* NOTE(josh): this should recursively call itself in cases where a unit/tile change should have compounding effects 
 * otherwise the game is prob not interesting at all if the affect only applies directly from the tile passed in here
 */
/* NOTE(josh):
 * we want this update logic to be as flexible as possible and based on the _behvaior of the tiles/units that we specify
 * when deciding rules (i.e. don't hardcode the logic have the logic be flexible to changes in _behavior of tiles/units)
 */
/* TODO(josh): at some point, we want to have some way to progress this function by 1 step per frame so that the results are like
 * animated, if that makes sense 
 */
static void game_update_tilemap(i32 tile_index, i32 unit_type, game_state *state)
{
	tile *t = &(state->memory.tiles[tile_index]);
	if(t->unit_type != UNIT_TYPE_NONE)
	{
		return;
	}
	t->unit_type = unit_type;


	/* placed unit affects tile */
	switch(t->unit_type)
	{
		case UNIT_TYPE_NONE:
		{
			/* do nothing */
		} break;
		case UNIT_TYPE_RED:
		{
			if(t->tile_type == TILE_TYPE_RED)
			{
				return;
			}
			t->tile_type = TILE_TYPE_RED;
		} break;
		case UNIT_TYPE_BLUE:
		{
			if(t->tile_type == TILE_TYPE_BASIC)
			{
				return;
			}
			t->tile_type = TILE_TYPE_BASIC;
		} break;
		case UNIT_TYPE_GREEN:
		{

			if(t->tile_type == TILE_TYPE_GREEN)
			{
				return;
			}
			t->tile_type = TILE_TYPE_GREEN;
		} break;
		default:
		{
			_assert(0);
		} break;
	}

	/* tile affects target tiles */
	tile_behavior behavior;
	switch(t->tile_type)
	{
		case TILE_TYPE_BASIC:
		{
			/* do nothing */
			behavior = tile_type_basic_behavior;
		} break;
		case TILE_TYPE_RED:
		{
			behavior = tile_type_red_behavior;
		} break;
		case TILE_TYPE_GREEN:
		{
			behavior = tile_type_green_behavior;
		} break;
		default:
		{
			_assert(0);
		} break;
	}
	game_update_tilemap_from_tile_behavior(tile_index, behavior, state);
}

static u32 game_check_red_count(game_state *state)
{
	u32 result = 0;
	u32 index = 0;
	for( ; index < TILEMAP_WIDTH * TILEMAP_HEIGHT; index++)
	{
		if(state->memory.tiles[index].unit_type == UNIT_TYPE_RED)
		{
			result++;
			break;
		}
		if(state->memory.tiles[index].tile_type == TILE_TYPE_RED)
		{
			result++;
			break;
		}
	}
	return result;
}

static void game_update_tilemap_from_tile_behavior(i32 tile_index, tile_behavior behavior, game_state *state)
{
	i32 target_tile_ids[64];
	u32 target_count = 0;

	switch(behavior.target_type)
	{
		case TARGET_TYPE_ADJACENT_STRAIGHT:
		{

			target_count = 0;

			if(tile_index % TILEMAP_WIDTH != 0)
			{
				/* not on left edge */
				target_tile_ids[target_count] = tile_index - 1;
				target_count++;
			}
			if(tile_index % TILEMAP_WIDTH != (TILEMAP_WIDTH - 1))
			{
				/* not on right edge */
				target_tile_ids[target_count] = tile_index + 1;
				target_count++;
			}
			if(tile_index >= TILEMAP_WIDTH)
			{
				/* not in first row */
				target_tile_ids[target_count] = tile_index - TILEMAP_WIDTH;
				target_count++;
			}
			if(tile_index < TILEMAP_WIDTH * (TILEMAP_HEIGHT - 1))
			{
				/* not in last row */
				target_tile_ids[target_count] = tile_index + TILEMAP_WIDTH;
				target_count++;
			}
		} break;

		case TARGET_TYPE_NONE:
		{
			/* do nothing */
		} break;
		/* TODO: all the other TARGET_TYPES */

		default:
		{
			_assert(0);
		}
	}

	u32 target_index = 0;
	i32 target;
	for( ; target_index < target_count; target_index++)
	{
		target = target_tile_ids[target_index];	
		if(target >= 0 && target < TILEMAP_WIDTH * TILEMAP_HEIGHT)
		{
			if(state->memory.tiles[target].tile_type != behavior.tile_type_shift_targets)
			{
				/* tile is not already what we wanted to shift to */
				state->memory.tiles[target].tile_type = behavior.tile_type_shift_targets;
				if(state->memory.tiles[target].unit_type != UNIT_TYPE_NONE)
				{
					state->memory.tiles[target].unit_type = behavior.unit_type_shift_targets;
				}
			}
		}
	}

	u32 red_count = game_check_red_count(state);
	if(!red_count)
	{
		state->state = STATE_WON;
	}
}
