typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef unsigned int b32;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#define NUM_X_INPUT_BUTTONS 66

enum {
	INPUT_BUTTON_STATE_UP = 0x00,
	INPUT_BUTTON_STATE_DOWN = 0x03,
	INPUT_BUTTON_STATE_RELEASED = 0x04,
	INPUT_BUTTON_STATE_PRESSED = 0x01
};

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
