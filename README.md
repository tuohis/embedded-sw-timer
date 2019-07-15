# embedded-sw-timer
A timer multiplexer library for embedded targets

## Usage

This library is intended to be used so that a single HW timer with interrupt capability drives the SW timer system. The user should provide the library an init struct with the following function pointers:

- register_interrupt_handler: a function the library can call to register its inner function to be called whenever the timer interrupt is triggered
- `void set_next_expire_interval(uint32_t useconds)`: a function that schedules the HW timer's next interrupt to `useconds` microseconds away
- `void start(void)`: a function that starts the HW timer. Can be NULL if the timer is running constantly.
- `void stop(void)`: a function that stops the HW timer to save power when no SW timers are running. Can be NULL if the functionality isn't required.
- `uint32_t get_elapsed(void)`: a function that returns the amount of microseconds that have passed since its previous invocation. Provides an abstraction that hides the HW timer's frequency and other params.

Example:
```C
#include <stdint.h>
#include "sw_timer.h"

uint32_t get_elapsed(void) {
    static uint32_t previous_counter_value = 0;

    const int us_per_tick = 10; // Timer ticking away at 100 kHz

    uint32_t new_counter_value = TIM2->CNT;; // Or whatever your HW interface is

    uint32_t elapsed_microseconds = (new_counter_value - previous_counter_value) * us_per_tick;

    previous_counter_value = new_counter_value;

    return elapsed_microseconds;
}

...

uint8_t callback_callcount = 0;
void timer_callback(void) {
    ++callback_callcount;
}

int main(void) {
    struct HWTimer hwTimerApi = {
        .get_elapsed = get_elapsed,
        ...
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

    // real time passed should be 10 * 100000us = 1s
}
```