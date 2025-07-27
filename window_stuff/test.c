#include <xcb/xcb.h>

#include <stdio.h>
#include <stdlib.h>

int main() {
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

	int res = xcb_connection_has_error(x_server_connection);
	if(res > 0) {
		printf("ERROR: xcb_connection_has_error returned %d\n", res);
		return(1);
	}

	x_setup = xcb_get_setup(x_server_connection);
	x_screen_iterator = xcb_setup_roots_iterator(x_setup);
	for( ; 
		x_screen_iterator.rem; 
		--x_screen_number, xcb_screen_next(&x_screen_iterator)) 
	{
		if(x_screen_number == 0) 
		{
			x_screen = x_screen_iterator.data; 
			break;
		}
	}

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

	xcb_void_cookie_t x_create_window_cookie;

	uint32_t x_win_value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t x_win_value_list[2];
	x_win_value_list[0] = x_screen->white_pixel;

	x_win_value_list[1] = XCB_EVENT_MASK_EXPOSURE |
							XCB_EVENT_MASK_NO_EVENT |
							XCB_EVENT_MASK_KEY_PRESS |
    						XCB_EVENT_MASK_KEY_RELEASE |
    						XCB_EVENT_MASK_BUTTON_PRESS |
    						XCB_EVENT_MASK_BUTTON_RELEASE |
    						XCB_EVENT_MASK_ENTER_WINDOW |
    						XCB_EVENT_MASK_LEAVE_WINDOW |
    						XCB_EVENT_MASK_POINTER_MOTION |
    						XCB_EVENT_MASK_POINTER_MOTION_HINT |
    						XCB_EVENT_MASK_BUTTON_1_MOTION |
    						XCB_EVENT_MASK_BUTTON_2_MOTION |
    						XCB_EVENT_MASK_BUTTON_3_MOTION |
    						XCB_EVENT_MASK_BUTTON_4_MOTION |
    						XCB_EVENT_MASK_BUTTON_5_MOTION |
    						XCB_EVENT_MASK_BUTTON_MOTION |
    						XCB_EVENT_MASK_KEYMAP_STATE |
    						XCB_EVENT_MASK_EXPOSURE |
    						XCB_EVENT_MASK_VISIBILITY_CHANGE |
    						XCB_EVENT_MASK_STRUCTURE_NOTIFY |
    						XCB_EVENT_MASK_RESIZE_REDIRECT |
    						XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
    						XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
    						XCB_EVENT_MASK_FOCUS_CHANGE |
    						XCB_EVENT_MASK_PROPERTY_CHANGE |
    						XCB_EVENT_MASK_COLOR_MAP_CHANGE |
    						XCB_EVENT_MASK_OWNER_GRAB_BUTTON;

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

	xcb_map_window(x_server_connection, x_window_id);

	if(xcb_flush(x_server_connection) < 1) { /*returns <= 0 on failure*/
		printf("xcb_flush failed\n");
		return(1);
	}

	printf("XCB_CLIENT_MESSAGE = %d\n", XCB_CLIENT_MESSAGE);

	xcb_generic_event_t *x_event = 0;
	char x_running = 1;
	while(x_running) 
	{
		x_event = xcb_poll_for_event(x_server_connection);
		if(x_event) 
		{
			printf("event type: %d\n", (x_event->response_type & ~0x80));
			switch(x_event->response_type & ~0x80) /*TODO: 
													 wtf this ~0x80 */
			{
				case XCB_CLIENT_MESSAGE:
				{
					printf("INFO: received XCB_CLIENT_MESSAGE\n");
					xcb_client_message_event_t *x_client_message_event =
						(xcb_client_message_event_t*)x_event;
					
					uint32_t x_client_message_data32 = 
						x_client_message_event->data.data32[0];
					if(x_client_message_data32 == x_atom_wm_delete_window)
					{
						printf("INFO: WM_DELETE_WINDOW event\n");
						x_running = 0;
					}
				} break;
			}
		}
		free(x_event);
	}
	xcb_destroy_window(x_server_connection, x_window_id);
	xcb_disconnect(x_server_connection);
	return 0;
}
