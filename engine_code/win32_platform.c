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

/* TODO: this should just be named input_state, not x_input_state */
#define NUM_X_INPUT_BUTTONS 66
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
		u8 x_input_buttons[NUM_X_INPUT_BUTTONS];
	};

	i16 mouse_x;
	i16 mouse_y;
} x_input_state;

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
	x_input_state *,
	char *);

int main()
{
	log_error("%s %d", "test program ", __COUNTER__);
	log_warn("%s %d", "test program ", __COUNTER__);
	log_info("%s %d", "test program ", __COUNTER__);
	log_debug("%s %d", "test program ", __COUNTER__);
	log_lib("%s %d", "test program ", __COUNTER__);
	log_trace("%s %d", "test program ", __COUNTER__);

	HMODULE dll_handle = LoadLibraryA("build/template_game.dll");
	_assert_log((b32)dll_handle,"failed to load .dll (error code: %d)", GetLastError());

	guar game_update_and_render = (guar)GetProcAddress(dll_handle, "game_update_and_render");
	_assert(game_update_and_render);

	game_update_and_render(0, 0, 0, 0, 0, 0, 0);

	_assert(FreeLibrary(dll_handle));

	/* TODO: open template_game.dll and run game_update_and_render from it */

	return(0);
}
