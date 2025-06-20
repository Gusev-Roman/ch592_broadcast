/********************************** (C) COPYRIGHT *******************************
 * File Name          : broadcaster.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : Broadcast application, initialize broadcast connection parameters, and then
 *                      keep broadcasting in broadcast state

 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "devinfoservice.h"
#include "broadcaster.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// What is the advertising interval when device is discoverable (units of 625us, min is 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL    160

// Company Identifier: WCH
#define WCH_COMPANY_ID                  0x07D7

// Length of bd addr as a string
#define B_ADDR_STR_LEN                  15

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
extern uint16_t __counter_cold, __counter_hot;
/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t Broadcaster_TaskID; // Task ID for internal task/event processing

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[] = {
    // complete name
    0x0c,                                 // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE, 0x42, // 'B'
    0x72,                                 // 'r'
    0x6f,                                 // 'o'
    0x61,                                 // 'a'
    0x64,                                 // 'd'
    0x63,                                 // 'c'
    0x61,                                 // 'a'
    0x73,                                 // 's'
    0x74,                                 // 't'
    0x65,                                 // 'e'
    0x72,                                 // 'r'

    // Tx power level
    0x02,                     // length of this data
    GAP_ADTYPE_POWER_LEVEL, 0 // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8_t advertData[30];

uint8_t ad_template[] = {
    // Flags; this sets the device to use limited discoverable
    // mode (advertises for 30 seconds at a time) instead of general
    // discoverable mode (advertises indefinitely)
    0x02, // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED

    // Broadcast of the data
    /*
    0x04,                             // length of this data including the data type byte
    GAP_ADTYPE_MANUFACTURER_SPECIFIC, // manufacturer specific advertisement data type
    'b', 'l', 'e',

    0x0A, GAP_ADTYPE_LOCAL_NAME_SHORT,
    'i', 'P', 'h','o','n','e','1','5',0
    */
};

char xbuf[32];
//char ybuf[32];
/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void Broadcaster_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void Broadcaster_StateNotificationCB(gapRole_States_t newState);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesBroadcasterCBs_t Broadcaster_BroadcasterCBs = {Broadcaster_StateNotificationCB, // Profile State Change Callbacks
                                                            NULL};

#if 0
void dump(uint8_t *buf){
    for(int i=0; i < tmos_strlen(buf); i++){
        printf("%02X ", buf[i]);
    }
    printf("\r\n");
}
#endif

// ���٧էѧ�� �ѧէӧ֧�� �� ���ڧާ֧ߧڧ�� �֧ԧ�...
/*
uint8_t *make_adv(){
    sprintf(xbuf, "  WC%d.%d:WH%d.%d", __counter_cold, (R8_GLOB_RESET_KEEP), __counter_hot, (R32_TMR1_CNT_END));
    uint8_t sl = tmos_strlen(xbuf);
    xbuf[0] = sl-1;
    xbuf[1] = GAP_ADTYPE_LOCAL_NAME_SHORT;

    tmos_memset(advertData, 0, 30);
    tmos_memcpy(advertData, ad_template, tmos_strlen(ad_template));

    strcat(advertData, xbuf);

    //GAPRole_SetParameter(GAPROLE_ADVERT_DATA, tmos_strlen(advertData), advertData);
    GAP_UpdateAdvertisingData(Broadcaster_TaskID, TRUE, 30, advertData );
    bStatus_t u = tmos_set_event(Broadcaster_TaskID, SBP_START_DEVICE_EVT);
    PRINT("tmos_set_event() rteturns %d\r\n", u);
    return xbuf;
}
*/
/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Broadcaster_Init
 *
 * @brief   Initialization function for the Broadcaster App
 *          Task. This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Broadcaster_Init()
{
    Broadcaster_TaskID = TMOS_ProcessEventRegister(Broadcaster_ProcessEvent); // task starting?

    // Setup the GAP Broadcaster Role Profile
    {
        // Device starts advertising upon initialization
        uint8_t initial_advertising_enable = TRUE;
        uint8_t initial_adv_event_type = GAP_ADTYPE_ADV_NONCONN_IND;
        // Set the GAP Role Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        // adv only, device cannot be connected!
        GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &initial_adv_event_type);
        // unused data?
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
        // adv data: may change at worktime!
        tmos_memset(advertData, 0, 30);
        tmos_memcpy(advertData, ad_template, tmos_strlen(ad_template));

        sprintf(xbuf, "  WC%d.%d:WH%d.%d", __counter_cold, (R8_GLOB_RESET_KEEP) - 1, __counter_hot, ((R32_TMR1_CNT_END) & ~0x10000)-1);
        uint8_t sl = tmos_strlen(xbuf);
        xbuf[0] = sl-1;
        xbuf[1] = GAP_ADTYPE_LOCAL_NAME_SHORT;

        strcat(advertData, xbuf);

        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, tmos_strlen(advertData), advertData);
//        GAP_UpdateAdvertisingData(Broadcaster_TaskID, TRUE, 30, advertData );
    }

    // Set advertising interval
    {
        uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;

        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, advInt);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, (advInt << 1));
    }

    // Setup a delayed profile startup
    tmos_set_event(Broadcaster_TaskID, SBP_START_DEVICE_EVT);   // �٧ѧ���� �ާѧ�ܧ� ��֧�֧� ��ڧ��֧ާ� ��ӧ֧ߧ���
    // �ӧ֧����ߧ�, ����ݧ� ���ݧ�էߧ�ԧ� ��ߧ� ����� �ܧ�� ���֧ҧ�֧� ���ӧ���֧ߧڧ�...
}

/*********************************************************************
 * @fn      Broadcaster_ProcessEvent
 *
 * @brief   Broadcaster Application Task event processor. This
 *          function is called to process all events for the task. Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t Broadcaster_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(Broadcaster_TaskID)) != NULL)
        {
            Broadcaster_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);

            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & SBP_START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_BroadcasterStartDevice(&Broadcaster_BroadcasterCBs);

        return (events ^ SBP_START_DEVICE_EVT);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      Broadcaster_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Broadcaster_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        default:
            break;
    }
}

/*********************************************************************
 * @fn      Broadcaster_StateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void Broadcaster_StateNotificationCB(gapRole_States_t newState)
{
    switch(newState)
    {
        case GAPROLE_STARTED:
            PRINT("Initialized..\r\n");
            break;

        case GAPROLE_ADVERTISING:
            PRINT("Advertising..\r\n");
            break;

        case GAPROLE_WAITING:
            PRINT("Waiting for advertising..\r\n");
            break;

        case GAPROLE_ERROR:
            PRINT("Error..\n");
            break;

        default:
            break;
    }
}
/*********************************************************************
 *********************************************************************/
