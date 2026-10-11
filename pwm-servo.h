#ifndef PWM_SERVO_H
#define PWM_SERVO_H

#include <stdbool.h>

void set_pwmchip(int chip);
int find_pwms(void);
bool get_pwm(int number, int period_us, int max_duty_us, int min_duty_us);
bool set_period(int number, long period_us);
bool set_duty(int number, long duty_us);
bool set_pos(int number, int position_degrees);
bool cstep(int number, int angle_degrees);
bool acstep(int number, int angle_degrees);
bool enable(int number);
bool disable(int number);
bool close_pwm(int number);

#endif