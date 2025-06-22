#include "pulse_counter.h"

#define GPIO_PULSE_COUNTER_COLD_DEBOUNCE_TIME_TICKS    (MS1_TO_SYSTEM_TIME(3))
#define GPIO_PULSE_COUNTER_HOT_DEBOUNCE_TIME_TICKS     (MS1_TO_SYSTEM_TIME(3))

#define GPIO_PULSE_COUNTER_COLD_PIN             GPIO_Pin_4
#define GPIO_PULSE_COUNTER_HOT_PIN              GPIO_Pin_5

static uint32_t pulse_count_cold = 0;
static BOOL pulse_trigger_flag_cold;
static uint8_t registered_state_cold;

static uint32_t pulse_count_hot = 0;
static BOOL pulse_trigger_flag_hot;
static uint8_t registered_state_hot;


__INTERRUPT
__HIGH_CODE
void GPIOA_IRQHandler(void)
{
    uint32_t f = GPIOA_ReadITFlagBit(GPIO_PULSE_COUNTER_COLD_PIN | GPIO_PULSE_COUNTER_HOT_PIN);

    if(f & GPIO_PULSE_COUNTER_COLD_PIN)
    {
        // ��ҧ�٧ѧ�֧ݧ�ߧ� ��ߧѧ�ѧݧ� ���ܧݧ��ѧ֧� ���֧��ӧѧߧڧ� �էݧ� ����ԧ� ��ڧߧ�,
        R16_PA_INT_EN &= ~(GPIO_PULSE_COUNTER_COLD_PIN); // ���ܧݧ��ѧ֧� ���֧��ӧѧߧڧ� �� ����ҧ�اէ֧ߧڧ� ��� ����ާ� �ܧ�ߧܧ�֧�ߧ�ާ� ��ڧߧ�
        // ��ݧѧԧ� ��ڧ��ڧ� ��ѧ٧է֧ݧ�ߧ�, ����ܧ�ݧ�ܧ� �ާ�ا֧� �ҧ��� �ԧ�ߧܧ� �էѧߧߧ��
        GPIOA_ClearITFlagBit(GPIO_PULSE_COUNTER_COLD_PIN);
        pulse_trigger_flag_cold = TRUE;
        switch(GPIOA_GetITMode(GPIO_PULSE_COUNTER_COLD_PIN))
        {
            case GPIO_ITMode_LowLevel:
            {
                pulse_count_cold++;
                registered_state_cold = 0;

            }break;
            case GPIO_ITMode_HighLevel:
            {
                registered_state_cold = 1;
            }break;
        }
    }

    if(f & GPIO_PULSE_COUNTER_HOT_PIN)
    {
        R16_PA_INT_EN &= ~(GPIO_PULSE_COUNTER_HOT_PIN);
        GPIOA_ClearITFlagBit(GPIO_PULSE_COUNTER_HOT_PIN);
        pulse_trigger_flag_hot = TRUE;
        switch(GPIOA_GetITMode(GPIO_PULSE_COUNTER_HOT_PIN))
        {
            case GPIO_ITMode_LowLevel:
            {
                pulse_count_hot++;
                registered_state_hot = 0;

            }break;
            case GPIO_ITMode_HighLevel:
            {
                registered_state_hot = 1;
            }break;
        }
    }
    HAL_SLEEP_IRQ_POSTPROCESS();
}


void PulseCounter_init(void)
{
    GPIOA_ModeCfg(GPIO_PULSE_COUNTER_COLD_PIN | GPIO_PULSE_COUNTER_HOT_PIN, GPIO_ModeIN_PU);
    GPIOA_ITModeCfg(GPIO_PULSE_COUNTER_COLD_PIN | GPIO_PULSE_COUNTER_HOT_PIN, GPIO_ITMode_LowLevel);
    PFIC_EnableIRQ(GPIO_A_IRQn);
    Broadcaster_update_pulse_counter_advertising(pulse_count_hot, pulse_count_cold);
}

uint32_t PulseCounter_get_counter_cold(void)
{
    return pulse_count_cold;
}

uint32_t PulseCounter_get_counter_hot(void)
{
    return pulse_count_hot;
}

void PulseCounter_prosess(void)
{
    if(pulse_trigger_flag_cold)
    {
        pulse_trigger_flag_cold = FALSE;
        tmos_start_task(Broadcaster_TaskID, SBP_TIMEOUT_COLD_EVT, GPIO_PULSE_COUNTER_COLD_DEBOUNCE_TIME_TICKS);
        if(registered_state_cold == 0)
        {
            PRINT("Pulse hot=%u, cold=%u\n", pulse_count_hot, pulse_count_cold);
            Broadcaster_update_pulse_counter_advertising(pulse_count_hot, pulse_count_cold);
        }
    }
    if(pulse_trigger_flag_hot)
    {
        pulse_trigger_flag_hot = FALSE;
        tmos_start_task(Broadcaster_TaskID, SBP_TIMEOUT_HOT_EVT, GPIO_PULSE_COUNTER_HOT_DEBOUNCE_TIME_TICKS);
        if(registered_state_hot == 0)
        {
            PRINT("Pulse hot=%u, cold=%u\n", pulse_count_hot, pulse_count_cold);
            Broadcaster_update_pulse_counter_advertising(pulse_count_hot, pulse_count_cold);
        }
    }
}

void PulseCounter_on_timer_cold(void)
{
    if(registered_state_cold == 1)
    {
        GPIOA_ITModeCfg(GPIO_PULSE_COUNTER_COLD_PIN, GPIO_ITMode_LowLevel);
    }
    else
    {
        GPIOA_ITModeCfg(GPIO_PULSE_COUNTER_COLD_PIN, GPIO_ITMode_HighLevel);
    }
}

void PulseCounter_on_timer_hot(void)
{
    if(registered_state_hot == 1)
    {
        GPIOA_ITModeCfg(GPIO_PULSE_COUNTER_HOT_PIN, GPIO_ITMode_LowLevel);
    }
    else
    {
        GPIOA_ITModeCfg(GPIO_PULSE_COUNTER_HOT_PIN, GPIO_ITMode_HighLevel);
    }
}
