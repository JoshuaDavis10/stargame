#include <math.h> /* TODO: replace with faster/better sqrt */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

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

typedef enum {
	ENTITY_BIG,
	ENTITY_MELEE,
	ENTITY_RANGED,
	ENTITY_HARVESTER,
	ENTITY_BARRACKS,
	ENTITY_NEXUS,
	ENTITY_ENUM_LENGTH
} entity_type;

struct struct_entity;

typedef struct struct_entity {
	struct struct_entity *aggro_target;
	/* TODO: status: moving, aggroed, etc? */
	/* TODO: range */
	f32 x;
	f32 y;
	f32 progress;
	f32 health;
	f32 target[2];
	entity_type type;
} struct_entity;

/* TODO: temp? */
#define NUM_UNITS 2
#define NUM_ENEMIES 2

typedef struct {
	b32 initialized;
	struct_entity units[NUM_UNITS];
	struct_entity *buildings;
	struct_entity enemies[NUM_ENEMIES];
	struct_entity *selected;
	u64 money;
	f64 last_time;
	f64 time_elapsed;
	/* TODO: delete this */
	f64 bs_variable; 
} struct_game_state;

#define MOVE_SPEED_BIG 1.0f
#define MOVE_SPEED_MELEE 1.0f
#define MOVE_SPEED_RANGED 1.0f

#define GAME_STATE_MEMORY_OFFSET (0)
#define STRING_MEMORY_OFFSET  \
(GAME_STATE_MEMORY_OFFSET + sizeof(struct_game_state)) 

#define STRING_MEMORY_SIZE 1024
#define END_OF_USED_MEMORY_OFFSET \
(GAME_STATE_MEMORY_OFFSET + STRING_MEMORY_OFFSET + STRING_MEMORY_SIZE)

typedef struct {
	i32 x;
	i32 y;
	i32 w;
	i32 h;
} struct_aabb;

/* NOTE: this goin' be useful for a looong time so keeping it */
b32 check_collision_aabb(struct_aabb first, struct_aabb second);

/* NOTE: x and y are center of unit */
void draw_entity_in_buffer(
		u8 *pixel_buffer,
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		struct_entity entity,
		struct_rgba_color color);

void update_entity(struct_entity *entity);

/* TODO: macros */
static struct_rgba_color blue = {0, 0, 255, 0};
static struct_rgba_color white = {255, 255, 255, 0};
static struct_rgba_color red = {255, 0, 0, 0};
static struct_rgba_color black = {0, 0, 0, 0};
static struct_rgba_color purple = {255, 0, 255, 0};

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
	if(!jstring_load_logging_function(LOG_LIB))
	{
		_assert(0);
	}


	if(!jstring_memory_activate(STRING_MEMORY_SIZE, string_mem))
	{
		_assert(0);
	}

	/* initialize */
	if(!state->initialized)
	{
		/* state */
		state->last_time = get_time_ms();

		state->units[0].aggro_target = 0;
		state->units[0].x = 300.0f;
		state->units[0].y = 300.0f;
		state->units[0].target[0] = 400.0f;
		state->units[0].target[1] = 200.0f;
		state->units[0].health = 100.0f;
		state->units[0].progress = 0.0f;
		state->units[0].type = ENTITY_BIG;

		state->units[1].aggro_target = 0;
		state->units[1].x = 500.0f;
		state->units[1].y = 300.0f;
		state->units[1].target[0] = 600.0f; 
		state->units[1].target[1] = 200.0f;
		state->units[1].health = 100.0f;
		state->units[1].progress = 0.0f;
		state->units[1].type = ENTITY_MELEE;

		state->enemies[0].aggro_target = 0;
		state->enemies[0].x = 300.0f;
		state->enemies[0].y = 700.0f;
		state->enemies[0].target[0] = 400.0f;
		state->enemies[0].target[1] = 600.0f;
		state->enemies[0].health = 100.0f;
		state->enemies[0].progress = 0.0f;
		state->enemies[0].type = ENTITY_BIG;

		state->enemies[1].aggro_target = 0;
		state->enemies[1].x = 500.0f;
		state->enemies[1].y = 700.0f;
		state->enemies[1].target[0] = 600.0f;
		state->enemies[1].target[1] = 600.0f;
		state->enemies[1].health = 100.0f;
		state->enemies[1].progress = 0.0f;
		state->enemies[1].type = ENTITY_MELEE;

		/* TODO: temp bs stuff */
		state->bs_variable = 0.0;

		state->initialized = true;
	}

	/* update */

	state->time_elapsed = get_time_ms() - state->last_time;	
	state->last_time = get_time_ms();

	/* TODO: temp bs stuff */
	state->bs_variable += state->time_elapsed; 
	if(state->bs_variable > 500.0)
	{
		state->money++;
		state->bs_variable = 0.0;
	}

	jstring money_string = jstring_create_temporary("MONEY ", 6);
	jstring number_string = jstring_create_integer(state->money);
	jstring_concatenate_jstring(&money_string, number_string);

	i32 unit_counter;
	i32 enemy_counter;

	for(unit_counter = 0; unit_counter < NUM_UNITS; unit_counter++)
	{
		update_entity(&(state->units[unit_counter]));
	}

	for(enemy_counter = 0; enemy_counter < NUM_UNITS; enemy_counter++)
	{
		update_entity(&(state->enemies[enemy_counter]));
	}
	/* TODO: update buildings */

	/* render */
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		black); 

	for(unit_counter = 0; unit_counter < NUM_UNITS; unit_counter++)
	{
		draw_entity_in_buffer(
			pixel_buffer,
			pixel_buffer_width, 
			pixel_buffer_height,
			state->units[unit_counter],
			blue);
	}
	/* TODO: update buildings */

	for(enemy_counter = 0; enemy_counter < NUM_UNITS; enemy_counter++)
	{
		draw_entity_in_buffer(
			pixel_buffer,
			pixel_buffer_width, 
			pixel_buffer_height,
			state->enemies[enemy_counter],
			red);
	}

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		10, 10, 
		2,
		money_string,
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

void draw_entity_in_buffer(
		u8 *pixel_buffer,
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		struct_entity entity,
		struct_rgba_color color)
{
	i32 x = (i32)entity.x;
	i32 y = (i32)entity.y;
	switch(entity.type)
	{
		case ENTITY_BIG:
		{
			/* draw square */
			draw_nofill_rectangle_in_buffer(
					pixel_buffer,
					pixel_buffer_width,
					pixel_buffer_height,
					x - 50, y - 50, 
					100, 100,
					color);
		} break;
		case ENTITY_MELEE:
		{
			/* draw triangle */
			draw_nofill_triangle_in_buffer(
					pixel_buffer,
					pixel_buffer_width,
					pixel_buffer_height,
					x, y - 50, 
					x - 50, y + 50,
					x + 50, y + 50,
					color);
		} break;
		case ENTITY_RANGED:
		{
			/* draw X */
			draw_line_in_buffer(
					pixel_buffer,
					pixel_buffer_width,
					pixel_buffer_height,
					x - 50, y - 50,
					x + 50, y + 50,
					color);
			draw_line_in_buffer(
					pixel_buffer,
					pixel_buffer_width,
					pixel_buffer_height,
					x + 50, y - 50,
					x - 50, y + 50,
					color);
		} break;
		case ENTITY_HARVESTER:
		{
			/* draw diamond */
			i32 points[8] =
				{ x, y + 90,
				  x + 60, y,
				  x, y - 90,
				  x - 60, y };
			draw_nofill_polygon_in_buffer(
					pixel_buffer, 
					pixel_buffer_width, 
					pixel_buffer_height,
					4,
					points, 
					color);

			/* draw health bar */
			draw_nofill_rectangle_in_buffer(
				pixel_buffer,
				pixel_buffer_width, 
				pixel_buffer_height,
				x - 60, y - 130,
				entity.health * 1.2f, 10,
				red);
			/* draw progress bar */
			if(entity.progress > 0.0f)
			{
				draw_nofill_rectangle_in_buffer(
					pixel_buffer,
					pixel_buffer_width, 
					pixel_buffer_height,
					x - 60, y - 160,
					entity.progress * 1.2f, 10,
					purple);
			}
		} break;
		case ENTITY_BARRACKS:
		{
			/* draw circle */
			draw_nofill_circle_in_buffer(
					pixel_buffer,
					pixel_buffer_width,
					pixel_buffer_height,
					x, y, 80.0f,
					color);
		} break;
		case ENTITY_NEXUS:
		{
			/* draw hexagon */
			i32 points[12] =
				{ x + 100, y,
				  x + 50, y + 86,
				  x - 50, y + 86,
				  x - 100, y,
				  x - 50, y - 86,
				  x + 50, y - 86 };
			draw_nofill_polygon_in_buffer(
				pixel_buffer, 
				pixel_buffer_width, 
				pixel_buffer_height,
				6,
				points, 
				color);

			/* draw health bar */
			draw_nofill_rectangle_in_buffer(
				pixel_buffer,
				pixel_buffer_width, 
				pixel_buffer_height,
				x - 80, y - 130,
				entity.health * 1.6f, 10,
				red);
		} break;
		default: 
		{
			_assert(0);
		} break;
	}
}

void update_entity(struct_entity *entity)
{
	/* TODO: if have an aggro target, 
	 *		- if in range, attack
	 *		- else move toward */
	if(entity->aggro_target)
	{
	}
	if(entity->target[0] != -1.0f && entity->target[1] != -1.0f)
	{
		f32 xdist = entity->target[0] - entity->x;
		f32 ydist = entity->target[1] - entity->y;
		f32 magnitude = (f32)sqrt(xdist * xdist + ydist * ydist);
		f32 xdist_unit = xdist / magnitude;
		f32 ydist_unit = ydist / magnitude;

		if(magnitude > 0.5f)
		{
			entity->x += xdist_unit * MOVE_SPEED_BIG;
			entity->y += ydist_unit * MOVE_SPEED_BIG;
		}
		else
		{
			entity->target[0] = -1.0f;
			entity->target[1] = -1.0f;
		}
	}

	/* TODO: if at target, look for aggro targets */
}
