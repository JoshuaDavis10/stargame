#include <xcb/xcb.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/time.h> /* gettimeofday */
#include <sys/mman.h> /* mmap */
#include <dlfcn.h> /* dlopen dlsym dlclose */

#define FRAME_RATE (60.0f)
#define FRAME_TIME (1.0f/FRAME_RATE)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef unsigned int b32;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#include "util.c"
#include "xcb_input.c" /* NOTE: my xcb input utils file */

/* TODO: platform state struct */
static x_keymap_info x_global_keymap_info = {0};
static u16 x_global_window_width;
static u16 x_global_window_height;
static x_input_state x_global_new_input_state = {0};
static x_input_state x_global_old_input_state = {0};
static x_input_state x_global_game_input_state = {0};
static void *x_global_game_memory_ptr;
static u64 x_global_game_memory_size;
static f64 x_global_timer = 0.0;

void x_generate_game_input()
{
	u8 temp_state;

	u32 counter;
	for(counter = 0; counter < NUM_X_INPUT_BUTTONS; counter++)
	{
		temp_state = 
			x_global_old_input_state.x_input_buttons[counter] << 1;
		temp_state += 
			x_global_new_input_state.x_input_buttons[counter];
		x_global_game_input_state.x_input_buttons[counter] = temp_state;
	}

	x_global_game_input_state.mouse_x = x_global_new_input_state.mouse_x;
	x_global_game_input_state.mouse_y = x_global_new_input_state.mouse_y;
}

int main(int argc, char **argv) 
{
	/*variable to hold pointer to server connection*/
	xcb_connection_t *x_connection;
	xcb_screen_t *x_screen; /*pointer to first screen*/
	const xcb_setup_t *x_setup;
	i32 x_screen_number; /*NOTE: I don't exactly understand this value rn*/
	xcb_screen_iterator_t x_screen_iterator; /*used to iterate over the 
										    *xcb_xcreen_t's
											*for our connection
											*/

	/* create connection to x server */
	x_connection = xcb_connect(NULL, &x_screen_number);
	LOG_INFO("Connected to X server.");

	/* check if connection worked. returns 0 if success */
	i32 res = xcb_connection_has_error(x_connection);
	if(res > 0) {
		LOG_ERROR("xcb_connection_has_error returned %d", res);
		return(1);
	}

	/* setup screen iterator, NOTE: i vaguely remember xcb.h saying
	  not to free the xcb_setup_t pointer that is returned */
	x_setup = xcb_get_setup(x_connection);
	x_screen_iterator = xcb_setup_roots_iterator(x_setup);

	/* loop to first screen */
	for( ; 
		x_screen_iterator.rem; 
		--x_screen_number, xcb_screen_next(&x_screen_iterator)) 
	{
		if(x_screen_number == 0) 
		{
			/* the first screen */
			x_screen = x_screen_iterator.data; 
			break;
		}
	}
	x_global_window_width = x_screen->width_in_pixels;
	x_global_window_height = x_screen->height_in_pixels;

	/*generate id to be passed to xcb_create_window*/
	xcb_window_t x_window_id = xcb_generate_id(x_connection);
	i32 x_win_value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	i32 x_win_value_list[2];
	x_win_value_list[0] = x_screen->black_pixel;

	/* all events to register for */
	x_win_value_list[1] = XCB_EVENT_MASK_EXPOSURE 
						| XCB_EVENT_MASK_BUTTON_PRESS
						| XCB_EVENT_MASK_BUTTON_RELEASE
						| XCB_EVENT_MASK_POINTER_MOTION
						| XCB_EVENT_MASK_ENTER_WINDOW
						| XCB_EVENT_MASK_LEAVE_WINDOW
						| XCB_EVENT_MASK_KEY_PRESS
						| XCB_EVENT_MASK_KEY_RELEASE;

	/*generate the window (see notes.txt for info about parameters)*/
	xcb_create_window(
		x_connection,			/*xcb_connection_t connection*/
	    XCB_COPY_FROM_PARENT,			/*depth of screen 
										  (inherit from root)*/
	    x_window_id,					/*id of window*/
		x_screen->root,					/*id of parent window*/
	    0, 0,						/*x and y*/ 
		x_global_window_width, /*x_screen->width_in_pixels,*/ 
		x_global_window_height, /*x_screen->height_in_pixels,*/		
		10,								/*border width*/
		XCB_WINDOW_CLASS_INPUT_OUTPUT,	/*window class: input/output*/
		XCB_COPY_FROM_PARENT,			/* "visual" (idk what that means)
											(inherit from root) */
		x_win_value_mask,				/*value_mask/value_list*/
		x_win_value_list);
	LOG_INFO("Created X window.");

	/* register for WM_DELETE_WINDOW event */
	/* create internal atom for WM_PROTOCOLS */
	xcb_generic_error_t *x_error;
	xcb_intern_atom_cookie_t x_internal_atom_wm_protocols =
		xcb_intern_atom(x_connection, 1, 12, "WM_PROTOCOLS");

	/* send internal atom */
	xcb_intern_atom_reply_t *x_wm_protocols_reply =
		xcb_intern_atom_reply(
				x_connection, 
				x_internal_atom_wm_protocols,
				&x_error);
	if(x_error) {
		LOG_ERROR("%u", x_error->error_code);
		_assert(0);
	}

	/* get reply */
	xcb_atom_t x_atom_wm_protocols = x_wm_protocols_reply->atom;
	if(!x_atom_wm_protocols) {
		LOG_ERROR("failed to get WM_PROTOCOLS atom");
		return(1);
	}
	free(x_wm_protocols_reply);

	/* create internal atom for WM_DELETE_WINDOW */
	xcb_intern_atom_cookie_t x_internal_atom_wm_delete_window =
		xcb_intern_atom(x_connection, 1, 16, "WM_DELETE_WINDOW");

	/* send internal atom */
	xcb_intern_atom_reply_t *x_wm_delete_window_reply =
		xcb_intern_atom_reply(
				x_connection, 
				x_internal_atom_wm_delete_window,
				&x_error);
	if(x_error) {
		LOG_ERROR("%u", x_error->error_code);
		_assert(0);
	}

	/* get reply */
	xcb_atom_t x_atom_wm_delete_window = x_wm_delete_window_reply->atom;
	if(!x_atom_wm_delete_window) {
		LOG_ERROR("failed to get WM_DELETE_WINDOW atom");
		return(1);
	}
	free(x_wm_delete_window_reply);

	/* change the property (i.e. register for that event I guess */
	xcb_change_property(
			x_connection,
			XCB_PROP_MODE_REPLACE,
			x_window_id,
			x_atom_wm_protocols,
			XCB_ATOM_ATOM,
			32, 1,
			&x_atom_wm_delete_window);

	xcb_intern_atom_cookie_t x_internal_atom_wm_state =
		xcb_intern_atom(x_connection, 1, 13, "_NET_WM_STATE");
	xcb_intern_atom_reply_t *x_wm_state_reply =
		xcb_intern_atom_reply(
				x_connection, 
				x_internal_atom_wm_state,
				&x_error);
	if(x_error) {
		LOG_ERROR("%u", x_error->error_code);
		_assert(0);
	}
	xcb_atom_t x_atom_wm_state = x_wm_state_reply->atom;
	if(!x_atom_wm_state) {
		LOG_ERROR("failed to get _NET_WM_STATE atom");
		return(1);
	}
	free(x_wm_state_reply);
	xcb_intern_atom_cookie_t x_internal_atom_wm_fullscreen =
		xcb_intern_atom(x_connection, 1, 24, "_NET_WM_STATE_FULLSCREEN");
	xcb_intern_atom_reply_t *x_wm_fullscreen_reply =
		xcb_intern_atom_reply(
				x_connection, 
				x_internal_atom_wm_fullscreen,
				&x_error);
	if(x_error) {
		LOG_ERROR("%u", x_error->error_code);
		_assert(0);
	}
	xcb_atom_t x_atom_wm_fullscreen = x_wm_fullscreen_reply->atom;
	if(!x_atom_wm_fullscreen) {
		LOG_ERROR("failed to get _NET_WM_STATE_FULLSCREEN atom");
		return(1);
	}
	free(x_wm_fullscreen_reply);
	xcb_change_property(
			x_connection,
			XCB_PROP_MODE_REPLACE,
			x_window_id,
			x_atom_wm_state,
			XCB_ATOM_ATOM,
			32, 1,
			&x_atom_wm_fullscreen);

	LOG_INFO("Set X window properties.");

	/* load key symbols */
	x_load_key_symbols(&x_global_keymap_info, x_connection, x_setup);

	/* create pixel buffer */
	u8 *x_pixmap_data;
	x_pixmap_data = malloc(sizeof(*x_pixmap_data) *
			(x_global_window_width * x_global_window_height * 4));

	/* create pixmap */
	xcb_pixmap_t x_backbuffer = xcb_generate_id(x_connection);
	xcb_gcontext_t x_graphics_context = xcb_generate_id(x_connection);
	i32 x_gc_values[2] = {x_screen->black_pixel, x_screen->white_pixel};
	xcb_create_pixmap(
			x_connection, 
			x_screen->root_depth,
			x_backbuffer,
			x_window_id,
			x_global_window_width, 
			x_global_window_height);					
	xcb_create_gc(x_connection, x_graphics_context, x_backbuffer,
				XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, x_gc_values);
	LOG_INFO("Set up pixel buffer.");

	u64 pagesize = sysconf(_SC_PAGESIZE);
	LOG_DEBUG("pagesize: %u", pagesize);
	x_global_game_memory_size = 16 * pagesize;

	/* NOTE: for some reason MAP_ANONYMOUS was breaking without
	 * MAP_PRIVATE
	 */
	x_global_game_memory_ptr = 
		mmap(0, x_global_game_memory_size, PROT_READ | PROT_WRITE, 
				MAP_ANONYMOUS | MAP_PRIVATE, 
				/* NOTE: no backing file, just main memory
				  baby */
				-1, 0); /* NOTE: fd should be -1 if MAP_ANONYMOUS
						   for portability apparently? */
	memset(x_global_game_memory_ptr, 0, x_global_game_memory_size);

	xcb_map_window(x_connection, x_window_id);
	LOG_INFO("Mapped X window.");

	/* NOTE: xcb_flush blocks until the write is complete */
	if(xcb_flush(x_connection) < 1) { /*returns <= 0 on failure*/
		LOG_ERROR("xcb_flush failed");
		return(1);
	}


	xcb_generic_event_t *x_event = 0;
	char x_running = 1;

	/* NOTE: temp for timing */
	struct timeval start_timeval;
	struct timeval end_timeval;
	struct timeval post_update_timeval;
	struct timeval temp_timeval;
	struct timeval frame_timeval;
	frame_timeval.tv_sec = 0;
	frame_timeval.tv_usec = FRAME_TIME * 1000000;

	LOG_INFO("Starting render loop...");

	start_timeval = timeval_get();
	while(x_running) 
	{
		f64 start_time_ms = get_time_ms();

		/* get all the events that happened since last loop */
		while((x_event = xcb_poll_for_event(x_connection))) {
			/* NOTE: I have no idea where ~0x80 comes from */
			switch(x_event->response_type & ~0x80) 
			{
				/* NOTE: error */
				case 0:
				{
					
					xcb_generic_error_t *x_error = 
						(xcb_generic_error_t *)x_event;
					LOG_ERROR("\nerror code: %u\nminor code: %u\n"
								"major code: %u\n",
								x_error->error_code,
								x_error->minor_code,
								x_error->major_code
								);
					_assert(0);
				} break;
				case XCB_EXPOSE: 
				{
					LOG_DEBUG("Expose event receieved");
				} break;
				case XCB_KEY_PRESS:
				{
					xcb_key_press_event_t *x_key_press_event = 
						(xcb_key_press_event_t *)x_event;

					/* NOTE: xcb_keycode_t is a uint8_t */
					xcb_keycode_t key_press_event_keycode = 
						x_key_press_event->detail;

					/*NOTE: get the keysym for the keycode*/
					xcb_keysym_t key_press_event_keysym = 
						x_get_keysym_from_keycode(
								x_global_keymap_info,
								key_press_event_keycode);
					x_register_key_stroke(
							&x_global_new_input_state, 
							key_press_event_keysym,
							true);
				} break;
				case XCB_KEY_RELEASE:
				{
					xcb_key_release_event_t *x_key_release_event = 
						(xcb_key_release_event_t *)x_event;

					/* NOTE: xcb_keycode_t is a uint8_t */
					xcb_keycode_t key_release_event_keycode = 
						x_key_release_event->detail;

					/*NOTE: get the keysym for the keycode*/
					xcb_keysym_t key_release_event_keysym = 
						x_get_keysym_from_keycode(
								x_global_keymap_info,
								key_release_event_keycode);

					x_register_key_stroke(
							&x_global_new_input_state, 
							key_release_event_keysym,
							false);
				} break;
				case XCB_BUTTON_PRESS:
				{
					xcb_button_press_event_t *x_mouse_button_press_event =
						(xcb_button_press_event_t *)x_event;

					u8 button = x_mouse_button_press_event->detail;

					x_register_mouse_stroke(
							&x_global_new_input_state,
							button,
							true);

					LOG_DEBUG("pressed button: %u", button);
				} break;
				case XCB_BUTTON_RELEASE:
				{
					xcb_button_release_event_t 
					*x_mouse_button_release_event =
						(xcb_button_release_event_t *)x_event;

					u8 button = x_mouse_button_release_event->detail;

					x_register_mouse_stroke(
							&x_global_new_input_state,
							button,
							false);
					LOG_DEBUG("released button: %u", button);
				} break;
				case XCB_MOTION_NOTIFY:
				{
					xcb_motion_notify_event_t *x_mouse_motion_event =
						(xcb_motion_notify_event_t *)x_event;

					i16 mouse_x = x_mouse_motion_event->event_x;
					i16 mouse_y = x_mouse_motion_event->event_y;

					x_global_new_input_state.mouse_x = mouse_x;
					x_global_new_input_state.mouse_y = mouse_y;
				} break;
				case XCB_ENTER_NOTIFY:
				{
					/* TODO: is this event useful to me at all? */
				} break;
				case XCB_LEAVE_NOTIFY:
				{
					/* TODO: is this event useful to me at all? */
				} break;
				case XCB_CLIENT_MESSAGE:
				{
					xcb_client_message_event_t *x_client_message_event =
						(xcb_client_message_event_t*)x_event;
					
					u32 x_client_message_data32 = 
						x_client_message_event->data.data32[0];
					if(x_client_message_data32 == x_atom_wm_delete_window)
					{
						LOG_DEBUG("WM_DELETE_WINDOW event");
						LOG_INFO("Closing window...");
						x_running = 0;
					}
				} break;
				default: {
							 /*
					LOG_DEBUG("default event:\nresponse type = %u", 
							x_event->response_type); 
							*/
				} break;
			}
			free(x_event); /*NOTE: every loop... seriously*/
		}

		x_generate_game_input();

		struct timeval before_update = timeval_get();

		struct timeval time_before_load = timeval_get();
		void *game_shared_object_handle = dlopen("./libgame.so", RTLD_NOW);
		if(!game_shared_object_handle)
		{
			LOG_ERROR("dlopen: %s", dlerror());
		}
		_assert(game_shared_object_handle);
		void (*game_update_and_render)() = dlsym(
				game_shared_object_handle, 
				"game_update_and_render");
		if(!game_update_and_render)
		{
			LOG_ERROR("dlsym: %s", dlerror());
		}
		_assert(game_update_and_render);
		struct timeval time_after_load = timeval_get();
		struct timeval load_time = 
			timeval_get_difference(
					time_after_load, time_before_load, 0);
		/*
		LOG_DEBUG("loading game code took: %us, %dus", 
				load_time.tv_sec, load_time.tv_usec);
				*/

		game_update_and_render(
				x_global_game_memory_ptr,
				x_global_game_memory_size,
				x_pixmap_data, 
				x_global_window_width,
				x_global_window_height,
	    			&x_global_game_input_state);

		struct timeval after_update = timeval_get();
		struct timeval update_time =
			timeval_get_difference(
					after_update, before_update, 0);

		dlclose(game_shared_object_handle);

		if(x_global_timer > 2000.0)
		{
			LOG_DEBUG("Periodic 2-second snapshot - "
					"game_update_and_render took: %us, %dus",
				update_time.tv_sec,
				update_time.tv_usec);
			x_global_timer = 0.0;
		}

		post_update_timeval = timeval_get();
		temp_timeval = 
			timeval_get_difference(
					post_update_timeval, start_timeval, 0);
		/*
		LOG_DEBUG("Time taken for frame update: %us, %dus",
				temp_timeval.tv_sec,
				temp_timeval.tv_usec);
				*/
		b32 success = true;
		temp_timeval = 
			timeval_get_difference(
					frame_timeval, temp_timeval, &success);
		if(!success)
		{
			LOG_WARN("Dropped frame.");
			start_timeval = timeval_get();
			continue;
		}
		timeval_sleep(temp_timeval); /* NOTE: wait remaining time 
											to present frame */
		end_timeval = timeval_get();
		f64 end_time_ms = get_time_ms();
		x_global_timer += (end_time_ms - start_time_ms);
		temp_timeval = 
			timeval_get_difference(
					end_timeval, start_timeval, 0);
		start_timeval = timeval_get();

		/* NOTE: present frame */
		xcb_put_image(
				x_connection,
				XCB_IMAGE_FORMAT_Z_PIXMAP,
				x_backbuffer,
				x_graphics_context, 
				x_global_window_width,
				x_global_window_height,
				0, 0, 0,
				x_screen->root_depth,
				x_global_window_width * 
				x_global_window_height * 4,
				x_pixmap_data);
		xcb_copy_area (
				x_connection,
				x_backbuffer,
				x_window_id,
				x_graphics_context,
				0, 0, 0, 0,
				x_global_window_width,
				x_global_window_height);
		
		/* flip input state */
		x_global_old_input_state = x_global_new_input_state;
	}

	/* NOTE: these are technically optional I think, but might
	 * as well cleanup
	 */
	LOG_INFO("Window closed.");
	free(x_pixmap_data);
	xcb_destroy_window(x_connection, x_window_id);
	LOG_INFO("Destroyed X window.");
	xcb_disconnect(x_connection);
	LOG_INFO("Disconnected from X server.");
	return 0;
}
