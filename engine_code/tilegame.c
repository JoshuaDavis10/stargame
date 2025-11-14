#include "game.h"

#include "linux_util.c"
#include "jstring.h"
#include "math.c"

#include "profiler.c"

#define MAX_VERTICES 512
#define JSTRING_MEMORY_SIZE 1024

#define TILEMAP_WIDTH 8
#define TILEMAP_HEIGHT 8 

enum {
	TILE_TYPE_EMPTY,
	TILE_TYPE_HAS_GUY,
	TILE_TYPE_COUNT
};

typedef enum {
	MESH_TYPE_TILE_EMPTY,
	MESH_TYPE_GUY,
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
	i32 *tiles;
	f32 tile_stride;
	f32 tilemap_offset_x;
	f32 tilemap_offset_y;
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
		state->memory.tiles[index] = TILE_TYPE_EMPTY;
	}
	state->memory.tiles[2] = TILE_TYPE_HAS_GUY;

	state->memory.tile_stride = 1.0f;
	state->memory.tilemap_offset_x = -3.5f;
	state->memory.tilemap_offset_y = -3.5f;
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

		/* TODO: store these values somewhere so that we know what tile
		 * we're clicking when a user clicks */
		pos.x = (x * state->memory.tile_stride) + state->memory.tilemap_offset_x;
		pos.y = (y * state->memory.tile_stride) + state->memory.tilemap_offset_y;

		switch(state->memory.tiles[index])
		{
			case TILE_TYPE_EMPTY:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_EMPTY, state);
				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary("TILE", jstring_length("TILE"));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					2, text, black);
			} break;
			case TILE_TYPE_HAS_GUY:
			{
				game_draw_mesh(pos, MESH_TYPE_TILE_EMPTY, state);
				game_draw_mesh(pos, MESH_TYPE_GUY, state);

				i32 screen_x, screen_y;
				jstring text = jstring_create_temporary("DUDE", jstring_length("DUDE"));
				world_to_screen(pos.x, pos.y, &screen_x, &screen_y, state);
				draw_text_in_buffer_centered(
					state->pixel_buffer,
					state->pixel_buffer_width,
					state->pixel_buffer_height,
					screen_x, screen_y,
					2, text, black);
					
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

		/* tilemap data */
	state->memory.tiles = 
		(i32*)game_memory_allocate(&used_memory, (sizeof(i32) * TILEMAP_WIDTH * TILEMAP_HEIGHT), game_memory, game_memory_size);

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

	const char *str = "WELCOME TO TILEGAME";
	jstring test_string = 
		jstring_create_temporary(str, jstring_length(str));

	if(input->mouse_left == INPUT_BUTTON_STATE_PRESSED)
	{
		i32 mouse_x = input->mouse_x;
		i32 mouse_y = input->mouse_y;
		f32 world_x;
		f32 world_y;
		screen_to_world(mouse_x, mouse_y, &world_x, &world_y, state);

		i32 tile = tile_from_world_coords(world_x, world_y, state);
		if(tile != -1)
		{
			state->memory.tiles[tile] = TILE_TYPE_HAS_GUY;	
		}
	}

	PROFILER_FINISH_TIMING_BLOCK(update);

	/* render */
	PROFILER_START_TIMING_BLOCK(render);

	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		gray); 

	game_draw_tilemap(state);

	draw_text_in_buffer_centered(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		200, 20, 
		2,
		test_string,
		white);


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
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions = state->memory.vertex_position_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	/* TODO: index-based rendering, then only need 4 vertices */
	/* TODO: read meshes from files */
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].vertex_count = 6;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[0].x = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[0].y = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[1].x =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[1].y = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[2].x = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[2].y =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[3].x = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[3].y =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[4].x =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[4].y = 0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[5].x =-0.5f;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].positions[5].y =-0.5f;

	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].colors[0] = cyan4;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].colors[1] = cyan4;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].colors[2] = cyan4;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].colors[3] = cyan4;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].colors[4] = cyan4;
	state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].colors[5] = black4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_TILE_EMPTY].vertex_count;

	state->memory.mesh_data[MESH_TYPE_GUY].positions = state->memory.vertex_position_data + state->memory.vertex_count;
	state->memory.mesh_data[MESH_TYPE_GUY].colors = state->memory.vertex_color_data + state->memory.vertex_count;

	state->memory.mesh_data[MESH_TYPE_GUY].vertex_count = 6;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[0].x = 0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[0].y = 0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[1].x =-0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[1].y = 0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[2].x = 0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[2].y =-0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[3].x = 0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[3].y =-0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[4].x =-0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[4].y = 0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[5].x =-0.4f;
	state->memory.mesh_data[MESH_TYPE_GUY].positions[5].y =-0.4f;

	state->memory.mesh_data[MESH_TYPE_GUY].colors[0] = white4;
	state->memory.mesh_data[MESH_TYPE_GUY].colors[1] = red4;
	state->memory.mesh_data[MESH_TYPE_GUY].colors[2] = red4;
	state->memory.mesh_data[MESH_TYPE_GUY].colors[3] = red4;
	state->memory.mesh_data[MESH_TYPE_GUY].colors[4] = red4;
	state->memory.mesh_data[MESH_TYPE_GUY].colors[5] = red4;

	state->memory.vertex_count += state->memory.mesh_data[MESH_TYPE_GUY].vertex_count;

	LOG_TRACE("game initialize meshes finished (vertex count: %u)", state->memory.vertex_count);
	_assert(state->memory.vertex_count <= MAX_VERTICES);
	return(true);
}
