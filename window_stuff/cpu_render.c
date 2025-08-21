typedef struct {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
} rgba_color;

/* TODO: create a .h file for all these as a quick reference */

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

void draw_line_in_buffer(
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

void draw_nofill_triangle_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width,
		u16 buffer_height,
		u32 x1, u32 y1, 
		u32 x2, u32 y2, 
		u32 x3, u32 y3, 
		rgba_color color)
{
	draw_line_in_buffer(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x1, y1,
		x2, y2,
		color);

	draw_line_in_buffer(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x2, y2,
		x3, y3,
		color);

	draw_line_in_buffer(
		pixel_buffer,
		buffer_width,
		buffer_height,
		x1, y1,
		x3, y3,
		color);
}

void draw_fill_rectangle_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width, u16 buffer_height,
		u32 x, u32 y, 
		u32 w, u32 h,
		rgba_color color) 
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
				pixel_buffer[4*pixel] = color.b; 
				pixel_buffer[4*pixel+1] = color.g; 
				pixel_buffer[4*pixel+2] = color.r; 
				pixel_buffer[4*pixel+3] = color.a; 
			}
		}
	}
}

void draw_nofill_rectangle_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width, u16 buffer_height,
		u32 x, u32 y, 
		u32 w, u32 h,
		rgba_color color)
{
	u32 col;
	u32 row;
	i32 pixel;

	/* top */
	for(col = 0; col < w; col++)
	{
		pixel = (y * buffer_width + x) + col;

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}
	}

	/* bottom */
	for(col = 0; col < w; col++)
	{
		pixel = ((y+h) * buffer_width + x) + col;

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}
	}

	/* left */
	for(row = 0; row < h; row++)
	{
		pixel = (y * buffer_width + x) + (row * buffer_width);

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}
	}

	/* right */
	for(row = 0; row < h; row++)
	{
		pixel = (y * buffer_width + x) + (row * buffer_width) + w - 1;

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}
	}
}

void draw_nofill_circle_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width, u16 buffer_height,
		i32 cx, i32 cy, f32 r,
		rgba_color color)
{
	f32 x = 0;
	f32 y = -r;

	f32 mid_y = y + 0.5;

	f32 dist_from_edge; /* TODO: sry can't think of better name rn */

	/* keep incrementing x, and if midpoint is outside circle, 
	 * decrement y 
	 */
	u32 pixel;
	while(x <= -y)
	{
		/* NOTE: all the math is rlly done for the first octant 
		 * then I just flip x/y -/+ for each octant cuz symmetry
		 */

		/* first octant (starting at top) */
		pixel = ((i32)(cy+y) * buffer_width + (i32)(cx+x));

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}

		/* second octant (going clockwise) */
		pixel = ((i32)(cy-x) * buffer_width + (i32)(cx-y));

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}

		/* third octant */
		pixel = ((i32)(cy+x) * buffer_width + (i32)(cx-y));

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}

		/* fourth octant */
		pixel = ((i32)(cy-y) * buffer_width + (i32)(cx+x));

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}

		/* fifth octant */
		pixel = ((i32)(cy-y) * buffer_width + (i32)(cx-x));

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}

		/* sixth octant */
		pixel = ((i32)(cy+x) * buffer_width + (i32)(cx+y));

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}

		/* seventh octant */
		pixel = ((i32)(cy-x) * buffer_width + (i32)(cx+y));

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}

		/* eigth octant */
		pixel = ((i32)(cy+y) * buffer_width + (i32)(cx-x));

		if(pixel >= 0 && pixel < buffer_width * buffer_height) {
			pixel_buffer[4*pixel] = color.b; 
			pixel_buffer[4*pixel+1] = color.g; 
			pixel_buffer[4*pixel+2] = color.r; 
			pixel_buffer[4*pixel+3] = color.a; 
		}

		dist_from_edge = (x * x) + (mid_y * mid_y) - (r * r);
		if(dist_from_edge > 0)
		{
			y += 1.0f;
			mid_y = y + 0.5;
		}
		x += 1.0f;
	}
}
		
void draw_nofill_polygon_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width, u16 buffer_height,
		i32 num_points,
		i32 *points, /* NOTE: x,y,x,y,x,y,etc... */
		rgba_color color)
{
	u32 counter;
	for(counter = 0; counter < num_points - 1; counter++)
	{
		draw_line_in_buffer(
				pixel_buffer,
				buffer_width, buffer_height,
				points[2*counter], points[2*counter+1],
				points[2*(counter+1)], points[2*(counter+1)+1],
				color);
	}
	draw_line_in_buffer(
	   	pixel_buffer,
	   	buffer_width, buffer_height,
		points[2*(num_points-1)], points[2*(num_points-1)+1],
		points[0], points[1],
	   	color);
}
