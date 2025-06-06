/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH59x_pwr.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2021/11/17
 * Description
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH59x_common.h"

/*********************************************************************
 * @fn      PWR_DCDCCfg
 *
 * @brief   启用内部DC/DC电源，用于节约系统功耗
 *
 * @param   s       - 是否打开DCDC电源
 *
 * @return  none
 */
void PWR_DCDCCfg(FunctionalState s)
{
    uint16_t adj = R16_AUX_POWER_ADJ;
    uint16_t plan = R16_POWER_PLAN;

    if(s == DISABLE)
    {
        
        adj &= ~RB_DCDC_CHARGE;
        plan &= ~(RB_PWR_DCDC_EN | RB_PWR_DCDC_PRE); // 旁路 DC/DC
        sys_safe_access_enable();
        R16_AUX_POWER_ADJ = adj;
        R16_POWER_PLAN = plan;
        sys_safe_access_disable();
    }
    else
    {
        uint32_t HW_Data[2];
        FLASH_EEPROM_CMD(CMD_GET_ROM_INFO, ROM_CFG_ADR_HW, HW_Data, 0);
        if((HW_Data[0]) & (1 << 13))
        {
            return;
        }
        adj |= RB_DCDC_CHARGE;
        plan |= RB_PWR_DCDC_PRE;
        sys_safe_access_enable();
        R16_AUX_POWER_ADJ = adj;
        R16_POWER_PLAN = plan;
        sys_safe_access_disable();
        DelayUs(10);
        sys_safe_access_enable();
        R16_POWER_PLAN |= RB_PWR_DCDC_EN;
        sys_safe_access_disable();
    }
}

/*********************************************************************
 * @fn      PWR_UnitModCfg
 *
 * @brief   可控单元模块的电源控制
 *
 * @param   s       - 是否打开电源
 * @param   unit    - please refer to unit of controllable power supply
 *
 * @return  none
 */
void PWR_UnitModCfg(FunctionalState s, uint8_t unit)
{
    uint8_t ck32k_cfg = R8_CK32K_CONFIG;

    if(s == DISABLE) //关闭
    {
        ck32k_cfg &= ~(unit & 0x03);
    }
    else //打开
    {
        ck32k_cfg |= (unit & 0x03);
    }

    sys_safe_access_enable();
    R8_CK32K_CONFIG = ck32k_cfg;
    sys_safe_access_disable();
}

/*********************************************************************
 * @fn      PWR_PeriphClkCfg
 *
 * @brief   外设时钟控制位
 *
 * @param   s       - 是否打开对应外设时钟
 * @param   perph   - please refer to Peripher CLK control bit define
 *
 * @return  none
 */
void PWR_PeriphClkCfg(FunctionalState s, uint16_t perph)
{
    uint32_t sleep_ctrl = R32_SLEEP_CONTROL;

    if(s == DISABLE)
    {
        sleep_ctrl |= perph;
    }
    else
    {
        sleep_ctrl &= ~perph;
    }

    sys_safe_access_enable();
    R32_SLEEP_CONTROL = sleep_ctrl;
    sys_safe_access_disable();
}

/*********************************************************************
 * @fn      PWR_PeriphWakeUpCfg
 *
 * @brief   睡眠唤醒源配置
 *
 * @param   s       - Whether to enable the sleep wake-up function of this external device
 * @param   perph   - Wake-up source that needs to be set
 *                    RB_SLP_USB_WAKE   -  USB 为唤醒源
 *                    RB_SLP_RTC_WAKE   -  RTC 为唤醒源
 *                    RB_SLP_GPIO_WAKE  -  GPIO Wake-up source
 *                    RB_SLP_BAT_WAKE   -  BAT 为唤醒源
 * @param   mode    - refer to WakeUP_ModeypeDef
 *
 * @return  none
 */
void PWR_PeriphWakeUpCfg(FunctionalState s, uint8_t perph, WakeUP_ModeypeDef mode)
{
    uint8_t m;

    if(s == DISABLE)
    {
        sys_safe_access_enable();
        R8_SLP_WAKE_CTRL &= ~perph;
        sys_safe_access_disable();
    }
    else
    {
        switch(mode)
        {
            case Short_Delay:
                m = 0x01;
                break;

            case Long_Delay:
                m = 0x00;
                break;

            default:
                m = 0x01;
                break;
        }

        sys_safe_access_enable();
        R8_SLP_WAKE_CTRL |= RB_WAKE_EV_MODE | perph;
        sys_safe_access_disable();
        sys_safe_access_enable();
        R8_SLP_POWER_CTRL &= ~(RB_WAKE_DLY_MOD);
        sys_safe_access_disable();
        sys_safe_access_enable();
        R8_SLP_POWER_CTRL |= m;
        sys_safe_access_disable();
    }
}

/*********************************************************************
 * @fn      PowerMonitor
 *
 * @brief   电源监控
 *
 * @param   s       - 是否打开此功能
 * @param   vl      - refer to VolM_LevelypeDef
 *
 * @return  none
 */
void PowerMonitor(FunctionalState s, VolM_LevelypeDef vl)
{
    uint8_t ctrl = R8_BAT_DET_CTRL;
    uint8_t cfg = R8_BAT_DET_CFG;

    if(s == DISABLE)
    {
        sys_safe_access_enable();
        R8_BAT_DET_CTRL = 0;
        sys_safe_access_disable();
    }
    else
    {
        if(vl & 0x80)
        {
            cfg = vl & 0x03;
            ctrl = RB_BAT_MON_EN | ((vl >> 2) & 1);
        }
        else
        {
            
            cfg = vl & 0x03;
            ctrl = RB_BAT_DET_EN;
        }
        sys_safe_access_enable();
        R8_BAT_DET_CTRL = ctrl;
        R8_BAT_DET_CFG = cfg;
        sys_safe_access_disable();

        mDelayuS(1);
        sys_safe_access_enable();
        R8_BAT_DET_CTRL |= RB_BAT_LOW_IE | RB_BAT_LOWER_IE;
        sys_safe_access_disable();
    }
}

/*********************************************************************
 * @fn      LowPower_Idle
 *
 * @brief   低功耗-Idle模式
 *
 * @param   none
 *
 * @return  none
 */
__HIGH_CODE
void LowPower_Idle(void)
{
    FLASH_ROM_SW_RESET();
    R8_FLASH_CTRL = 0x04; //flash关闭

    PFIC->SCTLR &= ~(1 << 2); // sleep
    __WFI();
    __nop();
    __nop();
}

/*********************************************************************
 * @fn      LowPower_Halt
 *
 * @brief   低功耗-Halt模式，此低功耗切到HSI/5时钟运行，唤醒后需要用户自己重新选择系统时钟源
 *
 * @param   none
 *
 * @return  none
 */
__HIGH_CODE
void LowPower_Halt(void)
{
    uint8_t x32Kpw, x32Mpw;

    FLASH_ROM_SW_RESET();
    R8_FLASH_CTRL = 0x04; //flash关闭
    x32Kpw = R8_XT32K_TUNE;
    x32Mpw = R8_XT32M_TUNE;
    x32Mpw = (x32Mpw & 0xfc) | 0x03; // 150%额定电流
    x32Kpw = (x32Kpw & 0xfc) | 0x01; // LSE驱动电流降低到额定电流

    sys_safe_access_enable();
    R8_BAT_DET_CTRL = 0; // 关闭电压监控
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_XT32K_TUNE = x32Kpw;
    R8_XT32M_TUNE = x32Mpw;
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_PLL_CONFIG |= (1 << 5);
    sys_safe_access_disable();

    PFIC->SCTLR |= (1 << 2); //deep sleep
    __WFI();
    __nop();
    __nop();
    sys_safe_access_enable();
    R8_PLL_CONFIG &= ~(1 << 5);
    sys_safe_access_disable();
}

/*******************************************************************************
* Function Name  : LowPower_Sleep
* Description    : Low power consumption - Sleep mode.
                   Note that when the main frequency is 80M, the sleep wake-up interrupt cannot
                   call the code in the flash, and a 30us delay is required before exiting this function.
* Input          : rm:
                    RB_PWR_RAM2K	-	2K retention SRAM 供电
                    RB_PWR_RAM24K	-	24K main SRAM 供电
                    RB_PWR_EXTEND	-	USB 和 BLE 单元保留区域供电
                    RB_PWR_XROM   - FlashROM 供电
                   NULL	-	以上单元都断电
* Return         : None
*******************************************************************************/
__HIGH_CODE
void LowPower_Sleep(uint16_t rm)
{
    __attribute__((aligned(4))) uint8_t MacAddr[6] = {0};
    uint8_t x32Mpw;
    uint16_t power_plan;

    GetMACAddress(MacAddr);

    x32Mpw = R8_XT32M_TUNE;
    x32Mpw = (x32Mpw & 0xfc) | 0x03; // 150%额定电流

    sys_safe_access_enable();
    R8_BAT_DET_CTRL = 0; // 关闭电压监控
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_XT32M_TUNE = x32Mpw;
    sys_safe_access_disable();

    sys_safe_access_enable();
    R16_POWER_PLAN &= ~RB_XT_PRE_EN;
    sys_safe_access_disable();

    PFIC->SCTLR |= (1 << 2); //deep sleep

    power_plan = R16_POWER_PLAN & (RB_PWR_DCDC_EN | RB_PWR_DCDC_PRE);
    power_plan |= RB_PWR_PLAN_EN | RB_PWR_CORE | rm | (2<<11);

    sys_safe_access_enable();
    R8_SLP_POWER_CTRL |= RB_RAM_RET_LV;
    R16_POWER_PLAN = power_plan;
    sys_safe_access_disable();
    if((rm & RB_XT_PRE_EN) == 0)
    {
        sys_safe_access_enable();
        R8_PLL_CONFIG |= (1 << 5);
        sys_safe_access_disable();
    }

    __WFI();
    __nop();
    __nop();

    sys_safe_access_enable();
    R16_POWER_PLAN &= ~RB_PWR_PLAN_EN;
    sys_safe_access_disable();

    sys_safe_access_enable();
    R16_POWER_PLAN &= ~RB_XT_PRE_EN;
    sys_safe_access_disable();

    if((rm & RB_XT_PRE_EN) == 0)
    {
        sys_safe_access_enable();
        R8_PLL_CONFIG &= ~(1 << 5);
        sys_safe_access_disable();
        DelayUs(20);
    }
}

/*********************************************************************
 * @fn      LowPower_Shutdown
 *
 * @brief   低功耗-Shutdown模式，此低功耗切到HSI/5时钟运行，唤醒后需要用户自己重新选择系统时钟源
 *          @note 注意调用此函数，DCDC功能强制关闭，唤醒后可以手动再次打开
 *
 * @param   rm      - 供电模块选择
 *                    RB_PWR_RAM2K  -   2K retention SRAM 供电
 *                    RB_PWR_RAM16K -   16K main SRAM 供电
 *                    NULL          -   All the above units are powered off
 *
 * @return  none
 */
__HIGH_CODE
void LowPower_Shutdown(uint16_t rm)
{
    uint8_t x32Kpw, x32Mpw;

    FLASH_ROM_SW_RESET();
    x32Kpw = R8_XT32K_TUNE;
    x32Mpw = R8_XT32M_TUNE;
    x32Mpw = (x32Mpw & 0xfc) | 0x03; // 150%额定电流
    x32Kpw = (x32Kpw & 0xfc) | 0x01; // LSE驱动电流降低到额定电流

    sys_safe_access_enable();
    R8_BAT_DET_CTRL = 0; // 关闭电压监控
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_XT32K_TUNE = x32Kpw;
    R8_XT32M_TUNE = x32Mpw;
    sys_safe_access_disable();
    SetSysClock(CLK_SOURCE_HSE_6_4MHz);

    PFIC->SCTLR |= (1 << 2); //deep sleep

    sys_safe_access_enable();
    R8_SLP_POWER_CTRL |= RB_RAM_RET_LV;
    sys_safe_access_disable();
    sys_safe_access_enable();
    R16_POWER_PLAN = RB_PWR_PLAN_EN | rm;
    sys_safe_access_disable();
    __WFI();
    __nop();
    __nop();
    FLASH_ROM_SW_RESET();
    sys_safe_access_enable();
    R8_RST_WDOG_CTRL |= RB_SOFTWARE_RESET;
    sys_safe_access_disable();
}
