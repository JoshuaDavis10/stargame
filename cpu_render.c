#include <string.h> /* for memcpy */

typedef struct {
	u8 r;
	u8 g;
	u8 b;
	u8 a;
} struct_rgba_color;

/* TODO: macros */
static struct_rgba_color red = {255, 0, 0, 0};
static struct_rgba_color green = {0, 255, 0, 0};
static struct_rgba_color blue = {0, 0, 255, 0};

static struct_rgba_color magenta = {255, 0, 255, 0};
static struct_rgba_color cyan = {0, 255, 255, 0};
static struct_rgba_color yellow = {255, 255, 0, 0};
static struct_rgba_color orange = {255, 150, 0, 0};

static struct_rgba_color white = {255, 255, 255, 0};
static struct_rgba_color black = {0, 0, 0, 0};
static struct_rgba_color gray = {40, 40, 40, 40};

static vector_4 cyan4 = {0.0f, 1.0f, 1.0f, 0.0f};
static vector_4 yellow4 = {1.0f, 1.0f, 0.0f, 0.0f};
static vector_4 magenta4 = {1.0f, 0.0f, 1.0f, 0.0f};
static vector_4 orange4 = {1.0f, 0.5f, 0.0f, 0.0f};

void draw_pixel_in_buffer_rgba(
		u8 *pixel_buffer,
		u16 buffer_width,
		u16 buffer_height,
		u32 pixel,
		struct_rgba_color color)
{
		if(pixel < buffer_width * buffer_height)
		{
			pixel_buffer[4*pixel] = color.b;
			pixel_buffer[4*pixel+1] = color.g;
			pixel_buffer[4*pixel+2] = color.r;
			pixel_buffer[4*pixel+3] = color.a;
		}
}

void draw_pixel_in_buffer_vec4(
		u8 *pixel_buffer,
		u16 buffer_width,
		u16 buffer_height,
		u32 pixel,
		vector_4 color)
{
		if(pixel < buffer_width * buffer_height)
		{
			pixel_buffer[4*pixel] =   (u8)(255.0f * color.z);
			pixel_buffer[4*pixel+1] = (u8)(255.0f * color.y);
			pixel_buffer[4*pixel+2] = (u8)(255.0f * color.x);
			pixel_buffer[4*pixel+3] = (u8)(255.0f * color.w);
		}
}

void draw_triangles_in_buffer(
	u8 *pixel_buffer,
	u16 buffer_width,
	u16 buffer_height,
	camera cam,
	vector_2 *vertex_positions,
	vector_4 *vertex_colors,	
	u32 vertex_count)
{
	vertex_count = vertex_count - (vertex_count % 3);
	f32 max_x;
	f32 max_y;
	f32 min_x;
	f32 min_y;

	vector_2 v0;
	vector_2 v1;
	vector_2 v2;

	vector_4 c0;
	vector_4 c1;
	vector_4 c2;

	u32 vertex_index = 0;
	for( ; vertex_index < vertex_count; vertex_index+=3)
	{
		v0 = vertex_positions[vertex_index];	
		v1 = vertex_positions[vertex_index + 1];	
		v2 = vertex_positions[vertex_index + 2];	

		c0 = vertex_colors[vertex_index];
		c1 = vertex_colors[vertex_index + 1];
		c2 = vertex_colors[vertex_index + 2];

		/* screen coords */
		f32 temp_x;
		f32 temp_y;

		temp_x = v0.x - cam.position.x;
		temp_y = v0.y - cam.position.y;
		v0.x = temp_x * (buffer_width/(cam.bounds.x)) + (buffer_width/2.0f);
		v0.y = temp_y * (buffer_height/(cam.bounds.y)) + (buffer_height/2.0f);
		temp_x = v1.x - cam.position.x;
		temp_y = v1.y - cam.position.y;
		v1.x = temp_x * (buffer_width/(cam.bounds.x)) + (buffer_width/2.0f);
		v1.y = temp_y * (buffer_height/(cam.bounds.y)) + (buffer_height/2.0f);
		temp_x = v2.x - cam.position.x;
		temp_y = v2.y - cam.position.y;
		v2.x = temp_x * (buffer_width/(cam.bounds.x)) + (buffer_width/2.0f);
		v2.y = temp_y * (buffer_height/(cam.bounds.y)) + (buffer_height/2.0f);

		/* get max/min */
		max_x = v0.x;
		if(v1.x > max_x) { max_x = v1.x; }
		if(v2.x > max_x) { max_x = v2.x; }
		min_x = v0.x;
		if(v1.x < min_x) { min_x = v1.x; }
		if(v2.x < max_x) { min_x = v2.x; }
		max_y = v0.y;
		if(v1.y > max_y) { max_y = v1.y; }
		if(v2.y > max_y) { max_y = v2.y; }
		min_y = v0.y;
		if(v1.y < min_y) { min_y = v1.y; }
		if(v2.y < min_y) { min_y = v2.y; }

		i32 aabb_min_x = (i32)min_x;
		i32 aabb_min_y = (i32)min_y;
		i32 aabb_max_x = (i32)max_x;
		i32 aabb_max_y = (i32)max_y;

		i32 x = aabb_min_x;
		i32 y = aabb_min_y;

		vector_2 from;
		vector_2 point;
		vector_2 start_to_point; 
		vector_2 edge;
		vector_2 edge_start;
		vector_2 edge_end;
		f32 det01p;
		f32 det12p;
		f32 det20p;
		for( ; x < aabb_max_x; x++)
		{
			if(x < 0 || x >= buffer_width)
			{
				continue;
			}
			for(y = aabb_min_y ; y < aabb_max_y; y++)
			{
				if(y < 0 || y >= buffer_height)
				{
					continue;
				}
				point.x = (f32)x;
				point.y = (f32)y;

				edge = sub_vec2(v1, v0);
				start_to_point = sub_vec2(point, v0);
				det01p = det_2x2_from_vectors(edge, start_to_point);

				edge = sub_vec2(v2, v1);
				start_to_point = sub_vec2(point, v1);
				det12p = det_2x2_from_vectors(edge, start_to_point);

				edge = sub_vec2(v0, v2);
				start_to_point = sub_vec2(point, v2);
				det20p = det_2x2_from_vectors(edge, start_to_point);

				if(det01p >= 0.0f && det12p >= 0.0f && det20p >= 0.0f)
				{

					f32 det012 = det_2x2_from_vectors( sub_vec2(v1, v0), sub_vec2(v2, v0) );

					f32 lambda0 = det12p / det012;
					f32 lambda1 = det20p / det012;
					f32 lambda2 = det01p / det012;

					vector_4 c0part = mult_vec4_by_const(lambda0, c0);
					vector_4 c1part = mult_vec4_by_const(lambda1, c1);
					vector_4 c2part = mult_vec4_by_const(lambda2, c2);

					vector_4 color = add_vec4(c0part, c1part);
					color = add_vec4(color, c2part);

					/* color the pixel */
					u32 pixel = y * buffer_width + x;
					draw_pixel_in_buffer_vec4(
						pixel_buffer, 
						buffer_width, 
						buffer_height,
						pixel,
						color);
				}
			}
		}
	}
}

void draw_background_in_buffer(
		u8 *pixel_buffer,
		u16 buffer_width, 
		u16 buffer_height,
		struct_rgba_color color) 
{
	pixel_buffer[0] = color.b;
	pixel_buffer[1] = color.g;
	pixel_buffer[2] = color.r;
	pixel_buffer[3] = color.a;

	u64 bytes_in_buffer = buffer_width * buffer_height * 4;
	u64 bytes_copied = 4;
	u64 bytes_next_copy;

	while(bytes_copied < bytes_in_buffer)
	{
		bytes_next_copy = bytes_copied << 1;

		if(bytes_next_copy > bytes_in_buffer)
		{
			bytes_next_copy = bytes_in_buffer;
		}

		memcpy(pixel_buffer + bytes_copied, 
				pixel_buffer, bytes_next_copy - bytes_copied); 	
		bytes_copied = bytes_next_copy;
	}
}

void draw_line_in_buffer(
		u8 *pixel_buffer,
		u16 buffer_width,
		u16 buffer_height,
		i32 x1, i32 y1,
		i32 x2, i32 y2,
		struct_rgba_color color)
{
	i32 xdist = x2 - x1;
	i32 ydist = y2 - y1;

	i8 xdir;
	i8 ydir;

	i32 xdist_abs;
	i32 ydist_abs;

	b32 xdist_greater = false;

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

			draw_pixel_in_buffer_rgba(
					pixel_buffer, buffer_width, 
					buffer_height, pixel, color); 
			
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

			draw_pixel_in_buffer_rgba(
					pixel_buffer, buffer_width, 
					buffer_height, pixel, color); 
			
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
		struct_rgba_color color)
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
		struct_rgba_color color) 
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

			draw_pixel_in_buffer_rgba(
					pixel_buffer, buffer_width, 
					buffer_height, pixel, color); 
		}
	}
}

void draw_nofill_rectangle_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width, u16 buffer_height,
		u32 x, u32 y, 
		u32 w, u32 h,
		struct_rgba_color color)
{
	u32 col;
	u32 row;
	i32 pixel;

	/* top */
	for(col = 0; col < w; col++)
	{
		pixel = (y * buffer_width + x) + col;

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 
	}

	/* bottom */
	for(col = 0; col < w; col++)
	{
		pixel = ((y+h) * buffer_width + x) + col;

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 
	}

	/* left */
	for(row = 0; row < h; row++)
	{
		pixel = (y * buffer_width + x) + (row * buffer_width);

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 
	}

	/* right */
	for(row = 0; row < h; row++)
	{
		pixel = (y * buffer_width + x) + (row * buffer_width) + w - 1;

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 
	}
}

void draw_nofill_circle_in_buffer(
		u8 *pixel_buffer, 
		u16 buffer_width, u16 buffer_height,
		i32 cx, i32 cy, f32 r,
		struct_rgba_color color)
{
	f32 x = 0;
	f32 y = -r;

	f32 mid_y = y + 0.5;

	f32 dist_from_edge; 

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

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 

		/* second octant (going clockwise) */
		pixel = ((i32)(cy-x) * buffer_width + (i32)(cx-y));

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 

		/* third octant */
		pixel = ((i32)(cy+x) * buffer_width + (i32)(cx-y));

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 

		/* fourth octant */
		pixel = ((i32)(cy-y) * buffer_width + (i32)(cx+x));

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 

		/* fifth octant */
		pixel = ((i32)(cy-y) * buffer_width + (i32)(cx-x));

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 

		/* sixth octant */
		pixel = ((i32)(cy+x) * buffer_width + (i32)(cx+y));

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 

		/* seventh octant */
		pixel = ((i32)(cy-x) * buffer_width + (i32)(cx+y));

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 

		/* eigth octant */
		pixel = ((i32)(cy+y) * buffer_width + (i32)(cx-x));

		draw_pixel_in_buffer_rgba(
				pixel_buffer, buffer_width, 
				buffer_height, pixel, color); 

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
		u32 num_points,
		i32 *points, /* NOTE: x,y,x,y,x,y,etc... */
		struct_rgba_color color)
{
	/* draw line from point -> point sequentially */
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

	/* last point -> first point to close polygon */
	draw_line_in_buffer(
	   	pixel_buffer,
	   	buffer_width, buffer_height,
		points[2*(num_points-1)], points[2*(num_points-1)+1],
		points[0], points[1],
	   	color);
}

/* TODO: next iteration of this should have a custom font file format
 * that you come up with that you can load with variable dimensions
 * (i.e. how to handle 10x10 vs 7x7 vs like 10x15 etc...)
 * TODO: potentially use BDF file format or smn down the line. 
 * At some point a robust text rendering system will need to be able to 
 * read common font file formats.
 *
 */
void draw_character_in_buffer(
		u8 *pixel_buffer, u16 buffer_width, u16 buffer_height,
		i32 x, i32 y,
		u8 font_size,
		char character,
		struct_rgba_color color)
{
	/* TODO: 49, because we have a hardcoded font size for now
	 * at some point obviously, I'll want a more complex text
	 * system if I haven't switched to 3D by then although
	 * I'm sure you also have to do nasty stuff for text in 3D
	 */
	b32 pixels[49] = {false}; /* each of these is either 0 or 1 
					 * i.e. drawn or
					 * not drawn, so each switch case will set this
					 * array accordingly and we'll do a loop to draw
					 * the color in the "on"/1 pixels
					 */

	/* 
	 0  1  2  3  4  5  6
	 7  8  9 10 11 12 13
	14 15 16 17 18 19 20
	21 22 23 24 25 26 27
	28 29 30 31 32 33 34
	35 36 37 38 39 40 41
	42 43 44 45 46 47 48
	*/

	switch(character)
	{
		/* NOTE: every character is 7x7 pixels for now */
		case 'A':
		{
			u8 values[18] = {3, 9, 11, 15, 19, 21, 22, 23, 24, 25, 26,
						27, 28, 34, 35, 41, 42, 48};
			u8 counter;
			for(counter = 0; counter < 18; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'B':
		{
			u8 values[26] = {0, 1, 2, 3, 4, 5, 7, 13, 14, 20, 21, 22, 23,
					24, 25, 26, 28, 34, 35, 41, 42, 43, 44, 45, 46, 47};
			u8 counter;
			for(counter = 0; counter < 26; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'C':
		{
			u8 values[15] = {2, 3, 4, 5, 8, 13, 14, 21, 28, 36, 41,
				44, 45, 46, 47};
			u8 counter;
			for(counter = 0; counter < 15; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'D':
		{
			u8 values[20] = {0, 1, 2, 3, 4, 7, 14, 21, 28, 35, 42, 43,
			44, 45, 46, 12, 20, 27, 34, 40};
			u8 counter;
			for(counter = 0; counter < 20; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'E':
		{
			u8 values[24] = {0, 1, 2, 3, 4, 5, 6, 7, 14, 28, 35, 42, 43,
			44, 45, 46, 47, 48, 21, 22, 23, 24, 25, 26};
			u8 counter;
			for(counter = 0; counter < 24; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'F':
		{
			u8 values[18] = {0, 1, 2, 3, 4, 5, 6, 7, 14, 21, 28, 35, 42,
			22, 23, 24, 25, 26};
			u8 counter;
			for(counter = 0; counter < 18; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'G':
		{
			u8 values[22] = {1, 2, 3, 4, 5, 6, 7, 14, 21, 28, 35, 43, 44,
			45, 46, 47, 48, 41, 34, 27, 26, 25};
			u8 counter;
			for(counter = 0; counter < 22; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'H':
		{
			u8 values[19] = {0, 7, 14, 21, 28, 35, 42, 6, 13, 20, 27, 34,
			41, 48, 22, 23, 24, 25, 26};
			u8 counter;
			for(counter = 0; counter < 19; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'I':
		{
			u8 values[19] = {0, 1, 2, 3, 4, 5, 6, 42, 43, 44, 45, 46, 47,
			48, 10, 17, 24, 31, 38};
			u8 counter;
			for(counter = 0; counter < 19; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'J':
		{
			u8 values[14] = {2, 3, 4, 5, 6, 11, 18, 25, 32, 39, 43, 44,
				45, 35};
			u8 counter;
			for(counter = 0; counter < 14; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'K':
		{
			u8 values[16] = {0, 7, 14, 21, 28, 35, 42, 22, 23, 24, 6, 12,
			18, 48, 40, 32};
			u8 counter;
			for(counter = 0; counter < 16; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'L':
		{
			u8 values[13] = {0, 7, 14, 21, 28, 35, 42, 43, 44, 45, 46, 47,
			48};
			u8 counter;
			for(counter = 0; counter < 13; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'M':
		{
			u8 values[19] = {0, 7, 14, 21, 28, 35, 42, 6, 13, 20, 27, 34,
			41, 48, 8, 16, 24, 12, 18};
			u8 counter;
			for(counter = 0; counter < 19; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'N':
		{
			u8 values[19] = {0, 7, 14, 21, 28, 35, 42, 6, 13, 20, 27, 34,
			41, 48, 8, 16, 24, 32, 40};
			u8 counter;
			for(counter = 0; counter < 19; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'O':
		{
			u8 values[20] = {1, 2, 3, 4, 5, 7, 14, 21, 28, 35, 43, 44,
			45, 46, 47, 13, 20, 27, 34, 41};
			u8 counter;
			for(counter = 0; counter < 20; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'P':
		{
			u8 values[19] = {0, 7, 14, 21, 28, 35, 42, 1, 2, 3, 4, 5,
			22, 23, 24, 25, 26, 20, 13};
			u8 counter;
			for(counter = 0; counter < 19; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'Q':
		{
			u8 values[21] = {1, 2, 3, 4, 5, 7, 14, 21, 28, 35, 43, 44, 45,
			46, 13, 20, 27, 34, 48, 40, 32};
			u8 counter;
			for(counter = 0; counter < 21; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'R':
		{
			u8 values[22] = {0, 7, 14, 21, 28, 35, 42, 1, 2, 3, 4, 5,
			22, 23, 24, 25, 26, 20, 13, 48, 40, 32};
			u8 counter;
			for(counter = 0; counter < 22; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'S':
		{
			u8 values[21] = {1, 2, 3, 4, 5, 22, 23, 24, 25, 26, 43, 44, 
			45, 46, 47, 7, 14, 13, 35, 41, 34};
			u8 counter;
			for(counter = 0; counter < 21; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'T':
		{
			u8 values[13] = {0, 1, 2, 3, 4, 5, 6, 10, 17, 24, 31, 38, 45};
			u8 counter;
			for(counter = 0; counter < 13; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'U':
		{
			u8 values[17] = {0, 7, 14, 21, 28, 35, 6, 13, 20, 27, 34, 41,
			43, 44, 45, 46, 47};
			u8 counter;
			for(counter = 0; counter < 17; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'V':
		{
			u8 values[13] = {0, 7, 14, 21, 6, 13, 20, 27, 33, 39, 45,
			37, 29};
			u8 counter;
			for(counter = 0; counter < 13; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'W':
		{
			u8 values[18] = {0, 7, 14, 21, 28, 35, 6, 13, 20, 27, 34, 41,
			43, 37, 31, 24, 39, 47};
			u8 counter;
			for(counter = 0; counter < 18; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'X':
		{
			u8 values[13] = {0, 8, 16, 24, 32, 40, 48, 6, 12, 18, 30, 36,
			42};
			u8 counter;
			for(counter = 0; counter < 13; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'Y':
		{
			u8 values[10] = {0, 8, 16, 6, 12, 18, 24, 31, 38, 45 };
			u8 counter;
			for(counter = 0; counter < 10; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case 'Z':
		{
			u8 values[19] = {0, 1, 2, 3, 4, 5, 6, 42, 43, 44, 45, 46,
			47, 48, 36, 30, 24, 18, 12};
			u8 counter;
			for(counter = 0; counter < 19; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '0':
		{
			u8 values[25] = {1, 2, 3, 4, 5, 7, 14, 21, 28, 35, 43, 44, 
			45, 46, 47, 41, 34, 27, 20, 13, 12, 18, 24, 30, 36};
			u8 counter;
			for(counter = 0; counter < 25; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '1':
		{
			u8 values[16] = {3, 10, 17, 24, 31, 38, 45, 42, 43, 44, 47,
			46, 48, 7, 8, 9};
			u8 counter;
			for(counter = 0; counter < 16; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '2':
		{
			u8 values[19] = {7, 1, 2, 3, 4, 5, 13, 19, 25, 24,
			30, 36, 42, 43, 44, 45, 46, 47, 48};
			u8 counter;
			for(counter = 0; counter < 19; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '3':
		{
			u8 values[19] = {0, 1, 2, 3, 4, 5, 6, 35, 43, 44, 45, 46, 47,
			41, 34, 12, 17, 18, 26};
			u8 counter;
			for(counter = 0; counter < 19; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '4':
		{
			u8 values[16] = {28, 29, 30, 31, 32, 33, 34, 4, 11, 18, 25,
			39, 46, 22, 16, 10};
			u8 counter;
			for(counter = 0; counter < 16; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '5':
		{
			u8 values[23] = {0, 1, 2, 3, 4, 5, 6, 7, 14, 15, 16, 17, 18,
			19, 27, 34, 41, 47, 46, 45, 44, 43, 35};
			u8 counter;
			for(counter = 0; counter < 23; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '6':
		{
			u8 values[21] = {2, 3, 4, 5, 8, 14, 21, 22, 23, 24, 25, 26, 
			28, 35, 34, 41, 43, 44, 45, 46, 47};
			u8 counter;
			for(counter = 0; counter < 21; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '7':
		{
			u8 values[13] = {0, 1, 2, 3, 4, 5, 6, 13, 19, 25, 31, 37, 44};
			u8 counter;
			for(counter = 0; counter < 13; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '8':
		{
			u8 values[23] = {1, 2, 3, 4, 5, 22, 23, 24, 25, 26, 43, 44,
			45, 46, 47, 7, 14, 28, 35, 13, 20, 34, 41};
			u8 counter;
			for(counter = 0; counter < 23; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		case '9':
		{
			u8 values[21] = {1, 2, 3, 4, 5, 22, 23, 24, 25, 26, 27, 7, 14,
			13, 20, 34, 40, 46, 45, 44, 43};
			u8 counter;
			for(counter = 0; counter < 21; counter++)
			{
				pixels[values[counter]] = true;
			}
		} break;
		/* TODO: other characters as needed */
		default:
		{
			_assert(0);
		} break;
	}

	u32 counter;
	u16 row;
	u16 col;
	u32 pixel;
	for(counter = 0; counter < 49; counter++)
	{
		if(pixels[counter])
		{
			row = ((counter / 7) * font_size) + y;
			col = ((counter % 7) * font_size) + x;
			pixel = row * buffer_width + col;
			draw_fill_rectangle_in_buffer(
				pixel_buffer, buffer_width, buffer_height,
				col, row, font_size, font_size, color);
		}
	}
}

void draw_text_in_buffer(
		u8 *pixel_buffer, u16 buffer_width, u16 buffer_height,
		i32 x, i32 y,
		u8 font_size,
		jstring text,
		struct_rgba_color color)
{
	/* TODO: */
	i32 char_x = x;
	i32 char_y = y;
	u32 character;
	for(character = 0; character < text.length; character++)
	{
		switch(text.data[character])
		{
			case ' ':
			{
				char_x += font_size * 7 + font_size;
			} break;
			case '\n':
			{
				char_y += font_size * 7 + 2 * font_size;
				char_x = x;
			} break;
			default:
			{
				draw_character_in_buffer(
					pixel_buffer, buffer_width, buffer_height,
					char_x, char_y,
					font_size,
					text.data[character],
					color);
				char_x += font_size * 7 + font_size;
			} break;
		}
		/* NOTE: we're basically making
		 * every character 7x7 for now
		 */
	}
}
