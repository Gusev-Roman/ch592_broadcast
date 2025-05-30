/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#include "CONFIG.h"
#include "HAL.h"
#include "broadcaster.h"
#include "RTC.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] =
    {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

#define led_pin GPIO_Pin_8
uint8_t led_enable = 1;
uint32_t test_time = 0;
uint32_t old_time = 0;
BOOLEAN is_sleep = FALSE;

__INTERRUPT
__HIGH_CODE
void GPIOA_IRQHandler(void)
{
    test_time = TMOS_GetSystemClock( ); //SYS_GetClockValue();
    if(RTC_TO_MS(test_time - old_time) > 150){    // debounce
      is_sleep = !is_sleep;
      old_time = test_time;
    }
    uint32_t f = GPIOA_ReadITFlagBit(GPIO_Pin_4);
    GPIOA_ClearITFlagBit(f);
}
/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   Endless loop for BLE stack
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation()
{
    while(1)
    {
        if(is_sleep){
          LowPower_Sleep(RB_PWR_RAM2K);
        }
        /*
        if(led_enable){
          DelayMs(100);
          GPIOA_ResetBits(led_pin);
        }
        */
        TMOS_SystemProcess();
        /*
        DelayMs(100);
        GPIOA_SetBits(led_pin);
        */
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Code start
 *
 * @return  none
 */
int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
    GPIOA_ModeCfg(led_pin, GPIO_ModeOut_PP_5mA);  // A8
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    GPIOA_ITModeCfg(GPIO_Pin_4, GPIO_ITMode_FallEdge);
    PFIC_EnableIRQ(GPIO_A_IRQn);

    PRINT("%s\n", VER_LIB);
    CH59x_BLEInit();
    HAL_Init();
    GAPRole_BroadcasterInit();    // library function
    HAL_TimeInit();
    Broadcaster_Init();         // broadcaster.c

    // 志 改找抉技 把快忪我技快 技我忍忘扶我快 扶快 把忘忌抉找忘快找. 忱我抉忱 忍抉把我找.
    //LowPower_Idle();      // go to idle mode
    //LowPower_Sleep(RB_PWR_RAM2K);
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);

    Main_Circulation();
}

/******************************** endfile @ main ******************************/
