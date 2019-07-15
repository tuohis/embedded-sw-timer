#include <config.h>
#include <stdlib.h>
#include <stdint.h>
#include <check.h>

#include "../src/sw_timer.h"
#include "hwtimer_mock.h"

void setup(void) {

}

void teardown(void) {

}

START_TEST(test_timer_allocate)
{
    struct SwTimerContext* timers[TIMER_N_TIMERS+1];

    for (int i = 0; i < TIMER_N_TIMERS; ++i) {
        timers[i] = sw_timer_allocate();
        ck_assert_ptr_ne(timers[i], NULL);
    }
    timers[TIMER_N_TIMERS] = sw_timer_allocate();
    ck_assert_ptr_eq(timers[TIMER_N_TIMERS], NULL);

    sw_timer_deallocate(timers[0]);
    timers[0] = NULL;

    timers[0] = sw_timer_allocate();
    ck_assert_ptr_ne(timers[0], NULL);

    for (int i = 0; i < TIMER_N_TIMERS; ++i) {
        sw_timer_deallocate(timers[i]);
    }
}
END_TEST

uint32_t callback1_call_count = 0;
void callback1(void) {
    ++callback1_call_count;
}

uint32_t callback2_call_count = 0;
void callback2(void) {
    ++callback2_call_count;
}

START_TEST(test_timer_operation)
{
    sw_timer_init(hwtimer_get()); // Note: arbitrary Time Units are microseconds here

    struct SwTimerContext* timerOnce = sw_timer_allocate();
    struct SwTimerContext* timerContinuous = sw_timer_allocate();

    ck_assert_int_eq(callback1_call_count, 0);
    ck_assert_int_eq(callback2_call_count, 0);

    sw_timer_start(timerOnce, Once, 500000, callback1); // 500ms once

    ck_assert_int_eq(sw_timer_get_value(timerOnce), 0);
    ck_assert_int_eq(hwtimer_get_time_to_expire(), 500000);

    sw_timer_start(timerContinuous, Continuous, 200000, callback2); // every 200ms

    ck_assert_int_eq(sw_timer_get_value(timerContinuous), 0);
    ck_assert_int_eq(hwtimer_get_time_to_expire(), 200000);

    hwtimer_tick(1000);

    ck_assert_int_eq(hwtimer_get_time_to_expire(), 199000);

    ck_assert_int_eq(sw_timer_get_value(timerOnce), 1000);
    ck_assert_int_eq(sw_timer_get_value(timerContinuous), 1000);

    ck_assert_int_eq(callback1_call_count, 0);
    ck_assert_int_eq(callback2_call_count, 0);

    hwtimer_tick(199000); // Total time: 200,000 us

    ck_assert_int_eq(hwtimer_get_time_to_expire(), 200000); // Next up: continuous @ 400,000 us

    ck_assert_int_eq(sw_timer_get_value(timerOnce), 200000);
    ck_assert_int_eq(sw_timer_get_value(timerContinuous), 0);

    ck_assert_int_eq(callback1_call_count, 0);
    ck_assert_int_eq(callback2_call_count, 1);

    hwtimer_tick(50000); // It's allowed that the timer issues an interrupt also when it's not really due
    // Total time now 250,000 us

    ck_assert_int_eq(hwtimer_get_time_to_expire(), 150000); // Next up: continuous @ 400,000 us

    ck_assert_int_eq(sw_timer_get_value(timerOnce), 250000);
    ck_assert_int_eq(sw_timer_get_value(timerContinuous), 50000);

    ck_assert_int_eq(callback1_call_count, 0);
    ck_assert_int_eq(callback2_call_count, 1);

    hwtimer_tick(150000); // Total time: 400,000 us

    ck_assert_int_eq(hwtimer_get_time_to_expire(), 100000); // Next up: once @ 500,000 us

    ck_assert_int_eq(sw_timer_get_value(timerOnce), 400000);
    ck_assert_int_eq(sw_timer_get_value(timerContinuous), 0);

    ck_assert_int_eq(callback1_call_count, 0);
    ck_assert_int_eq(callback2_call_count, 2);

    hwtimer_tick(100000); // Total time: 500,000 us

    ck_assert_int_eq(hwtimer_get_time_to_expire(), 100000); // Next up: continuous @ 600,000 us

    ck_assert_int_eq(sw_timer_get_value(timerOnce), 500000);
    ck_assert_int_eq(sw_timer_get_value(timerContinuous), 100000);

    ck_assert_int_eq(callback1_call_count, 1);
    ck_assert_int_eq(callback2_call_count, 2);

    hwtimer_tick(110000); // The interrupt is late for whatever reason; total time: 610,000 us

    ck_assert_int_eq(hwtimer_get_time_to_expire(), 190000); // Next up: continuous @ 800,000 us

    ck_assert_int_eq(sw_timer_get_value(timerOnce), 500000); // The timer is stopped and doesn't advance
    ck_assert_int_eq(sw_timer_get_value(timerContinuous), 10000); // The timer wraps

    ck_assert_int_eq(callback1_call_count, 1);
    ck_assert_int_eq(callback2_call_count, 3);

    sw_timer_stop(timerContinuous);

    hwtimer_tick(190000); // total time 800,000 us

    ck_assert_int_eq(sw_timer_get_value(timerOnce), 500000); // The timer is stopped and doesn't advance
    ck_assert_int_eq(sw_timer_get_value(timerContinuous), 10000); // The timer wraps

    ck_assert_int_eq(callback1_call_count, 1);
    ck_assert_int_eq(callback2_call_count, 3);
}
END_TEST

Suite * timer_suite(void)
{
    Suite *s;
    TCase *tc_allocation;
    TCase *tc_operation;

    s = suite_create("Timer");

    /* Allocation test case */
    tc_allocation = tcase_create("Allocation");

    // tcase_add_checked_fixture(tc_allocation, setup, teardown);
    tcase_add_test(tc_allocation, test_timer_allocate);
    suite_add_tcase(s, tc_allocation);

    tc_operation = tcase_create("Operation");

    tcase_add_test(tc_operation, test_timer_operation);
    suite_add_tcase(s, tc_operation);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = timer_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
