#include "game.h"
#include "cpu_render.c"

/* TODO: totally temp... nexuses won't move */
#define NEXUS_MOVE_SPEED 0.1f

typedef enum {
	ENTITY_BIG,
	ENTITY_MELEE,
	ENTITY_RANGED,
	ENTITY_HARVESTER,
	ENTITY_BARRACKS,
	ENTITY_NEXUS,
	ENTITY_ENUM_LENGTH
} entity_type;

typedef struct {
	i32 x;
	i32 y;
	i32 w;
	i32 h;
} aabb;

typedef struct {
	f32 nexus_x;
	f32 nexus_y;
	f64 last_time;
	f64 time_elapsed;
} game_state;

/* NOTE: this goin' be useful for a looong time so keeping it */
b8 check_collision_aabb(aabb first, aabb second);

/* NOTE: x and y are center of unit */
void draw_entity(
		u8 *pixel_buffer,
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		i32 x, i32 y, 
		entity_type type, 
		rgba_color color);


static b8 game_initialized = false;
static game_state *state = 0;
static rgba_color blue = {0, 0, 255, 0};
static rgba_color red = {255, 0, 0, 0};
static rgba_color black = {0, 0, 0, 0};
/* NOTE: runs every frame */
void game_update_and_render(
		void *game_memory,
		u64 game_memory_size,
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		game_input_state *input) /* NOTE: passing pointer so as not to 
									push massive input struct onto stack
									*/
{
	if(!game_initialized)
	{
		memset(game_memory, 0, game_memory_size);
		state = (game_state*)game_memory;
		state->nexus_x = 500.0f;
		state->nexus_y = 300.0f;
		game_initialized = true;
		state->last_time = get_time_ms();
	}

	/* "physics" */
	state->time_elapsed = get_time_ms() - state->last_time;	
	if(state->nexus_x < pixel_buffer_width - 100)
	{
		/* NOTE: go ~10 pixels in a second */
		state->nexus_x += (state->time_elapsed) * NEXUS_MOVE_SPEED; 
	}

	state->last_time = get_time_ms();

	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		black); 

	draw_entity(pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		100, 100, ENTITY_BIG, blue);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		300, 100, ENTITY_MELEE, blue);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		100, 300, ENTITY_RANGED, blue);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		300, 300, ENTITY_HARVESTER, blue);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		500, 100, ENTITY_BARRACKS, blue);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		(i32)state->nexus_x, (i32)state->nexus_y, ENTITY_NEXUS, blue);


	draw_entity(pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		100, 500, ENTITY_BIG, red);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		300, 500, ENTITY_MELEE, red);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		100, 700, ENTITY_RANGED, red);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		300, 700, ENTITY_HARVESTER, red);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		500, 500, ENTITY_BARRACKS, red);

	draw_entity( pixel_buffer, pixel_buffer_width, pixel_buffer_height,
		500, 700, ENTITY_NEXUS, red);

}

b8 check_collision_aabb(aabb first, aabb second)
{
	/* if first is not to the left, to the right, above, or below second,
	 * then collison
	 */

	b8 left = false;
	b8 right = false;
	b8 above = false;
	b8 below = false;

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

void draw_entity(
		u8 *pixel_buffer,
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		i32 x, i32 y, 
		entity_type type, 
		rgba_color color)
{
	switch(type)
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
		} break;
		default: 
		{
			_assert(0);
		} break;
	}
}
