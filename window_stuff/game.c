#include <math.h> /* cosf, sinf, sqrt TODO write your own versions */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define PI 3.14159265f

#include "jstring.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef unsigned int b32;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#include "util.c"

/* TODO: put this into a header */
#define NUM_X_INPUT_BUTTONS 66

typedef struct {
	union {
		struct {
			u8 backspace;
			u8 tab;
			u8 enter;
			u8 escape;
			u8 spacebar;

			u8 left_arrow;
			u8 up_arrow;
			u8 right_arrow;
			u8 down_arrow;

			u8 fkeys[12]; /* NOTE: fkeys[0] is F1 */

			u8 left_shift;
			u8 right_shift;
			u8 left_control;
			u8 right_control;

			u8 letters[26]; /* NOTE: letters[0] is A, letters[25] is Z */

			u8 numbers[10]; /* NOTE: numbers[0] is 0, numbers[9] is 9 */

			u8 mouse_left;
			u8 mouse_right;
			u8 mouse_wheel;
			u8 mouse_wheel_up;
			u8 mouse_wheel_down;
		};
		u8 x_input_buttons[NUM_X_INPUT_BUTTONS];
	};

	i16 mouse_x;
	i16 mouse_y;
} x_input_state;

#include "cpu_render.c"
#include "math.c"

typedef enum {
	SHAPE_TYPE_TRIANGLE,
	SHAPE_TYPE_SQUARE,
	SHAPE_TYPE_DIAMOND,
	SHAPE_TYPE_TRAPEZOID,
	SHAPE_TYPE_HEXAGON,
	SHAPE_TYPE_COUNT
} shape_type;

typedef struct {
	shape_type type;	
	vector_2 position;  /* NOTE: world space coords */
	vector_2 *vertices; /* NOTE: object space coords */
} shape;

typedef struct {
	vector_2 position;
	/* NOTE: -x and +x maximums + -y and +y maximums for camera coords 
	 * that will actually be drawn 
	 */
	vector_2 bounds; 
} camera;

typedef struct {
	shape triangle;
	camera game_camera; /* NOTE: world space coords */
	f64 last_time;
	f64 time_elapsed;
	f64 timer;
	b32 initialized;
} struct_game_state;

#define GAME_STATE_MEMORY_OFFSET (0)

#define STRING_MEMORY_OFFSET  \
(GAME_STATE_MEMORY_OFFSET + sizeof(struct_game_state)) 

#define STRING_MEMORY_SIZE 1024
#define VERTEX_MEMORY_OFFSET \
(STRING_MEMORY_OFFSET + STRING_MEMORY_SIZE)

/* TODO; this is a totally arbitrary number... */
/* NOTE: it's enough memory for VERTEX_MEMORY_SIZE / 8 vertices */
/* e.g. 1024 for VERTEX_MEMORY_SIZE means we can hold 128 vertices */
#define VERTEX_MEMORY_SIZE 1024 
#define END_OF_USED_MEMORY_OFFSET \
(VERTEX_MEMORY_OFFSET + VERTEX_MEMORY_SIZE)

typedef struct {
	i32 x;
	i32 y;
	i32 w;
	i32 h;
} struct_aabb;

/* NOTE: this goin' be useful for a looong time so keeping it */
b32 check_collision_aabb(struct_aabb first, struct_aabb second);

/* TODO: macros */
static struct_rgba_color blue = {0, 0, 255, 0};
static struct_rgba_color white = {255, 255, 255, 0};
static struct_rgba_color red = {255, 0, 0, 0};
static struct_rgba_color black = {0, 0, 0, 0};
static struct_rgba_color purple = {255, 0, 255, 0};

void game_draw_shape(
		u8 *pixel_buffer, 
		u16 buffer_width, 
		u16 buffer_height, 
		shape s,
		camera c,
		struct_rgba_color color);

void game_rotate_shape(
		shape s,
		f32 radians);

/* NOTE: runs every frame */
void game_update_and_render(
		void *game_memory,
		u64 game_memory_size,
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		x_input_state *input) /* NOTE: passing pointer so as not to 
									push massive input struct onto stack
									*/
{
	/* memory stuff */
	_assert(game_memory != 0);
	_assert(game_memory_size > END_OF_USED_MEMORY_OFFSET)
	struct_game_state *state = (struct_game_state*)(game_memory + 
		GAME_STATE_MEMORY_OFFSET);
	void *string_mem = game_memory + STRING_MEMORY_OFFSET;
	vector_2 *vertex_data = game_memory + VERTEX_MEMORY_OFFSET;
	if(!jstring_load_logging_function(LOG_LIB))
	{
		_assert(0);
	}

	if(!jstring_memory_activate(STRING_MEMORY_SIZE, string_mem))
	{
		_assert(0);
	}

	/* initialize */
	/* NOTE: game code expects platform layer to
	 * initially hand it zeroed out memory 
	 */
	if(!state->initialized)
	{
		/* state */
		state->last_time = get_time_ms();
		state->initialized = true;
		state->timer = 0.0;

			/* camera */
		state->game_camera.position.x = 0.0f;
		state->game_camera.position.y = 0.0f;
		state->game_camera.bounds.x = 16.0f;
		state->game_camera.bounds.y = 9.0f;

			/* triangle */
		state->triangle.type = SHAPE_TYPE_TRIANGLE;
		/* NOTE: world space coords for triangle position */
		state->triangle.position.x = 0.0f;
		state->triangle.position.y = 0.0f;
		/* NOTE: object space coords for triangle vertices */
		vertex_data[0].x =  0.0f; /* "top" vertex */
		vertex_data[0].y = -0.433;
		vertex_data[1].x =  0.5f; /* "right" vertex */
		vertex_data[1].y =  0.433;
		vertex_data[2].x = -0.5f; /* "left" vertex */
		vertex_data[2].y =  0.433;
		state->triangle.vertices = vertex_data;
		LOG_INFO("initialized game state.");
	}

	/* update */
	state->time_elapsed = get_time_ms() - state->last_time;	
	state->timer += state->time_elapsed;
	state->last_time = get_time_ms();

	const char *str = "This is a test string";
	jstring test_string = 
		jstring_create_temporary(str, jstring_length(str));
	jstring_to_upper_in_place(&test_string);

	game_rotate_shape(
		state->triangle,
		(PI / 256.0f));


	/* render */
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		black); 

	game_draw_shape(
		pixel_buffer, 
		pixel_buffer_width, 
		pixel_buffer_height, 
		state->triangle,
		state->game_camera,
		purple);



	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 10, 
		2,
		test_string,
		white);
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
		shape s,
		camera c,
		struct_rgba_color color)
{
	switch(s.type)
	{
		case SHAPE_TYPE_TRIANGLE:
		{
			i32 points[6]; /* NOTE: screen space: x, y, x, y, x, y */
			vector_2 temp[3]; /* NOTE: for doing transformations */
			temp[0] = s.vertices[0];
			temp[1] = s.vertices[1];
			temp[2] = s.vertices[2];
			
			/* translate to world space by adding shape's position */
			u32 index;
			for(index = 0; index < 3; index++)
			{
				temp[index].x += s.position.x;
				temp[index].y += s.position.y;
			}

			/* translate to camera space by subtracting camera origin */
			for(index = 0; index < 3; index++)
			{
				temp[index].x -= c.position.x;
				temp[index].y -= c.position.y;
			}

			/* translate to screen space */
			for(index = 0; index < 3; index++)
			{
				points[2*index] = 
					temp[index].x * (buffer_width/c.bounds.x) + 
					(buffer_width/2.0f);
				points[2*index+1] = 
					temp[index].y * (buffer_height/c.bounds.y) + 
					(buffer_height/2.0f);
			}

			/* draw the shape */
			draw_nofill_polygon_in_buffer(
					pixel_buffer, buffer_width, buffer_height,
					3, points, color);
		} break;
		default:
		{
			_assert(0);
		} break;
	}
}

void game_rotate_shape(
		shape s,
		f32 radians)
{
	matrix_2x2 rotation_matrix;
	rotation_matrix.m11 = cos(radians);
	rotation_matrix.m12 = -sin(radians);
	rotation_matrix.m21 = sin(radians);
	rotation_matrix.m22 = cos(radians);
	LOG_DEBUG("matrix:\n"
		"| %.2f %.2f |\n"
		"| %.2f %.2f |",
		rotation_matrix.m11,
		rotation_matrix.m12,
		rotation_matrix.m21,
		rotation_matrix.m22);

	switch(s.type)
	{
		case SHAPE_TYPE_TRIANGLE: 
		{
			u32 index;
			for(index = 0; index < 3; index++)
			{
				/* multiply the triangle's vertices by the rotation
				 * matrix
				 */
				s.vertices[index] = 
					multiply_vector_2_by_matrix_2x2(
						s.vertices[index], rotation_matrix);
			}
		} break;
		default:
		{
			_assert(0);
		} break;
	}
}
