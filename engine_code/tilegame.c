#include "game.h"

#include "util.c"
#include "jstring.h"
#include "math.c"

#include "profiler.c"

#define MAX_VERTICES 512
#define JSTRING_MEMORY_SIZE 2048

#define FONTSIZE 1

#define STEP_TIME 300000.0

static const char *unit_type_red_label = "RED";
static const char *unit_type_green_label = "GREEN";
static const char *unit_type_blue_label = "BLUE";
static const char *tile_type_blue_label = "BLUE";
static const char *tile_type_red_label = "RED";
static const char *tile_type_green_label = "GREEN";
static const char *tile_type_transitioning_label = "...";

#define BRIGHT_SKEW_TILE 0.1f
#define BRIGHT_SKEW_UNIT 0.2f
#define DARK_SKEW_TILE 0.65f
#define DARK_SKEW_UNIT 1.0f

static const vector_4 tile_color_blue = {BRIGHT_SKEW_TILE, BRIGHT_SKEW_TILE, DARK_SKEW_TILE, 1.0f};
static const vector_4 tile_color_red = {DARK_SKEW_TILE, BRIGHT_SKEW_TILE, BRIGHT_SKEW_TILE, 1.0f};
static const vector_4 tile_color_green = {BRIGHT_SKEW_TILE, DARK_SKEW_TILE, BRIGHT_SKEW_TILE, 1.0f};

static const vector_4 unit_color_blue = {BRIGHT_SKEW_UNIT, BRIGHT_SKEW_UNIT, DARK_SKEW_UNIT, 1.0f};
static const vector_4 unit_color_red = {DARK_SKEW_UNIT, BRIGHT_SKEW_UNIT, BRIGHT_SKEW_UNIT, 1.0f};
static const vector_4 unit_color_green = {BRIGHT_SKEW_UNIT, DARK_SKEW_UNIT, BRIGHT_SKEW_UNIT, 1.0f};

#include "tile_types.h"

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
	STATE_WAITING_FOR_MOVE,
	STATE_STEPPING_THROUGH_MOVE,
	STATE_WON,
	STATE_COUNT
};

typedef struct {
	i32 target_type;
	i32 tile_type_shift_targets;
	i32 unit_type_shift_targets;
} tile_behavior;

enum {
	MOVE_STEP_PLACE_UNIT,
	MOVE_STEP_CHANGE_TILE,
	MOVE_STEP_CHANGE_TARGET,
	MOVE_STEP_CHANGE_TARGET_UNIT,
	MOVE_STEP_ANIMATE_TARGET_UNIT_CHANGE,
	MOVE_STEP_COUNT
};
typedef struct {
	i32 current_step; /* NOTE(josh): so we know which step of the process we are on */
	i32 placed_unit_type;
	i32 placed_tile_type;
	i32 placed_tile_index;
	f64 time_of_last_step_us;
	i32 *target_tile_ids;
	i32 target_count;
} move_steps;

static const tile_behavior tile_behavior_list[TILE_TYPE_COUNT] = {
	{TARGET_TYPE_ADJACENT_STRAIGHT, TILE_TYPE_GREEN, UNIT_TYPE_RED}, /* blue */
	{TARGET_TYPE_ADJACENT_STRAIGHT, TILE_TYPE_RED, UNIT_TYPE_NONE}, /* red */
	{TARGET_TYPE_ADJACENT_STRAIGHT, TILE_TYPE_RED, UNIT_TYPE_RED}  /* green */
};

typedef enum {
	MESH_TYPE_TILE_BLUE,
	MESH_TYPE_TILE_RED,
	MESH_TYPE_TILE_GREEN,
	MESH_TYPE_TILE_TRANSITIONING,
	MESH_TYPE_UNIT_BLUE,
	MESH_TYPE_UNIT_RED,
	MESH_TYPE_UNIT_GREEN,
	MESH_TYPE_UNIT_TRANSITIONING,
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
	/* TODO: vertex data should just be packed together instead of sep
	 * cuz currently each drawn triangle is reading from 2 very disparate places
	 * in mem?
	 */
	vector_2 *vertex_position_data;
	vector_4 *vertex_color_data;
	u32 vertex_count;
	tile *tiles;
	f32 tile_stride;
	f32 tilemap_offset_x;
	f32 tilemap_offset_y;
	i32 tilemap_width;
	i32 tilemap_height;
	u32 blue_count;
	u32 red_count;
	u32 green_count;
	move_steps move;
} game_memory;

typedef struct {
	camera game_camera; /* NOTE: world space coords */
	u64 last_time;
	u64 time_elapsed;
	f64 timer;
	b32 initialized;
	game_memory memory;
	b32 tilemap_initialized;
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

static b32 game_initialize_tilemap(game_state *state, u64 *used_memory, void *game_memory, u64 game_memory_size, char *level_filename)
{
	u64 level_file_size = get_file_size(level_filename);

	log_trace("level file size: %llu", level_file_size);

	/* TODO: there's gotta be a way to just read the file directly into the game state, instead of this
	 * temp buffer, but this is init stuff so maybe its just ok...*/
	u32 *temp_level_data_buffer = (u32*)malloc(level_file_size);

	read_file_into_buffer(level_filename, (void*)temp_level_data_buffer, level_file_size);

	state->memory.blue_count = temp_level_data_buffer[0];
	state->memory.green_count = temp_level_data_buffer[1];
	state->memory.red_count = temp_level_data_buffer[2];

	i32 *temp_width_height_data_buffer = (i32*)(temp_level_data_buffer + 3);
	state->memory.tilemap_width = temp_width_height_data_buffer[0];
	state->memory.tilemap_height = temp_width_height_data_buffer[1];

	tile *level_tile_data_pointer = (tile*)(temp_level_data_buffer + 5);

	log_trace("width: %d, height: %d", state->memory.tilemap_width, state->memory.tilemap_height);
	log_trace("blue: %d, green: %d, red: %d", state->memory.blue_count, state->memory.green_count, state->memory.red_count);

	if(!state->tilemap_initialized)
	{
	/* tilemap data */
	state->memory.tiles = 
		(tile*)game_memory_allocate(
			used_memory, 
			(sizeof(tile) * state->memory.tilemap_width * state->memory.tilemap_height), 
			game_memory, 
			game_memory_size);

	state->memory.move.target_tile_ids = 
		(i32*)game_memory_allocate(
			used_memory, 
			(sizeof(i32) * state->memory.tilemap_width * state->memory.tilemap_height), 
			game_memory, 
			game_memory_size);
	}

	i32 index = 0;
	for( ; index < (state->memory.tilemap_width * state->memory.tilemap_height); index++)
	{
		state->memory.tiles[index] = level_tile_data_pointer[index];
	}

	state->memory.tile_stride = 8.0f / state->memory.tilemap_width;
	state->memory.tilemap_offset_x = -(state->memory.tile_stride * state->memory.tilemap_width/2 - state->memory.tile_stride/2);
	state->memory.tilemap_offset_y = -(state->memory.tile_stride * state->memory.tilemap_height/2 - state->memory.tile_stride/2);

	free(temp_level_data_buffer);

	state->tilemap_initialized = true;

	return(true);
}

static void game_reset_tilemap(game_state *state)
{
	i32 index = 0;
	for( ; index < (state->memory.tilemap_width * state->memory.tilemap_height); index++)
	{
		state->memory.tiles[index].tile_type = TILE_TYPE_BLUE;
		state->memory.tiles[index].unit_type = UNIT_TYPE_NONE;
	}
	state->memory.tiles[2].tile_type = TILE_TYPE_RED;
	state->memory.tiles[4].tile_type = TILE_TYPE_GREEN;
	state->memory.tiles[6].tile_type = TILE_TYPE_GREEN;
	state->memory.tiles[2].unit_type = UNIT_TYPE_RED;
	state->memory.tiles[3].tile_type = TILE_TYPE_GREEN;

	state->memory.tile_stride = 8.0f / state->memory.tilemap_width;
	state->memory.tilemap_offset_x = -(state->memory.tile_stride * state->memory.tilemap_width/2 - state->memory.tile_stride/2);
	state->memory.tilemap_offset_y = -(state->memory.tile_stride * state->memory.tilemap_height/2 - state->memory.tile_stride/2);

	state->memory.blue_count = 3;
	state->memory.green_count = 1;
	state->memory.red_count = 0;

}

static void game_draw_mesh(vector_2 position, mesh_type type, game_state *state)
{
	draw_mesh(state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 
		   state->memory.mesh_data[type], position, state->game_camera);
}

static void game_draw_tilemap(game_state *state)
{
	i32 index = 0;
	vector_2 pos;
	u32 x;
	u32 y;
	for( ; index < (state->memory.tilemap_width * state->memory.tilemap_height); index++)
	{
		x = index % state->memory.tilemap_width;
		y = index / state->memory.tilemap_width;

		pos.x = (x * state->memory.tile_stride) + state->memory.tilemap_offset_x;
		pos.y = (y * state->memory.tile_stride) + state->memory.tilemap_offset_y;

		switch(state->memory.tiles[index].tile_type)
		{
			case TILE_TYPE_BLUE:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_BLUE, state);

				if(state->memory.tiles[index].unit_type == UNIT_TYPE_NONE)
				{
				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary(tile_type_blue_label, jstring_length(tile_type_blue_label));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					FONTSIZE, text, white);
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
					FONTSIZE, text, white);
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
					FONTSIZE, text, white);
				}
			} break;
			case TILE_TYPE_TRANSITIONING:
			{
				u32  vertex_color_index = 0;
				for( ; vertex_color_index < state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].vertex_count; vertex_color_index++)
				{
					vector_4 color;
					vector_4 from_color;
					vector_4 to_color;

					switch(state->memory.tiles[index].transition_tile_type_from)
					{
						case TILE_TYPE_BLUE:
						{
							from_color = state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[vertex_color_index];
						} break;
						case TILE_TYPE_GREEN:
						{
							from_color = state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[vertex_color_index];
						} break;
						case TILE_TYPE_RED:
						{
							from_color = state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[vertex_color_index];
						} break;
						default:
						{
							log_error("tiletype from: %d", state->memory.tiles[index].transition_tile_type_from);
							log_error("tile: %d", index);
							_assert(0);
						} break;
					}

					switch(state->memory.tiles[index].transition_tile_type_to)
					{
						case TILE_TYPE_BLUE:
						{
							to_color = state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[vertex_color_index];
						} break;
						case TILE_TYPE_GREEN:
						{
							to_color = state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[vertex_color_index];
						} break;
						case TILE_TYPE_RED:
						{
							to_color = state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[vertex_color_index];
						} break;
						default:
						{
							log_error("tiletype to: %d", state->memory.tiles[index].transition_tile_type_to);
							log_error("tile: %d", index);
							_assert(0);
						} break;
					}

					f64 elapsed = read_os_timer() - state->memory.move.time_of_last_step_us;
					f64 normalized_elapsed = (elapsed / STEP_TIME) * (elapsed / STEP_TIME);
					if(normalized_elapsed > 1.0f)
					{
						normalized_elapsed = 1.0f;
					}
					color.x = from_color.x + ((to_color.x - from_color.x) * normalized_elapsed);
					color.y = from_color.y + ((to_color.y - from_color.y) * normalized_elapsed);
					color.z = from_color.z + ((to_color.z - from_color.z) * normalized_elapsed);
					color.w = from_color.w + ((to_color.w - from_color.w) * normalized_elapsed);

					state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].colors[vertex_color_index] = color;
				}

				game_draw_mesh(pos, MESH_TYPE_TILE_TRANSITIONING, state);

				if(state->memory.tiles[index].unit_type == UNIT_TYPE_NONE)
				{
					i32 screen_x, screen_y;
					jstring text = 
							jstring_create_temporary(tile_type_transitioning_label, jstring_length(tile_type_transitioning_label));
					world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
					draw_text_in_buffer_centered(
						state->pixel_buffer,
						state->pixel_buffer_width,
						state->pixel_buffer_height,
						screen_x, screen_y,
						FONTSIZE, text, white);
				}

			} break;
			default:
			{
				log_error("tiletype: %d", state->memory.tiles[index].tile_type);
				log_error("tile: %d", index);
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
					FONTSIZE, text, black);
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
			case UNIT_TYPE_TRANSITIONING:
			{
				u32  vertex_color_index = 0;
				for( ; vertex_color_index < state->memory.mesh_data[MESH_TYPE_UNIT_TRANSITIONING].vertex_count; vertex_color_index++)
				{
					vector_4 color;
					vector_4 from_color;
					vector_4 to_color;

					switch(state->memory.tiles[index].transition_unit_type_from)
					{
						case UNIT_TYPE_BLUE:
						{
							from_color = state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[vertex_color_index];
						} break;
						case UNIT_TYPE_GREEN:
						{
							from_color = state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[vertex_color_index];
						} break;
						case UNIT_TYPE_RED:
						{
							from_color = state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[vertex_color_index];
						} break;
						default:
						{
							_assert(0);
						} break;
					}

					switch(state->memory.tiles[index].transition_unit_type_to)
					{
						case UNIT_TYPE_BLUE:
						{
							to_color = state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[vertex_color_index];
						} break;
						case UNIT_TYPE_GREEN:
						{
							to_color = state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[vertex_color_index];
						} break;
						case UNIT_TYPE_RED:
						{
							to_color = state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[vertex_color_index];
						} break;
						case UNIT_TYPE_NONE:
						{
							to_color.x = from_color.x;
							to_color.y = from_color.y;
							to_color.z = from_color.z;
							to_color.w = 0.0f;
						} break;
						default:
						{
							_assert(0);
						} break;
					}

					f64 elapsed = read_os_timer() - state->memory.move.time_of_last_step_us;
					_assert(elapsed >= 0.0f);
					f64 normalized_elapsed = (elapsed / STEP_TIME) * (elapsed / STEP_TIME);
					if(normalized_elapsed > 1.0f)
					{
						normalized_elapsed = 1.0f;
					}
					color.x = from_color.x + ((to_color.x - from_color.x) * normalized_elapsed);
					color.y = from_color.y + ((to_color.y - from_color.y) * normalized_elapsed);
					color.z = from_color.z + ((to_color.z - from_color.z) * normalized_elapsed);
					color.w = from_color.w + ((to_color.w - from_color.w) * normalized_elapsed);

					state->memory.mesh_data[MESH_TYPE_UNIT_TRANSITIONING].colors[vertex_color_index] = color;
				}

				game_draw_mesh(pos, MESH_TYPE_UNIT_TRANSITIONING, state);
				i32 screen_x, screen_y;
				jstring text = 
						jstring_create_temporary(tile_type_transitioning_label, jstring_length(tile_type_transitioning_label));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					FONTSIZE, text, black);
			} break;
			default:
			{
				log_error("unit type: %d", state->memory.tiles[index].unit_type);
				log_error("tile: %d", index);
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

	if(tile_x >= state->memory.tilemap_width || tile_y >= state->memory.tilemap_height)
	{
		/* world coords do not match a tile */
		return -1;
	}
	return(tile_y * state->memory.tilemap_width + tile_x);
}

static void game_step_through_move(game_state *state);

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
	/* memory stuff */	
	u64 used_memory = 0;

		/* game state */
	game_state *state = (game_state*)game_memory_allocate(&used_memory, sizeof(game_state), game_memory, game_memory_size); 

		/* set up jstring stuff */
	state->memory.jstring_memory = game_memory_allocate(&used_memory, JSTRING_MEMORY_SIZE, game_memory, game_memory_size);
	if(!jstring_load_logging_function(log_lib))
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

	PROFILER_FINISH_TIMING_BLOCK(memory_stuff);

	/* initialize */
	/* NOTE: game code expects platform layer to
	 * initially hand it zeroed out memory 
	 */
	if(!state->initialized)
	{

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

		if(!game_initialize_tilemap(state, &used_memory, game_memory, game_memory_size, level_filename))
		{
			log_error("failed to initialize game state! tilemap initialization failed.");
			init_success = false;
		}

		if(!game_initialize_meshes(state))
		{
			log_error("failed to initialize game state! mesh data initialization failed.");
			init_success = false;
		}

		log_debug("game memory addr: %p", game_memory);
		log_debug("->game state mem: %p", state);
		log_debug("->jstring mem:    %p", state->memory.jstring_memory);
		log_debug("->mesh data mem:  %p", state->memory.mesh_data);
		log_debug("->position mem:   %p", state->memory.vertex_position_data);
		log_debug("->color mem:      %p", state->memory.vertex_color_data);
		log_debug("->tilemap:        %p", state->memory.tiles);
		log_debug("->target tile ids:%p", state->memory.move.target_tile_ids);
		log_debug("game memory size: %u", game_memory_size);
		log_debug("used memory     : %u", used_memory);

		if(init_success)
		{
			log_info("initialized game state.");
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

	if(state->state == STATE_WAITING_FOR_MOVE)
	{
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
						state->memory.move.current_step = MOVE_STEP_PLACE_UNIT;
						state->memory.move.placed_unit_type = UNIT_TYPE_BLUE;
						state->memory.move.placed_tile_type = state->memory.tiles[tile_index].tile_type;
						state->memory.move.placed_tile_index = tile_index;
						state->memory.move.time_of_last_step_us = read_os_timer();
						state->memory.move.target_count = 0;
						state->state = STATE_STEPPING_THROUGH_MOVE;
					}
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
						state->memory.move.current_step = MOVE_STEP_PLACE_UNIT;
						state->memory.move.placed_unit_type = UNIT_TYPE_GREEN;
						state->memory.move.placed_tile_type = state->memory.tiles[tile_index].tile_type;
						state->memory.move.placed_tile_index = tile_index;
						state->memory.move.time_of_last_step_us = read_os_timer();
						state->memory.move.target_count = 0;
						state->state = STATE_STEPPING_THROUGH_MOVE;
					}
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
						state->memory.move.current_step = MOVE_STEP_PLACE_UNIT;
						state->memory.move.placed_unit_type = UNIT_TYPE_RED;
						state->memory.move.placed_tile_type = state->memory.tiles[tile_index].tile_type;
						state->memory.move.placed_tile_index = tile_index;
						state->memory.move.time_of_last_step_us = read_os_timer();
						state->memory.move.target_count = 0;
						state->state = STATE_STEPPING_THROUGH_MOVE;
					}
				}
			}
		}
		else if(input->spacebar == INPUT_BUTTON_STATE_PRESSED)
		{

			/* XXX: this assumes the map hasn't been edited btw, since it avoids reallocating for larger 
			 * sizes... maybe we need to do a free, then reallocate to reset, similar to like in the editor
			 */
			_assert(game_initialize_tilemap(state, &used_memory, game_memory, game_memory_size, level_filename));
		}
	}
	else if(state->state == STATE_STEPPING_THROUGH_MOVE)
	{
		game_step_through_move(state);
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
		10, 25, 
		FONTSIZE,
		ui_string,
		white);
	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 10, 
		FONTSIZE,
		ui_string2,
		white);

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 40, 
		FONTSIZE,
		blue_count_string,
		blue);

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 55, 
		FONTSIZE,
		green_count_string,
		green);

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 70, 
		FONTSIZE,
		red_count_string,
		red);

	if(state->state == STATE_STEPPING_THROUGH_MOVE)
	{
		jstring move_string = jstring_create_temporary("PROCESSING MOVE", jstring_length("PROCESSING MOVE"));
		draw_text_in_buffer(
			pixel_buffer,
			pixel_buffer_width, 
			pixel_buffer_height,
			10, 85, 
			FONTSIZE,
			move_string,
			red);

	}

	if(state->state == STATE_WON)
	{
		/*
		draw_background_in_buffer_asm(pixel_buffer, pixel_buffer_width, pixel_buffer_height, gray);	
		vector_2 pos_blue = {7.0f, -3.0f};
		vector_2 pos_green = {-4.0f, 2.0f};
		vector_2 pos_red = {3.0f, 3.0f};
		game_draw_mesh(pos_red, MESH_TYPE_UNIT_RED, state);
		game_draw_mesh(pos_green, MESH_TYPE_UNIT_GREEN, state);
		game_draw_mesh(pos_blue, MESH_TYPE_UNIT_BLUE, state);
		*/

		jstring win_string = jstring_create_temporary("YOU WIN", jstring_length("YOU WIN"));
		jstring instruction_string = jstring_create_temporary("PRESS SPACEBAR", jstring_length("PRESS SPACEBAR"));
		jstring instruction_string2 = jstring_create_temporary("TO RESTART", jstring_length("TO RESTART"));
		draw_text_in_buffer_centered(
			pixel_buffer,
			pixel_buffer_width,
			pixel_buffer_height,
			pixel_buffer_width/2, pixel_buffer_height/2 - FONTSIZE * 4 * 10,
			FONTSIZE * 4, win_string, magenta);
		draw_text_in_buffer_centered(
			pixel_buffer,
			pixel_buffer_width,
			pixel_buffer_height,
			pixel_buffer_width/2, pixel_buffer_height/2,
			FONTSIZE * 4, instruction_string, magenta);
		draw_text_in_buffer_centered(
			pixel_buffer,
			pixel_buffer_width,
			pixel_buffer_height,
			pixel_buffer_width/2, pixel_buffer_height/2 + FONTSIZE * 4 * 10,
			FONTSIZE * 4, instruction_string2, magenta);

		if(input->spacebar == INPUT_BUTTON_STATE_PRESSED)
		{
			state->state = STATE_WAITING_FOR_MOVE;
			_assert(game_initialize_tilemap(state, &used_memory, game_memory, game_memory_size, level_filename));
		}
	}

	PROFILER_FINISH_TIMING_BLOCK(render);
	if(state->timer > 2000000.0)
	{
		/* TODO: read cpu_frequency at startup so that this doesn't take 100ms */
		finish_and_print_profile(log_trace);
		state->timer = 0.0;
	}
}

static b32 game_initialize_meshes(game_state *state)
{
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions = state->memory.vertex_position_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	/* TODO: index-based rendering, then only need 4 vertices */
	/* TODO: read meshes from files */
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].vertex_count = 6;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[0].x = state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[0].y = state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[1].x =-state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[1].y = state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[2].x = state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[2].y =-state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[3].x = state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[3].y =-state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[4].x =-state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[4].y = state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[5].x =-state->memory.tile_stride/2;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[5].y =-state->memory.tile_stride/2;

	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[0] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[1] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[2] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[3] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[4] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[5] = black4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_BLUE].vertex_count;

	state->memory.mesh_data[MESH_TYPE_TILE_RED].positions = 
		state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[0] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[1] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[2] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[3] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[4] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[5] = black4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_RED].vertex_count;

	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].positions = 
		state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[0] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[1] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[2] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[3] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[4] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[5] = black4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_GREEN].vertex_count;

	state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].positions = 
		state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions;
	state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].vertex_count = 6;
	/* NOTE(josh): don't need to initialize color data, since anything that draws this mesh will manually set the color data */

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions = state->memory.vertex_position_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[0].x = state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[0].y = state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[1].x =-state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[1].y = state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[2].x = state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[2].y =-state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[3].x = state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[3].y =-state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[4].x =-state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[4].y = state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[5].x =-state->memory.tile_stride/3;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[5].y =-state->memory.tile_stride/3;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[0] = white4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[1] = unit_color_red;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[2] = unit_color_red;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[3] = unit_color_red;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[4] = unit_color_red;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[5] = unit_color_red;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_UNIT_RED].vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].positions = 
		state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[0] = white4;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[1] = unit_color_blue;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[2] = unit_color_blue;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[3] = unit_color_blue;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[4] = unit_color_blue;
	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[5] = unit_color_blue;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].positions = 
		state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[0] = white4;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[1] = unit_color_green;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[2] = unit_color_green;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[3] = unit_color_green;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[4] = unit_color_green;
	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[5] = unit_color_green;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_TRANSITIONING].positions = 
		state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions;
	state->memory.mesh_data[MESH_TYPE_UNIT_TRANSITIONING].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_UNIT_TRANSITIONING].vertex_count = 6;
	/* NOTE(josh): don't need to initialize color data, since anything that draws this mesh will manually set the color data */

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_UNIT_TRANSITIONING].vertex_count;

	log_trace("game initialize meshes finished (vertex count: %u)", state->memory.vertex_count);
	_assert(state->memory.vertex_count <= MAX_VERTICES);
	return(true);
} 

static b32 game_check_red_count(game_state *state)
{
	i32 index = 0;
	for( ; index < state->memory.tilemap_width * state->memory.tilemap_height; index++)
	{
		if(state->memory.tiles[index].tile_type == TILE_TYPE_RED)
		{
			return true;
		}
		if(state->memory.tiles[index].unit_type == UNIT_TYPE_RED)
		{
			return true;
		}
	}
	return false;
}

static void game_step_through_move(game_state *state)
{
	move_steps *move = &(state->memory.move);

	_assert(move);
	_assert(move->target_tile_ids);

	f64 elapsed = read_os_timer() - move->time_of_last_step_us;

	if(move->current_step == MOVE_STEP_PLACE_UNIT)
	{
		i32 tile = move->placed_tile_index;
		switch(move->placed_unit_type)
		{
			case UNIT_TYPE_BLUE:
			{
				state->memory.tiles[tile].unit_type = UNIT_TYPE_BLUE;
			} break;
			case UNIT_TYPE_GREEN:
			{
				state->memory.tiles[tile].unit_type = UNIT_TYPE_GREEN;
			} break;
			case UNIT_TYPE_RED:
			{
				state->memory.tiles[tile].unit_type = UNIT_TYPE_RED;
			} break;
			default:
			{
				_assert(0);
			} break;
		}
		move->current_step = MOVE_STEP_CHANGE_TILE;
	}

	if(move->current_step == MOVE_STEP_CHANGE_TILE)
	{
		i32 tile = move->placed_tile_index;
		switch(move->placed_unit_type)
		{
			case UNIT_TYPE_BLUE:
			{
				if(state->memory.tiles[tile].tile_type != TILE_TYPE_BLUE)
				{
					state->memory.tiles[tile].transition_tile_type_from = state->memory.tiles[tile].tile_type;
					state->memory.tiles[tile].tile_type = TILE_TYPE_TRANSITIONING;
					state->memory.tiles[tile].transition_tile_type_to = TILE_TYPE_BLUE;
				}
				else
				{
					state->state = STATE_WAITING_FOR_MOVE;
				}
			} break;
			case UNIT_TYPE_GREEN:
			{
				if(state->memory.tiles[tile].tile_type != TILE_TYPE_GREEN)
				{
					state->memory.tiles[tile].transition_tile_type_from = state->memory.tiles[tile].tile_type;
					state->memory.tiles[tile].tile_type = TILE_TYPE_TRANSITIONING;
					state->memory.tiles[tile].transition_tile_type_to = TILE_TYPE_GREEN;
				}
				else
				{
					state->state = STATE_WAITING_FOR_MOVE;
				}
			} break;
			case UNIT_TYPE_RED:
			{
				if(state->memory.tiles[tile].tile_type != TILE_TYPE_RED)
				{
					state->memory.tiles[tile].transition_tile_type_from = state->memory.tiles[tile].tile_type;
					state->memory.tiles[tile].tile_type = TILE_TYPE_TRANSITIONING;
					state->memory.tiles[tile].transition_tile_type_to = TILE_TYPE_RED;
				}
				else
				{
					state->state = STATE_WAITING_FOR_MOVE;
				}
			} break;
			default:
			{
				_assert(0);
			} break;
		}
		move->current_step = MOVE_STEP_CHANGE_TARGET;
		move->time_of_last_step_us = read_os_timer();
		return;
	}

	if(elapsed > STEP_TIME)
	{
		switch(move->current_step)
		{
			case MOVE_STEP_CHANGE_TARGET:
			{
				i32 tile_index = move->placed_tile_index;
				tile *placed_tile = &(state->memory.tiles[tile_index]);
				placed_tile->tile_type = placed_tile->transition_tile_type_to;
				tile_behavior behavior = tile_behavior_list[placed_tile->tile_type];

				switch(behavior.target_type)
				{
					case TARGET_TYPE_ADJACENT_STRAIGHT:
					{
						move->target_count = 0;

						if(tile_index % state->memory.tilemap_width != 0)
						{
							/* not on left edge */
							move->target_tile_ids[move->target_count] = tile_index - 1;
							move->target_count++;
						}
						if(tile_index % state->memory.tilemap_width != (state->memory.tilemap_width - 1))
						{
							/* not on right edge */
							move->target_tile_ids[move->target_count] = tile_index + 1;
							move->target_count++;
						}
						if(tile_index >= state->memory.tilemap_width)
						{
							/* not in first row */
							move->target_tile_ids[move->target_count] = tile_index - state->memory.tilemap_width;
							move->target_count++;
						}
						if(tile_index < state->memory.tilemap_width * (state->memory.tilemap_height - 1))
						{
							/* not in last row */
							move->target_tile_ids[move->target_count] = tile_index + state->memory.tilemap_width;
							move->target_count++;
						}
					} break;
					/* TODO: implement other target types */
					default:
					{
						_assert(0);
					} break;
				}

				i32 index = 0;
				_assert(move->target_count <= state->memory.tilemap_width * state->memory.tilemap_height);
				u32 temp = 0;
				for( ; index < move->target_count; index++)
				{
					i32 target_tile_index = move->target_tile_ids[index];
					tile *t = &(state->memory.tiles[target_tile_index]);
					if(t->tile_type != behavior.tile_type_shift_targets)
					{
						t->transition_tile_type_from = 
								t->tile_type;
						t->tile_type = TILE_TYPE_TRANSITIONING;
						t->transition_tile_type_to = behavior.tile_type_shift_targets;
						move->target_tile_ids[temp] = target_tile_index;
						temp++;
					}
				}
				move->target_count = temp;
				move->current_step = MOVE_STEP_CHANGE_TARGET_UNIT;
			} break;
			case MOVE_STEP_CHANGE_TARGET_UNIT:
			{
				i32 index = 0;
				b32 change = false;
				for( ; index < move->target_count; index++)
				{
					tile *t = &(state->memory.tiles[move->target_tile_ids[index]]);
					t->tile_type = t->transition_tile_type_to;
					tile_behavior behavior = tile_behavior_list[t->tile_type];

					if(t->unit_type != behavior.unit_type_shift_targets && t->unit_type != UNIT_TYPE_NONE)
					{
						t->transition_unit_type_from = t->unit_type;
						t->unit_type = UNIT_TYPE_TRANSITIONING;
						t->transition_unit_type_to = behavior.unit_type_shift_targets;
						change = true;
					}
				}
				if(change)
				{
					move->current_step = MOVE_STEP_ANIMATE_TARGET_UNIT_CHANGE;
				}
				else
				{
					state->state = STATE_WAITING_FOR_MOVE;
				}
			} break;
			case MOVE_STEP_ANIMATE_TARGET_UNIT_CHANGE:
			{
				i32 index = 0;
				for( ; index < move->target_count; index++)
				{
					tile *t = &(state->memory.tiles[move->target_tile_ids[index]]);
					if(t->unit_type == UNIT_TYPE_TRANSITIONING)
					{
						t->unit_type = t->transition_unit_type_to;
					}
				}
				state->state = STATE_WAITING_FOR_MOVE;
			} break;
			default:
			{
				_assert(0);
			} break;
		}
		move->time_of_last_step_us = read_os_timer();
	}

	/* TODO: if a tile change causes a unit change, have that unit change the tile below it and have compounding
	 * follow through results of the first unit placement, does that make sense? yes, do it */

	u32 red_count = game_check_red_count(state);
	if(!red_count && state->state == STATE_WAITING_FOR_MOVE)
	{
		state->state = STATE_WON;
	}
}
