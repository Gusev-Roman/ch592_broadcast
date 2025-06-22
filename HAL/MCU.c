/********************************** (C) COPYRIGHT *******************************
 * File Name          : MCU.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : Hardware task processing function and BLE and hardware initialization
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* Header file contains */
#include "HAL.h"

#define led_pin GPIO_Pin_8


tmosTaskID halTaskID;
uint32_t g_LLE_IRQLibHandlerLocation;

/*******************************************************************************
 * @fn      Lib_Calibration_LSI
 *
 * @brief   Internal 32k calibration
 *
 * @param   None.
 *
 * @return  None.
 */
void Lib_Calibration_LSI(void)
{
    Calibration_LSI(Level_64);
}

#if(defined(BLE_SNV)) && (BLE_SNV == TRUE)
/*******************************************************************************
 * @fn      Lib_Read_Flash
 *
 * @brief   Callback function used for BLE lib.
 *
 * @param   addr - Read start address
 * @param   num - Number of units to read (unit: 4 bytes)
 * @param   pBuf - Buffer to store read data
 *
 * @return  None.
 */
uint32_t Lib_Read_Flash(uint32_t addr, uint32_t num, uint32_t *pBuf)
{
    EEPROM_READ(addr, pBuf, num * 4);
    return 0;
}
/*******************************************************************************
 * @fn      Lib_Write_Flash_592A
 *
 * @brief   Callback function used for BLE lib.
 *
 * @param   addr - Write start address
 * @param   num - Number of units to write (unit: 4 bytes)
 * @param   pBuf - Buffer with data to be written
 *
 * @return  None.
 */
void Lib_Write_Flash_592A(uint32_t addr, uint32_t num, uint32_t *pBuf)
{
    __attribute__((aligned(4))) uint32_t FLASH_BUF[(BLE_SNV_BLOCK*BLE_SNV_NUM) / 4];
    EEPROM_READ(addr&0xFFFFF000, FLASH_BUF, BLE_SNV_BLOCK*BLE_SNV_NUM);
    tmos_memcpy(&FLASH_BUF[addr&0xFFF], pBuf, num * 4);
    EEPROM_ERASE(addr&0xFFFFF000, ((BLE_SNV_BLOCK*BLE_SNV_NUM+EEPROM_BLOCK_SIZE-1)/EEPROM_BLOCK_SIZE)*EEPROM_BLOCK_SIZE);
    EEPROM_WRITE(addr&0xFFFFF000, FLASH_BUF, BLE_SNV_BLOCK*BLE_SNV_NUM);
}

/*******************************************************************************
 * @fn      Lib_Write_Flash
 *
 * @brief   Callback function used for BLE lib.
 *
 * @param   addr - Write start address
 * @param   num - Number of units to write (unit: 4 bytes)
 * @param   pBuf - Buffer with data to be written
 *
 * @return  None.
 */
uint32_t Lib_Write_Flash(uint32_t addr, uint32_t num, uint32_t *pBuf)
{
    if(((*(uint32_t*)ROM_CFG_VERISON)&0xFF) == DEF_CHIP_ID_CH592A)
    {
        Lib_Write_Flash_592A(addr, num, pBuf);
    }
    else
    {
        EEPROM_ERASE(addr, num * 4);
        EEPROM_WRITE(addr, pBuf, num * 4);
    }
    return 0;
}
#endif

/*******************************************************************************
 * @fn      CH59x_BLEInit
 *
 * @brief   BLE ¿â³õÊ¼»¯
 *
 * @param   None.
 *
 * @return  None.
 */
void CH59x_BLEInit(void)
{
    uint8_t     i;
    bleConfig_t cfg;
    if(tmos_memcmp(VER_LIB, VER_FILE, strlen(VER_FILE)) == FALSE)
    {
        PRINT("head file error...\n");
        while(1);
    }

    SysTick_Config(SysTick_LOAD_RELOAD_Msk);// Configure SysTick and enable interrupts
    PFIC_DisableIRQ(SysTick_IRQn);

    g_LLE_IRQLibHandlerLocation = (uint32_t)LLE_IRQLibHandler;
    PFIC_SetPriority(BLEL_IRQn, 0xF0);
    tmos_memset(&cfg, 0, sizeof(bleConfig_t));
    cfg.MEMAddr = (uint32_t)MEM_BUF;
    cfg.MEMLen = (uint32_t)BLE_MEMHEAP_SIZE;
    cfg.BufMaxLen = (uint32_t)BLE_BUFF_MAX_LEN;
    cfg.BufNumber = (uint32_t)BLE_BUFF_NUM;
    cfg.TxNumEvent = (uint32_t)BLE_TX_NUM_EVENT;
    cfg.TxPower = (uint32_t)BLE_TX_POWER;
#if(defined(BLE_SNV)) && (BLE_SNV == TRUE)
    if((BLE_SNV_ADDR + BLE_SNV_BLOCK * BLE_SNV_NUM) > (0x78000 - FLASH_ROM_MAX_SIZE))
    {
        PRINT("SNV config error...\n");
        while(1);
    }
    cfg.SNVAddr = (uint32_t)BLE_SNV_ADDR;
    cfg.SNVBlock = (uint32_t)BLE_SNV_BLOCK;
    cfg.SNVNum = (uint32_t)BLE_SNV_NUM;
    cfg.readFlashCB = Lib_Read_Flash;
    cfg.writeFlashCB = Lib_Write_Flash;
#endif
    cfg.ConnectNumber = (PERIPHERAL_MAX_CONNECTION & 3) | (CENTRAL_MAX_CONNECTION << 2);
    cfg.srandCB = SYS_GetSysTickCnt;
#if(defined TEM_SAMPLE) && (TEM_SAMPLE == TRUE)
    cfg.tsCB = HAL_GetInterTempValue; // Calibrate RF and internal RC based on temperature variation (greater than 7 degrees Celsius)
  #if(CLK_OSC32K)
    cfg.rcCB = Lib_Calibration_LSI; // Internal 32K clock calibration
  #endif
#endif
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    cfg.idleCB = CH59x_LowPower; // Enable Sleep
#endif
#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
    for(i = 0; i < 6; i++)
    {
        cfg.MacAddr[i] = MacAddr[5 - i]; // custom MAC addr
    }
#else
    {
        uint8_t MacAddr[6];
        GetMACAddress(MacAddr);
        for(i = 0; i < 6; i++)
        {
            cfg.MacAddr[i] = MacAddr[i]; // Use chip mac address
        }
    }
#endif
    if(!cfg.MEMAddr || cfg.MEMLen < 4 * 1024)
    {
        while(1);
    }
    i = BLE_LibInit(&cfg);
    if(i)
    {
        PRINT("LIB init error code: %x ...\n", i);
        while(1);
    }
}

/*******************************************************************************
 * @fn      HAL_ProcessEvent
 *
 * @brief   Hardware-level transaction processing
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events  - events to process.  This is a bit map and can
 *                      contain more than one event.
 *
 * @return  events.
 */
tmosEvents HAL_ProcessEvent(tmosTaskID task_id, tmosEvents events)
{
    uint8_t *msgPtr;
    //static uint16_t wks = 0;

    if(events & SYS_EVENT_MSG)
    { // Process HAL layer messages, call tmos_msg_receive to read the messages, and delete the messages after processing.
        msgPtr = tmos_msg_receive(task_id);
        if(msgPtr)
        {
            /* De-allocate */
            tmos_msg_deallocate(msgPtr);
        }
        return events ^ SYS_EVENT_MSG;
    }
    if(events & LED_TIMER_EXPIRED_EVENT)
    {
        GPIOA_InverseBits(led_pin);
        return (events ^ LED_TIMER_EXPIRED_EVENT); // §ã§Ò§â§à§ã§Ú§Ý§Ú §á§â§Ú§Ù§ß§Ñ§Ü §ï§Ó§Ö§ß§ä§Ñ, §à§ß §à§Ò§ã§Ý§å§Ø§Ö§ß
    }

    if(events & LED_BLINK_EVENT)
    {
#if(defined HAL_LED) && (HAL_LED == TRUE)
        HalLedUpdate();
#endif // HAL_LED
        return events ^ LED_BLINK_EVENT;
    }
    if(events & HAL_KEY_EVENT)
    {
#if(defined HAL_KEY) && (HAL_KEY == TRUE)
        HAL_KeyPoll(); /* Check for keys */
        tmos_start_task(halTaskID, HAL_KEY_EVENT, MS1_TO_SYSTEM_TIME(100));
        return events ^ HAL_KEY_EVENT;
#endif
    }
    if(events & HAL_REG_INIT_EVENT)
    {
        uint8_t x32Kpw;
#if(defined BLE_CALIBRATION_ENABLE) && (BLE_CALIBRATION_ENABLE == TRUE) // Calibration task, single calibration takes less than 10ms
        BLE_RegInit();                                                  // Calibrate RF
#if(CLK_OSC32K)
        Lib_Calibration_LSI(); // Calibrate internal RC
#else
        x32Kpw = (R8_XT32K_TUNE & 0xfc) | 0x01;
        sys_safe_access_enable();
        R8_XT32K_TUNE = x32Kpw; // LSE drive current is reduced to the rated current
        sys_safe_access_disable();
#endif
        tmos_start_task(halTaskID, HAL_REG_INIT_EVENT, MS1_TO_SYSTEM_TIME(BLE_CALIBRATION_PERIOD));
        return events ^ HAL_REG_INIT_EVENT;
#endif
    }
    if(events & HAL_TEST_EVENT)
    {
        PRINT("* \n");
        tmos_start_task(halTaskID, HAL_TEST_EVENT, MS1_TO_SYSTEM_TIME(1000));
        return events ^ HAL_TEST_EVENT;
    }
    return 0;
}

/*******************************************************************************
 * @fn      HAL_Init
 *
 * @brief   Hardware Initialization
 *
 * @param   None.
 *
 * @return  None.
 */
void HAL_Init()
{
    halTaskID = TMOS_ProcessEventRegister(HAL_ProcessEvent);
    HAL_TimeInit();
#if(defined HAL_SLEEP) && (HAL_SLEEP == TRUE)
    HAL_SleepInit();
#endif
#if(defined HAL_LED) && (HAL_LED == TRUE)
    HAL_LedInit();
#endif
#if(defined HAL_KEY) && (HAL_KEY == TRUE)
    HAL_KeyInit();
#endif
#if(defined BLE_CALIBRATION_ENABLE) && (BLE_CALIBRATION_ENABLE == TRUE)
    tmos_start_task(halTaskID, HAL_REG_INIT_EVENT, 800); // Added calibration task, started in 500ms, single calibration takes less than 10ms
#endif
//    tmos_start_task( halTaskID, HAL_TEST_EVENT, 1600 );    // Add a test task
    GPIOA_ModeCfg(led_pin, GPIO_ModeOut_PP_5mA);  // Main LED
    tmos_start_reload_task(halTaskID, LED_TIMER_EXPIRED_EVENT, MS1_TO_SYSTEM_TIME(200));
}

/*******************************************************************************
 * @fn      HAL_GetInterTempValue
 *
 * @brief   Get the internal temperature sampling value. If ADC interrupt sampling is used, the interrupt must be temporarily disabled in this function.
 *
 * @return  Internal temperature sampling value.
 */
uint16_t HAL_GetInterTempValue(void)
{
    uint8_t  sensor, channel, config, tkey_cfg;
    uint16_t adc_data;

    tkey_cfg = R8_TKEY_CFG;
    sensor = R8_TEM_SENSOR;
    channel = R8_ADC_CHANNEL;
    config = R8_ADC_CFG;
    ADC_InterTSSampInit();
    R8_ADC_CONVERT |= RB_ADC_START;
    while(R8_ADC_CONVERT & RB_ADC_START);
    adc_data = R16_ADC_DATA;
    R8_TEM_SENSOR = sensor;
    R8_ADC_CHANNEL = channel;
    R8_ADC_CFG = config;
    R8_TKEY_CFG = tkey_cfg;
    return (adc_data);
}

/******************************** endfile @ mcu ******************************/
