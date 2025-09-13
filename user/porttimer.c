#include "porttimer.h"
#include <math.h>

/* 使用 TIM2，频率 1MHz (1 us per tick) */
static TIM_HandleTypeDef *htim = MB_TIMER_HANDLE;

void vMBPortTimersInit(void)
{
    /* 假设 TIM2 已在 CubeMX 初始化为 1MHz，直接使能中断即可 */
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
    HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0);
}

void vMBPortTimersEnableTxTimeout(uint32_t us)
{
    __HAL_TIM_DISABLE(htim);
    __HAL_TIM_SET_AUTORELOAD(htim, us);
    __HAL_TIM_SET_COUNTER(htim, 0);
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(htim, TIM_IT_UPDATE);
    HAL_TIM_Base_Start(htim);
}

void vMBPortTimersEnableRxTimeout(uint32_t us)
{
    /* Rx timeout (通常用于 T3.5) */
    vMBPortTimersEnableTxTimeout(us);
}

void vMBPortTimersDisable(void)
{
    HAL_TIM_Base_Stop_IT(htim);
    __HAL_TIM_DISABLE_IT(htim, TIM_IT_UPDATE);
}

/* TIM 中断回调 */
void MBPortTimerIRQHandler(TIM_HandleTypeDef *htim_local)
{
    if (htim_local->Instance == htim->Instance) {
        __HAL_TIM_CLEAR_FLAG(htim_local, TIM_FLAG_UPDATE);
        HAL_TIM_Base_Stop_IT(htim_local);
        /* 通知 Modbus core：到期 */
        extern volatile BOOL mb_timer_expired_for_master;
        extern volatile BOOL mb_timer_expired_for_slave;
        /* 谁启的定时器由上层记录；此处为了简单：调用两个信号 */
        mb_timer_expired_for_master = TRUE;
        mb_timer_expired_for_slave = TRUE;
    }
}
