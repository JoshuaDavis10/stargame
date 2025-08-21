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
typedef struct {
	xcb_keysym_t *x_key_symbols_table;
	i32 x_key_symbols_table_size;
	u8 x_max_key_code;
	u8 x_min_key_code;
} x_keymap_info;

/*
 * @brief translates keycode into a keysym
 * @param x_keycode: keycode to be translated
 * @return: corresponding keysym of keycode
 */
xcb_keysym_t x_get_keysym_from_keycode(
		x_keymap_info info, 
		xcb_keycode_t x_keycode)
{
	_assert(x_keycode >= info.x_min_key_code);
	_assert(x_keycode <= info.x_max_key_code);
	return info.x_key_symbols_table[x_keycode];
}

/* 
 * @brief registers the key stroke into the game input state
 * @param input: pointer to input state
 * @param keysym: key symbol of key that was pressed/released
 * @param pressed: true if pressed, false if released
 */
void x_register_key_stroke(
	game_input_state *input, 
	xcb_keysym_t keysym,
	b8 pressed)
{
	switch(keysym) {
		case X_KEY_BACKSPACE: {
			input->backspace = pressed;
		} break;
		case X_KEY_TAB: {
			input->tab = pressed;
		} break;
		case X_KEY_ENTER: {
			input->enter = pressed;
		} break;
		case X_KEY_ESCAPE: {
			input->escape = pressed;
		} break;
		case X_KEY_LEFT_ARROW: {
			input->left_arrow = pressed;
		} break;
		case X_KEY_UP_ARROW: {
			input->up_arrow = pressed;
		} break;
		case X_KEY_RIGHT_ARROW: {
			input->right_arrow = pressed;
		} break;
		case X_KEY_DOWN_ARROW: {
			input->down_arrow = pressed;
		} break;
		case X_KEY_F1: {
			input->fkeys[0] = pressed;
		} break;
		case X_KEY_F2: {
			input->fkeys[1] = pressed;
		} break;
		case X_KEY_F3: {
			input->fkeys[2] = pressed;
		} break;
		case X_KEY_F4: {
			input->fkeys[3] = pressed;
		} break;
		case X_KEY_F5: {
			input->fkeys[4] = pressed;
		} break;
		case X_KEY_F6: {
			input->fkeys[5] = pressed;
		} break;
		case X_KEY_F7: {
			input->fkeys[6] = pressed;
		} break;
		case X_KEY_F8: {
			input->fkeys[7] = pressed;
		} break;
		case X_KEY_F9: {
			input->fkeys[8] = pressed;
		} break;
		case X_KEY_F10: {
			input->fkeys[9] = pressed;
		} break;
		case X_KEY_F11: {
			input->fkeys[10] = pressed;
		} break;
		case X_KEY_F12: {
			input->fkeys[11] = pressed;
		} break;

		case X_KEY_LEFT_SHIFT: {
			input->left_shift = pressed;
		} break;
		case X_KEY_RIGHT_SHIFT: {
			input->right_shift = pressed;
		} break;
		case X_KEY_LEFT_CONTROL: {
			input->left_control = pressed;
		} break;
		case X_KEY_RIGHT_CONTROL: {
			input->right_control = pressed;
		} break;

		case X_KEY_A: {
			input->letters[0] = pressed;
		} break;
		case X_KEY_B: {
			input->letters[1] = pressed;
		} break;
		case X_KEY_C: {
			input->letters[2] = pressed;
		} break;
		case X_KEY_D: {
			input->letters[3] = pressed;
		} break;
		case X_KEY_E: {
			input->letters[4] = pressed;
		} break;
		case X_KEY_F: {
			input->letters[5] = pressed;
		} break;
		case X_KEY_G: {
			input->letters[6] = pressed;
		} break;
		case X_KEY_H: {
			input->letters[7] = pressed;
		} break;
		case X_KEY_I: {
			input->letters[8] = pressed;
		} break;
		case X_KEY_J: {
			input->letters[9] = pressed;
		} break;
		case X_KEY_K: {
			input->letters[10] = pressed;
		} break;
		case X_KEY_L: {
			input->letters[11] = pressed;
		} break;
		case X_KEY_M: {
			input->letters[12] = pressed;
		} break;
		case X_KEY_N: {
			input->letters[13] = pressed;
		} break;
		case X_KEY_O: {
			input->letters[14] = pressed;
		} break;
		case X_KEY_P: {
			input->letters[15] = pressed;
		} break;
		case X_KEY_Q: {
			input->letters[16] = pressed;
		} break;
		case X_KEY_R: {
			input->letters[17] = pressed;
		} break;
		case X_KEY_S: {
			input->letters[18] = pressed;
		} break;
		case X_KEY_T: {
			input->letters[19] = pressed;
		} break;
		case X_KEY_U: {
			input->letters[20] = pressed;
		} break;
		case X_KEY_V: {
			input->letters[21] = pressed;
		} break;
		case X_KEY_W: {
			input->letters[22] = pressed;
		} break;
		case X_KEY_X: {
			input->letters[23] = pressed;
		} break;
		case X_KEY_Y: {
			input->letters[24] = pressed;
		} break;
		case X_KEY_Z: {
			input->letters[25] = pressed;
		} break;
		case X_KEY_SPACE: {
			input->spacebar = pressed;
		} break;

		case X_KEY_ZERO: {
			input->numbers[0] = pressed;
		} break;
		case X_KEY_ONE: {
			input->numbers[1] = pressed;
		} break;
		case X_KEY_TWO: {
			input->numbers[2] = pressed;
		} break;
		case X_KEY_THREE: {
			input->numbers[3] = pressed;
		} break;
		case X_KEY_FOUR: {
			input->numbers[4] = pressed;
		} break;
		case X_KEY_FIVE: {
			input->numbers[5] = pressed;
		} break;
		case X_KEY_SIX: {
			input->numbers[6] = pressed;
		} break;
		case X_KEY_SEVEN: {
			input->numbers[7] = pressed;
		} break;
		case X_KEY_EIGHT: {
			input->numbers[8] = pressed;
		} break;
		case X_KEY_NINE: {
			input->numbers[9] = pressed;
		} break;
		default: {
			LOG_WARN("x_register_key_press: unidentified keysym %x", 
					keysym);
		} break;
	}

}

/*
 * @brief loads key symbols into a table that is indexed by key codes
 * so that key symbols can easily be accessed when a key code is gotten
 * from a key press/release event
 * @param x_connection: X protocol connection
 * @param x_setup: setup for x connection
 */
void x_load_key_symbols(
		x_keymap_info *info,
		xcb_connection_t *x_connection, 
		const xcb_setup_t *x_setup) 
{
	LOG_INFO("loading key symbols...");

	info->x_max_key_code = x_setup->max_keycode;
	info->x_min_key_code = x_setup->min_keycode;

	LOG_INFO("max keycode = %u", info->x_max_key_code);	
	LOG_INFO("min keycode = %u", info->x_min_key_code);	

	/* NOTE: initializing table. index is keycode. so this table
	 * translates keycodes to keysymbols. that's why it is
	 * length = max keycode instead of 
	 * length = (max keycode - min keycode)
	 *
	 * NOTE: using realloc since I will probalby call this function 
	 * on expose events or something. in case keyboard got switched 
	 * I guess?
	 */
	info->x_key_symbols_table_size = info->x_max_key_code + 1;
	info->x_key_symbols_table = 
		realloc(info->x_key_symbols_table,
				sizeof(*(info->x_key_symbols_table)) * 
					info->x_key_symbols_table_size);
	memset(info->x_key_symbols_table, 0,
			sizeof(*(info->x_key_symbols_table)) * 
				info->x_key_symbols_table_size);

	LOG_INFO("initialized key symbol table");

	/*
	LOG_INFO("key codes: ");
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
	i32 x_keysyms_count;
	u8 x_keysyms_per_keycode;

	x_get_keyboard_mapping_cookie = 
		xcb_get_keyboard_mapping(
				x_connection, 
				info->x_min_key_code,
				info->x_max_key_code - info->x_min_key_code + 1);

	xcb_generic_error_t *x_error;
	x_get_keyboard_mapping_reply =
		xcb_get_keyboard_mapping_reply(
				x_connection,
				x_get_keyboard_mapping_cookie,
				&x_error);
	if(x_error) {
		LOG_ERROR("%u", x_error->error_code);
		_assert(0);
	}

	x_keysyms_per_keycode = 
		x_get_keyboard_mapping_reply->keysyms_per_keycode;

	x_keysyms = 
		xcb_get_keyboard_mapping_keysyms(
			x_get_keyboard_mapping_reply);

	x_keysyms_count = 
		xcb_get_keyboard_mapping_keysyms_length(
			x_get_keyboard_mapping_reply);

	LOG_DEBUG("keysyms per keycode = %u", x_keysyms_per_keycode);	
	LOG_DEBUG("keysym count = %d", x_keysyms_count);	

	int keysym_i;
	for(keysym_i = info->x_min_key_code; 
		keysym_i <= info->x_max_key_code; 
		keysym_i++) 
	{
		/* NOTE: multiplying by keysyms per keycode just gives us
		 * the first keysym for each keycode
		 * I think for now that's good enough for our purposes
		 * printf("%x ", x_keysyms[x_keysyms_per_keycode * keysym_i]);	
		 */

		/* NOTE: populating key symbols table */
		info->x_key_symbols_table[keysym_i] = 
			x_keysyms[x_keysyms_per_keycode * 
			(keysym_i-info->x_min_key_code)];
	}

	LOG_DEBUG("key symbol table: n");
	int keytable_i;
	for(keytable_i = 0; 
		keytable_i < info->x_key_symbols_table_size; 
		keytable_i++) 
	{
		LOG_DEBUG("\tkeycode: %d | keysym: %x", 
				keytable_i, info->x_key_symbols_table[keytable_i]);
	}

	free(x_get_keyboard_mapping_reply);
	LOG_INFO("key symbols loaded.");
}

void x_print_mouse_button(x_mouse_button x_mb) {
	LOG_DEBUG(" ");
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
	LOG_DEBUG(" ");
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
