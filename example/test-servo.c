// SPDX-License-Identifier: GPL-2.0-only OR MIT
// Copyright (c) 2025 Nagesh Sonwane <nagesh.sonwane@hotmail.com>

#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>

#include "../pwm-servo.h"

int main(void)
{
	int duty[2] = { 1500, 1500 };
	int key;
	int result = -1;
	bool pwm_open[2] = { false, false };
	bool terminal_changed = false;
	struct termios original_terminal;
	struct termios raw_terminal;

	if (get_pwm(0, 0, 0, 0))
		goto cleanup;
	pwm_open[0] = true;
	if (get_pwm(1, 0, 0, 0))
		goto cleanup;
	pwm_open[1] = true;

	if (set_duty(0, duty[0]) || set_duty(1, duty[1]) || enable(0) || enable(1))
		goto cleanup;
	if (tcgetattr(STDIN_FILENO, &original_terminal) != 0)
		goto cleanup;
	raw_terminal = original_terminal;
	raw_terminal.c_lflag &= (tcflag_t)~(ICANON | ECHO);
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_terminal) != 0)
		goto cleanup;
	terminal_changed = true;

	while ((key = getchar()) != 'q' && key != EOF) {
		int channel = -1;
		int change = 0;

		if (key == 27 || key == '[')
			continue;
		if (key == 'C') {
			channel = 0;
			change = -50;
		} else if (key == 'D') {
			channel = 0;
			change = 50;
		} else if (key == 'A') {
			channel = 1;
			change = -50;
		} else if (key == 'B') {
			channel = 1;
			change = 50;
		}

		if (channel >= 0) {
			int next_duty = duty[channel] + change;
			if (next_duty >= 500 && next_duty <= 2500) {
				if (set_duty(channel, next_duty))
					goto cleanup;
				duty[channel] = next_duty;
			}
		}
	}
	result = 0;

cleanup:
	if (terminal_changed && tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal) != 0)
		result = -1;
	if (pwm_open[1] && close_pwm(1))
		result = -1;
	if (pwm_open[0] && close_pwm(0))
		result = -1;
	return result;
}