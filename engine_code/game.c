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
#include <math.h> /* cosf, sinf, sqrt TODO write your own versions */

#define MAX_SHAPE_COUNT 512

enum {
	SHAPE_TYPE_TRIANGLE,
	SHAPE_TYPE_SQUARE,
	SHAPE_TYPE_DIAMOND,
	SHAPE_TYPE_TRAPEZOID,
	SHAPE_TYPE_HEXAGON,
	SHAPE_TYPE_COUNT
};

typedef struct {
	camera game_camera; /* NOTE: world space coords */

	/* TODO: these are gonna be darrays */
	vector_2 *shape_vertex_list;
	i32 *shape_adjacency_list;
	u32 *shape_id_list;
	i32 *shape_type_list;
	vector_2 *shape_position_list;

	u32 shape_count; 
	u32 shape_next_id;

	u64 last_time;
	u64 time_elapsed;
	f64 timer;
	b32 initialized;
} struct_game_state;

/* NOTE: this goin' be useful for a looong time so keeping it */
typedef struct {
	i32 x;
	i32 y;
	i32 w;
	i32 h;
} struct_aabb;

b32 check_collision_aabb(struct_aabb first, struct_aabb second);

void game_draw_shape(
		u8 *pixel_buffer, 
		u16 buffer_width, 
		u16 buffer_height, 
		struct_game_state *state,
		u32 shape_index,
		struct_rgba_color color);

void game_rotate_shape(
		struct_game_state *state,
		u32 shape_index,
		f32 radians);

b32 game_add_shape(
		struct_game_state *state,
		vector_2 position,
		i32 shape_type)
{
	if(state->shape_count < MAX_SHAPE_COUNT)
	{
		switch(shape_type)
		{
			case SHAPE_TYPE_TRIANGLE:
			{
				state->shape_vertex_list[6*state->shape_count].x = 
					0.0f;
				state->shape_vertex_list[6*state->shape_count].y =
					-0.577f;
				state->shape_vertex_list[6*state->shape_count+1].x =
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+1].y = 
					0.289f;
				state->shape_vertex_list[6*state->shape_count+2].x = 
					0.5f;
				state->shape_vertex_list[6*state->shape_count+2].y = 
					0.289f;
				state->shape_type_list[state->shape_count] = 
					SHAPE_TYPE_TRIANGLE;

				state->shape_position_list[state->shape_count].x = 
					position.x;
				state->shape_position_list[state->shape_count].y = 
					position.y;
				state->shape_id_list[state->shape_count] = 
					state->shape_next_id;
				state->shape_adjacency_list[state->shape_count] = -1;
				state->shape_adjacency_list[state->shape_count + 1] = -1;
				state->shape_adjacency_list[state->shape_count + 2] = -1;
			} break;
			case SHAPE_TYPE_SQUARE:
			{
				state->shape_vertex_list[6*state->shape_count].x = 
					0.5f;
				state->shape_vertex_list[6*state->shape_count].y =
					0.5f;
				state->shape_vertex_list[6*state->shape_count+1].x = 
					0.5f;
				state->shape_vertex_list[6*state->shape_count+1].y =
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+2].x = 
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+2].y =
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+3].x = 
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+3].y =
					0.5f;
				state->shape_type_list[state->shape_count] = 
					SHAPE_TYPE_SQUARE;

				state->shape_position_list[state->shape_count].x = 
					position.x;
				state->shape_position_list[state->shape_count].y = 
					position.y;
				state->shape_id_list[state->shape_count] = 
					state->shape_next_id;
				state->shape_adjacency_list[state->shape_count] = -1;
				state->shape_adjacency_list[state->shape_count + 1] = -1;
				state->shape_adjacency_list[state->shape_count + 2] = -1;
				state->shape_adjacency_list[state->shape_count + 3] = -1;
			} break;
			case SHAPE_TYPE_DIAMOND:
			{
				state->shape_vertex_list[6*state->shape_count].x = 
					0.5f;
				state->shape_vertex_list[6*state->shape_count].y =
					0.0f;
				state->shape_vertex_list[6*state->shape_count+1].x = 
					0.0f;
				state->shape_vertex_list[6*state->shape_count+1].y =
					-0.866f;
				state->shape_vertex_list[6*state->shape_count+2].x = 
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+2].y =
					0.0f;
				state->shape_vertex_list[6*state->shape_count+3].x = 
					0.0f;
				state->shape_vertex_list[6*state->shape_count+3].y =
					0.866f;
				state->shape_type_list[state->shape_count] = 
					SHAPE_TYPE_DIAMOND;

				state->shape_position_list[state->shape_count].x = 
					position.x;
				state->shape_position_list[state->shape_count].y = 
					position.y;
				state->shape_id_list[state->shape_count] = 
					state->shape_next_id;
				state->shape_adjacency_list[state->shape_count] = -1;
				state->shape_adjacency_list[state->shape_count + 1] = -1;
				state->shape_adjacency_list[state->shape_count + 2] = -1;
				state->shape_adjacency_list[state->shape_count + 3] = -1;
			} break;
			case SHAPE_TYPE_TRAPEZOID:
			{
				state->shape_vertex_list[6*state->shape_count].x = 
					1.0f;
				state->shape_vertex_list[6*state->shape_count].y =
					0.433f;
				state->shape_vertex_list[6*state->shape_count+1].x = 
					0.5f;
				state->shape_vertex_list[6*state->shape_count+1].y =
					-0.433f;
				state->shape_vertex_list[6*state->shape_count+2].x = 
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+2].y =
					-0.433f;
				state->shape_vertex_list[6*state->shape_count+3].x = 
					-1.0f;
				state->shape_vertex_list[6*state->shape_count+3].y =
					0.433f;
				state->shape_type_list[state->shape_count] = 
					SHAPE_TYPE_TRAPEZOID;

				state->shape_position_list[state->shape_count].x = 
					position.x;
				state->shape_position_list[state->shape_count].y = 
					position.y;
				state->shape_id_list[state->shape_count] = 
					state->shape_next_id;
				state->shape_adjacency_list[state->shape_count] = -1;
				state->shape_adjacency_list[state->shape_count + 1] = -1;
				state->shape_adjacency_list[state->shape_count + 2] = -1;
				state->shape_adjacency_list[state->shape_count + 3] = -1;
			} break;
			case SHAPE_TYPE_HEXAGON:
			{
				state->shape_vertex_list[6*state->shape_count].x = 
					1.0f;
				state->shape_vertex_list[6*state->shape_count].y =
					0.0f;
				state->shape_vertex_list[6*state->shape_count+1].x = 
					0.5f;
				state->shape_vertex_list[6*state->shape_count+1].y =
					-0.866f;
				state->shape_vertex_list[6*state->shape_count+2].x = 
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+2].y =
					-0.866f;
				state->shape_vertex_list[6*state->shape_count+3].x = 
					-1.0f;
				state->shape_vertex_list[6*state->shape_count+3].y =
					0.0f;
				state->shape_vertex_list[6*state->shape_count+4].x = 
					-0.5f;
				state->shape_vertex_list[6*state->shape_count+4].y =
					0.866f;
				state->shape_vertex_list[6*state->shape_count+5].x = 
					0.5f;
				state->shape_vertex_list[6*state->shape_count+5].y =
					0.866f;
				state->shape_type_list[state->shape_count] = 
					SHAPE_TYPE_HEXAGON;

				state->shape_position_list[state->shape_count].x = 
					position.x;
				state->shape_position_list[state->shape_count].y = 
					position.y;
				state->shape_id_list[state->shape_count] = 
					state->shape_next_id;
				state->shape_adjacency_list[state->shape_count] = -1;
				state->shape_adjacency_list[state->shape_count + 1] = -1;
				state->shape_adjacency_list[state->shape_count + 2] = -1;
				state->shape_adjacency_list[state->shape_count + 3] = -1;
				state->shape_adjacency_list[state->shape_count + 4] = -1;
				state->shape_adjacency_list[state->shape_count + 5] = -1;
			} break;
			default:
			{
				_assert(0);
			} break;
		}
		state->shape_count++;
		state->shape_next_id++;
		return true;
	}
	else
	{
		LOG_WARN("cannot create more shapes. reached maximum of %u",
				MAX_SHAPE_COUNT);
		return false;
	}
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
	u64 jstring_memory_size = 1024;
	u64 game_state_memory_size = sizeof(struct_game_state);
	u64 shape_vertex_memory_size = 6*sizeof(vector_2)*MAX_SHAPE_COUNT;
	u64 shape_adjacency_memory_size = 6*sizeof(i32)*MAX_SHAPE_COUNT;
	u64 shape_type_memory_size = sizeof(i32)*MAX_SHAPE_COUNT;
	u64 shape_id_memory_size = sizeof(u32)*MAX_SHAPE_COUNT;
	u64 shape_position_memory_size = sizeof(vector_2)*MAX_SHAPE_COUNT;

	u64 used_memory = jstring_memory_size + game_state_memory_size +
		shape_vertex_memory_size + shape_type_memory_size +
		shape_adjacency_memory_size + shape_id_memory_size + 
		shape_position_memory_size;
	_assert(game_memory != 0);
	_assert(game_memory_size >= used_memory)

		/* game state */
	struct_game_state *state = (struct_game_state*)(game_memory); 

		/* set up jstring stuff */
		/* TODO: store this in game state? or have a game memory struct */
	void *string_mem = ((char*)game_memory + jstring_memory_size);
	if(!jstring_load_logging_function(LOG_LIB))
	{
		_assert(0);
	}

	if(!jstring_memory_activate(1024, string_mem))
	{
		_assert(0);
	}
	PROFILER_FINISH_TIMING_BLOCK(memory_stuff);

	/* initialize */
	/* NOTE: game code expects platform layer to
	 * initially hand it zeroed out memory 
	 */
	if(!state->initialized)
	{
		LOG_DEBUG("game memory addr: %p", game_memory);
		LOG_DEBUG("game memory size: %u", game_memory_size);
		LOG_DEBUG("used memory     : %u", used_memory);

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

			/* shape data */
		state->shape_count = 0;
		state->shape_next_id = 0;

		/* TODO: make all of these darrays */
		state->shape_vertex_list = (vector_2 *)
			((char*)string_mem + jstring_memory_size);

		state->shape_adjacency_list = (i32 *)
			((char*)(state->shape_vertex_list) + 
			 shape_vertex_memory_size);

		state->shape_id_list = (u32 *)
			((char*)(state->shape_adjacency_list) + 
			shape_adjacency_memory_size);

		state->shape_type_list = (i32 *)
			((char*)(state->shape_id_list) + shape_id_memory_size);

		state->shape_position_list = (vector_2 *)
			((char*)(state->shape_type_list) + shape_type_memory_size);

		LOG_INFO("initialized game state.");
	}

	/* update */
	PROFILER_START_TIMING_BLOCK(update);
	state->time_elapsed = read_os_timer() - state->last_time;	
	state->timer += state->time_elapsed;
	state->last_time = read_os_timer();

	const char *str = "This is a test string";
	jstring test_string = 
		jstring_create_temporary(str, jstring_length(str));
	jstring_to_upper_in_place(&test_string);

	u32 shape_counter;
	for(
		shape_counter = 0; 
		shape_counter < state->shape_count; 
		shape_counter++)
	{
		switch(state->shape_type_list[shape_counter])
		{
			case SHAPE_TYPE_TRIANGLE:
			{
				game_rotate_shape(
					state,
					shape_counter,
		    		(PI / 200.0f));
			} break;
			case SHAPE_TYPE_SQUARE:
			{
				game_rotate_shape(
					state,
					shape_counter,
		    		(PI / 220.0f));
			} break;
			case SHAPE_TYPE_DIAMOND:
			{
				game_rotate_shape(
					state,
					shape_counter,
		    		(PI / 240.0f));
			} break;
			case SHAPE_TYPE_TRAPEZOID:
			{
				game_rotate_shape(
					state,
					shape_counter,
		    		(PI / 260.0f));
			} break;
			case SHAPE_TYPE_HEXAGON:
			{
				game_rotate_shape(
					state,
					shape_counter,
		    		(PI / 280.0f));
			} break;
			default:
			{
				_assert(0);
			} break;
		}
	}

	if(input->letters[16] == INPUT_BUTTON_STATE_PRESSED)
	{
		vector_2 world_pos;
		f32 world_x = ((input->mouse_x) * 
			((state->game_camera.bounds.x)/pixel_buffer_width)) - 
			(state->game_camera.bounds.x/2.0f) - 
			state->game_camera.position.x;
		f32 world_y = ((input->mouse_y) *
			((state->game_camera.bounds.y)/pixel_buffer_height)) - 
			(state->game_camera.bounds.y/2.0f) - 
			state->game_camera.position.y;
		world_pos.x = world_x;
		world_pos.y = world_y;

		if(game_add_shape(
			state,
			world_pos,
			SHAPE_TYPE_TRIANGLE))
		{
			LOG_TRACE("created triangle at:\n"
				"mouse: (%u, %u)\n"
				"shape: (%.2f, %.2f)",
				(input->mouse_x), (input->mouse_y),
				world_pos.x, world_pos.y);
		}
	}
	else if(input->letters[22] == INPUT_BUTTON_STATE_PRESSED)
	{
		vector_2 world_pos;
		f32 world_x = ((input->mouse_x) * 
			((state->game_camera.bounds.x)/pixel_buffer_width)) - 
			(state->game_camera.bounds.x/2.0f) - 
			state->game_camera.position.x;
		f32 world_y = ((input->mouse_y) *
			((state->game_camera.bounds.y)/pixel_buffer_height)) - 
			(state->game_camera.bounds.y/2.0f) - 
			state->game_camera.position.y;
		world_pos.x = world_x;
		world_pos.y = world_y;

		if(game_add_shape(
			state,
			world_pos,
			SHAPE_TYPE_SQUARE))
		{
			LOG_TRACE("created square at:\n"
				"mouse: (%u, %u)\n"
				"shape: (%.2f, %.2f)",
				(input->mouse_x), (input->mouse_y),
				world_pos.x, world_pos.y);
		}
	}
	else if(input->letters[4] == INPUT_BUTTON_STATE_PRESSED)
	{
		vector_2 world_pos;
		f32 world_x = ((input->mouse_x) * 
			((state->game_camera.bounds.x)/pixel_buffer_width)) - 
			(state->game_camera.bounds.x/2.0f) - 
			state->game_camera.position.x;
		f32 world_y = ((input->mouse_y) *
			((state->game_camera.bounds.y)/pixel_buffer_height)) - 
			(state->game_camera.bounds.y/2.0f) - 
			state->game_camera.position.y;
		world_pos.x = world_x;
		world_pos.y = world_y;

		if(game_add_shape(
			state,
			world_pos,
			SHAPE_TYPE_DIAMOND))
		{
			LOG_TRACE("created diamond at:\n"
				"mouse: (%u, %u)\n"
				"shape: (%.2f, %.2f)",
				(input->mouse_x), (input->mouse_y),
				world_pos.x, world_pos.y);
		}
	}
	else if(input->letters[17] == INPUT_BUTTON_STATE_PRESSED)
	{
		vector_2 world_pos;
		f32 world_x = ((input->mouse_x) * 
			((state->game_camera.bounds.x)/pixel_buffer_width)) - 
			(state->game_camera.bounds.x/2.0f) - 
			state->game_camera.position.x;
		f32 world_y = ((input->mouse_y) *
			((state->game_camera.bounds.y)/pixel_buffer_height)) - 
			(state->game_camera.bounds.y/2.0f) - 
			state->game_camera.position.y;
		world_pos.x = world_x;
		world_pos.y = world_y;

		if(game_add_shape(
			state,
			world_pos,
			SHAPE_TYPE_TRAPEZOID))
		{
			LOG_TRACE("created trapezoid at:\n"
				"mouse: (%u, %u)\n"
				"shape: (%.2f, %.2f)",
				(input->mouse_x), (input->mouse_y),
				world_pos.x, world_pos.y);
		}
	}
	else if(input->letters[19] == INPUT_BUTTON_STATE_PRESSED)
	{
		vector_2 world_pos;
		f32 world_x = ((input->mouse_x) * 
			((state->game_camera.bounds.x)/pixel_buffer_width)) - 
			(state->game_camera.bounds.x/2.0f) - 
			state->game_camera.position.x;
		f32 world_y = ((input->mouse_y) *
			((state->game_camera.bounds.y)/pixel_buffer_height)) - 
			(state->game_camera.bounds.y/2.0f) - 
			state->game_camera.position.y;
		world_pos.x = world_x;
		world_pos.y = world_y;

		if(game_add_shape(
			state,
			world_pos,
			SHAPE_TYPE_HEXAGON))
		{
			LOG_TRACE("created hexagon at:\n"
				"mouse: (%u, %u)\n"
				"shape: (%.2f, %.2f)",
				(input->mouse_x), (input->mouse_y),
				world_pos.x, world_pos.y);
		}
	}

	if(input->spacebar == INPUT_BUTTON_STATE_PRESSED)
	{
		state->shape_count = 0;
	}
	PROFILER_FINISH_TIMING_BLOCK(update);

	/* render */
	PROFILER_START_TIMING_BLOCK(render);
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		gray); 

	for(
		shape_counter = 0; 
		shape_counter < state->shape_count; 
		shape_counter++)
	{
		switch(state->shape_type_list[shape_counter])
		{
			case SHAPE_TYPE_TRIANGLE:
			{
				game_draw_shape(
					pixel_buffer, 
					pixel_buffer_width, 
					pixel_buffer_height, 
					state,
					shape_counter,
					magenta);
			} break;
			case SHAPE_TYPE_SQUARE:
			{
				game_draw_shape(
					pixel_buffer, 
					pixel_buffer_width, 
					pixel_buffer_height, 
					state,
					shape_counter,
					cyan);
			} break;
			case SHAPE_TYPE_DIAMOND:
			{
				game_draw_shape(
					pixel_buffer, 
					pixel_buffer_width, 
					pixel_buffer_height, 
					state,
					shape_counter,
					yellow);
			} break;
			case SHAPE_TYPE_TRAPEZOID:
			{
				game_draw_shape(
					pixel_buffer, 
					pixel_buffer_width, 
					pixel_buffer_height, 
					state,
					shape_counter,
					red);
			} break;
			case SHAPE_TYPE_HEXAGON:
			{
				game_draw_shape(
					pixel_buffer, 
					pixel_buffer_width, 
					pixel_buffer_height, 
					state,
					shape_counter,
					blue);
			} break;
			default:
			{
				_assert(0);
			} break;
		}
	}

	vector_2 vertices[12] = {
		{0.5f, 0.5f},
		{-0.5f, 0.5f},
		{0.5f, -0.5f},
		{0.5f, -0.5f},
		{-0.5f, 0.5f},
		{-0.5f, -0.5f},
		{1.5f, 1.5f},
		{0.5f, 1.5f},
		{1.5f, 0.5f},
		{1.5f, 0.5f},
		{0.5f, 1.5f},
		{0.5f, 0.5f}
	};

	vector_4 colors[12]; 
	colors[0] = cyan4;
	colors[1] = magenta4;
	colors[2] = white4;
	colors[3] = white4;
	colors[4] = magenta4;
	colors[5] = yellow4;
	colors[6] = orange4;
	colors[7] = black4;
	colors[8] = white4;
	colors[9] = white4;
	colors[10] = black4;
	colors[11] = cyan4;

	render_mesh squares;
	squares.positions = vertices;
	squares.colors = colors;
	squares.vertex_count = 12;

	vector_2 pos;
	/* TODO: world coords <-> screen coords functions */
	pos.x = (f32)(input->mouse_x * (state->game_camera.bounds.x) / pixel_buffer_width) - 
			(state->game_camera.bounds.x/2.0f) - state->game_camera.position.x;
	pos.y = (f32)(input->mouse_y * (state->game_camera.bounds.y) / pixel_buffer_height) - 
			(state->game_camera.bounds.y/2.0f) - state->game_camera.position.y;

	draw_mesh(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		squares,
		pos,
		state->game_camera);



	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 10, 
		1,
		test_string,
		white);

	PROFILER_FINISH_TIMING_BLOCK(render);
	if(state->timer > 2000000.0)
	{
		finish_and_print_profile(LOG_TRACE);
		state->timer = 0.0;
	}
}

b32 check_collision_aabb(struct_aabb first, struct_aabb second)
{
	/* if first is not to the left, to the right, above, or below second,
	 * then collison
	 */

	b32 left = false;
	b32 right = false;
	b32 above = false;
	b32 below = false;

	if((first.x + first.w) < second.x)
	{
		left = true;
	}
	if((first.y + first.h) < second.y)
	{
		above = true;
	}
	if(first.x > (second.x + second.w))
	{
		right = true;
	}
	if(first.y > (second.y + second.h))
	{
		below = true;
	}
	if(left || above || right || below) 
	{
		return false;
	}
	return true;
}

void game_draw_shape(
		u8 *pixel_buffer, 
		u16 buffer_width, 
		u16 buffer_height, 
		struct_game_state *state,
		u32 shape_index,
		struct_rgba_color color)
{
	switch(state->shape_type_list[shape_index])
	{
		case SHAPE_TYPE_TRIANGLE:
		{
			i32 points[6]; /* NOTE: screen space: x, y, x, y, x, y */
			vector_2 temp[3]; /* NOTE: for doing transformations */
			temp[0] = state->shape_vertex_list[6*shape_index];
			temp[1] = state->shape_vertex_list[6*shape_index+1];
			temp[2] = state->shape_vertex_list[6*shape_index+2];
			
			/* translate to world space by adding shape's position */
			u32 index;
			for(index = 0; index < 3; index++)
			{
				temp[index].x += 
					state->shape_position_list[shape_index].x;
				temp[index].y += 
					state->shape_position_list[shape_index].y;
			}

			/* translate to camera space by subtracting camera origin */
			for(index = 0; index < 3; index++)
			{
				temp[index].x -= state->game_camera.position.x;
				temp[index].y -= state->game_camera.position.y;
			}

			/* translate to screen space */
			for(index = 0; index < 3; index++)
			{
				points[2*index] = 
					temp[index].x * 
						(buffer_width/(state->game_camera.bounds.x)) + 
							(buffer_width/2.0f);
				points[2*index+1] = 
					temp[index].y * 
						(buffer_height/(state->game_camera.bounds.y)) + 
							(buffer_height/2.0f);
			}

			/* draw the shape */
			draw_nofill_polygon_in_buffer(
					pixel_buffer, buffer_width, buffer_height,
					3, points, color);
		} break;
		case SHAPE_TYPE_SQUARE:
		case SHAPE_TYPE_DIAMOND:
		case SHAPE_TYPE_TRAPEZOID:
		{
			i32 points[8]; /* NOTE: screen space: x, y, x, y, x, y */
			vector_2 temp[4]; /* NOTE: for doing transformations */
			temp[0] = state->shape_vertex_list[6*shape_index];
			temp[1] = state->shape_vertex_list[6*shape_index+1];
			temp[2] = state->shape_vertex_list[6*shape_index+2];
			temp[3] = state->shape_vertex_list[6*shape_index+3];
			
			/* translate to world space by adding shape's position */
			u32 index;
			for(index = 0; index < 4; index++)
			{
				temp[index].x += 
					state->shape_position_list[shape_index].x;
				temp[index].y += 
					state->shape_position_list[shape_index].y;
			}

			/* translate to camera space by subtracting camera origin */
			for(index = 0; index < 4; index++)
			{
				temp[index].x -= state->game_camera.position.x;
				temp[index].y -= state->game_camera.position.y;
			}

			/* translate to screen space */
			for(index = 0; index < 4; index++)
			{
				points[2*index] = 
					temp[index].x * 
						(buffer_width/(state->game_camera.bounds.x)) + 
							(buffer_width/2.0f);
				points[2*index+1] = 
					temp[index].y * 
						(buffer_height/(state->game_camera.bounds.y)) + 
							(buffer_height/2.0f);
			}

			/* draw the shape */
			draw_nofill_polygon_in_buffer(
					pixel_buffer, buffer_width, buffer_height,
					4, points, color);
		} break;
		case SHAPE_TYPE_HEXAGON:
		{
			i32 points[12]; /* NOTE: screen space: x, y, x, y, x, y */
			vector_2 temp[6]; /* NOTE: for doing transformations */
			temp[0] = state->shape_vertex_list[6*shape_index];
			temp[1] = state->shape_vertex_list[6*shape_index+1];
			temp[2] = state->shape_vertex_list[6*shape_index+2];
			temp[3] = state->shape_vertex_list[6*shape_index+3];
			temp[4] = state->shape_vertex_list[6*shape_index+4];
			temp[5] = state->shape_vertex_list[6*shape_index+5];
			
			/* translate to world space by adding shape's position */
			u32 index;
			for(index = 0; index < 6; index++)
			{
				temp[index].x += 
					state->shape_position_list[shape_index].x;
				temp[index].y += 
					state->shape_position_list[shape_index].y;
			}

			/* translate to camera space by subtracting camera origin */
			for(index = 0; index < 6; index++)
			{
				temp[index].x -= state->game_camera.position.x;
				temp[index].y -= state->game_camera.position.y;
			}

			/* translate to screen space */
			for(index = 0; index < 6; index++)
			{
				points[2*index] = 
					temp[index].x * 
						(buffer_width/(state->game_camera.bounds.x)) + 
							(buffer_width/2.0f);
				points[2*index+1] = 
					temp[index].y * 
						(buffer_height/(state->game_camera.bounds.y)) + 
							(buffer_height/2.0f);
			}

			/* draw the shape */
			draw_nofill_polygon_in_buffer(
					pixel_buffer, buffer_width, buffer_height,
					6, points, color);
		} break;
		default:
		{
			_assert(0);
		} break;
	}
}

void game_rotate_shape(
		struct_game_state *state,
		u32 shape_index,
		f32 radians)
{
	matrix_2x2 rotation_matrix;
	rotation_matrix.m11 = cos(radians);
	rotation_matrix.m12 = -sin(radians);
	rotation_matrix.m21 = sin(radians);
	rotation_matrix.m22 = cos(radians);

	switch(state->shape_type_list[shape_index])
	{
		case SHAPE_TYPE_TRIANGLE: 
		{
			u32 index;
			for(index = 0; index < 3; index++)
			{
				/* multiply the triangle's vertices by the rotation
				 * matrix
				 */
				state->shape_vertex_list[6*shape_index + index] = 
					multiply_vector_2_by_matrix_2x2(
						(state->shape_vertex_list[6*shape_index + index]),
						rotation_matrix);
			}
		} break;
		case SHAPE_TYPE_SQUARE: 
		case SHAPE_TYPE_DIAMOND:
		case SHAPE_TYPE_TRAPEZOID:
		{
			u32 index;
			for(index = 0; index < 4; index++)
			{
				/* multiply the triangle's vertices by the rotation
				 * matrix
				 */
				state->shape_vertex_list[6*shape_index + index] = 
					multiply_vector_2_by_matrix_2x2(
						(state->shape_vertex_list[6*shape_index + index]),
						rotation_matrix);
			}
		} break;
		case SHAPE_TYPE_HEXAGON:
		{
			u32 index;
			for(index = 0; index < 6; index++)
			{
				/* multiply the triangle's vertices by the rotation
				 * matrix
				 */
				state->shape_vertex_list[6*shape_index + index] = 
					multiply_vector_2_by_matrix_2x2(
						(state->shape_vertex_list[6*shape_index + index]),
						rotation_matrix);
			}
		} break;
		default:
		{
			_assert(0);
		} break;
	}
}
