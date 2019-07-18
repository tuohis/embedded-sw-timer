#ifndef SW_TIMER_H
#define SW_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TIMER_N_TIMERS 8

typedef void (*VoidVoidFn)(void);

struct SwTimerContext;

typedef enum TimerMode { Once, Continuous } TimerMode;
typedef VoidVoidFn TimerCallbackFn;

typedef void (*InterruptHandlerRegistrationFn)(VoidVoidFn handler);
typedef void (*SetNextExpireIntervalFn)(uint32_t time_units);
typedef uint32_t (*GetElapsedTimeUnitsFn)(void);

struct HWTimer {
    /**
     * A function should allow registering another function to be run at the
     * HW timer's expiration interrupt
     */
    InterruptHandlerRegistrationFn register_interrupt_handler;

    /**
     * Schedule the HW timer's next expiration to `time_units` Time Units away. It
     * is allowed for it to expire sooner, too, if its period isn't long enough.
     */
    SetNextExpireIntervalFn set_next_expire_interval;

    /**
     * Start the HW timer
     */
    VoidVoidFn start;

    /**
     * Stop the HW timer. Useful when no SW timers allocated, can save some power
     */
    VoidVoidFn stop;

    /**
     * Get elapsed Time Units since previous invocation.
     */
    GetElapsedTimeUnitsFn get_elapsed;
};

/**
 * Init the whole timer module. Pass in the functions needed to operate the actual HW timer.
 */
void sw_timer_init(const struct HWTimer*);

/**
 * Allocate a timer from the timer pool
 */
struct SwTimerContext* sw_timer_allocate(void);

/**
 * Start a timer with the selected mode and target count. If callback_fn is defined, it's called
 * in an interrupt handler after reaching the target count.
 *
 * @param timer       A pointer to the timer context struct
 * @param mode        The timer mode (Once or Continuous)
 * @param period      The timer period in Units
 * @param callback_fn A function pointer which is called after timer reaches `period`.
 *                    NOTE: as this is called in an interrupt context it should be MINIMAL,
 *                    i.e. just setting a flag or similar.
 */
void sw_timer_start(struct SwTimerContext* timer, TimerMode mode, uint32_t period, TimerCallbackFn callback_fn);

/**
 * Get the elapsed Time Units of a timer which is running or stopped.
 */
uint32_t sw_timer_get_value(const struct SwTimerContext* timer);

/**
 * Stop a timer.
 */
void sw_timer_stop(struct SwTimerContext* timer);

/**
 * Deallocate a timer, i.e. release the resource so it can be allocated by someone else.
 * The pointer shouldn't be referenced after deallocating it.
 */
void sw_timer_deallocate(struct SwTimerContext* timer);

#ifdef __cplusplus
}
#endif

#endif // SW_TIMER_H
