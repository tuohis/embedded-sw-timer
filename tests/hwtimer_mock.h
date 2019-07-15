#ifndef HWTIMER_MOCK_H
#define HWTIMER_MOCK_H

#include "sw_timer.h"

struct HWTimer* hwtimer_get(void);

void hwtimer_tick(uint32_t useconds);
uint32_t hwtimer_get_time_to_expire(void);

#endif
