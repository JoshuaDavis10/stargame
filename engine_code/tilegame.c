#include "game.h"

#include "util.c"
#include "jstring.h"
#include "math.c"

#include "profiler.c"

#define MAX_VERTICES 512
#define JSTRING_MEMORY_SIZE 2048

#define FONTSIZE 1

#define STEP_TIME 300000.0

#include "tile_types.h"

typedef struct {
	vector_2 position;
	/* NOTE: -x and +x maximums + -y and +y maximums for camera coords 
	 * that will actually be drawn 
	 */
	vector_2 bounds; 
} camera;

#include "cpu_render.c"

typedef struct {
	u32 blue_counter;
	u32 green_counter;
	u32 red_counter;
	i32 width;
	i32 height;
	tile tiles[0];
} tilemap;


typedef struct {
	void *jstring_memory;
	render_mesh *mesh_data;
	vertex *vertex_data;
	u32 vertex_count;
	tilemap *tilemap_data;
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

/* small funcs that Ima just define up here */
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


static void game_draw_mesh(vector_2 position, mesh_type type, game_state *state)
{
	draw_mesh(state->pixel_buffer, state->pixel_buffer_width, state->pixel_buffer_height, 
		   state->memory.mesh_data[type], position, state->game_camera);
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

	if(tile_x >= state->memory.tilemap_data->tilemap_width || tile_y >= state->memory.tilemap_height)
	{
		/* world coords do not match a tile */
		return -1;
	}
	return(tile_y * state->memory.tilemap_width + tile_x);
}

/* forward declarations for bigger funcs */
static b32 game_initialize_meshes(game_state *state);
static b32 game_initialize_tilemap(
		game_state *state, 
		u64 *used_memory, 
		void *game_memory, 
		u64 game_memory_size, 
		char *level_filename);

static void game_draw_tilemap(game_state *state);
static void game_register_move(game_state *state, input_state *input, i32 unit_type);
static void game_step_through_move(game_state *state);

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
	start_profile();

	PROFILER_START_TIMING_BLOCK(memory_stuff);
	/* memory stuff */	
	u64 used_memory = 0;

		/* game state */
	game_state *state = (game_state*)game_memory_allocate(&used_memory, 
			sizeof(game_state), 
			game_memory, 
			game_memory_size); 

		/* set up jstring stuff */
	state->memory.jstring_memory = game_memory_allocate(&used_memory, 
			JSTRING_MEMORY_SIZE, 
			game_memory, 
			game_memory_size);
	if(!jstring_load_logging_function(log_lib))
	{
		_assert(0);
	}

	/* NOTE(josh): this is resetting the jstring memory so don't need to call jstring_memory_reset...
	 * since jstring is loaded as part of the .dll, all the jstring memory values get set to 0
	 */
	if(!jstring_memory_activate(JSTRING_MEMORY_SIZE, state->memory.jstring_memory))
	{
		_assert(0);
	}
		/* mesh data */
	state->memory.mesh_data = 
		(render_mesh*)game_memory_allocate(&used_memory, 
				sizeof(render_mesh) * MESH_TYPE_COUNT, 
				game_memory, 
				game_memory_size);
	state->memory.vertex_position_data = 
		(vector_2*)game_memory_allocate(&used_memory, 
				sizeof(vector_2) * MAX_VERTICES, 
				game_memory, 
				game_memory_size); 
	state->memory.vertex_color_data = 
		(vector_4*)game_memory_allocate(&used_memory, 
				sizeof(vector_4) * MAX_VERTICES, 
				game_memory, 
				game_memory_size); 

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
		log_debug("->target tile ids:%p", state->memory.move.target_tile_ids);
		log_debug("->tilemap:        %p", state->memory.tilemap_data);
		log_debug("game memory size: %u", game_memory_size);
		log_debug("used memory     : %u", used_memory);

			/* time stuff */
		state->last_time = read_os_timer();
		state->timer = 0.0;

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

	if(state->state == STATE_WAITING_FOR_MOVE)
	{
		if(input->letters[1] == INPUT_BUTTON_STATE_PRESSED)
		{
			game_register_move(state, input, UNIT_TYPE_BLUE);
		}
		else if(input->letters[6] == INPUT_BUTTON_STATE_PRESSED)
		{
			game_register_move(state, input, UNIT_TYPE_GREEN);
		}
		else if(input->letters[17] == INPUT_BUTTON_STATE_PRESSED)
		{
			game_register_move(state, input, UNIT_TYPE_RED);
		}
		else if(input->spacebar == INPUT_BUTTON_STATE_PRESSED)
		{
			/* XXX: this assumes the map hasn't been edited btw, since it avoids reallocating for larger 
			 * sizes... maybe we need to do a free, then reallocate to reset, similar to like in the editor
			 */
			_assert(game_initialize_tilemap(state, 
						&used_memory, 
						game_memory, 
						game_memory_size, 
						level_filename));
			_assert(game_initialize_meshes(state));
		}
	}
	else if(state->state == STATE_STEPPING_THROUGH_MOVE)
	{
		game_step_through_move(state);
	}
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
		/* TODO: read cpu_frequency at startup so that this doesn't take 100ms */
		finish_and_print_profile(log_trace);
		state->timer = 0.0;
	}
}

static b32 game_initialize_meshes(game_state *state)
{
	state->memory.vertex_count = 0;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions = state->memory.vertex_position_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	/* TODO: index-based rendering, then only need 4 vertices */
	/* TODO: read meshes from files */
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].vertex_count = 12;
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
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[6].x =15* state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[6].y =15* state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[7].x =15*-state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[7].y =15* state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[8].x =15* state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[8].y =15*-state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[9].x =15* state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[9].y =15*-state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[10].x =15*-state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[10].y =15* state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[11].x =15*-state->memory.tile_stride/32;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions[11].y =15*-state->memory.tile_stride/32;

	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[0] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[1] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[2] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[3] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[4] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[5] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[6] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[7] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[8] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[9] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[10] = tile_color_blue;
	state->memory.mesh_data[MESH_TYPE_TILE_BLUE].colors[11] = tile_color_blue;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_BLUE].vertex_count;

	state->memory.mesh_data[MESH_TYPE_TILE_RED].positions = 
		state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].vertex_count = 12;

	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[0] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[1] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[2] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[3] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[4] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[5] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[6] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[7] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[8] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[9] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[10] = tile_color_red;
	state->memory.mesh_data[MESH_TYPE_TILE_RED].colors[11] = tile_color_red;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_RED].vertex_count;

	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].positions = 
		state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].vertex_count = 12;

	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[0] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[1] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[2] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[3] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[4] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[5] = black4;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[6] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[7] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[8] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[9] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[10] = tile_color_green;
	state->memory.mesh_data[MESH_TYPE_TILE_GREEN].colors[11] = tile_color_green;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_GREEN].vertex_count;

	state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].positions = 
		state->memory.mesh_data[MESH_TYPE_TILE_BLUE].positions;
	state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].colors = state->memory.vertex_color_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].vertex_count = 12;
	/* NOTE(josh): don't need to initialize color data, since anything that draws this mesh will manually set the color data */

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions = state->memory.vertex_position_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].vertex_count = 6;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[0].x = state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[0].y = state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[1].x =-state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[1].y = state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[2].x = state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[2].y =-state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[3].x = state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[3].y =-state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[4].x =-state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[4].y = state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[5].x =-state->memory.tile_stride/4;
	state->memory.mesh_data[MESH_TYPE_UNIT_RED].positions[5].y =-state->memory.tile_stride/4;

	state->memory.mesh_data[MESH_TYPE_UNIT_RED].colors[0] = unit_color_red;
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

	state->memory.mesh_data[MESH_TYPE_UNIT_BLUE].colors[0] = unit_color_blue;
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

	state->memory.mesh_data[MESH_TYPE_UNIT_GREEN].colors[0] = unit_color_green;
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

static void game_register_move(game_state *state, input_state *input, i32 unit_type)
{
	i32 mouse_x = input->mouse_x;
	i32 mouse_y = input->mouse_y;
	f32 world_x;
	f32 world_y;
	screen_to_world(mouse_x, mouse_y, &world_x, &world_y, state);

	i32 tile_index = tile_from_world_coords(world_x, world_y, state);
	if(tile_index == -1) { return; }

	log_debug("%d on tile %d", unit_type, tile_index);
}

/* TODO: cache the red count and increment/decrement when one is removed or added 
 * - although, this is definitely NOT a performance bottleneck */
static b32 game_check_red_count(game_state *state)
{
	i32 index = 0;
	for( ; index < state->memory.tilemap_data.width * state->memory.tilemap_data.height; index++)
	{
		if(state->memory.tilemap_data->tiles[index].tile_type == TILE_TYPE_RED)
		{
			return true;
		}
		if(state->memory.tilemap_data->tiles[index].unit_type == UNIT_TYPE_RED)
		{
			return true;
		}
	}
	return false;
}

static void game_step_through_move(game_state *state)
{
	state->state = STATE_WAITING_FOR_MOVE;
	u32 red_count = game_check_red_count(state);
	if(!red_count && state->state == STATE_WAITING_FOR_MOVE)
	{
		state->state = STATE_WON;
	}
}

static b32 game_initialize_tilemap(
		game_state *state, 
		u64 *used_memory, 
		void *game_memory, 
		u64 game_memory_size, 
		char *level_filename)
{
	/* TODO: this should rlly do it similar to editor */
	u64 level_file_size = get_file_size(level_filename);

	_assert(level_file_size);
	log_trace("level file size: %llu", level_file_size);

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
		state->memory.move.target_tile_ids = 
			(i32*)game_memory_allocate(
				used_memory, 
				(sizeof(i32) * state->memory.tilemap_width * state->memory.tilemap_height), 
				game_memory, 
				game_memory_size);
		/* TODO: this needs to get reallocated if width/height changed... like how editor does it */
		state->memory.tilemap_data->tiles = 
			(tile*)game_memory_allocate(
				used_memory, 
				(sizeof(tile) * state->memory.tilemap_width * state->memory.tilemap_height), 
				game_memory, 
				game_memory_size);
	}

	i32 index = 0;
	for( ; index < (state->memory.tilemap_width * state->memory.tilemap_height); index++)
	{
		state->memory.tilemap_data->tiles[index] = level_tile_data_pointer[index];
	}

	log_debug("tilemap width: %d", state->memory.tilemap_width);
	state->memory.tile_stride = 8.0f / max_i32(state->memory.tilemap_width, state->memory.tilemap_height);
	log_debug("tile stride : %.2f", state->memory.tile_stride);
	state->memory.tilemap_offset_x = -(state->memory.tile_stride * state->memory.tilemap_width/2 - state->memory.tile_stride/2);
	state->memory.tilemap_offset_y = -(state->memory.tile_stride * state->memory.tilemap_height/2 - state->memory.tile_stride/2);

	free(temp_level_data_buffer);

	state->tilemap_initialized = true;

	return(true);
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

		switch(state->memory.tilemap_data->tiles[index].tile_type)
		{
			case TILE_TYPE_BLUE:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_BLUE, state);
			} break;
			case TILE_TYPE_RED:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_RED, state);
			} break;
			case TILE_TYPE_GREEN:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_GREEN, state);
			} break;
			case TILE_TYPE_TRANSITIONING:
			{
				u32  vertex_color_index = 0;
				for(
						; 
						vertex_color_index < state->memory.mesh_data[MESH_TYPE_TILE_TRANSITIONING].vertex_count; 
						vertex_color_index++)
				{
					vector_4 color;
					vector_4 from_color;
					vector_4 to_color;

					switch(state->memory.tilemap_data->tiles[index].transition_tile_type_from)
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
							log_error("tiletype from: %d", 
									state->memory.tilemap_data->tiles[index].transition_tile_type_from);
							log_error("tile: %d", index);
							_assert(0);
						} break;
					}

					switch(state->memory.tilemap_data->tiles[index].transition_tile_type_to)
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
							log_error("tiletype to: %d", 
									state->memory.tilemap_data->tiles[index].transition_tile_type_to);
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
			} break;
			default:
			{
				log_error("tiletype: %d", state->memory.tilemap_data->tiles[index].tile_type);
				log_error("tile: %d", index);
				_assert(0);
			} break;
		}

		switch(state->memory.tilemap_data->tiles[index].unit_type)
		{
			case UNIT_TYPE_RED:
			{
				game_draw_mesh(pos, MESH_TYPE_UNIT_RED, state);
			} break;
			case UNIT_TYPE_BLUE:
			{
				game_draw_mesh(pos, MESH_TYPE_UNIT_BLUE, state);
			} break;
			case UNIT_TYPE_GREEN:
			{
				game_draw_mesh(pos, MESH_TYPE_UNIT_GREEN, state);
			} break;
			case UNIT_TYPE_NONE:
			{
				/* do nothing */
			} break;
			case UNIT_TYPE_TRANSITIONING:
			{
				u32  vertex_color_index = 0;
				for(
					; 
					vertex_color_index < state->memory.mesh_data[MESH_TYPE_UNIT_TRANSITIONING].vertex_count; 
					vertex_color_index++)
				{
					vector_4 color;
					vector_4 from_color;
					vector_4 to_color;

					switch(state->memory.tilemap_data->tiles[index].transition_unit_type_from)
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

					switch(state->memory.tilemap_data->tiles[index].transition_unit_type_to)
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
			} break;
			default:
			{
				log_error("unit type: %d", state->memory.tilemap_data->tiles[index].unit_type);
				log_error("tile: %d", index);
				_assert(0);
			} break;
		}
	}
}
