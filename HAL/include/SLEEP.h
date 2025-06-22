/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#ifndef __SLEEP_H
#define __SLEEP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern volatile BOOL hal_sleep_no_low_power_flag;

/*********************************************************************
 * FUNCTIONS
 */

/**
 * @brief   Configure sleep wake-up mode - RTC wake-up, trigger mode
 */
extern void HAL_SleepInit(void);

static inline void HAL_SLEEP_IRQPostProcess(void)
{
    hal_sleep_no_low_power_flag = TRUE;
}

/**
 * @brief   Æô¶¯Ë¯Ãß
 *
 * @param   time    - Wake-up time (RTC absolute value)
 *
 * @return  state.
 */
extern uint32_t CH59x_LowPower(uint32_t time);

#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
#define HAL_SLEEP_IRQ_POSTPROCESS()             HAL_SLEEP_IRQPostProcess();
#else
#define HAL_SLEEP_IRQ_POSTPROCESS()
#endif


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
