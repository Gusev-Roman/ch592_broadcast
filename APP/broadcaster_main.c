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
//extern uint8_t *make_adv();

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

typedef struct __sea {
    uint16_t tag1;
    uint16_t val1;
    uint16_t tag2;
    uint16_t val2;
}SEA;

#define CNT_CUBES 902
#define CNT_BUCKETS 76
#define CNT_CUBES2 362
#define CNT_BUCKETS2 16

#define WC_TAG 0x4357
#define WH_TAG 0x4857

volatile uint32_t _old_time_0 = 0;
volatile uint32_t _old_time_1 = 0;
volatile uint8_t is_sleep = 0;
volatile uint8_t _pulse1 = 0;
volatile uint8_t _pulse2 = 0;
volatile uint16_t _wks = 0;

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];     // буфер для BLE стека

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] =
    {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

#define led_pin GPIO_Pin_8
#define led_pin2 GPIO_Pin_10

SEA s;

uint16_t __counter_cold, __counter_hot;

/*
 *  Проблема в том, что мы не знаем, холодный сон или горячий
 *  при холодном BLE перестает работать!
 *  Если горячий - менять счетчики надо сразу
*/
__INTERRUPT
__HIGH_CODE
void GPIOA_IRQHandler(void)
{
    // не забыть про дребезг
    uint32_t test_time =  MS1_TO_SYSTEM_TIME(TMOS_GetSystemClock()); //(R32_RTC_CNT_32K);    // SYS_GetSysTickCnt();
    uint32_t f = GPIOA_ReadITFlagBit(GPIO_Pin_4 | GPIO_Pin_5);

    if(is_sleep == 2){
          is_sleep = 0;         // set active
          PRINT("* Wake (%d, %d)...\r\n", (R8_GLOB_RESET_KEEP), (R32_TMR1_CNT_END) & ~0x10000); // Слово не пропечатывается (?)
          GPIOA_ResetBits(led_pin); // LED on
        /*
        Допустим, _pulse2 == 1, а пришел _pulse1
        _pulse1 поменяется на 1 (правильно), а _pulse2 останется 1 (неправильно)
        */
          if(f & GPIO_Pin_4){ // update cold
              _pulse1 = 1;
          }
          if(f & GPIO_Pin_5){ // update hot
              _pulse2 = 1;
          }
          // на горячую может случиться сброс и импульс будет потерян!
    }
    else{   // is_sleep == 0
        if(f & GPIO_Pin_4){ // update cold
            if(test_time - _old_time_0 > 2000){
                _pulse1 = 1;
                _old_time_0 = test_time;
            }
        }
        if(f & GPIO_Pin_5){ // update hot
            if(test_time - _old_time_1 > 2000){
                _pulse2 = 1;
                _old_time_1 = test_time;
            }
        }
        PRINT("pulses:%d,%d\r\n", _pulse1, _pulse2);
    }
    GPIOA_ClearITFlagBit(f);
}

void update_counter(uint8_t chan){
    uint8_t register _cnt;

    if(chan == 0){
        _cnt = (R8_GLOB_RESET_KEEP);
    }
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
    if(chan == 0){
        SYS_ResetKeepBuf(_cnt);   // UPDATED VALUE
        _old_time_0 = MS1_TO_SYSTEM_TIME(TMOS_GetSystemClock());   // счетчик увеличился, предотвратим дребезг
    }
    else {
        (R32_TMR1_CNT_END) = _cnt;
        _old_time_1 = MS1_TO_SYSTEM_TIME(TMOS_GetSystemClock());   // счетчик увеличился, предотвратим дребезг
        (R32_TMR1_CNT_END) |= 0x10000;                              // set flag to protect by time
    }
    PRINT("Counters: %d.%d; %d.%d\r\n", __counter_cold, (R8_GLOB_RESET_KEEP), __counter_hot, (R32_TMR1_CNT_END) & ~0x10000);
}

extern             tmosTaskID halTaskID;

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
        if(_pulse1){
            update_counter(0);
            _pulse1 = 0;
        }
        if(_pulse2){
            update_counter(1);
            _pulse2 = 0;
        }
        if(is_sleep == 1){              // период активности закончен, отправим спать до нового пульса...
            PRINT("Go Sleep...\r\n");
            is_sleep = 2;
            GPIOA_SetBits(GPIO_Pin_8);  // LED off
            mDelaymS(10);

            LowPower_Sleep(RB_PWR_RAM2K | RB_PWR_RAM24K | RB_PWR_EXTEND | RB_XT_PRE_EN);
            HSECFG_Current(HSE_RCur_100); // Reduced to rated current (HSE bias current increased in low power function)
            is_sleep = 0;

            PRINT("Wake up, Neo! _pulse1 = %d, _pulse2 = %d\r\n", _pulse1, _pulse2);

            // сюда он дойти до перезапуска успевает! Попробуем обновить счетчики?
            if(_pulse1){
                update_counter(0);
                _pulse1 = 0;
            }
            if(_pulse2){
                update_counter(1);
                _pulse2 = 0;
            }
            // Сон завершен! Счетчики скорректированы. Можно делать сброс!
#ifdef USE_RESET
            DelayMs(10);                // time to print messege
            SYS_ResetExecute();         // принудительный резет, т.к. после выхода он где-то застревает через 15 минут
#endif
            // этот код не будет выполнен, т.к. прписходит перезапуск
            tmos_set_event(halTaskID, LED_TIMER_EXPIRED_EVENT);     // fire event at this moment!
        }
        TMOS_SystemProcess();
    }
}

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

        // Тут таится опасность: если младший счетчик был мал, и обнулился, код возьмет дефолтное значение, а оно большое!
        // хорошо, что такая ситуация происходит только при отключении питания, и пока батарея не села - она не грозит!
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
    GPIOA_ModeCfg(led_pin, GPIO_ModeOut_PP_5mA);  // Main LED
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    PFIC_EnableIRQ(GPIO_A_IRQn);

    GPIOA_ITModeCfg(GPIO_Pin_4|GPIO_Pin_5, GPIO_ITMode_FallEdge); // A4 as interrupt source for PULSE1
    //GPIOA_ITModeCfg(GPIO_Pin_5, GPIO_ITMode_FallEdge); // A5 as interrupt source for PULSE2

    load_counter();     // либо EEPROM либо хардкод

    PRINT("%s\r\n", VER_LIB);
    CH59x_BLEInit();            // инициализация стека BLE
    HAL_Init();
    GAPRole_BroadcasterInit();    // library function
    HAL_TimeInit();
    Broadcaster_Init();         // broadcaster.c

    PRINT("*** R8_RESET_STATUS =%02x\r\n", R8_RESET_STATUS & 0x07);
    // поскольку сброс приходит в момент пульса, а счетчик изменялся перед сбросом, нужно защитить от дребезга.
    // Но очень желательно знать, какой из счетчиков требуется защитить
    if((R32_TMR1_CNT_END) & 0x10000) _old_time_1 = MS1_TO_SYSTEM_TIME(TMOS_GetSystemClock());
    // теперь мы защитили только тот пин, который привел к сбросу
    else _old_time_0 = MS1_TO_SYSTEM_TIME(TMOS_GetSystemClock());
    (R32_TMR1_CNT_END) &= ~0x10000;                                     // Reset flag, time is loaded

    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);  // enable wake up by GPIO
    Main_Circulation(); // loop forewer
}

/******************************** endfile @ main ******************************/

