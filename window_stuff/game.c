#include "game.h"

#define GAME_SPEED 1 

typedef struct {
	i32 x;
	i32 y;
	i32 w;
	i32 h;
} aabb;

typedef struct {
	i32 x;
	i32 y;
} i32_point;

typedef struct {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
} rgba_color;

/* NOTE: this goin' be useful for a looong time so keeping it */
b8 check_collision_aabb(aabb first, aabb second);

/* TODO: add rendering functions:
 * - pass colors as parameters
 * - current goals is to be able to draw all the pattern block stuff
 *   you had as a kid:
 *		- hexagon
 *		- triangle
 *		- parallelogram (?)
 *		- skinny parallelogram (?)
 *		- square
 *		- trapezoid
 *		- that's all I can remember, but I guess there were the big
 *		rectangle guys, but I can already draw rectangles
 *		- NOTE: since this is currently just all rendering into a
 *		pixel buffer by manually setting pixels, I don't think rotations
 *		are possible. So I'll have a set rotation for each (like facing
 *		"up" I guess)
 *		- NOTE: having a draw_line and like a draw_area (that takes points
 *		as parameters?) would be SUPER helpful for this rather
 *		than like doing manual pixels for each
 *		- NOTE: chance to have some anti-aliasing or is that
 *		wayy to complicated for me right now?
 */

void draw_line_in_buffer_coords(
		u8 *pixel_buffer,
		u16 buffer_width,
		u16 buffer_height,
		i32 x1, i32 y1,
		i32 x2, i32 y2,
		rgba_color color);

void draw_nofill_rectangle_with_lines_in_buffer(
		u8 *pixel_buffer,
		u16 buffer_width, 
		u16 buffer_height,
		u16 x, u16 y,
		u16 w, u16 h,
		rgba_color color);

void draw_nofill_triangle_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width,
		u16 buffer_height,
		u32 x1, u32 y1, 
		u32 x2, u32 y2, 
		u32 x3, u32 y3,
		rgba_color color);

void draw_background_in_buffer(
		u8 *pixel_buffer,
		u16 buffer_width, 
		u16 buffer_height,
		rgba_color color); 

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
	rgba_color bg_color = {0, 0, 0, 0};
	rgba_color line_color = {0, 255, 0, 0};
	rgba_color square_color = {255, 0, 0, 0};
	rgba_color triangle_color = {0, 0, 255, 0};

	/* NOTE: game code here */
	draw_background_in_buffer(
		pixel_buffer,
		pixel_buffer_width, 
		pixel_buffer_height,
		bg_color); 

	draw_nofill_rectangle_with_lines_in_buffer(
		pixel_buffer,
		pixel_buffer_width,
		pixel_buffer_height,
		250, 250,
		100, 100,
		square_color);

	draw_nofill_triangle_in_buffer(
		pixel_buffer,
		pixel_buffer_width,
		pixel_buffer_height,
		600, 100,
		550, 186,
		650, 186,
		triangle_color);
}

void draw_background_in_buffer(
		u8 *pixel_buffer,
		u16 buffer_width, 
		u16 buffer_height,
		rgba_color color) 
{
	u32 pixel;
	for(pixel = 0; pixel < (buffer_width * buffer_height); pixel++) 
	{
		pixel_buffer[4*pixel] = color.b;
		pixel_buffer[4*pixel+1] = color.g;
		pixel_buffer[4*pixel+2] = color.r;
		pixel_buffer[4*pixel+3] = color.a;

	}
}

void draw_line_in_buffer_coords(
		u8 *pixel_buffer,
		u16 buffer_width,
		u16 buffer_height,
		i32 x1, i32 y1,
		i32 x2, i32 y2,
		rgba_color color)
{
	i32 xdist = x2 - x1;
	i32 ydist = y2 - y1;

	i8 xdir;
	i8 ydir;

	i32 xdist_abs;
	i32 ydist_abs;

	b8 xdist_greater = false;

	if(xdist < 0)
	{
		xdist_abs = -xdist;
		xdir = -1;
	}
	else
	{
		xdist_abs = xdist;
		xdir = 1;
	}
	if(ydist < 0)
	{
		ydist_abs = -ydist;
		ydir = -1;
	}
	else
	{
		ydist_abs = ydist;
		ydir = 1;
	}


	if(xdist_abs > ydist_abs)
	{
		xdist_greater = true;
	}

	f32 increment;
	if(xdist_greater)
	{
		increment = (f32)ydist_abs/(f32)xdist_abs;
		i32 pixel_x = x1;
		i32 pixel_y;
		f32 y = (f32)y1;

		u32 pixel;
		do
		{
			pixel_y = (i32)y;
			pixel = pixel_x + pixel_y * buffer_width;

			if(pixel >= 0 && pixel < buffer_width * buffer_height)
			{
				pixel_buffer[4*pixel] = color.b; 
				pixel_buffer[4*pixel+1] = color.g; 
				pixel_buffer[4*pixel+2] = color.r; 
				pixel_buffer[4*pixel+3] = color.a; 
			}
			
			pixel_x += xdir;
			y += ydir * increment;
		}
		while(pixel_x != x2);

		return;
	}
	else
	{
		increment = (f32)xdist_abs/(f32)ydist_abs;
		i32 pixel_y = y1;
		i32 pixel_x;
		f32 x = (f32)x1;

		u32 pixel;
		do
		{
			pixel_x = (i32)x;
			pixel = pixel_x + pixel_y * buffer_width;

			if(pixel >= 0 && pixel < buffer_width * buffer_height)
			{
				pixel_buffer[4*pixel] = color.b; 
				pixel_buffer[4*pixel+1] = color.g; 
				pixel_buffer[4*pixel+2] = color.r; 
				pixel_buffer[4*pixel+3] = color.a; 
			}
			
			pixel_y += ydir;
			x += xdir * increment;
		}
		while(pixel_y != y2);

		return;
	}
}

void draw_nofill_rectangle_with_lines_in_buffer(
		u8 *pixel_buffer,
		u16 buffer_width,
		u16 buffer_height,
		u16 x, u16 y,
		u16 w, u16 h,
		rgba_color color)
{
	i32 x1 = x;
	i32 y1 = y;
	i32 x2 = x + w;
	i32 y2 = y;
	i32 x3 = x;
	i32 y3 = y + h;
	i32 x4 = x + w;
	i32 y4 = y + h;
	
	draw_line_in_buffer_coords(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x1, y1,
		x2, y2,
		color);

	draw_line_in_buffer_coords(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x1, y1,
		x3, y3,
		color);

	draw_line_in_buffer_coords(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x2, y2,
		x4, y4,
		color);

	draw_line_in_buffer_coords(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x3, y3,
		x4, y4,
		color);
}

void draw_nofill_triangle_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width,
		u16 buffer_height,
		u32 x1, u32 y1, 
		u32 x2, u32 y2, 
		u32 x3, u32 y3, 
		rgba_color color)
{
	draw_line_in_buffer_coords(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x1, y1,
		x2, y2,
		color);

	draw_line_in_buffer_coords(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x2, y2,
		x3, y3,
		color);

	draw_line_in_buffer_coords(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x1, y1,
		x3, y3,
		color);
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
