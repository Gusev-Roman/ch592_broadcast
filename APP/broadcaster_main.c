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
#include "SLEEP.h"
#include <stdlib.h>
#include <stdio.h>

void update_counter(uint8_t chan);

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

typedef struct __sea {
    uint16_t tag1;
    uint16_t val1;
    uint16_t tag2;
    uint16_t val2;
}SEA;

#define WC_TAG 0x4357
#define WH_TAG 0x4857

uint8_t _pulse = 0;
uint8_t _pulse2 = 0;

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];     // буфер для BLE стека

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] =
    {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

#define led_pin GPIO_Pin_8
SEA s;
uint8_t led_enable = 1;
uint32_t test_time = 0;
uint32_t old_time = 0;
uint8_t is_sleep = 0;   // 0: working, 1: sleep request 2: sleeping
uint16_t __counter_cold, __counter_hot;


__INTERRUPT
__HIGH_CODE
void HardFault_Handler(void){
    PRINT("*** HardFault ***\r\n");
}

__INTERRUPT
__HIGH_CODE
void WDOG_BAT_IRQHandler(){
    PRINT("*** WDOG_BAT ***\r\n");
}

__INTERRUPT
__HIGH_CODE
void GPIOA_IRQHandler(void)
{
    test_time =  MS1_TO_SYSTEM_TIME(TMOS_GetSystemClock()); //(R32_RTC_CNT_32K);    // SYS_GetSysTickCnt();

    uint32_t f = GPIOA_ReadITFlagBit(GPIO_Pin_4 | GPIO_Pin_5);

    // не забыть про дребезг
    if(is_sleep == 2){
          is_sleep = 0;         // set active
          _pulse = 1;           // этот пульс не отработает, т.к. предстоит сброс и переменные обнулятся. Увеличим значение в регистре!
          update_counter(0);
          PRINT("$$$ Wake %X...\r\n", (R8_GLOB_RESET_KEEP)); // Слово не пропечатывается (?)
          GPIOA_ResetBits(led_pin); // LED on
          //tmos_start_task(halTaskID, LED_TIMER_EXPIRED_EVENT, MS1_TO_SYSTEM_TIME(1000));
#ifdef USE_RESET
          sys_safe_access_enable();
          R8_RST_WDOG_CTRL |= RB_SOFTWARE_RESET;        // принудительный резет, т.к. после выхода он где-то застревает через 15 минут
          sys_safe_access_disable();
#endif

    }
    else{   // is_sleep == 0
        if(f & GPIO_Pin_4){ // update cold
            update_counter(0);
        }
        if(f & GPIO_Pin_5){ // update hot
            update_counter(1);
        }
        PRINT("sleep=%d; old:%d; test:%d\r\n", is_sleep, old_time, test_time);

        if(test_time - old_time > 2000){   // около 2 сек для избежания дребезга
            old_time = test_time;
            //PRINT("*PULSE*\r\n");
            _pulse = 1;
        }
    }
    // при просыпании надо запустить таймер, иначе мигалка не работает!
    PRINT("#%X ", (uint8_t)(R32_TMR1_CNT_END)); // сколько решеток, столько дребезга

    //uint32_t z = GPIOA_ReadITFlagBit(GPIO_Pin_5);
    PRINT("Interrupt flags: %x\r\n", f);
    GPIOA_ClearITFlagBit(f);
}

void update_counter(uint8_t chan){
    uint8_t register _cnt;

    if(chan == 0)
        _cnt = (R8_GLOB_RESET_KEEP);
    else{
        // если использовать переменную, ближайший сброс обнулит ее. Попробовать использовать таймер в стоячем режиме
        _cnt = (R32_TMR1_CNT_END);
    }
    _cnt++;
    if(_cnt == 101){
        // увеличить основной счетчик!
        if(chan == 0){
            __counter_cold++;
        }
        else {
            __counter_hot++;
        }
        // прописать его в EEPROM
        s.tag1 = WC_TAG;
        s.val1 = __counter_cold;
        s.tag2 = WH_TAG;
        s.val2 = __counter_hot;

        EEPROM_WRITE(0, &s, sizeof(SEA));
        _cnt = 1;
    }
    SYS_ResetKeepBuf(_cnt);   // UPDATED VALUE
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
        /**
         * Прочитать значение счетчика и вставить его в ADV
         */
        if(_pulse){
            uint8_t x8 = (R8_GLOB_RESET_KEEP);      // увеличивать счетчик нужно не по входу в сон, а по импульсу.
            x8++;
            if(x8 == 101){
                x8 = 1;
                // увеличить основной счетчик!
                __counter_cold++;
                // прописать его в EEPROM
                //sprintf(eebuf, "WC%d.%d\0", __counter, x8);
                //EEPROM_WRITE(0, eebuf, 16);
            }
            SYS_ResetKeepBuf(x8);   // UPDATED VALUE
            _pulse = 0;
            PRINT("CNT:%d.%d\r\n", __counter_cold, x8);
        }
        if(_pulse2){

        }
        if(is_sleep == 1){
            PRINT("Go Sleep...\r\n");
            is_sleep = 2;
            GPIOA_SetBits(GPIO_Pin_8);  // LED off
            //LowPower_Halt(); //(RB_PWR_RAM2K);
            LowPower_Sleep(RB_PWR_RAM2K | RB_PWR_RAM24K | RB_PWR_EXTEND | RB_XT_PRE_EN);
        }
        TMOS_SystemProcess();
    }
}
#define CNT_CUBES 898
#define CNT_BUCKETS 90
#define CNT_CUBES2 360
#define CNT_BUCKETS2 68

static void load_counter(){
    uint8_t _lcounter, _hcounter;   // эти переменные действуют только в этой процедуре

    EEPROM_READ(0, &s, sizeof(SEA));
    if(s.tag1 == 0xFFFF){  // first read, load hardcoded value
        // счетчик показывает с точностью до литра, но импульс приходит на каждые 10л
        s.tag1 = WC_TAG;
        s.tag2 = WH_TAG;
        s.val1 = CNT_CUBES;
        s.val2 = CNT_CUBES2;

        EEPROM_WRITE(0, &s, sizeof(SEA));
        __counter_cold = CNT_CUBES;
        __counter_hot = CNT_CUBES2;
        _lcounter = CNT_BUCKETS;
        _hcounter = CNT_BUCKETS2;
        SYS_ResetKeepBuf(_lcounter);
        (R32_TMR1_CNT_END) = _hcounter; // timer1 is off!
        PRINT("SET default value %d.%d, %d.%d\r\n", s.val1, _lcounter, s.val2, _hcounter);
    }
    else{
        __counter_cold = s.val1;
        __counter_hot = s.val2;
        if(0 == (R8_GLOB_RESET_KEEP)){
            SYS_ResetKeepBuf(CNT_BUCKETS);  // если отключалось питание, там лежит ноль, неверно!
            _hcounter = CNT_BUCKETS2;
            (R32_TMR1_CNT_END) = _hcounter;
        }
        _lcounter = (R8_GLOB_RESET_KEEP);
        _hcounter = (R32_TMR1_CNT_END);     // если в регистрах не нули - читаем их в переменные (отобразим, чтобы проконтролировать)

        PRINT("EEPROM is not empty, using value %d.%d, %d.%d\r\n", __counter_cold, _lcounter, __counter_hot, _hcounter);
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
    uint16_t readbuf[8];
    // bucket counter
    static uint8_t cbyte = 0;

    if(R8_GLOB_RESET_KEEP == 0){
        (R32_TMR1_CNT_END) = CNT_BUCKETS2;      // писать только если R8_GLOB_RESET_KEEP равен нулю.
    }
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
    WWDG_ResetCfg(ENABLE);
    WWDG_ITCfg(ENABLE);
    GPIOA_ITModeCfg(GPIO_Pin_4, GPIO_ITMode_FallEdge); // A4 as interrupt source for PULSE1
    GPIOA_ITModeCfg(GPIO_Pin_5, GPIO_ITMode_FallEdge); // A5 as interrupt source for PULSE2
    PFIC_EnableIRQ(GPIO_A_IRQn);

    // в cbyte будет счетчик ведер, как только он достигнет 101 - изменяем значение в EEPROM, в cbyte кладем 1
    load_counter();     // либо EEPROM либо хардкод
    cbyte = (R8_GLOB_RESET_KEEP);

    if(!cbyte){ // этот код не сможет исполниться
        cbyte++;
        SYS_ResetKeepBuf(cbyte);
    }
    PRINT("%s; %x\r\n", VER_LIB, cbyte);
    CH59x_BLEInit();
    HAL_Init();
    GAPRole_BroadcasterInit();    // library function
    HAL_TimeInit();
    Broadcaster_Init();         // broadcaster.c

    // Do not translate this!
    // 志 改找抉技 把快忪我技快 技我忍忘扶我快 扶快 把忘忌抉找忘快找. 忱我抉忱 忍抉把我找.
    //LowPower_Idle();      // go to idle mode
    //LowPower_Sleep(RB_PWR_RAM2K);
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE | RB_GPIO_WAKE_MODE, Long_Delay);  // enable wake up by GPIO
    Main_Circulation(); // loop forewer
}

/******************************** endfile @ main ******************************/
