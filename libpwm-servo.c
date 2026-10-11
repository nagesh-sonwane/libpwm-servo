// SPDX-License-Identifier: GPL-2.0-only OR MIT
// Copyright (c) 2024 Nagesh Sonwane <nagesh.sonwane@hotmail.com>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "pwm-servo.h"

#define MAX_PWM_CHANNELS 2
#define DEFAULT_PERIOD_US 20000
#define MAX_PERIOD_US 5000000
#define SERVO_STEP_US 11.11

typedef struct {
	int max_duty_us;
	int min_duty_us;
	int period_us;
} pwm_state;

static int pwm_chip;
static int pwm_count;
static pwm_state *pwm_channels[MAX_PWM_CHANNELS];

static bool write_value(const char *path, unsigned long long value)
{
	char buffer[32];
	int fd;
	int length;
	ssize_t written;

	length = snprintf(buffer, sizeof(buffer), "%llu", value);
	if (length < 0 || (size_t)length >= sizeof(buffer))
		return true;

	fd = open(path, O_WRONLY);
	if (fd < 0)
		return true;

	written = write(fd, buffer, (size_t)length);
	close(fd);
	return written != length;
}

static bool attribute_path(char *path, size_t size, int number, const char *attribute)
{
	int length = snprintf(path, size, "/sys/class/pwm/pwmchip%d/pwm%d/%s",
					  pwm_chip, number, attribute);
	return length < 0 || (size_t)length >= size;
}

static bool valid_pwm(int number)
{
	return number >= 0 && number < pwm_count && number < MAX_PWM_CHANNELS &&
		pwm_channels[number] != NULL;
}

void set_pwmchip(int chip)
{
	if (chip >= 0)
		pwm_chip = chip;
}

int find_pwms(void)
{
	char path[128];
	char buffer[32];
	char *end;
	ssize_t count;
	int fd;
	long channels;

	pwm_count = 0;
	if (snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/npwm", pwm_chip) >= (int)sizeof(path))
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	count = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (count <= 0)
		return -1;

	buffer[count] = '\0';
	errno = 0;
	channels = strtol(buffer, &end, 10);
	if (errno || end == buffer || channels <= 0 || channels > INT_MAX)
		return -1;

	pwm_count = (int)channels;
	return pwm_count;
}

bool get_pwm(int number, int period_us, int max_duty_us, int min_duty_us)
{
	char path[128];
	pwm_state *state;
	int count;
	int initial_duty;

	count = find_pwms();
	if (count < 0 || number < 0 || number >= count ||
		number >= MAX_PWM_CHANNELS || pwm_channels[number])
		return true;

	if (period_us == 0)
		period_us = DEFAULT_PERIOD_US;
	if (max_duty_us == 0)
		max_duty_us = 2500;
	if (min_duty_us == 0)
		min_duty_us = 500;
	if (period_us < 1 || period_us > MAX_PERIOD_US || min_duty_us < 0 ||
		max_duty_us <= min_duty_us || max_duty_us > period_us)
		return true;

	state = calloc(1, sizeof(*state));
	if (!state)
		return true;
	state->period_us = period_us;
	state->min_duty_us = min_duty_us;
	state->max_duty_us = max_duty_us;
	pwm_channels[number] = state;

	if (snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/export", pwm_chip) >= (int)sizeof(path) ||
		write_value(path, (unsigned long long)number) || set_period(number, period_us))
		goto error_unexport;

	initial_duty = min_duty_us + (max_duty_us - min_duty_us) / 2;
	if (set_duty(number, initial_duty))
		goto error_unexport;
	return false;

error_unexport:
	if (snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/unexport", pwm_chip) < (int)sizeof(path))
		write_value(path, (unsigned long long)number);
	free(pwm_channels[number]);
	pwm_channels[number] = NULL;
	return true;
}

bool set_period(int number, long period_us)
{
	char path[128];

	if (!valid_pwm(number) || period_us < 1 || period_us > MAX_PERIOD_US ||
		pwm_channels[number]->max_duty_us > period_us ||
		attribute_path(path, sizeof(path), number, "period"))
		return true;
	if (write_value(path, (unsigned long long)period_us * 1000ULL))
		return true;
	pwm_channels[number]->period_us = (int)period_us;
	return false;
}

bool set_duty(int number, long duty_us)
{
	char path[128];

	if (!valid_pwm(number) || duty_us < 0 || duty_us > pwm_channels[number]->period_us ||
		attribute_path(path, sizeof(path), number, "duty_cycle"))
		return true;
	return write_value(path, (unsigned long long)duty_us * 1000ULL);
}

bool enable(int number)
{
	char path[128];
	if (!valid_pwm(number) || attribute_path(path, sizeof(path), number, "enable"))
		return true;
	return write_value(path, 1);
}

bool disable(int number)
{
	char path[128];
	if (!valid_pwm(number) || attribute_path(path, sizeof(path), number, "enable"))
		return true;
	return write_value(path, 0);
}

bool close_pwm(int number)
{
	char path[128];
	bool failed;

	if (!valid_pwm(number))
		return true;
	failed = disable(number);
	if (snprintf(path, sizeof(path), "/sys/class/pwm/pwmchip%d/unexport", pwm_chip) >= (int)sizeof(path) ||
		write_value(path, (unsigned long long)number))
		failed = true;
	free(pwm_channels[number]);
	pwm_channels[number] = NULL;
	return failed;
}

bool set_pos(int number, int position_degrees)
{
	long duty_us;

	if (!valid_pwm(number) || position_degrees < 0 || position_degrees > 180)
		return true;
	duty_us = pwm_channels[number]->max_duty_us -
		((long)(pwm_channels[number]->max_duty_us - pwm_channels[number]->min_duty_us) *
		 position_degrees) / 180;
	return set_duty(number, duty_us);
}

static bool read_duty(int number, long *duty_us)
{
	char path[128];
	char buffer[32];
	char *end;
	ssize_t count;
	int fd;
	unsigned long long value;

	if (!valid_pwm(number) || !duty_us ||
		attribute_path(path, sizeof(path), number, "duty_cycle"))
		return true;
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return true;
	count = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (count <= 0)
		return true;
	buffer[count] = '\0';
	errno = 0;
	value = strtoull(buffer, &end, 10);
	if (errno || end == buffer || value / 1000ULL > LONG_MAX)
		return true;
	*duty_us = (long)(value / 1000ULL);
	return false;
}

bool cstep(int number, int angle_degrees)
{
	long duty_us;
	if (angle_degrees < 0 || angle_degrees > 180 || read_duty(number, &duty_us))
		return true;
	return set_duty(number, duty_us - (long)(angle_degrees * SERVO_STEP_US));
}

bool acstep(int number, int angle_degrees)
{
	long duty_us;
	if (angle_degrees < 0 || angle_degrees > 180 || read_duty(number, &duty_us))
		return true;
	return set_duty(number, duty_us + (long)(angle_degrees * SERVO_STEP_US));
}