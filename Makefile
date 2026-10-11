CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?=
LDFLAGS ?=

LIBRARY = libpwm-servo.so
OBJECT = libpwm-servo.o
EXAMPLE = example/test-servo

.PHONY: all clean

all: $(LIBRARY) $(EXAMPLE)

$(LIBRARY): $(OBJECT)
	$(CC) -shared $(LDFLAGS) -o $@ $^

$(OBJECT): libpwm-servo.c pwm-servo.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -c -o $@ $<

$(EXAMPLE): example/test-servo.c pwm-servo.h $(OBJECT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ example/test-servo.c $(OBJECT) $(LDFLAGS)

clean:
	rm -f $(OBJECT) $(LIBRARY) $(EXAMPLE)