#include "game.h"

#define GAME_SPEED 1 

typedef struct {
	u32 positions[2];	
	i32 ball_position[2];
	i32 ball_direction[2];
} game_state;

typedef struct {
	i32 x;
	i32 y;
	u32 w;
	u32 h;
} rectangle;

static game_state game_global_state = {0};
static b8 game_global_initialized = 0;

b8 check_collision_aabb(rectangle first, rectangle second);

void draw_background_in_buffer(
		u8 *pixel_buffer,
		u16 buffer_width, 
		u16 buffer_height); 

void draw_fill_rectangle_in_buffer(
		u8 *pixel_buffer, 
		u32 x, u32 y, 
		u32 w, u32 h,
		u16 buffer_width, u16 buffer_height);

void draw_nofill_rectangle_in_buffer(
		u8 *pixel_buffer, 
		u32 x, u32 y, 
		u32 w, u32 h,
		u16 buffer_width, u16 buffer_height);

/* NOTE: runs every frame */
void game_update_and_render(
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		game_input_state *input) /* NOTE: passing pointer so as not to 
									push massive input struct onto stack
									*/
{
	if(!game_global_initialized)
	{
		game_global_state.positions[0] = pixel_buffer_height/2;
		game_global_state.positions[1] = pixel_buffer_height/2;
		game_global_state.ball_position[0] = pixel_buffer_width/2;
		game_global_state.ball_position[1] = pixel_buffer_height/2;
		game_global_state.ball_direction[0] = GAME_SPEED;
		game_global_state.ball_direction[1] = GAME_SPEED;
		game_global_initialized = true;
	}

	game_global_state.ball_position[0]+=
		game_global_state.ball_direction[0];
	game_global_state.ball_position[1]+=
		game_global_state.ball_direction[1];

	/* NOTE: collision detection */
	rectangle left_paddle;
	rectangle right_paddle;
	rectangle ball;
	left_paddle.x = 100;
	left_paddle.y = game_global_state.positions[0] - 100;
	left_paddle.w = 50;
	left_paddle.h = 200;
	right_paddle.x = pixel_buffer_width - 100;
	right_paddle.y = game_global_state.positions[1] - 100;
	right_paddle.w = 50;
	right_paddle.h = 200;
	ball.x = game_global_state.ball_position[0];
	ball.y = game_global_state.ball_position[1];
	ball.w = 50;
	ball.h = 50;
	i32 ydist;
	i32 xdist;
	if(check_collision_aabb(left_paddle, ball))
	{
		if(left_paddle.y > ball.y) 
		{
			xdist = left_paddle.x - ball.x;
			if(xdist < 0) { xdist = -xdist; }
			ydist = left_paddle.y - ball.y;
			if(ydist < 0) { ydist = -ydist; }
			if(xdist > ydist) 
			{
				game_global_state.ball_direction[0] =
					-game_global_state.ball_direction[0]; 
			}
			else {
				game_global_state.ball_direction[1] =
					-GAME_SPEED; 
			}
		}
		else if((left_paddle.y + left_paddle.h) < (ball.y + ball.h))
		{
			xdist = left_paddle.x - ball.x;
			if(xdist < 0) { xdist = -xdist; }
			ydist = (left_paddle.y + left_paddle.h) - 
				(ball.y + ball.h);
			if(ydist < 0) { ydist = -ydist; }
			if(xdist > ydist) 
			{
				game_global_state.ball_direction[0] =
					-game_global_state.ball_direction[0]; 
			}
			else {
				game_global_state.ball_direction[1] =
					GAME_SPEED; 
			}
		}
		else
		{
			game_global_state.ball_direction[0] =
				-game_global_state.ball_direction[0]; 
		}
	}
	if(check_collision_aabb(right_paddle, ball))
	{
		/* ball is above paddle */
		if(right_paddle.y > ball.y) 
		{
			xdist = right_paddle.x - ball.x;
			if(xdist < 0) { xdist = -xdist; }
			ydist = right_paddle.y - ball.y;
			if(ydist < 0) { ydist = -ydist; }
			if(xdist > ydist) 
			{
				game_global_state.ball_direction[0] =
					-game_global_state.ball_direction[0]; 
			}
			else
			{
				game_global_state.ball_direction[1] =
					-GAME_SPEED; 
			}
		}
		/* ball is below paddle */
		else if((right_paddle.y + right_paddle.h) < (ball.y + ball.h))
		{
			xdist = right_paddle.x - ball.x;
			if(xdist < 0) { xdist = -xdist; }
			ydist = (right_paddle.y + right_paddle.h) - 
				(ball.y + ball.h);
			if(ydist < 0) { ydist = -ydist; }
			if(xdist > ydist) 
			{
				game_global_state.ball_direction[0] =
					-game_global_state.ball_direction[0]; 
			}
			else {
				game_global_state.ball_direction[1] =
					GAME_SPEED; 
			}
		}
		else
		{
			game_global_state.ball_direction[0] =
				-game_global_state.ball_direction[0]; 
		}
	}

	if((game_global_state.ball_position[1] + 50) >= 
			pixel_buffer_height)
	{
		game_global_state.ball_direction[1] =
			-game_global_state.ball_direction[1]; 
	}
	if((game_global_state.ball_position[1]) <= 
			0)
	{
		game_global_state.ball_direction[1] =
			-game_global_state.ball_direction[1]; 
	}
	if((game_global_state.ball_position[0]) >= 
			pixel_buffer_width - 50)
	{
		game_global_state.ball_position[0] = pixel_buffer_width/2; 
		game_global_state.ball_position[1] = pixel_buffer_height/2; 
	}
	if((game_global_state.ball_position[0]) <= 
			0)
	{
		game_global_state.ball_position[0] = pixel_buffer_width/2; 
		game_global_state.ball_position[1] = pixel_buffer_height/2; 
	}

	if(input->letters[22]) 
	{
		if(!check_collision_aabb(left_paddle, ball))
		{
			if(game_global_state.positions[0] > 100)
			{
				game_global_state.positions[0]-=GAME_SPEED;
			}
		}
	}
	else if(input->letters[18])
	{
		if(!check_collision_aabb(left_paddle, ball))
		{
			if(game_global_state.positions[0] < 400)
			{
				game_global_state.positions[0]+=GAME_SPEED;
			}
		}
	}

	if(input->letters[8]) 
	{
		if(!check_collision_aabb(right_paddle, ball))
		{
			if(game_global_state.positions[1] > 100)
			{
				game_global_state.positions[1]-=GAME_SPEED;
			}
		}
	}
	else if(input->letters[10])
	{
		if(!check_collision_aabb(right_paddle, ball))
		{
			if(game_global_state.positions[1] < 400)
			{
				game_global_state.positions[1]+=GAME_SPEED;
			}
		}
	}

	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height);

	draw_fill_rectangle_in_buffer(
			pixel_buffer,
			100, 
			game_global_state.positions[0] - 100, 
			50, 200,
			pixel_buffer_width,
			pixel_buffer_height);
	draw_nofill_rectangle_in_buffer(
			pixel_buffer,
			100, 
			game_global_state.positions[0] - 100, 
			50, 200,
			pixel_buffer_width,
			pixel_buffer_height);

	draw_fill_rectangle_in_buffer(
			pixel_buffer,
			pixel_buffer_width - 100, 
			game_global_state.positions[1] - 100, 
			50, 200,
			pixel_buffer_width,
			pixel_buffer_height);

	draw_nofill_rectangle_in_buffer(
			pixel_buffer,
			pixel_buffer_width - 100, 
			game_global_state.positions[1] - 100, 
			50, 200,
			pixel_buffer_width,
			pixel_buffer_height);


	draw_fill_rectangle_in_buffer(
			pixel_buffer,
			game_global_state.ball_position[0], 
			game_global_state.ball_position[1], 
			50, 50,
			pixel_buffer_width,
			pixel_buffer_height);

	draw_nofill_rectangle_in_buffer(
			pixel_buffer,
			game_global_state.ball_position[0], 
			game_global_state.ball_position[1], 
			50, 50,
			pixel_buffer_width,
			pixel_buffer_height);
}

void draw_background_in_buffer(
		u8 *pixel_buffer,
		u16 buffer_width, 
		u16 buffer_height) 
{
	u32 pixel;
	for(pixel = 0; pixel < (buffer_width * buffer_height); pixel++) 
	{
		pixel_buffer[4*pixel] = 100;
		pixel_buffer[4*pixel+1] = 100;
		pixel_buffer[4*pixel+2] = 0;
		pixel_buffer[4*pixel+3] = 0;

	}
}

void draw_fill_rectangle_in_buffer(
		u8 *pixel_buffer, 
		u32 x, u32 y, 
		u32 w, u32 h,
		u16 buffer_width, u16 buffer_height) 
{
	u32 col;
	u32 row;
	i32 pixel;
	for(row = 0; row < h; row++) 
	{
		for(col = 0; col < w; col++) 
		{
			pixel = (y * buffer_width + x) + 
					(row * buffer_width + col);	
			if(pixel >= 0 && pixel < buffer_width * buffer_height)
			{
				pixel_buffer[4*pixel] = 100; 
				pixel_buffer[4*pixel+1] = 0; 
				pixel_buffer[4*pixel+2] = 100; 
				pixel_buffer[4*pixel+3] = 0; 
			}
		}
	}
}

void draw_nofill_rectangle_in_buffer(
		u8 *pixel_buffer, 
		u32 x, u32 y, 
		u32 w, u32 h,
		u16 buffer_width, u16 buffer_height) 
{
	u32 col;
	u32 row;
	i32 pixel;

	/* top */
	for(col = 0; col < w; col++)
	{
		pixel = (y * buffer_width + x) + col;

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = 0; 
			pixel_buffer[4*pixel+1] = 0; 
			pixel_buffer[4*pixel+2] = 255; 
			pixel_buffer[4*pixel+3] = 0; 
		}
	}

	/* bottom */
	for(col = 0; col < w; col++)
	{
		pixel = ((y+h) * buffer_width + x) + col;

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = 0; 
			pixel_buffer[4*pixel+1] = 0; 
			pixel_buffer[4*pixel+2] = 255; 
			pixel_buffer[4*pixel+3] = 0; 
		}
	}

	/* left */
	for(row = 0; row < h; row++)
	{
		pixel = (y * buffer_width + x) + (row * buffer_width);

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = 0; 
			pixel_buffer[4*pixel+1] = 0; 
			pixel_buffer[4*pixel+2] = 255; 
			pixel_buffer[4*pixel+3] = 0; 
		}
	}

	/* right */
	for(row = 0; row < h; row++)
	{
		pixel = (y * buffer_width + x) + (row * buffer_width) + w - 1;

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = 0; 
			pixel_buffer[4*pixel+1] = 0; 
			pixel_buffer[4*pixel+2] = 255; 
			pixel_buffer[4*pixel+3] = 0; 
		}
	}
}

b8 check_collision_aabb(rectangle first, rectangle second)
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
