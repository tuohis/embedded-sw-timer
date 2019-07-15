/**
 * This piece of library uses a single hardware timer to multiplex several timers. It works by
 * keeping track of time with the HW timer and setting the next interrupt point to be when the
 * next SW timer is about to expire. Then with each interrupt the SW timer table is updated.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "sw_timer.h"

/**
 * A defined pattern which indicates that the timer struct is allocated.
 * This protects against the memory being more or less random after startup.
 */
#define ALLOCATED_PATTERN 0x91U
/**
 * Mark used but deallocated timers so that we can check afterwards how many
 * were in use simultaneously
 */
#define DEALLOCATED_PATTERN 0x19U

typedef uint32_t TimerValue;

struct SwTimerContext {
    uint8_t allocated;
    bool running;
    TimerMode mode;
    TimerValue value;
    TimerValue period;
    TimerCallbackFn callback;
};

// A counter of how many SW timers are running; easy to turn the HW timer off when reaches zero.
uint8_t running_count = 0;

// This stores the actual timer structs
struct SwTimerContext _timers[TIMER_N_TIMERS];

// This stores the HW timer interface
const struct HWTimer* hwTimerApi = NULL;

// Forward declaration
void _advance_timers(void);
void _handle_expire_interrupt(void);
void _set_expire_interval(void);

// ------------------------------------
// Timer API
// ------------------------------------

void sw_timer_init(const struct HWTimer* timer) {
    // Don't init if no context, also don't reinit if already operational
    if (timer == NULL || hwTimerApi != NULL) {
        return;
    }

    hwTimerApi = timer;

    // Register interrupt handler
    hwTimerApi->register_interrupt_handler(_handle_expire_interrupt);
}

struct SwTimerContext* sw_timer_allocate(void) {
    // Go through the timers and return the first non-allocated one
    struct SwTimerContext* timer = _timers;
    while (timer < _timers + TIMER_N_TIMERS) {
        if (timer->allocated != ALLOCATED_PATTERN) {
            // Not yet allocated -> mark as allocated and use it
            timer->allocated = ALLOCATED_PATTERN;
            return timer;
        }
        timer++;
    }
    return NULL;
}

void sw_timer_deallocate(struct SwTimerContext* timer) {
    timer->running = false;
    timer->mode = Once;
    timer->value = 0;
    timer->period = 0;
    timer->callback = NULL;
    timer->allocated = DEALLOCATED_PATTERN;
}

void sw_timer_start(struct SwTimerContext* timer, TimerMode mode, uint32_t period_us, TimerCallbackFn callback_fn) {
    if (!timer->running) {
        if (running_count == 0 && hwTimerApi->start != NULL) {
            hwTimerApi->start();
        }

        ++running_count;
    }

    _advance_timers();

    timer->mode = mode;
    timer->period = period_us;
    timer->callback = callback_fn;
    timer->value = 0;
    timer->running = true;

    _set_expire_interval();
}

uint32_t sw_timer_stop(struct SwTimerContext* timer) {
    if (timer->running) {
        timer->running = false;
        if (running_count > 0) {
            --running_count;

            if (running_count == 0 && hwTimerApi->stop != NULL) {
                hwTimerApi->stop();
            }
        }
    }
}

uint32_t sw_timer_get_value(const struct SwTimerContext* timer) {
    return timer->value;
}

// ----------------------------------
// Timer internals
// ----------------------------------

void _advance_timers(void) {
    if (running_count == 0) {
        return;
    }

    uint32_t elapsed = hwTimerApi->get_elapsed();
    uint8_t handled_count = 0;

    for (struct SwTimerContext* timer = _timers; handled_count < running_count && timer < _timers + TIMER_N_TIMERS; ++timer) {

        if (timer->allocated == ALLOCATED_PATTERN && timer->running) {
            // Avoid overflow by subtracting instead of adding
            if (elapsed >= timer->period || timer->value >= timer->period - elapsed) {
                // Timer expires
                switch (timer->mode) {
                    case Once:
                        timer->value = timer->period;
                        timer->running = false;
                        break;

                    case Continuous:
                        timer->value = timer->value + elapsed - timer->period;
                        break;

                    default:
                        break;
                }

                if (timer->callback) {
                    timer->callback();
                }

            }
            else {
                timer->value += elapsed;
            }

            ++handled_count;
        }
    }
}

uint32_t _get_shortest_remaining_interval(void) {
    if (running_count == 0) {
        return 0;
    }

    uint32_t value = UINT32_MAX;
    uint8_t handled_count = 0;
    for (struct SwTimerContext* timer = _timers; handled_count < running_count && timer < _timers + TIMER_N_TIMERS; ++timer) {
        if (timer->allocated == ALLOCATED_PATTERN && timer->running) {
            if (timer->value < timer->period && timer->period - timer->value < value) {
                value = timer->period - timer->value;
            }
        }
    }
    return value;
}

void _set_expire_interval(void) {
    uint32_t shortest_interval = _get_shortest_remaining_interval();
    if (shortest_interval > 0) {
        hwTimerApi->set_next_expire_interval(shortest_interval);
    }
}

void _handle_expire_interrupt(void) {
    _advance_timers();
    _set_expire_interval();
}
