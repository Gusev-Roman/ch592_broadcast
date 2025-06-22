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
#include "pulse_counter.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// What is the advertising interval when device is discoverable (units of 625us, min is 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL    320

// Company Identifier: WCH
#define WCH_COMPANY_ID                  0x07D7

#define SOME_COMPANY_ID                 0xAABB

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

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8_t Broadcaster_TaskID; // Task ID for internal task/event processing

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
//static uint8_t advertData[30];

uint8_t ad_template[] = {
    // Flags; this sets the device to use limited discoverable
    // mode (advertises for 30 seconds at a time) instead of general
    // discoverable mode (advertises indefinitely)
    0x02, // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // Broadcast of the data

    0x04,                             // length of this data including the data type byte
    GAP_ADTYPE_MANUFACTURER_SPECIFIC, // manufacturer specific advertisement data type
    'b', 'l', 'e',

//    0x0A, GAP_ADTYPE_LOCAL_NAME_SHORT,
//    'i', 'P', 'h','o','n','e','1','5',0
//    */
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

// §ã§à§Ù§Õ§Ñ§ä§î §Ñ§Õ§Ó§Ö§â§ä §Ú §á§â§Ú§Þ§Ö§ß§Ú§ä§î §Ö§Ô§à...
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
        // Set advertising interval
        {
            uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;

            GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, advInt);
            GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, advInt);
        }

        // §¥§Ñ§ß§ß§í§Ö §Ñ§Õ§Ó§Ö§â§ä§Ñ§Û§Ù§Ú§ß§Ô§Ñ §å§ã§ä§Ñ§ß§à§Ó§ñ§ä§ã§ñ §Ó PulseCounter
    }

    // Setup a delayed profile startup
    tmos_set_event(Broadcaster_TaskID, SBP_START_DEVICE_EVT);   // §Ù§Ñ§á§å§ã§Ü §Þ§Ñ§ñ§Ü§Ñ §é§Ö§â§Ö§Ù §ã§Ú§ã§ä§Ö§Þ§å §ï§Ó§Ö§ß§ä§à§Ó
}

/*********************************************************************
 * @fn      Broadcaster_update_pulse_counter_advertising
 *
 * @brief
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
bStatus_t Broadcaster_update_pulse_counter_advertising(uint32_t pulse_count_hot, uint32_t pulse_count_cold)
{
    uint8_t ad_data[] = {

        0x02, // length of this data
        GAP_ADTYPE_FLAGS,
        GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

        // Broadcast of the data

        0x0B,                             // length of this data including the data type byte
        GAP_ADTYPE_MANUFACTURER_SPECIFIC, // manufacturer specific advertisement data type
        LO_UINT16(SOME_COMPANY_ID),
        HI_UINT16(SOME_COMPANY_ID),
        0x00,0x00,0x00,0x00, // cold water placeholder little endian
        0x00,0x00,0x00,0x00, // hot water placeholder little endian
    };

    const int cold_water_pulse_cnt_offset = 7;
    const int hot_water_pulse_cnt_offset = 11;

    tmos_memcpy(&(ad_data[cold_water_pulse_cnt_offset]), &pulse_count_cold, sizeof(pulse_count_cold));
    tmos_memcpy(&(ad_data[hot_water_pulse_cnt_offset]), &pulse_count_hot, sizeof(pulse_count_hot));

    uint8_t gap_state;
    GAPRole_GetParameter(GAPROLE_STATE, &gap_state);

    bStatus_t adv_upd_status;
    switch(gap_state)
    {
        case GAPROLE_INIT:
        case GAPROLE_STARTED:
        case GAPROLE_WAITING:
        case GAPROLE_CONNECTED:
        {
            adv_upd_status = GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(ad_data), ad_data);
            break;
        }
        case GAPROLE_ADVERTISING:
        case GAPROLE_CONNECTED_ADV:
        {
            adv_upd_status = GAP_UpdateAdvertisingData(Broadcaster_TaskID, TRUE, sizeof(ad_data), ad_data);
            break;
        }
        default:
        {
            adv_upd_status = bleNotReady;
            break;
        }
    }
        return adv_upd_status;
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
    if(events & SBP_TIMEOUT_COLD_EVT)
    {
        PulseCounter_on_timer_cold();
        return (events ^ SBP_TIMEOUT_COLD_EVT);
    }
    if(events & SBP_TIMEOUT_HOT_EVT)
    {
        PulseCounter_on_timer_hot();
        return (events ^ SBP_TIMEOUT_HOT_EVT);
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
