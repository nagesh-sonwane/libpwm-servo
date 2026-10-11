# libpwm-servo

`libpwm-servo` is a small C library for controlling hobby servo motors through the Linux PWM sysfs interface. It was created for use on a Raspberry Pi (RPI) in the Nia DoorGuard robot project.

**Project status:** This code is currently in testing. PWM behavior depends on the Raspberry Pi model, kernel, device-tree configuration, and connected servo hardware. Test carefully with the servo mechanism clear of obstructions.

## Requirements

- A Raspberry Pi running Linux with a PWM controller enabled and exposed under `/sys/class/pwm/pwmchipN`.
- The PWM sysfs interface and permission to export/configure PWM channels. Running the example generally requires `sudo`.
- A C compiler and GNU Make.

The library defaults to PWM chip 0 and currently supports two channels, numbered 0 and 1. Set up the appropriate PWM overlay or device-tree configuration for your Raspberry Pi before running it.

## Build

From this directory, run:

```sh
make
```

This builds `libpwm-servo.so` and the interactive `example/test-servo` program. Remove generated build outputs with:

```sh
make clean
```

## Run the example

With PWM channels 0 and 1 available, run:

```sh
sudo ./example/test-servo
```

The example adjusts channel 0 with the left/right arrow keys and channel 1 with the up/down arrow keys. Press `q` to exit. It restores the terminal settings and closes channels it opened when it exits.

## Library API

Include `pwm-servo.h` and link against `libpwm-servo.so`. The API uses microseconds for period and duty values; the library converts them to nanoseconds for the Linux PWM sysfs interface.

```c
#include "pwm-servo.h"

if (get_pwm(0, 0, 0, 0)) { /* true indicates failure */
    return 1;
}

if (set_pos(0, 90) || enable(0)) {
    close_pwm(0);
    return 1;
}

disable(0);
close_pwm(0);
```

For `get_pwm(number, period_us, max_duty_us, min_duty_us)`, zero-valued configuration arguments select defaults: a 20,000 us period, 2,500 us maximum duty, and 500 us minimum duty. `find_pwms()` returns the channel count for the selected chip, or `-1` if it cannot read the chip configuration. The functions returning `bool` use `false` for success and `true` for failure.

## Author and license

Author: Nagesh Sonwane, [nagesh.sonwane@hotmail.com](mailto:nagesh.sonwane@hotmail.com).

This project is available under either the **GNU General Public License v2.0 only** or the **MIT License**, at your choice, consistent with the source SPDX identifiers (`GPL-2.0-only OR MIT`). See the [GPL-2.0-only license text](https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt) and the [MIT License](https://opensource.org/license/mit/).