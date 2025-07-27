#include <xcb/xcb.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>

typedef enum {
	X_KEY_BACKSPACE = 0xFF08,
	X_KEY_TAB = 0xFF09,
	X_KEY_ENTER = 0xFF0D,
	X_KEY_ESCAPE = 0xFF1B,

	X_KEY_LEFT_ARROW = 0xFF51,
	X_KEY_UP_ARROW = 0xFF52,
	X_KEY_RIGHT_ARROW = 0xFF53,
	X_KEY_DOWN_ARROW = 0xFF54,

	X_KEY_F1 = 0xFFBE,
	X_KEY_F2 = 0xFFBF,
	X_KEY_F3 = 0xFFC0,
	X_KEY_F4 = 0xFFC1,
	X_KEY_F5 = 0xFFC2,
	X_KEY_F6 = 0xFFC3,
	X_KEY_F7 = 0xFFC4,
	X_KEY_F8 = 0xFFC5,
	X_KEY_F9 = 0xFFC6,
	X_KEY_F10 = 0xFFC7,
	X_KEY_F11 = 0xFFC8,
	X_KEY_F12 = 0xFFC9,

	X_KEY_LEFT_SHIFT = 0xFFE1,
	X_KEY_RIGHT_SHIFT = 0xFFE2,
	X_KEY_LEFT_CONTROL = 0xFFE3,
	X_KEY_RIGHT_CONTROL = 0xFFE4,

	X_KEY_A = 0x61,
	X_KEY_B = 0x62,
	X_KEY_C = 0x63,
	X_KEY_D = 0x64,
	X_KEY_E = 0x65,
	X_KEY_F = 0x66,
	X_KEY_G = 0x67,
	X_KEY_H = 0x68,
	X_KEY_I = 0x69,
	X_KEY_J = 0x6A,
	X_KEY_K = 0x6B,
	X_KEY_L = 0x6C,
	X_KEY_M = 0x6D,
	X_KEY_N = 0x6E,
	X_KEY_O = 0x6F,
	X_KEY_P = 0x70,
	X_KEY_Q = 0x71,
	X_KEY_R = 0x72,
	X_KEY_S = 0x73,
	X_KEY_T = 0x74,
	X_KEY_U = 0x75,
	X_KEY_V = 0x76,
	X_KEY_W = 0x77,
	X_KEY_X = 0x78,
	X_KEY_Y = 0x79,
	X_KEY_Z = 0x7A,
	X_KEY_SPACE = 0x20,

	X_KEY_ZERO = 0x30,
	X_KEY_ONE = 0x31,
	X_KEY_TWO = 0x32,
	X_KEY_THREE = 0x33,
	X_KEY_FOUR = 0x34,
	X_KEY_FIVE = 0x35,
	X_KEY_SIX = 0x36,
	X_KEY_SEVEN = 0x37,
	X_KEY_EIGHT = 0x38,
	X_KEY_NINE = 0x39,

	/* NOTE: add keys as needed */

	X_KEY_COUNT
} x_key;

typedef enum {
	X_MOUSE_LEFT = 1,
	X_MOUSE_RIGHT = 3,
	X_MOUSE_WHEEL = 2,
	X_MOUSE_WHEEL_UP = 4,
	X_MOUSE_WHEEL_DOWN = 5,
	X_MOUSE_BUTTON_COUNT
} x_mouse_button;

/* 
 * table to store key code -> key symbol conversion
 * initialize this array to size of max->keycode + 1
 */
static xcb_keysym_t *x_key_symbols_table = 0;
/* TODO: typedef your own types */
static int x_key_symbols_table_size = 0;
static uint8_t x_max_key_code = 0;
static uint8_t x_min_key_code = 0;

/* TODO: process "input state" based on a keysym 
 * void x_update_input_state(xcb_keysym_t x_keysym) 
 */

void x_print_mouse_button(x_mouse_button x_mb) {
	printf("DEBUG: ");
	switch(x_mb) {
		case X_MOUSE_LEFT: {
			printf("'LEFT MOUSE BUTTON' ");
		} break;
		case X_MOUSE_RIGHT: {
			printf("'RIGHT MOUSE BUTTON' ");
		} break;
		case X_MOUSE_WHEEL: {
			printf("'MOUSE WHEEL' ");
		} break;
		case X_MOUSE_WHEEL_DOWN: {
			printf("'MOUSE WHEEL DOWN' ");
		} break;
		case X_MOUSE_WHEEL_UP: {
			printf("'MOUSE WHEEL UP' ");
		} break;
		default: {
			printf("'UNSPECIFIED' ");
		} break;
	}
}

void x_print_keysym(x_key x_keysym) {
	printf("DEBUG: ");
	switch(x_keysym) {
		case X_KEY_BACKSPACE: {
			printf("'BACKSPACE'");
		} break;
		case X_KEY_TAB: {
			printf("'TAB'");
		} break;
		case X_KEY_ENTER: {
			printf("'ENTER'");
		} break;
		case X_KEY_ESCAPE: {
			printf("'ESCAPE'");
		} break;
		case X_KEY_LEFT_ARROW: {
			printf("'LEFT'");
		} break;
		case X_KEY_UP_ARROW: {
			printf("'UP'");
		} break;
		case X_KEY_RIGHT_ARROW: {
			printf("'RIGHT'");
		} break;
		case X_KEY_DOWN_ARROW: {
			printf("'DOWN'");
		} break;
		case X_KEY_F1: {
			printf("'F1'");
		} break;
		case X_KEY_F2: {
			printf("'F2'");
		} break;
		case X_KEY_F3: {
			printf("'F3'");
		} break;
		case X_KEY_F4: {
			printf("'F4'");
		} break;
		case X_KEY_F5: {
			printf("'F5'");
		} break;
		case X_KEY_F6: {
			printf("'F6'");
		} break;
		case X_KEY_F7: {
			printf("'F7'");
		} break;
		case X_KEY_F8: {
			printf("'F8'");
		} break;
		case X_KEY_F9: {
			printf("'F9'");
		} break;
		case X_KEY_F10: {
			printf("'F10'");
		} break;
		case X_KEY_F11: {
			printf("'F11'");
		} break;
		case X_KEY_F12: {
			printf("'F12'");
		} break;

		case X_KEY_LEFT_SHIFT: {
			printf("'LEFT SHIFT'");
		} break;
		case X_KEY_RIGHT_SHIFT: {
			printf("'RIGHT SHIFT'");
		} break;
		case X_KEY_LEFT_CONTROL: {
			printf("'LEFT CONTROL'");
		} break;
		case X_KEY_RIGHT_CONTROL: {
			printf("'RIGHT CONTROL'");
		} break;

		case X_KEY_A: {
			printf("'A'");
		} break;
		case X_KEY_B: {
			printf("'B'");
		} break;
		case X_KEY_C: {
			printf("'C'");
		} break;
		case X_KEY_D: {
			printf("'D'");
		} break;
		case X_KEY_E: {
			printf("'E'");
		} break;
		case X_KEY_F: {
			printf("'F'");
		} break;
		case X_KEY_G: {
			printf("'G'");
		} break;
		case X_KEY_H: {
			printf("'H'");
		} break;
		case X_KEY_I: {
			printf("'I'");
		} break;
		case X_KEY_J: {
			printf("'J'");
		} break;
		case X_KEY_K: {
			printf("'K'");
		} break;
		case X_KEY_L: {
			printf("'L'");
		} break;
		case X_KEY_M: {
			printf("'M'");
		} break;
		case X_KEY_N: {
			printf("'N'");
		} break;
		case X_KEY_O: {
			printf("'O'");
		} break;
		case X_KEY_P: {
			printf("'P'");
		} break;
		case X_KEY_Q: {
			printf("'Q'");
		} break;
		case X_KEY_R: {
			printf("'R'");
		} break;
		case X_KEY_S: {
			printf("'S'");
		} break;
		case X_KEY_T: {
			printf("'T'");
		} break;
		case X_KEY_U: {
			printf("'U'");
		} break;
		case X_KEY_V: {
			printf("'V'");
		} break;
		case X_KEY_W: {
			printf("'W'");
		} break;
		case X_KEY_X: {
			printf("'X'");
		} break;
		case X_KEY_Y: {
			printf("'Y'");
		} break;
		case X_KEY_Z: {
			printf("'Z'");
		} break;
		case X_KEY_SPACE: {
			printf("' '");
		} break;

		case X_KEY_ZERO: {
			printf("'0'");
		} break;
		case X_KEY_ONE: {
			printf("'1'");
		} break;
		case X_KEY_TWO: {
			printf("'2'");
		} break;
		case X_KEY_THREE: {
			printf("'3'");
		} break;
		case X_KEY_FOUR: {
			printf("'4'");
		} break;
		case X_KEY_FIVE: {
			printf("'5'");
		} break;
		case X_KEY_SIX: {
			printf("'6'");
		} break;
		case X_KEY_SEVEN: {
			printf("'7'");
		} break;
		case X_KEY_EIGHT: {
			printf("'8'");
		} break;
		case X_KEY_NINE: {
			printf("'9'");
		} break;
		default: {
			printf("'!UNSPECIFIED!'");
		} break;
	}
	printf(" key ");
}

xcb_keysym_t x_get_keysym_from_keycode(xcb_keycode_t x_keycode)
{
	/* TODO assertion */
	if(x_keycode < x_min_key_code || x_keycode > x_max_key_code) {
		return 0;
	}
	return x_key_symbols_table[x_keycode];
}

/*
 * @brief loads key symbols into a table that is indexed by key codes
 * so that key symbols can easily be accessed when a key code is gotten
 * from a key press/release event
 * @param x_connection: X protocol connection
 * @param x_setup: setup for x connection
 */
/* TODO: have this return an error on error */
void x_load_key_symbols(
		xcb_connection_t *x_connection, 
		const xcb_setup_t *x_setup) 
{
	printf("INFO: loading key symbols...\n");

	x_max_key_code = x_setup->max_keycode;
	x_min_key_code = x_setup->min_keycode;

	printf("INFO: max keycode = %u\n", x_max_key_code);	
	printf("INFO: min keycode = %u\n", x_min_key_code);	

	/* NOTE: initializing table. index is keycode. so this table
	 * translates keycodes to keysymbols. that's why it is
	 * length = max keycode instead of 
	 * length = (max keycode - min keycode)
	 *
	 * NOTE: using realloc since I will probalby call this function 
	 * on expose events or something. in case keyboard got switched 
	 * I guess?
	 */
	x_key_symbols_table_size = x_max_key_code + 1;
	x_key_symbols_table = 
		realloc(x_key_symbols_table,
				sizeof(*x_key_symbols_table) * x_key_symbols_table_size);
	memset(x_key_symbols_table, 0,
			sizeof(*x_key_symbols_table) * x_key_symbols_table_size);

	printf("INFO: initialized key symbol table\n");

	/*
	printf("INFO: key codes: ");
	uint16_t keycode_i;
	for(keycode_i = x_min_key_code; 
		keycode_i <= x_max_key_code; 
		keycode_i++) 
	{
		printf("%d ", (uint8_t)keycode_i);
	}
	printf("\n");
	*/

	/* get keysyms and keysym count */
	xcb_get_keyboard_mapping_cookie_t x_get_keyboard_mapping_cookie;
	xcb_get_keyboard_mapping_reply_t *x_get_keyboard_mapping_reply;
	xcb_keysym_t *x_keysyms;
	int x_keysyms_count;
	uint8_t x_keysyms_per_keycode;

	x_get_keyboard_mapping_cookie = 
		xcb_get_keyboard_mapping(
				x_connection, 
				x_min_key_code,
				x_max_key_code - x_min_key_code + 1);

	/* TODO: 3rd parameter is a generic error double pointer 
	 * is NULL okay for 3rd param?
	 */

	xcb_generic_error_t *x_error;

	x_get_keyboard_mapping_reply =
		xcb_get_keyboard_mapping_reply(
				x_connection,
				x_get_keyboard_mapping_cookie,
				&x_error);

	if(x_error) {
		printf("ERROR: %u\n", x_error->error_code);
		/* TODO: assert */
	}

	x_keysyms_per_keycode = 
		x_get_keyboard_mapping_reply->keysyms_per_keycode;

	x_keysyms = 
		xcb_get_keyboard_mapping_keysyms(
			x_get_keyboard_mapping_reply);

	x_keysyms_count = 
		xcb_get_keyboard_mapping_keysyms_length(
			x_get_keyboard_mapping_reply);

	printf("INFO: keysyms per keycode = %u\n", x_keysyms_per_keycode);	
	printf("INFO: keysym count = %d\n", x_keysyms_count);	

	 printf("INFO: key symbols: "); 
	int keysym_i;
	for(keysym_i = x_min_key_code; 
		keysym_i <= x_max_key_code; 
		keysym_i++) 
	{
		/* NOTE: multiplying by keysyms per keycode just gives us
		 * the first keysym for each keycode
		 * I think for now that's good enough for our purposes
		 * printf("%x ", x_keysyms[x_keysyms_per_keycode * keysym_i]);	
		 */

		/* NOTE: populating key symbols table */
		x_key_symbols_table[keysym_i] = 
			x_keysyms[x_keysyms_per_keycode * (keysym_i-x_min_key_code)];
	}
	printf("\n");

	printf("INFO: key symbol table: \n");
	int keytable_i;
	for(keytable_i = 0; 
		keytable_i < x_key_symbols_table_size; 
		keytable_i++) 
	{
		printf("\tkeycode: %d | keysym: %x\n", 
				keytable_i, x_key_symbols_table[keytable_i]);
	}

	free(x_get_keyboard_mapping_reply);
	printf("INFO: key symbols loaded.\n");
}

int main() 
{

	/*variable to hold pointer to server connection*/
	xcb_connection_t *x_server_connection;
	xcb_screen_t *x_screen; /*pointer to first screen*/
	const xcb_setup_t *x_setup;
	int x_screen_number; /*NOTE: I don't exactly understand this value rn*/
	xcb_screen_iterator_t x_screen_iterator; /*used to iterate over the 
										    *xcb_xcreen_t's
											*for our connection
											*/

	/*create connection to x server*/
	x_server_connection = xcb_connect(NULL, &x_screen_number);

	/*check if connection worked. returns 0 if success
	 *TODO: print useful information based on errors returned
	 *on error. supre minor detail that's only relevant for 
	 *like code you want to ship 
	 *imo, but just putting it here. see xcb/xcb.h for info
	 *on errors that this functio ncan return
	 */
	int res = xcb_connection_has_error(x_server_connection);
	if(res > 0) {
		printf("ERROR: xcb_connection_has_error returned %d\n", res);
		return(1);
	}

	/*setup screen iterator, NOTE: i vaguely remember xcb.h saying
	  not to free the xcb_setup_t pointer that is returned*/
	x_setup = xcb_get_setup(x_server_connection);
	x_screen_iterator = xcb_setup_roots_iterator(x_setup);

	/*loop to first screen (i.e. when x_screen_iterator.rem == 0, so I'm
	 *not really sure what the x_screen_number stuff is for)
	 *if there's only one screen, we can just grab out of the
	 *iterator as soon as it's setup
	 */
	for( ; 
		x_screen_iterator.rem; 
		--x_screen_number, xcb_screen_next(&x_screen_iterator)) 
	{
		if(x_screen_number == 0) 
		{
			/*set our screen to whichever
			 *one iterator is on (i.e.
			 *the first screen*/
			x_screen = x_screen_iterator.data; 
			break;
		}
	}

	/* NOTE: example code creates graphics context here,
	 * but I don't believe there's a strict restriction as to
	 * which gets created first: window or graphics context 
	 */
	xcb_gcontext_t x_graphics_context = 
		xcb_generate_id(x_server_connection);

	/* NOTE: xcb_generate_id returns an unsigned int so idk
	 * why the documentation/comments in header say that it returns
	 * -1 on failure 
	if(x_graphics_context == -1) {
		printf("ERROR: xcb_generate_id failed\n");
		return(1);
	}
	 */

	uint32_t x_gc_value_mask; /*NOTE: value mask values must be stored
							  *in value_mask in the order they are
							  *specified in enum and similarly listed
							  *here in that order (bitwise or)*/
	uint32_t x_gc_value_list[1]; /*values specified in value_mask
									   *get stored in here
									   */

	x_gc_value_mask = XCB_GC_FOREGROUND; /*specifies foreground "value"?
										 color? idk TODO*/
	/*foreground color is black? I think? TODO*/
	x_gc_value_list[0] = x_screen->black_pixel; 
	xcb_create_gc(x_server_connection, x_graphics_context,
				  x_screen->root, x_gc_value_mask, x_gc_value_list);

	/*TODO: temp stuff to make sure screen iterator code stuff works*/
	printf ("\n");
	printf ("Informations of screen %d:\n", x_screen->root);
	printf ("  width.........: %d\n", x_screen->width_in_pixels);
	printf ("  height........: %d\n", x_screen->height_in_pixels);
	printf ("  white pixel...: %d\n", x_screen->white_pixel);
	printf ("  black pixel...: %d\n", x_screen->black_pixel);
	printf ("\n");

	/*generate id to be passed to xcb_create_window*/
	xcb_window_t x_window_id = xcb_generate_id(x_server_connection);

	/* NOTE: xcb_generate_id returns an unsigned int so idk
	 * why the documentation/comments in header say that it returns
	 * -1 on failure 
	if(x_window_id == -1) {
		printf("ERROR: xcb_generate_id failed\n");
		return(1);
	}
	*/

	/*TODO: idk what this is but its what xcb_create_window returns
	 *NOTE: okay so this isn't necessary... can just call the function
	 *directly because I don't think we'll need to do anything with it
	 *but maybe there is something to do with it? TODO like error
	 *handling or smn?
	 */
	xcb_void_cookie_t x_create_window_cookie;

	uint32_t x_win_value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t x_win_value_list[2];
	x_win_value_list[0] = x_screen->white_pixel;
	/* all events to register for:
	 * exposure, button press/relase, mouse motion, 
	 * mouse enter/leave, key press/release */
	x_win_value_list[1] = XCB_EVENT_MASK_EXPOSURE 
						| XCB_EVENT_MASK_BUTTON_PRESS
						| XCB_EVENT_MASK_BUTTON_RELEASE
						| XCB_EVENT_MASK_POINTER_MOTION
						| XCB_EVENT_MASK_ENTER_WINDOW
						| XCB_EVENT_MASK_LEAVE_WINDOW
						| XCB_EVENT_MASK_KEY_PRESS
						| XCB_EVENT_MASK_KEY_RELEASE;

	/*generate the window (see notes.txt for info about parameters)*/
	x_create_window_cookie = xcb_create_window(
		x_server_connection,			/*xcb_connection_t connection*/
	    XCB_COPY_FROM_PARENT,			/*depth of screen 
										  (inherit from root)*/
	    x_window_id,					/*id of window*/
		x_screen->root,					/*id of parent window*/
	    100, 100,						/*x and y*/ 
		1000, 500,						/*width and height*/
		10,								/*border width*/
		XCB_WINDOW_CLASS_INPUT_OUTPUT,	/*window class: input/output*/
		XCB_COPY_FROM_PARENT,			/* "visual" (idk what that means)
											(inherit from root) */
		x_win_value_mask,				/*value_mask/value_list*/
		x_win_value_list);

	/* register for WM_DELETE_WINDOW event */
	/* create internal atom for WM_PROTOCOLS */
	xcb_intern_atom_cookie_t x_internal_atom_wm_protocols =
		xcb_intern_atom(x_server_connection, 1, 12, "WM_PROTOCOLS");

	/* TODO: handle error instead of passing 0 @3rd param */
	/* send internal atom */
	xcb_intern_atom_reply_t *x_wm_protocols_reply =
		xcb_intern_atom_reply(
				x_server_connection, 
				x_internal_atom_wm_protocols,
				0);

	/* get reply (NOTE: this is the actual atom I guess?) */
	xcb_atom_t x_atom_wm_protocols = x_wm_protocols_reply->atom;
	if(!x_atom_wm_protocols) {
		printf("ERROR: failed to get WM_PROTOCOLS atom\n");
		return(1);
	}
	free(x_wm_protocols_reply);

	/* create internal atom for WM_DELETE_WINDOW */
	xcb_intern_atom_cookie_t x_internal_atom_wm_delete_window =
		xcb_intern_atom(x_server_connection, 1, 16, "WM_DELETE_WINDOW");

	/* TODO: handle error instead of passing 0 @3rd param */
	/* send internal atom */
	xcb_intern_atom_reply_t *x_wm_delete_window_reply =
		xcb_intern_atom_reply(
				x_server_connection, 
				x_internal_atom_wm_delete_window,
				0);

	/* get reply (NOTE: this is the actual atom I guess?) */
	xcb_atom_t x_atom_wm_delete_window = x_wm_delete_window_reply->atom;
	if(!x_atom_wm_delete_window) {
		printf("ERROR: failed to get WM_DELETE_WINDOW atom\n");
		return(1);
	}
	free(x_wm_delete_window_reply);

	/* change the property (i.e. register for that event I guess */
	xcb_change_property(
			x_server_connection,
			XCB_PROP_MODE_REPLACE,
			x_window_id,
			x_atom_wm_protocols,
			XCB_ATOM_ATOM,
			32, 1,
			&x_atom_wm_delete_window);

		/* load key symbols */
	/* TODO: have this return an error */
	x_load_key_symbols(x_server_connection, x_setup);

	/*map window. NOTE: this also generates a void cookie
	  that we may need to do smn with?*/
	xcb_map_window(x_server_connection, x_window_id);

	/*NOTE: do either xcb_flush/xcb_aux_sync
	but xcb_flush is fine bc we don't NEED to wait for 
	server response (according to documentation, so I'm curious why)
	flush all pending requests to X server
	NOTE: xcb_flush blocks until the write is complete
	*/
	if(xcb_flush(x_server_connection) < 1) { /*returns <= 0 on failure*/
		printf("xcb_flush failed\n");
		return(1);
	}
	/*
	 * flush then wait for X server to finish processing
	 * flushed requests
	 * xcb_aux_sync(x_server_connection);
	 * NOTE: couldn't find any documentation on xcb_aux_sync besides
	 * documentation example that I am following
	 * TODO: what is the int that they return???
	 */

	 /*NOTE: stuff to draw*/
	xcb_point_t points[2] = {
		{250, 250}, /*x, y*/
		{250, 251}  /*int16_t's*/
	};	

	xcb_rectangle_t rectangles[2] = {
		{100, 100, 20, 20}, /*x,y,width,height*/
		{300, 300, 50, 50}, /*x,y,width,height*/
							/*int16_t's, uint16_t's*/
	};

	/*TODO: segments, lines, arcs if u feel like it*/



	xcb_generic_event_t *x_event = 0;
	char x_running = 1;
	while(x_running) 
	{
		/* NOTE: this function returns NULL if there's no events
		 * so I moved it out of the while()
		 */

		/* TODO: do you need to loop polls until there's no more events?
		 * like I don't think it'll just be 1 event every time this loop
		 * runs. like is this a queue? you get what I'm asking future
		 * Josh, okay...
		 */
		x_event = xcb_poll_for_event(x_server_connection);
		if(x_event) {
			switch(x_event->response_type & ~0x80) /*TODO: 
													 wtf this ~0x80 */
			{
				case XCB_EXPOSE: 
				{
					xcb_poly_point(
							x_server_connection,	
							/*use origin as coord base*/
							XCB_COORD_MODE_ORIGIN,	
							x_window_id,			
							x_graphics_context, 
							2, points);

					xcb_poly_rectangle(
							x_server_connection,	
							x_window_id,			
							x_graphics_context, 
							1, rectangles);

					xcb_poly_fill_rectangle(
							x_server_connection,	
							x_window_id,			
							x_graphics_context, 
							1, rectangles+1);

					xcb_flush(x_server_connection); /*flush requests*/

					/* NOTE: it looks like expose events only happen
					 * at start?
					 */
					printf("Expose event\n");
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
								key_release_event_keycode);

					x_print_keysym(key_release_event_keysym); 
					printf("released\n");

					/* TODO: create some input state that is 
					 * changed by key press events
					 */

				} break;
				case XCB_BUTTON_PRESS:
				{
					xcb_button_press_event_t *x_mouse_button_press_event =
						(xcb_button_press_event_t *)x_event;

					x_print_mouse_button(
							x_mouse_button_press_event->detail);

					printf("button pressed\n");

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

					int16_t mouse_x = x_mouse_motion_event->event_x;
					int16_t mouse_y = x_mouse_motion_event->event_y;

					printf("mouse move; pos: %d, %d\n", mouse_x, mouse_y);
				} break;
				case XCB_ENTER_NOTIFY:
				{
					/* NOTE: I mean there's not really any info to 
					 * get here right? just that it happened? ... 
					 * oh wait I guess you'd want to store the mouse pos
					 * but doesn't that happen in motion anyways? wtf
					 */
					printf("mouse entered window\n");
					
				} break;
				case XCB_LEAVE_NOTIFY:
				{
					printf("mouse left window\n");
				} break;
				case XCB_CLIENT_MESSAGE:
				{
					xcb_client_message_event_t *x_client_message_event =
						(xcb_client_message_event_t*)x_event;
					
					uint32_t x_client_message_data32 = 
						x_client_message_event->data.data32[0];
					if(x_client_message_data32 == x_atom_wm_delete_window)
					{
						printf("INFO: WM_DELETE_WINDOW event\n");
						printf("INFO: closing window.\n");
						x_running = 0;
					}
				} break;
				default: {
					printf("default event:\nresponse type = %u\n", 
							x_event->response_type); 
				} break;
			}
			free(x_event); /*NOTE: every loop... seriously*/
		}
	}

	/*Ctrl-C to terminate program and close window
	  pause(); //NOTE: i don't think this is necessary now bc
	  window stays open to respond to events ^ cuz of while loop
	*/

	/* destroy window. is this necessary or will this be done for us
	 * by os/x server ?
	 */
	xcb_destroy_window(x_server_connection, x_window_id);

	/*disconnect from x server TODO: this is probably unnecessary
	  I assume OS automatically disconnects when program ends but 
	  MAKE SURE THAT'S TRUE FIRST
	*/
	xcb_disconnect(x_server_connection);

	return 0;
}
