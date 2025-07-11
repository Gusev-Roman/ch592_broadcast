/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH59x_SYS.h
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2021/11/17
 * Description
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef __CH59x_SYS_H__
#define __CH59x_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  rtc interrupt event define
 */
typedef enum
{
    RST_STATUS_SW = 0, // 软件复位
    RST_STATUS_RPOR,   // 上电复位
    RST_STATUS_WTR,    // Watchdog timeout reset
    RST_STATUS_MR,     // 外部手动复位
    RST_STATUS_LRM0,   // 唤醒复位-软复位引起
    RST_STATUS_GPWSM,  // 下电模式唤醒复位
    RST_STATUS_LRM1,   //	唤醒复位-看门狗引起
    RST_STATUS_LRM2,   //	唤醒复位-手动复位引起

} SYS_ResetStaTypeDef;

/**
 * @brief  rtc interrupt event define
 */
typedef enum
{
    INFO_ROM_READ = 0, // FlashROM 代码和数据区 是否可读
    INFO_RESET_EN = 2, // RST#外部手动复位输入功能是否开启
    INFO_BOOT_EN,      // 系统引导程序 BootLoader 是否开启
    INFO_DEBUG_EN,     // 系统仿真调试接口是否开启
    INFO_LOADER,       // 当前系统是否处于Bootloader 区
    STA_SAFEACC_ACT,   // 当前系统是否处于安全访问状态，否则RWA属性区域不可访问

} SYS_InfoStaTypeDef;

/**
 * @brief  获取芯片ID类，一般为固定值
 */
#define SYS_GetChipID()      R8_CHIP_ID

/**
 * @brief  获取安全访问ID，一般为固定值
 */
#define SYS_GetAccessID()    R8_SAFE_ACCESS_ID

/**
 * @brief   配置系统运行时钟
 *
 * @param   sc      - 系统时钟源选择 refer to SYS_CLKTypeDef
 */
void SetSysClock(SYS_CLKTypeDef sc);

/**
 * @brief   获取当前系统时钟
 *
 * @return  Hz
 */
uint32_t GetSysClock(void);

/**
 * @brief   获取当前系统信息状态
 *
 * @param   i       - refer to SYS_InfoStaTypeDef
 *
 * @return  是否开启
 */
uint8_t SYS_GetInfoSta(SYS_InfoStaTypeDef i);

/**
 * @brief   获取系统上次复位状态
 *
 * @return  refer to SYS_ResetStaTypeDef
 */
#define SYS_GetLastResetSta()    (R8_RESET_STATUS & RB_RESET_FLAG)

/**
 * @brief   执行系统软件复位
 */
void SYS_ResetExecute(void);

/**
 * @brief   Set the reset saving register value, which is not affected by manual reset, software reset, watchdog reset or normal wake-up reset
 *
 * @param   i       - refer to SYS_InfoStaTypeDef
 */
#define SYS_ResetKeepBuf(d)    (R8_GLOB_RESET_KEEP = d)

/**
 * @brief   关闭所有中断，并保留当前中断值
 *
 * @param   pirqv   - 当前保留中断值
 */
void SYS_DisableAllIrq(uint32_t *pirqv);

/**
 * @brief   恢复之前关闭的中断值
 *
 * @param   irq_status  - 当前保留中断值
 */
void SYS_RecoverIrq(uint32_t irq_status);

/**
 * @brief   获取当前系统(SYSTICK)计数值
 *
 * @return  当前计数值
 */
uint32_t SYS_GetSysTickCnt(void);

/**
 * @brief   Load the initial value of the watchdog count, increasing type
 *
 * @param   c       - Watchdog count initial value
 */
#define WWDG_SetCounter(c)    (R8_WDOG_COUNT = c)

/**
 * @brief   Watchdog timer overflow interrupt enable
 *
 * @param   s       - Overflow interrupt
 */
void WWDG_ITCfg(FunctionalState s);

/**
 * @brief   Watchdog timer reset function
 *
 * @param   s       - Overflow reset
 */
void WWDG_ResetCfg(FunctionalState s);

/**
 * @brief   Get the current watchdog timer overflow flag
 *
 * @return  看门狗定时器溢出标志
 */
#define WWDG_GetFlowFlag()    (R8_RST_WDOG_CTRL & RB_WDOG_INT_FLAG)

/**
 * @brief   Clear the watchdog interrupt flag, reload the count value to clear it
 */
void WWDG_ClearFlag(void);

/**
 * @brief   uS 延时
 *
 * @param   t       - 时间参数
 */
void mDelayuS(uint16_t t);

/**
 * @brief   mS 延时
 *
 * @param   t       - 时间参数
 */
void mDelaymS(uint16_t t);

/**
 * @brief Enter safe access mode.
 * 
 * @NOTE: After enter safe access mode, about 16 system frequency cycles 
 * are in safe mode, and one or more secure registers can be rewritten 
 * within the valid period. The safe mode will be automatically 
 * terminated after the above validity period is exceeded.
 *  if sys_safe_access_enable() is called,
 *  you must call sys_safe_access_disable() before call sys_safe_access_enable() again.
 */
#define sys_safe_access_enable()        do{volatile uint32_t mpie_mie;mpie_mie=__risc_v_disable_irq();SAFEOPERATE;\
                                        R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;SAFEOPERATE;

#define sys_safe_access_disable()       R8_SAFE_ACCESS_SIG = 0;__risc_v_enable_irq(mpie_mie);SAFEOPERATE;}while(0)

#ifdef __cplusplus
}
#endif

#endif // __CH59x_SYS_H__
