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
} struct_aabb;

typedef struct {
	f32 x;
	f32 y;
	f32 progress;
	f32 health;
	entity_type type;
} struct_entity;

typedef struct {
	/* TODO: initialize these lists */
	struct_entity *nexus;
	struct_entity *units;
	/* TODO: temp, just a harvester to use rn so I don't have to 
	 * initialize a building list yet
	 */
	struct_entity *harvester;
	struct_entity *buildings;
	struct_entity *enemies;
	u64 money;
		
	f64 last_time;
	f64 time_elapsed;
} struct_game_state;

/* NOTE: this goin' be useful for a looong time so keeping it */
b8 check_collision_aabb(struct_aabb first, struct_aabb second);

/* NOTE: x and y are center of unit */
/* TODO: edit this function to take in an entity struct */
/* TODO: this function also needs to draw the health bars/progress
 * bars of the entity it draws
 */
void draw_entity_in_buffer(
		u8 *pixel_buffer,
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		struct_entity entity,
		struct_rgba_color color);

static b8 game_initialized = false;
static struct_game_state *state = 0;
static struct_rgba_color blue = {0, 0, 255, 0};
static struct_rgba_color white = {255, 255, 255, 0};
static struct_rgba_color red = {255, 0, 0, 0};
static struct_rgba_color black = {0, 0, 0, 0};
static struct_rgba_color purple = {255, 0, 255, 0};
static struct_string test_string;

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
	/* initialize */
	if(!game_initialized)
	{
		memset(game_memory, 0, game_memory_size);
		state = (struct_game_state*)game_memory;

		/* TODO: bump allocator of some sort */
		state->nexus = (struct_entity*)(((char*)state) + 
				sizeof(*(state)));
		state->harvester = (struct_entity*)(((char*)state) + 
				sizeof(*(state->nexus)) + sizeof(*(state)));
		game_initialized = true;
		state->last_time = get_time_ms();
		state->nexus->x = 300.0f;
		state->nexus->y = 300.0f;
		state->nexus->progress = 0.0f;
		state->nexus->health = 100.0f;
		state->nexus->type = ENTITY_NEXUS;

		state->harvester->x = 700.0f;
		state->harvester->y = 300.0f;
		state->harvester->progress = 0.0f;
		state->harvester->health = 100.0f;
		state->harvester->type = ENTITY_HARVESTER;

		string_create(&test_string, 
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
	}

	/* update */
	state->time_elapsed = get_time_ms() - state->last_time;	
	state->last_time = get_time_ms();

	state->harvester->progress += state->time_elapsed / 10.0f;
	if(state->harvester->progress > 100.0f)
	{
		state->harvester->progress = 0.0f;
		if(state->nexus->progress <= 95.0f)
		{
			state->nexus->progress += 5.0f;
		}
	}

	/* render */
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		black); 

	draw_entity_in_buffer(
		pixel_buffer,
		pixel_buffer_width,
		pixel_buffer_height,
		*(state->nexus),
		blue);

	draw_entity_in_buffer(
		pixel_buffer,
		pixel_buffer_width,
		pixel_buffer_height,
		*(state->harvester),
		blue);

	draw_text_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		100, 200, 
		5,
		test_string,
		white);
}

b8 check_collision_aabb(struct_aabb first, struct_aabb second)
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

			/* TODO: temp, draw progress bar to keep track
			 * of resources collected
			 */
			if(entity.progress > 0.0f)
			{
				draw_nofill_rectangle_in_buffer(
					pixel_buffer,
					pixel_buffer_width, 
					pixel_buffer_height,
					x - 80, y - 160,
					entity.progress * 1.6f, 10,
					purple);
			}
		} break;
		default: 
		{
			_assert(0);
		} break;
	}
}
