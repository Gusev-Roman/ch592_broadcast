/********************************** (C) COPYRIGHT *******************************
 * File Name          : CONFIG.h
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : Configuration description and default value. It is recommended to modify the current value in the preprocessing in the project configuration.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#ifndef __CONFIG_H
#define __CONFIG_H

#define	ID_CH592							0x92

#define CHIP_ID								ID_CH592

#ifndef USE_CH58x_LIB
#ifdef CH59xBLE_ROM
#include "CH59xBLE_ROM.h"
#else
#include "CH59xBLE_LIB.h"
#endif // #ifdef CH59xBLE_ROM
#else
#ifdef CH58xBLE_ROM
#include "CH58xBLE_ROM.H"
#else
#include "CH58xBLE_LIB.H"
#endif
#endif // #ifndef USE_CH58x_LIB
#include "CH59x_common.h"

/*********************************************************************
 【MAC】
 BLE_MAC                                    - Whether to customize the Bluetooth Mac address (default: FALSE - use the chip Mac address), you need to modify the Mac address definition in main.c

 【DCDC】
 DCDC_ENABLE                                - Whether to enable DCDC (default: FALSE)

 【SLEEP】
 HAL_SLEEP                                  - Whether to enable the sleep function (default: FALSE)
 SLEEP_RTC_MIN_TIME                         - Minimum sleep time in non-idle mode (unit: one RTC cycle)
 SLEEP_RTC_MAX_TIME                         - Maximum sleep time in non-idle mode (unit: one RTC cycle)
 WAKE_UP_RTC_MAX_TIME                       - Waiting for 32M crystal oscillator to stabilize (unit: one RTC cycle)
                                              According to different sleep types, the values ​​can be divided into: Sleep mode/power-off mode - 45 (default)
                                              Pause Mode    - 45
                                              Idle Mode - 5
 【TEMPERATION】
 TEM_SAMPLE                                 - Whether to enable the function of calibrating according to temperature changes, and a single calibration takes less than 10ms (default: TRUE)
 
 【CALIBRATION】
 BLE_CALIBRATION_ENABLE                     - Whether to enable the timed calibration function, and a single calibration takes less than 10ms (default: TRUE)
 BLE_CALIBRATION_PERIOD                     - The period of timing calibration, in ms (default: 120000)
 
 【SNV】
 BLE_SNV                                    - Whether to enable the SNV function to store binding information (default: TRUE)
 BLE_SNV_ADDR                               - SNV information storage address, use the last 512 bytes of data flash (default: 0x77E00)
 BLE_SNV_BLOCK                              - SNV information saving block size (default: 256)
 BLE_SNV_NUM                                - Number of SNV information to save (default: 1)

 【RTC】
 CLK_OSC32K                                 - RTC clock selection, if the host role is included, external 32K must be used (0 external (32768Hz), default: 1: internal (32000Hz), 2: internal (32768Hz))

 【MEMORY】
 BLE_MEMHEAP_SIZE                           - 蓝牙协议栈使用的RAM大小，不小于6K ( 默认:(1024*6) )

 【DATA】
 BLE_BUFF_MAX_LEN                           - 单个连接最大包长度( 默认:27 (ATT_MTU=23)，取值范围[27~516] )
 BLE_BUFF_NUM                               - 控制器缓存的包数量( 默认:5 )
 BLE_TX_NUM_EVENT                           - 单个连接事件最多可以发多少个数据包( 默认:1 )
 BLE_TX_POWER                               - 发射功率( 默认:LL_TX_POWEER_0_DBM (0dBm) )
 
 【MULTICONN】
 PERIPHERAL_MAX_CONNECTION                  - 最多可同时做多少从机角色( 默认:1 )
 CENTRAL_MAX_CONNECTION                     - 最多可同时做多少主机角色( 默认:3 )

 **********************************************************************/

/*********************************************************************
 * 默认配置值
 */
#ifndef BLE_MAC
#define BLE_MAC                             FALSE
#endif
#ifndef DCDC_ENABLE
#define DCDC_ENABLE                         TRUE
#endif
#ifndef HAL_SLEEP
#define HAL_SLEEP                           TRUE
#endif
#ifndef SLEEP_RTC_MIN_TIME                   
#define SLEEP_RTC_MIN_TIME                  US_TO_RTC(1000)
#endif
#ifndef SLEEP_RTC_MAX_TIME                   
#define SLEEP_RTC_MAX_TIME                  (RTC_MAX_COUNT - 1000 * 1000 * 30)
#endif
#ifndef WAKE_UP_RTC_MAX_TIME
#define WAKE_UP_RTC_MAX_TIME                US_TO_RTC(1600)
#endif
#ifndef HAL_KEY
#define HAL_KEY                             FALSE
#endif
#ifndef HAL_LED
#define HAL_LED                             FALSE
#endif
#ifndef TEM_SAMPLE
#define TEM_SAMPLE                          TRUE
#endif
#ifndef BLE_CALIBRATION_ENABLE
#define BLE_CALIBRATION_ENABLE              TRUE
#endif
#ifndef BLE_CALIBRATION_PERIOD
#define BLE_CALIBRATION_PERIOD              120000
#endif
#ifndef BLE_SNV
#define BLE_SNV                             TRUE
#endif
#ifndef BLE_SNV_ADDR
#define BLE_SNV_ADDR                        0x77000-FLASH_ROM_MAX_SIZE
#endif
#ifndef BLE_SNV_BLOCK
#define BLE_SNV_BLOCK                       256
#endif
#ifndef BLE_SNV_NUM
#define BLE_SNV_NUM                         1
#endif
#ifndef CLK_OSC32K
#define CLK_OSC32K                          1   // Do not modify this item here. You must modify it in the preprocessing of the project configuration. If the host role is included, an external 32K
#endif
#ifndef BLE_MEMHEAP_SIZE
#define BLE_MEMHEAP_SIZE                    (1024*6)
#endif
#ifndef BLE_BUFF_MAX_LEN
#define BLE_BUFF_MAX_LEN                    27
#endif
#ifndef BLE_BUFF_NUM
#define BLE_BUFF_NUM                        5
#endif
#ifndef BLE_TX_NUM_EVENT
#define BLE_TX_NUM_EVENT                    1
#endif
#ifndef BLE_TX_POWER
#define BLE_TX_POWER                        LL_TX_POWEER_0_DBM
#endif
#ifndef PERIPHERAL_MAX_CONNECTION
#define PERIPHERAL_MAX_CONNECTION           1
#endif
#ifndef CENTRAL_MAX_CONNECTION
#define CENTRAL_MAX_CONNECTION              3
#endif

extern uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
extern const uint8_t MacAddr[6];

#endif

