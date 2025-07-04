/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH59x_SYS.c
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
 * @fn      SetSysClock
 *
 * @brief   配置系统运行时钟
 *
 * @param   sc      - 系统时钟源选择 refer to SYS_CLKTypeDef
 *
 * @return  none
 */
__HIGH_CODE
void SetSysClock(SYS_CLKTypeDef sc)
{
    sys_safe_access_enable();
    R8_PLL_CONFIG &= ~(1 << 5); //
    sys_safe_access_disable();
    if(sc & 0x20)  // HSE div
    {
        sys_safe_access_enable();
        R32_CLK_SYS_CFG = (0 << 6) | (sc & 0x1f) | RB_TX_32M_PWR_EN | RB_PLL_PWR_EN;
        __nop();
        __nop();
        __nop();
        __nop();
        sys_safe_access_disable();
        sys_safe_access_enable();
        SAFEOPERATE;
        R8_FLASH_CFG = 0X51;
        sys_safe_access_disable();
    }

    else if(sc & 0x40) // PLL div
    {
        sys_safe_access_enable();
        R32_CLK_SYS_CFG = (1 << 6) | (sc & 0x1f) | RB_TX_32M_PWR_EN | RB_PLL_PWR_EN;
        __nop();
        __nop();
        __nop();
        __nop();
        sys_safe_access_disable();
        sys_safe_access_enable();
        R8_FLASH_CFG = 0X52;
        sys_safe_access_disable();
    }
    else
    {
        sys_safe_access_enable();
        R32_CLK_SYS_CFG |= RB_CLK_SYS_MOD;
        sys_safe_access_disable();
    }
    //更改FLASH clk的驱动能力
    sys_safe_access_enable();
    R8_PLL_CONFIG |= 1 << 7;
    sys_safe_access_disable();
}

/*********************************************************************
 * @fn      GetSysClock
 *
 * @brief   获取当前系统时钟
 *
 * @param   none
 *
 * @return  Hz
 */
uint32_t GetSysClock(void)
{
    uint16_t rev;

    rev = R32_CLK_SYS_CFG & 0xff;
    if((rev & 0x40) == (0 << 6))
    { // 32M进行分频
        return (32000000 / (rev & 0x1f));
    }
    else if((rev & RB_CLK_SYS_MOD) == (1 << 6))
    { // PLL进行分频
        return (480000000 / (rev & 0x1f));
    }
    else
    { // 32K做主频
        return (32000);
    }
}

/*********************************************************************
 * @fn      SYS_GetInfoSta
 *
 * @brief   获取当前系统信息状态
 *
 * @param   i       - refer to SYS_InfoStaTypeDef
 *
 * @return  是否开启
 */
uint8_t SYS_GetInfoSta(SYS_InfoStaTypeDef i)
{
    if(i == STA_SAFEACC_ACT)
    {
        return (R8_SAFE_ACCESS_SIG & RB_SAFE_ACC_ACT);
    }
    else
    {
        return (R8_GLOB_CFG_INFO & (1 << i));
    }
}

/*********************************************************************
 * @fn      SYS_ResetExecute
 *
 * @brief   执行系统软件复位
 *
 * @param   none
 *
 * @return  none
 */
__HIGH_CODE
void SYS_ResetExecute(void)
{
    FLASH_ROM_SW_RESET();
    sys_safe_access_enable();
    R8_RST_WDOG_CTRL |= RB_SOFTWARE_RESET;
    sys_safe_access_disable();
}

/*********************************************************************
 * @fn      SYS_DisableAllIrq
 *
 * @brief   关闭所有中断，并保留当前中断值
 *
 * @param   pirqv   - 当前保留中断值
 *
 * @return  none
 */
void SYS_DisableAllIrq(uint32_t *pirqv)
{
    *pirqv = (PFIC->ISR[0] >> 8) | (PFIC->ISR[1] << 24);
    PFIC->IRER[0] = 0xffffffff;
    PFIC->IRER[1] = 0xffffffff;
}

/*********************************************************************
 * @fn      SYS_RecoverIrq
 *
 * @brief   恢复之前关闭的中断值
 *
 * @param   irq_status  - 当前保留中断值
 *
 * @return  none
 */
void SYS_RecoverIrq(uint32_t irq_status)
{
    PFIC->IENR[0] = (irq_status << 8);
    PFIC->IENR[1] = (irq_status >> 24);
}

/*********************************************************************
 * @fn      SYS_GetSysTickCnt
 *
 * @brief   Get the current system (SYSTICK) count value
 *
 * @param   none
 *
 * @return  Current count value
 */
uint32_t SYS_GetSysTickCnt(void)
{
    uint32_t val;

    val = SysTick->CNT;
    return (val);
}

/*********************************************************************
 * @fn      WWDG_ITCfg
 *
 * @brief   看门狗定时器溢出中断使能
 *
 * @param   s       - 溢出是否中断
 *
 * @return  none
 */
void WWDG_ITCfg(FunctionalState s)
{
    uint8_t ctrl = R8_RST_WDOG_CTRL;

    if(s == DISABLE)
    {
        ctrl &= ~RB_WDOG_INT_EN;
    }
    else
    {
        ctrl |= RB_WDOG_INT_EN;
    }

    sys_safe_access_enable();
    R8_RST_WDOG_CTRL = ctrl;
    sys_safe_access_disable();
}

/*********************************************************************
 * @fn      WWDG_ResetCfg
 *
 * @brief   Watchdog timer reset function
 *
 * @param   s       - Overflow reset
 *
 * @return  none
 */
void WWDG_ResetCfg(FunctionalState s)
{
    uint8_t ctrl = R8_RST_WDOG_CTRL;

    if(s == DISABLE)
    {
        ctrl &= ~RB_WDOG_RST_EN;
    }
    else
    {
        ctrl |= RB_WDOG_RST_EN;
    }

    sys_safe_access_enable();
    R8_RST_WDOG_CTRL = ctrl;
    sys_safe_access_disable();
}

/*********************************************************************
 * @fn      WWDG_ClearFlag
 *
 * @brief   Clear the watchdog interrupt flag, reload the count value to clear it
 *
 * @param   none
 *
 * @return  none
 */
void WWDG_ClearFlag(void)
{
    sys_safe_access_enable();
    R8_RST_WDOG_CTRL |= RB_WDOG_INT_FLAG;
    sys_safe_access_disable();
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   硬件错误中断，进入后执行复位，复位类型为上电复位
 *
 * @param   none
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
__attribute__((weak))
void HardFault_Handler(void)
{
    FLASH_ROM_SW_RESET();
    sys_safe_access_enable();
    R16_INT32K_TUNE = 0xFFFF;
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_RST_WDOG_CTRL |= RB_SOFTWARE_RESET;
    sys_safe_access_disable();
    while(1);
}

/*********************************************************************
 * @fn      mDelayuS
 *
 * @brief   uS 延时
 *
 * @param   t       - 时间参数
 *
 * @return  none
 */
__HIGH_CODE
void mDelayuS(uint16_t t)
{
    uint32_t i;
#if(FREQ_SYS == 80000000)
    i = t * 20;
#elif(FREQ_SYS == 60000000)
    i = t * 15;
#elif(FREQ_SYS == 48000000)
    i = t * 12;
#elif(FREQ_SYS == 40000000)
    i = t * 10;
#elif(FREQ_SYS == 32000000)
    i = t << 3;
#elif(FREQ_SYS == 24000000)
    i = t * 6;
#elif(FREQ_SYS == 16000000)
    i = t << 2;
#elif(FREQ_SYS == 8000000)
    i = t << 1;
#elif(FREQ_SYS == 4000000)
    i = t;
#elif(FREQ_SYS == 2000000)
    i = t >> 1;
#elif(FREQ_SYS == 1000000)
    i = t >> 2;
#else
    i = t << 1;
#endif
    do
    {
        __nop();
    } while(--i);
}

/*********************************************************************
 * @fn      mDelaymS
 *
 * @brief   mS 延时
 *
 * @param   t       - 时间参数
 *
 * @return  none
 */
__HIGH_CODE
void mDelaymS(uint16_t t)
{
    uint16_t i;

    for(i = 0; i < t; i++)
    {
        mDelayuS(1000);
    }
}

#ifdef DEBUG
int _write(int fd, char *buf, int size)
{
    int i;
    for(i = 0; i < size; i++)
    {
#if DEBUG == Debug_UART0
        while(R8_UART0_TFC == UART_FIFO_SIZE);                  /* 等待数据发送 */
        R8_UART0_THR = *buf++; /* 发送数据 */
#elif DEBUG == Debug_UART1
        while(R8_UART1_TFC == UART_FIFO_SIZE);                  /* Waiting for data to be sent */
        R8_UART1_THR = *buf++; /* Sending Data */
#elif DEBUG == Debug_UART2
        while(R8_UART2_TFC == UART_FIFO_SIZE);                  /* 等待数据发送 */
        R8_UART2_THR = *buf++; /* 发送数据 */
#elif DEBUG == Debug_UART3       
        while(R8_UART3_TFC == UART_FIFO_SIZE);                  /* 等待数据发送 */
        R8_UART3_THR = *buf++; /* 发送数据 */
#endif
    }
    return size;
}

#endif

/*********************************************************************
 * @fn      _sbrk
 *
 * @brief   Change the spatial position of data segment.
 *
 * @return  size: Data length
 */
__attribute__((used))
void *_sbrk(ptrdiff_t incr)
{
    extern char _end[];
    extern char _heap_end[];
    static char *curbrk = _end;

    if ((curbrk + incr < _end) || (curbrk + incr > _heap_end))
    return NULL - 1;

    curbrk += incr;
    return curbrk - incr;
}


