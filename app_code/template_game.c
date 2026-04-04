#include "game.h"

#include "util.c"
#include "profiler.c"
#include "math.c"

#include "jstring.h"

typedef struct {
	vector_2 position;
	/* NOTE: -x and +x maximums + -y and +y maximums for camera coords 
	 * that will actually be drawn 
	 */
	vector_2 bounds; 
} camera;

#include "cpu_render.c"
#include "particle.c"

static void *game_memory_allocate(u64 *used_memory, u64 size, void *game_memory, u64 game_memory_size)
{
	_assert((*used_memory) + size <= game_memory_size);
	void *result = (void*) ((char*)game_memory + (*used_memory));
	(*used_memory) += size;
	return result;
}

#ifdef __linux__

void game_update_and_render(
		void *game_memory,
		u64 game_memory_size,
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		input_state *input,
		char *level_filename) 

#endif

#ifdef _WIN32

__declspec(dllexport) void game_update_and_render(
		void *game_memory,
		u64 game_memory_size,
		u8 *pixel_buffer, 
		u16 pixel_buffer_width,
		u16 pixel_buffer_height,
		input_state *input,
		char *level_filename) 

#endif
{
	u64 particle_count = 1024;
	u64 used_memory = 0;
	particle_source particle_system; 
	particle_system.particle_count = particle_count;
	particle_system.position.x = 0.0f;
	particle_system.position.y = 0.0f;
	particle_system.update_routine = example_update_routine;
	particle_system.render_routine = example_render_routine;
	particle_system.particles = 
		(particle *)game_memory_allocate(
			&used_memory, 
			particle_count * sizeof(particle), 
			game_memory,
			game_memory_size);

	particle_source particle_system_2; 
	particle_system_2.particle_count = particle_count;
	particle_system_2.position.x = 1.0f;
	particle_system_2.position.y = 1.0f;
	particle_system_2.update_routine = kb_update_routine;
	particle_system_2.render_routine = example_render_routine;
	particle_system_2.particles = 
		(particle *)game_memory_allocate(
			&used_memory, 
			particle_count * sizeof(particle), 
			game_memory,
			game_memory_size);


	f64 *timer = 
		(f64*)game_memory_allocate(
			&used_memory, 
			sizeof(f64),
			game_memory,
			game_memory_size);

	f64 *timer_2 = 
		(f64*)game_memory_allocate(
			&used_memory, 
			sizeof(f64),
			game_memory,
			game_memory_size);

	f64 *last_time = 
		(f64*)game_memory_allocate(
			&used_memory, 
			sizeof(f64),
			game_memory,
			game_memory_size);

	camera cam;
	cam.bounds.x = 8.0;
	cam.bounds.y = 4.5;
	cam.position.x = 0.0;
	cam.position.y = 0.0;

	vector_2 positions[6] =
	{
		{-0.02f,-0.02f}, 
		{ 0.02f,-0.02f}, 
		{ 0.02f, 0.02f}, 
		{ 0.02f, 0.02f}, 
		{-0.02f, 0.02f},
		{-0.02f,-0.02f} 
	};

	vector_4 colors[6] =
	{
		{0.0f, 0.0f, 1.0f, 1.0f}, 
		{0.0f, 0.0f, 1.0f, 1.0f}, 
		{0.0f, 0.0f, 1.0f, 1.0f}, 
		{0.0f, 0.0f, 1.0f, 1.0f}, 
		{0.0f, 0.0f, 1.0f, 1.0f}, 
		{0.0f, 0.0f, 1.0f, 1.0f} 
	};

	render_mesh particle_mesh;
	particle_mesh.vertex_count = 6;
	particle_mesh.positions = positions;
	particle_mesh.colors = colors;

	vector_4 colors_2[12] =
	{
		{1.0f, 0.0f, 0.7f, 1.0f}, 
		{1.0f, 0.0f, 0.7f, 1.0f}, 
		{1.0f, 0.0f, 0.7f, 1.0f}, 
		{1.0f, 0.0f, 0.7f, 1.0f}, 
		{1.0f, 0.0f, 0.7f, 1.0f}, 
		{1.0f, 0.0f, 0.7f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f}, 
		{1.0f, 1.0f, 1.0f, 1.0f}, 
		{1.0f, 1.0f, 1.0f, 1.0f}, 
		{1.0f, 1.0f, 1.0f, 1.0f}, 
		{1.0f, 1.0f, 1.0f, 1.0f}, 
		{1.0f, 1.0f, 1.0f, 1.0f} 
	};

	vector_2 positions_2[12] =
	{
		{-0.2f,-0.2f}, 
		{ 0.2f,-0.2f}, 
		{ 0.2f, 0.2f}, 
		{ 0.2f, 0.2f}, 
		{-0.2f, 0.2f},
		{-0.2f,-0.2f},
		{ 0.0f, -0.05f},
		{ 0.05f, 0.0f},
		{ 0.0f,  0.05f},
		{ 0.0f,  0.05f},
		{-0.05f, 0.0f},
		{ 0.0f, -0.05f}
	};

	render_mesh square_mesh;
	square_mesh.vertex_count = 12;
	square_mesh.positions = positions_2;
	square_mesh.colors = colors_2;

	vector_2 square_pos = {1.0f, 1.0f};

	/*
	start_profile();

	PROFILER_START_TIMING_BLOCK(game_update_and_render);
	*/

	f64 elapsed_time = read_os_timer() - (*last_time);
	*last_time = read_os_timer();

	draw_background_in_buffer(pixel_buffer, pixel_buffer_width, pixel_buffer_height, gray);	


	draw_mesh(
		pixel_buffer, 
		pixel_buffer_width, 
		pixel_buffer_height, 
		square_mesh, 
		square_pos, 
		cam);

	particle_system.update_routine(
		particle_system.particles,
		particle_system.particle_count,
		timer,
		elapsed_time,
		particle_system.position);

	particle_system.render_routine(
		particle_system.particles,
		particle_system.particle_count,
		pixel_buffer,
		pixel_buffer_width,
		pixel_buffer_height,
		cam,
		&particle_mesh);

	particle_system_2.update_routine(
		particle_system_2.particles,
		particle_system_2.particle_count,
		timer_2,
		elapsed_time,
		particle_system_2.position);

	particle_system_2.render_routine(
		particle_system_2.particles,
		particle_system_2.particle_count,
		pixel_buffer,
		pixel_buffer_width,
		pixel_buffer_height,
		cam,
		&particle_mesh);

	/*
	PROFILER_FINISH_TIMING_BLOCK(game_update_and_render);
	finish_and_print_profile(log_trace);
	*/
}
