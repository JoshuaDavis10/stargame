#include <xcb/xcb.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef char b8;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#include "util.c"
#include "xcb_input.c" /* NOTE: my xcb input utils file */

static x_keymap_info x_global_keymap_info = {0};
static u16 x_global_window_width = 1000;
static u16 x_global_window_height = 500;

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

	/*NOTE: temp stuff to make sure screen iterator code stuff works*/
	printf ("\n");
	printf ("Informations of screen %d:\n", x_screen->root);
	printf ("  width.........: %d\n", x_screen->width_in_pixels);
	printf ("  height........: %d\n", x_screen->height_in_pixels);
	printf ("  white pixel...: %d\n", x_screen->white_pixel);
	printf ("  black pixel...: %d\n", x_screen->black_pixel);
	printf ("\n");

	/*generate id to be passed to xcb_create_window*/
	xcb_window_t x_window_id = xcb_generate_id(x_connection);

	/* NOTE: xcb_generate_id returns an unsigned int so idk
	 * why the documentation/comments in header say that it returns
	 * -1 on failure 
	if(x_window_id == -1) {
		LOG_ERROR("xcb_generate_id failed");
		return(1);
	}
	*/

	i32 x_win_value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	i32 x_win_value_list[2];
	x_win_value_list[0] = x_screen->white_pixel;

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

	/* set to full screen */
	/*
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
	*/

	/* load key symbols */
	x_load_key_symbols(&x_global_keymap_info, x_connection, x_setup);

	/* NOTE: fill pixmap data */
	u8 *x_pixmap_data;
	x_pixmap_data = malloc(sizeof(*x_pixmap_data) *
			(x_global_window_width * x_global_window_height * 4));
	u32 pixel;
	u32 row;
	u32 col;
	for(pixel = 0; 
			pixel < (x_global_window_width * x_global_window_height);
			pixel++) 
	{
				row = pixel / x_global_window_width;
				col = pixel % x_global_window_width;
				x_pixmap_data[4*pixel] = row;
				x_pixmap_data[4*pixel+1] = 0;
				x_pixmap_data[4*pixel+2] = col;
				x_pixmap_data[4*pixel+3] = 0;
	}

	/* NOTE: create pixmap and put pixel array into it */
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

	xcb_put_image(
			x_connection,
			XCB_IMAGE_FORMAT_Z_PIXMAP,
			x_backbuffer,
			x_graphics_context, 
			x_global_window_width,
			x_global_window_height,
			0, 0, 0,
			x_screen->root_depth,
			x_global_window_width * x_global_window_height * 4,
			x_pixmap_data);

	xcb_map_window(x_connection, x_window_id);

	/* NOTE: xcb_flush blocks until the write is complete */
	if(xcb_flush(x_connection) < 1) { /*returns <= 0 on failure*/
		LOG_ERROR("xcb_flush failed");
		return(1);
	}

	xcb_generic_event_t *x_event = 0;
	char x_running = 1;
	while(x_running) 
	{
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
					xcb_copy_area (
							x_connection,
							x_backbuffer,
							x_window_id,
							x_graphics_context,
							0, 0, 0, 0,
							x_global_window_width,
							x_global_window_height);

					xcb_flush(x_connection);
					LOG_DEBUG("Expose event receieved");
				} break;
				case XCB_KEY_PRESS:
				{
					/* TODO: merge press/release events */
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

					x_print_keysym(key_press_event_keysym); 
					printf("pressed\n");

					/* TODO: create some input state that is 
					 * changed by key press events
					 */

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

					x_print_keysym(key_release_event_keysym); 
					printf("released\n");

					/* TODO: create some input state that is 
					 * changed by key events
					 */

				} break;
				case XCB_BUTTON_PRESS:
				{
					/* TODO: merge press/release events */
					xcb_button_press_event_t *x_mouse_button_press_event =
						(xcb_button_press_event_t *)x_event;

					x_print_mouse_button(
							x_mouse_button_press_event->detail);

					printf("button pressed\n");

					/* TODO: create some input state that is 
					 * changed by mouse button events
					 */

				} break;
				case XCB_BUTTON_RELEASE:
				{
					xcb_button_release_event_t 
					*x_mouse_button_release_event =
						(xcb_button_release_event_t *)x_event;

					x_print_mouse_button(
							x_mouse_button_release_event->detail);

					printf("button released\n");

				} break;
				case XCB_MOTION_NOTIFY:
				{
					xcb_motion_notify_event_t *x_mouse_motion_event =
						(xcb_motion_notify_event_t *)x_event;

					i16 mouse_x = x_mouse_motion_event->event_x;
					i16 mouse_y = x_mouse_motion_event->event_y;

					LOG_DEBUG("mouse move; pos: %d, %d", mouse_x, 
							mouse_y);

					/* TODO: create some input state that is 
					 * changed by mouse move events
					 */
				} break;
				case XCB_ENTER_NOTIFY:
				{
					/* NOTE: I mean there's not really any info to 
					 * get here right? just that it happened? ... 
					 * oh wait I guess you'd want to store the mouse pos
					 * but doesn't that happen in motion anyways? wtf
					 */
					LOG_DEBUG("mouse entered window");
					
				} break;
				case XCB_LEAVE_NOTIFY:
				{
					LOG_DEBUG("mouse left window");
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
						LOG_INFO("closing window.");
						x_running = 0;
					}
				} break;
				default: {
					LOG_DEBUG("default event:\nresponse type = %u", 
							x_event->response_type); 
				} break;
			}
			free(x_event); /*NOTE: every loop... seriously*/
		}
	}

	/* NOTE: these are technically optional I think, but might
	 * as well cleanup
	 */
	xcb_destroy_window(x_connection, x_window_id);
	xcb_disconnect(x_connection);
	free(x_pixmap_data);
	return 0;
}
