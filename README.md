# embedded-sw-timer
A timer multiplexer library for embedded targets

## Summary

Timers can be a scarce resource on embedded targets and their use not very portable.
This library aims to provide easy-ish one-place setup and an intuitive interface for
creating and accessing timers without having to worry about binding them to different physical
timers and configuring each of them.

The library uses just one HW timer and lets the user decide whether it should run in continuous
mode or something else. The user is just required to implement the abstract HW timer interface
for the library to use.

No dynamic allocations. The number of SW timers is predetermined in `src/sw_timer.h` and the
associated structs are statically allocated.

The library should be easily integrated using CMake. The file `tests/CMakeLists.txt` can provide
some hints about that.

## Usage

This library is intended to be used so that a single HW timer with interrupt capability drives the SW timer system. The user should provide the library an init struct with the following function pointers:

- `void register_interrupt_handler(void (*handler)(void))`: a function the library can call to register its inner function to be called whenever the timer interrupt is triggered
- `void set_next_expire_interval(uint32_t useconds)`: a function that schedules the HW timer's next interrupt to `useconds` microseconds away
- `uint32_t get_elapsed(void)`: a function that returns the amount of microseconds that have passed since its previous invocation. Provides an abstraction that hides the HW timer's frequency and other params.
- `void start(void) [optional]`: a function that starts the HW timer. Can be NULL if the timer is running constantly.
- `void stop(void) [optional]`: a function that stops the HW timer to save power when no SW timers are running. Can be NULL if the functionality isn't required.

Example:

```C
#include <stdint.h>
#include <math.h>
#include "sw_timer.h"

// 100kHz timer -> time unit = microseconds,
// 100 Hz timer -> time unit = milliseconds, etc.
// This depends on your HW timer configuration and your choice of Time Unit
const int time_units_per_tick = 10;
const uint32_t max_counter_value = 0xFFFFUL // HW-dependent conf: a 16-bit counter here

uint32_t get_elapsed(void) {
    static uint32_t previous_counter_value = 0;

    uint32_t new_counter_value = TIM2->CNT;; // Or whatever your HW interface is

    uint32_t elapsed_time_units = (new_counter_value - previous_counter_value) * time_units_per_tick;

    previous_counter_value = new_counter_value;

    return elapsed_time_units;
}

void set_next_expire_interval(uint32_t time_units) {
    uint32_t ticks = time_units / time_units_per_tick;

    if (time_units % ticks != 0) {
        // round up so that the timers will expire
        ++ticks;
    }

    if (ticks > max_counter_value) {
        ticks = max_counter_value;
    }

    // Again, this is very much hardware dependent
    TIM2->CNT = 0;
    TIM2->CCR1 = ticks;
}

VoidVoidFn registered_timer_interrupt_handler = NULL;

void TIM2_IRQHandler(void) {
    if (registered_timer_interrupt_handler) {
        registered_timer_interrupt_handler();
    }
}

void register_interrupt_handler(VoidVoidFn handler) {
    registered_timer_interrupt_handler = handler;
}

uint8_t callback_callcount = 0;
void timer_callback(void) {
    ++callback_callcount;
}

int main(void) {
    struct HWTimer hwTimerApi = {
        .register_interrupt_handler = register_interrupt_handler,
        .set_next_expire_interval = set_next_expire_interval,
        .start = NULL,
        .stop = NULL,
        .get_elapsed = get_elapsed,
    };

    sw_timer_init(&hwTimerApi);

    TimerContext* timer = sw_timer_allocate();
    sw_timer_start(timer, Continuous, 100000, timer_callback);

    while(callback_callcount < 10) {
        // Go to low power mode until the callback has been called 10 times
        low_power_mode_0();
    }

    sw_timer_stop(timer); // Stop the timer
    sw_timer_deallocate(timer); // Deallocate the timer (shouldn't be touched after this)

    // real time passed should be 10 * 100000 time units
}
```
## Tests

To build and run the unit tests, do the following:

```sh
# create the buildenv-init and buildenv aliases
source .bashrc

# build the Docker image for compiling
buildenv-init

# use the image to build and run the tests
buildenv bash -c "./scripts/build.sh && build/tests/sw_timer_test"
```
