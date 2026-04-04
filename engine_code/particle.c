#include <math.h> /* XXX: for sin function, write your own tho */
#include <stdlib.h> /* XXX: for rand, write your own tho */

/* NOTE(josh): this is all the renderer needs, particle behavior will
 * otherwise be applied to all particles at once every frame */
typedef struct {
	/* XXX: rendering vec4 pixels (see draw_pixel_in_buffer_vec4) 
	 * can currently handle transparency but is slower than rendering
	 * rgba_color (see draw_pixel_in_buffer_rgba)
	 */
	vector_4 color; 
	vector_2 position;
	f64 age;
	/* NOTE(josh): inactive particles should be skipped by the particle
	 * update routine, and should not be rendered.
	 * when a new particle is created, it should "replace" an inactive
	 * one. I know this is a fairly bad system with lots of O(n), but
	 * we're are proof of concepting right now.
	 */
	b32 active;
} particle;

/* XXX: at this point this function is just taking in every value in
 * the particle_source struct */
typedef b32 (*particle_update_routine)(
	particle *particles, 
	u64 particle_count, 
	f64 *timer,
	f64 elapsed_time,
	vector_2 position);

typedef b32 (*particle_render_routine)(
	particle *particles, 
	u64 particle_count, 
	u8 *pixel_buffer, 
	u16 buffer_width, 
	u16 buffer_height,
	camera cam,
	render_mesh *mesh);

typedef struct {
	/* max # of particles, not necessarily current # */
	u64 particle_count; 
	vector_2 position;
	particle_update_routine update_routine;
	particle_render_routine render_routine;
	particle *particles;
} particle_source;

/* XXX: particle source initialization function? prob just hand it 
 * zeroed out memory, and they will all be inactive tho. so not 
 * necessary for now I think */

/* example update routine, particles that fade in and out, and go up */
b32 example_update_routine(
	particle *particles, 
	u64 particle_count, 
	f64 *timer, 
	f64 elapsed_time, 
	vector_2 position)
{
	*timer += elapsed_time;
	/* generate 1 per second */
	f64 particles_per_second = 2.0;
	if(*timer >= 1000000.0 / particles_per_second)
	{
		/* generate new particle */
		u64 particle_index = 0;
		while(particle_index < particle_count)
		{
			if(particles[particle_index].active == false)
			{
				srand((u64)elapsed_time);
				particles[particle_index].active = true;
				particles[particle_index].age = 0;
				particles[particle_index].position.x = 
					position.x - 0.2 + (double)(rand() % 400) / 1000.0;
				particles[particle_index].position.y = 
					position.y - 0.2 + (double)(rand() % 400) / 1000.0;
				break;
			}
			particle_index++;
		}

		*timer = 0.0;
	}

	vector_2 velocity = {0.0f, -0.05f * elapsed_time / 1000000.0};
	/* 5 seconds */
	f64 age_out = 5000000.0;
	u64 particle_index = 0;
	while(particle_index < particle_count)
	{
		/* active ? */
		if(particles[particle_index].active == false)
		{
			particle_index++;
			continue;
		}

		/* aged out ? */
		particles[particle_index].age += elapsed_time;
		if(particles[particle_index].age > age_out)
		{
			particles[particle_index].active = false;
			particle_index++;
			continue;
		}

		/* update position and color */
		particles[particle_index].position =
			add_vec2(particles[particle_index].position, velocity);


		/* TODO: update shouldn't care about color only position, let the render
		 * routine deal with coloring each particle based on it's age or position
		 */
		particles[particle_index].color.x = 0.0f;	
		particles[particle_index].color.y = 
			(f32) (sin(particles[particle_index].age / 1000000.0) / 2.0 + 0.5);	
		particles[particle_index].color.z = 0.0f;	
		/* gives range 0.0 - 1.0 */
		particles[particle_index].color.w = 
			(f32) (sin(particles[particle_index].age / 1000000.0) / 2.0 + 0.5);	


		/* go next particle */
		particle_index++;
	}

	return(true);
}

b32 example_render_routine(
	particle *particles, 
	u64 particle_count,
	u8 *pixel_buffer,
	u16 buffer_width,
	u16 buffer_height,
	camera cam,
	render_mesh *mesh)
{
	u64 particle_index = 0;
	while(particle_index < particle_count)
	{
		if(particles[particle_index].active)
		{
			/* TODO: particles shouldn't store colors, just set the mesh to some value we calculate instead */
			mesh->colors[0].x = particles[particle_index].color.x;
			mesh->colors[0].y = particles[particle_index].color.y;
			mesh->colors[0].z = particles[particle_index].color.z;
			mesh->colors[0].w = particles[particle_index].color.w;
			mesh->colors[1].x = particles[particle_index].color.x;
			mesh->colors[1].y = particles[particle_index].color.y;
			mesh->colors[1].z = particles[particle_index].color.z;
			mesh->colors[1].w = particles[particle_index].color.w;
			mesh->colors[2].x = particles[particle_index].color.x;
			mesh->colors[2].y = particles[particle_index].color.y;
			mesh->colors[2].z = particles[particle_index].color.z;
			mesh->colors[2].w = particles[particle_index].color.w;
			mesh->colors[3].x = particles[particle_index].color.x;
			mesh->colors[3].y = particles[particle_index].color.y;
			mesh->colors[3].z = particles[particle_index].color.z;
			mesh->colors[3].w = particles[particle_index].color.w;
			mesh->colors[4].x = particles[particle_index].color.x;
			mesh->colors[4].y = particles[particle_index].color.y;
			mesh->colors[4].z = particles[particle_index].color.z;
			mesh->colors[4].w = particles[particle_index].color.w;
			mesh->colors[5].x = particles[particle_index].color.x;
			mesh->colors[5].y = particles[particle_index].color.y;
			mesh->colors[5].z = particles[particle_index].color.z;
			mesh->colors[5].w = particles[particle_index].color.w;

			draw_mesh(
				pixel_buffer,
				buffer_width,
				buffer_height,
				*mesh,
				particles[particle_index].position,
				cam);
		}
		particle_index++;
	}
	return(true);
}

/* XXX: particle function used for knucklebones dice 
 * need to have like a "data" variable that's a ptr and size
 * that the update_routine can optionally take in and interpret, so that we
 * could pass in for example the spawn rate/age/speed that we want.
 */
b32 kb_update_routine(
	particle *particles, 
	u64 particle_count, 
	f64 *timer, 
	f64 elapsed_time, 
	vector_2 position)
{
	*timer += elapsed_time;
	/* generate 1 per second */
	f64 particles_per_second = 16.0;
	if(*timer >= 1000000.0 / particles_per_second)
	{
		/* generate new particle */
		u64 particle_index = 0;
		while(particle_index < particle_count)
		{
			if(particles[particle_index].active == false)
			{
				srand((u64)elapsed_time);
				particles[particle_index].active = true;
				particles[particle_index].age = 0;
				particles[particle_index].position.x = 
					position.x - 0.18 + (double)(rand() % 360) / 1000.0;
				particles[particle_index].position.y = 
					position.y + (double)(rand() % 200) / 1000.0;
				break;
			}
			particle_index++;
		}

		*timer = 0.0;
	}

	vector_2 velocity = {0.0f, -0.1f * elapsed_time / 1000000.0};
	/* 5 seconds */
	f64 age_out = 2000000.0;
	u64 particle_index = 0;
	while(particle_index < particle_count)
	{
		/* active ? */
		if(particles[particle_index].active == false)
		{
			particle_index++;
			continue;
		}

		/* aged out ? */
		particles[particle_index].age += elapsed_time;
		if(particles[particle_index].age > age_out)
		{
			particles[particle_index].active = false;
			particle_index++;
			continue;
		}

		/* update position and color */
		particles[particle_index].position =
			add_vec2(particles[particle_index].position, velocity);


		/* TODO: update shouldn't care about color only position, let the render
		 * routine deal with coloring each particle based on it's age or position
		 */

		/* normalize age */
		f32 age_norm = (f32)(particles[particle_index].age / age_out) * 2.0f;
		particles[particle_index].color.x = 1.0f;	
		particles[particle_index].color.y = 0.8f;
		particles[particle_index].color.z = 1.0f;
		/* gives range 0.0 - 1.0 */
		particles[particle_index].color.w = 
			(f32)(-((age_norm - 1.0f) * (age_norm - 1.0f)) + 1.0f);


		/* go next particle */
		particle_index++;
	}

	return(true);
}
