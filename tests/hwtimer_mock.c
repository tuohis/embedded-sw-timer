#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "hwtimer_mock.h"

// Forward declarations
void register_handler(VoidVoidFn handler);
void set_expire_interval(uint32_t useconds);
void start(void);
void stop(void);
uint32_t get_elapsed(void);


struct HWTimerState {
    uint64_t counter;
    bool running;
    uint64_t expire_count;
    VoidVoidFn interruptHandler;
};

struct HWTimerState hwTimerState = {
    .counter = 0,
    .running = false,
    .expire_count = UINT64_MAX,
    .interruptHandler = NULL,
};

struct HWTimer timerconf;

struct HWTimer* hwtimer_get(void) {
    timerconf.register_interrupt_handler = register_handler;
    timerconf.set_next_expire_interval = set_expire_interval;
    timerconf.start = start;
    timerconf.stop = stop;
    timerconf.get_elapsed = get_elapsed;

    return &timerconf;
}

void hwtimer_tick(uint32_t useconds) {
    if (hwTimerState.running) {
        hwTimerState.counter += useconds;
        if (hwTimerState.interruptHandler) {
            hwTimerState.interruptHandler();
        }
    }
}

uint32_t hwtimer_get_time_to_expire(void) {
    if (hwTimerState.running) {
        return hwTimerState.expire_count - hwTimerState.counter;
    }
    return 0;
}

//--------------------------------------------
// Internal
//--------------------------------------------

void register_handler(VoidVoidFn handler) {
    hwTimerState.interruptHandler = handler;
}

void set_expire_interval(uint32_t useconds) {
    hwTimerState.expire_count = hwTimerState.counter + useconds;
}

void start(void) {
    hwTimerState.running = true;
}

void stop(void) {
    hwTimerState.running = false;
}

uint32_t get_elapsed(void) {
    static uint64_t previous_counter_value = 0;

    uint32_t value = (uint32_t)(hwTimerState.counter - previous_counter_value);

    previous_counter_value = hwTimerState.counter;

    return value;
}
