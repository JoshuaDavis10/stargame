#include <windows.h>

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

typedef unsigned int b32;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#include "util.c"

enum {
	INPUT_BUTTON_STATE_UP = 0x00,
	INPUT_BUTTON_STATE_DOWN = 0x03,
	INPUT_BUTTON_STATE_RELEASED = 0x04,
	INPUT_BUTTON_STATE_PRESSED = 0x01
};

#define NUM_INPUT_BUTTONS 66

typedef struct {
	union {
		struct {
			u8 backspace;
			u8 tab;
			u8 enter;
			u8 escape;
			u8 spacebar;

			u8 left_arrow;
			u8 up_arrow;
			u8 right_arrow;
			u8 down_arrow;

			u8 fkeys[12]; /* NOTE: fkeys[0] is F1 */

			u8 left_shift;
			u8 right_shift;
			u8 left_control;
			u8 right_control;

			u8 letters[26]; /* NOTE: letters[0] is A, letters[25] is Z */

			u8 numbers[10]; /* NOTE: numbers[0] is 0, numbers[9] is 9 */

			u8 mouse_left;
			u8 mouse_right;
			u8 mouse_wheel;
			u8 mouse_wheel_up;
			u8 mouse_wheel_down;
		};
		u8 input_buttons[NUM_INPUT_BUTTONS];
	};

	i16 mouse_x;
	i16 mouse_y;
} input_state;

_static_assert(sizeof(unsigned char) == 1, unexpected_data_type_size);
_static_assert(sizeof(unsigned short) == 2, unexpected_data_type_size);
_static_assert(sizeof(unsigned int) == 4, unexpected_data_type_size);
_static_assert(sizeof(unsigned long long) == 8, unexpected_data_type_size);
_static_assert(sizeof(signed char) == 1, unexpected_data_type_size);
_static_assert(sizeof(signed short) == 2, unexpected_data_type_size);
_static_assert(sizeof(signed int) == 4, unexpected_data_type_size);
_static_assert(sizeof(signed long long) == 8, unexpected_data_type_size);
_static_assert(sizeof(float) == 4, unexpected_data_type_size);
_static_assert(sizeof(double) == 8, unexpected_data_type_size);

/* NOTE(josh): guar is for game update and render lol */
typedef void (*guar)(
	void *, 
	u64, 
	u8 *, 
	u16, 
	u16, 
	input_state *,
	char *);

static i32 global_window_width  = 896;
static i32 global_window_height = 504;
static b32 global_window_running = true;
static input_state global_old_input_state;
static input_state global_new_input_state;
static input_state global_game_input_state;

LRESULT stargame_win32_callback(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param)
{
	LRESULT result = 0;
	switch(message)
	{
		case WM_CLOSE:
		{
			log_info("WM_CLOSE event occured, setting global_window_running to 'false'");
			global_window_running = false;
		} break;
		case WM_DESTROY:
		{
			log_info("WM_DESTROY event occured, setting global_window_running to 'false'");
			global_window_running = false;
		} break;
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		{
			b32 down = ( (l_param & (1 << 31)) == 0);  

			switch(w_param)
			{
				/* misc */
				case VK_BACK: { global_new_input_state.backspace = (u8)down; } break;
				case VK_TAB: { global_new_input_state.tab = (u8)down; } break;
				case VK_RETURN: { global_new_input_state.tab = (u8)down; } break;
				case VK_ESCAPE: { global_new_input_state.escape= (u8)down; } break;
				case VK_SPACE: { global_new_input_state.spacebar = (u8)down; } break;
				case VK_LCONTROL: { global_new_input_state.left_control = (u8)down; } break;
				case VK_RCONTROL: { global_new_input_state.right_control = (u8)down; } break;
				case VK_LSHIFT: { global_new_input_state.left_shift = (u8)down; } break;
				case VK_RSHIFT: { global_new_input_state.right_shift = (u8)down; } break;

				/* arrows */
				case VK_LEFT: { global_new_input_state.left_arrow = (u8)down; } break;
				case VK_UP: { global_new_input_state.up_arrow = (u8)down; } break;
				case VK_RIGHT: { global_new_input_state.right_arrow = (u8)down; } break;
				case VK_DOWN: { global_new_input_state.down_arrow = (u8)down; } break;

				/* f keys */
				case VK_F1: { global_new_input_state.fkeys[0] = (u8)down; } break;
				case VK_F2: { global_new_input_state.fkeys[1] = (u8)down; } break;
				case VK_F3: { global_new_input_state.fkeys[2] = (u8)down; } break;
				case VK_F4: { global_new_input_state.fkeys[3] = (u8)down; } break;
				case VK_F5: { global_new_input_state.fkeys[4] = (u8)down; } break;
				case VK_F6: { global_new_input_state.fkeys[5] = (u8)down; } break;
				case VK_F7: { global_new_input_state.fkeys[6] = (u8)down; } break;
				case VK_F8: { global_new_input_state.fkeys[7] = (u8)down; } break;
				case VK_F9: { global_new_input_state.fkeys[8] = (u8)down; } break;
				case VK_F10: { global_new_input_state.fkeys[9] = (u8)down; } break;
				case VK_F11: { global_new_input_state.fkeys[10] = (u8)down; } break;
				case VK_F12: { global_new_input_state.fkeys[11] = (u8)down; } break;

				/* letters */
				case 'A': { global_new_input_state.letters[0] = (u8)down; } break;
				case 'B': { global_new_input_state.letters[1] = (u8)down; } break;
				case 'C': { global_new_input_state.letters[2] = (u8)down; } break;
				case 'D': { global_new_input_state.letters[3] = (u8)down; } break;
				case 'E': { global_new_input_state.letters[4] = (u8)down; } break;
				case 'F': { global_new_input_state.letters[5] = (u8)down; } break;
				case 'G': { global_new_input_state.letters[6] = (u8)down; } break;
				case 'H': { global_new_input_state.letters[7] = (u8)down; } break;
				case 'I': { global_new_input_state.letters[8] = (u8)down; } break;
				case 'J': { global_new_input_state.letters[9] = (u8)down; } break;
				case 'K': { global_new_input_state.letters[10] = (u8)down; } break;
				case 'L': { global_new_input_state.letters[11] = (u8)down; } break;
				case 'M': { global_new_input_state.letters[12] = (u8)down; } break;
				case 'N': { global_new_input_state.letters[13] = (u8)down; } break;
				case 'O': { global_new_input_state.letters[14] = (u8)down; } break;
				case 'P': { global_new_input_state.letters[15] = (u8)down; } break;
				case 'Q': { global_new_input_state.letters[16] = (u8)down; } break;
				case 'R': { global_new_input_state.letters[17] = (u8)down; } break;
				case 'S': { global_new_input_state.letters[18] = (u8)down; } break;
				case 'T': { global_new_input_state.letters[19] = (u8)down; } break;
				case 'U': { global_new_input_state.letters[20] = (u8)down; } break;
				case 'V': { global_new_input_state.letters[21] = (u8)down; } break;
				case 'W': { global_new_input_state.letters[22] = (u8)down; } break;
				case 'X': { global_new_input_state.letters[23] = (u8)down; } break;
				case 'Y': { global_new_input_state.letters[24] = (u8)down; } break;
				case 'Z': { global_new_input_state.letters[25] = (u8)down; } break;

				/* numbers */
				case '0': { global_new_input_state.numbers[0] = (u8)down; } break;
				case '1': { global_new_input_state.numbers[1] = (u8)down; } break;
				case '2': { global_new_input_state.numbers[2] = (u8)down; } break;
				case '3': { global_new_input_state.numbers[3] = (u8)down; } break;
				case '4': { global_new_input_state.numbers[4] = (u8)down; } break;
				case '5': { global_new_input_state.numbers[5] = (u8)down; } break;
				case '6': { global_new_input_state.numbers[6] = (u8)down; } break;
				case '7': { global_new_input_state.numbers[7] = (u8)down; } break;
				case '8': { global_new_input_state.numbers[8] = (u8)down; } break;
				case '9': { global_new_input_state.numbers[9] = (u8)down; } break; 
			}
		} break;
		case WM_LBUTTONDOWN:
		{
			log_debug("left mouse button is down");
			global_new_input_state.mouse_left = 1;
		} break;
		case WM_LBUTTONUP:
		{
			log_debug("left mouse button is up");
			global_new_input_state.mouse_left = 0;
		} break;
		case WM_RBUTTONDOWN:
		{
			log_debug("right mouse button is down");
			global_new_input_state.mouse_right = 1;
		} break;
		case WM_RBUTTONUP:
		{
			log_debug("right mouse button is up");
			global_new_input_state.mouse_right = 0;
		} break;
		case WM_MBUTTONDOWN:
		{
			log_debug("middle mouse button is down");
			global_new_input_state.mouse_wheel = 1;
		} break;
		case WM_MBUTTONUP:
		{
			log_debug("middle mouse button is up");
			global_new_input_state.mouse_wheel = 0;
		} break;
		case WM_MOUSEWHEEL:
		{
			if(w_param > 0)
			{
				global_new_input_state.mouse_wheel_up = 1;
				log_debug("mouse wheel up");
			}
			if(w_param < 0)
			{
				global_new_input_state.mouse_wheel_down = 1;
				log_debug("mouse wheel down");
			}
		} break;
		case WM_MOUSEMOVE:
		{
			/* XXX: I guess this works? the numbers seem a bit off tho. like top left is 0,0 but bot right is like 20-40 pixels off it seems ? */
			global_new_input_state.mouse_x = (i16)(l_param);
			log_debug("sizeof(l_param): %u", sizeof(l_param));
			log_debug("l_param: %x", l_param);
			if(sizeof(l_param) == 4)
			{
				global_new_input_state.mouse_y = (i16)(l_param >> 8);
			}
			if(sizeof(l_param) == 8)
			{
				global_new_input_state.mouse_y = (i16)(l_param >> 16);
			}

			log_debug("%d, %d", global_new_input_state.mouse_x, global_new_input_state.mouse_y);
		} break;
		default:
		{
			log_trace("stargame_win32_callback (message: %u). handing to DefWindowProcA", message);
			result = DefWindowProcA(hwnd, message, w_param, l_param);
		} break;
	}
	return(result);
}

int main(int argc, char **argv)
{
	HINSTANCE h_instance = GetModuleHandle(NULL);
	_assert(h_instance != NULL);
	WNDCLASSA stargame_window_class;
	stargame_window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  	stargame_window_class.lpfnWndProc = stargame_win32_callback;
  	stargame_window_class.cbClsExtra = 0;
  	stargame_window_class.cbWndExtra = 0;
  	stargame_window_class.hInstance = h_instance;
  	stargame_window_class.hIcon = 0;
  	stargame_window_class.hCursor = 0;
  	stargame_window_class.hbrBackground = 0;
  	stargame_window_class.lpszMenuName = 0;
  	stargame_window_class.lpszClassName = "stargame_window_class";

	_assert(RegisterClassA(&stargame_window_class));

	HWND stargame_window = 
		CreateWindowEx(
			0, 
			stargame_window_class.lpszClassName, 
			"stargame_win32", 
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			global_window_width,
			global_window_height,
			0, 
			0, 
			h_instance, 
			0);

	if(stargame_window == NULL)
	{
		_assert_log(0, "failed to create window (error code: %d)", GetLastError());
	}

	log_info("Window successfully created. Starting engine loop");

	while(global_window_running)
	{
		MSG message; 
		while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
		{
			if(message.message == WM_QUIT)
			{
				global_window_running = false;
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		/* TODO: prepare pixelmap */

		HMODULE dll_handle = LoadLibraryA("build/template_game.dll");
		if(dll_handle == 0)
		{
			_assert_log(0, "failed to load .dll (error code: %d)", GetLastError());
		}

		guar game_update_and_render = (guar)GetProcAddress(dll_handle, "game_update_and_render");
		_assert(game_update_and_render);

		
		u8 temp_state;
		u32 input_button_counter;
		for(input_button_counter = 0; input_button_counter < NUM_INPUT_BUTTONS; input_button_counter++)
		{
			temp_state = global_old_input_state.input_buttons[input_button_counter] << 1;
			temp_state += global_new_input_state.input_buttons[input_button_counter];
			global_game_input_state.input_buttons[input_button_counter] = temp_state;
		}

		global_game_input_state.mouse_x = global_new_input_state.mouse_x;
		global_game_input_state.mouse_y = global_new_input_state.mouse_y;

		global_old_input_state = global_new_input_state;

		/* TODO: game_memory_ptr, game_memory, pixmap_data_ptr 
		 *   virtualalloc 
		 *   pixmap stuff? idk look at handmade i say
		 */


		game_update_and_render(0, 0, 0, global_window_width, global_window_height, &global_game_input_state, argv[2]);

		_assert(FreeLibrary(dll_handle));
	}

	log_info("Window closed.");

	return(0);
}
