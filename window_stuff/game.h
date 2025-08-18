#ifndef GAME_H
#define GAME_H

typedef struct {
	b8 backspace;
	b8 tab;
	b8 enter;
	b8 escape;
	b8 spacebar;

	b8 left_arrow;
	b8 up_arrow;
	b8 right_arrow;
	b8 down_arrow;

	b8 fkeys[12]; /* NOTE: fkeys[0] is F1 */

	b8 left_shift;
	b8 right_shift;
	b8 left_control;
	b8 right_control;

	b8 letters[26]; /* NOTE: letters[0] is A, letters[25] is Z */

	b8 numbers[10]; /* NOTE: numbers[0] is 0, numbers[9] is 9 */

	b8 mouse_left;
	b8 mouse_right;
	b8 mouse_wheel;
	b8 mouse_wheel_up;
	b8 mouse_wheel_down;

	i16 mouse_x;
	i16 mouse_y;
} game_input_state;

#endif
