/********************************** (C) COPYRIGHT *******************************
 * File Name          : broadcaster.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/11
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef BROADCASTER_H
#define BROADCASTER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Simple BLE Broadcaster Task Events
#define SBP_START_DEVICE_EVT         0x0001
#define SBP_PERIODIC_EVT             0x0002
#define SBP_ADV_IN_CONNECTION_EVT    0x0004
#define SBP_TIMEOUT_COLD_EVT         0x0008
#define SBP_TIMEOUT_HOT_EVT          0x0010

#define SL_ACTIVE 0
#define SL_REQ 1
#define SL_SLEEP 2

/*********************************************************************
 * MACROS
 */

extern uint8_t Broadcaster_TaskID;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Broadcaster Application
 */
extern void Broadcaster_Init(void);

bStatus_t Broadcaster_update_pulse_counter_advertising(uint32_t pulse_count_hot, uint32_t pulse_count_cold);

/*
 * Task Event Processor for the BLE Broadcaster Application
 */
extern uint16_t Broadcaster_ProcessEvent(uint8_t task_id, uint16_t events);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
