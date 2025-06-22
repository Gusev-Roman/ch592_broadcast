#ifndef PULSE_COUNTER_H
#define PULSE_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "CONFIG.h"
#include "HAL.h"
#include "broadcaster.h"
#include "RTC.h"
#include "SLEEP.h"
#include <stdlib.h>
#include <stdio.h>

void PulseCounter_init(void);

uint32_t PulseCounter_get_counter_cold(void);

uint32_t PulseCounter_get_counter_hot(void);

// §Ó§í§Ù§í§Ó§Ñ§ä§î §Ó §Ü§à§Ý§Ò§ï§Ü§Ö §ä§Ñ§Û§Þ§Ö§â§Ñ
void PulseCounter_on_timer_cold(void);
// §Ó§í§Ù§í§Ó§Ñ§ä§î §Ó §Ü§à§Ý§Ò§ï§Ü§Ö §ä§Ñ§Û§Þ§Ö§â§Ñ
void PulseCounter_on_timer_hot(void);

void PulseCounter_prosess(void);

#ifdef __cplusplus
}
#endif

#endif // PULSE_COUNTER_H
