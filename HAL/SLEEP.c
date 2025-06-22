/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : 睡眠配置及其初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "HAL.h"

volatile BOOL hal_sleep_no_low_power_flag = FALSE;

/*******************************************************************************
 * @fn          CH59x_LowPower
 *
 * @brief       Start Sleep
 *
 * @param   time    - Wake-up time (RTC absolute value)
 *
 * @return      state.
 */
uint32_t CH59x_LowPower(uint32_t time)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    volatile uint32_t i;
    uint32_t time_sleep, time_curr;
    irq_ctx_t irq_ctx;
    
    // Early wake-up
    if (time <= WAKE_UP_RTC_MAX_TIME) {
        time = time + (RTC_MAX_COUNT - WAKE_UP_RTC_MAX_TIME);
    } else {
        time = time - WAKE_UP_RTC_MAX_TIME;
    }

    irq_ctx = irq_save_ctx_and_disable();
    time_curr = RTC_GetCycle32k();
    // Early wake-up
    if (time < time_curr) {
        time_sleep = time + (RTC_MAX_COUNT - time_curr);
    } else {
        time_sleep = time - time_curr;
    }
    
    irq_restore_ctx(irq_ctx);

    // If the sleep time is less than the minimum sleep time or greater than the maximum sleep time, then do not sleep
    if ((time_sleep < SLEEP_RTC_MIN_TIME) || 
        (time_sleep > SLEEP_RTC_MAX_TIME)) {
        return 2;
    }

    RTC_SetTignTime(time);
  #if(DEBUG == Debug_UART1) // To use other serial ports to print information, you need to modify this line of code
    while((R8_UART1_LSR & RB_LSR_TX_ALL_EMP) == 0)
    {
        __nop();
    }
  #endif
    // LOW POWER-sleep mode
    irq_ctx = irq_save_ctx_and_disable();
    if(!hal_sleep_no_low_power_flag && !RTCTigFlag)
    {
        LowPower_Sleep(RB_PWR_RAM2K | RB_PWR_RAM24K | RB_PWR_EXTEND | RB_XT_PRE_EN );
        irq_restore_ctx(irq_ctx);
        // To handle all pending interrupts

        // Because there may be interrupts
        // longer than one RTC step
        irq_ctx = irq_save_ctx_and_disable();
        i = RTC_GetCycle32k();
        while(i == RTC_GetCycle32k());
        HSECFG_Current(HSE_RCur_100); // Reduced to rated current (HSE bias current increased in low power function)
    }
    hal_sleep_no_low_power_flag = FALSE;
    irq_restore_ctx(irq_ctx);

    return 0;
#endif
    return 3;
}

/*******************************************************************************
 * @fn      HAL_SleepInit
 *
 * @brief   Configure sleep wake-up mode - RTC wake-up, trigger mode
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_SleepInit(void)
{
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    sys_safe_access_enable();
    R8_SLP_WAKE_CTRL |= RB_SLP_RTC_WAKE; // RTC wake-up
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_RTC_MODE_CTRL |= RB_RTC_TRIG_EN;  // Trigger Mode
    sys_safe_access_disable();
    PFIC_EnableIRQ(RTC_IRQn);
#endif
}
