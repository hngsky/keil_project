#ifndef PORTTIMER_H
#define PORTTIMER_H

#include "mb_defs.h"
#include "mb_config.h"

void vMBPortTimersInit(void);
void vMBPortTimersEnableTxTimeout(uint32_t us);
void vMBPortTimersEnableRxTimeout(uint32_t us);
void vMBPortTimersDisable(void);

/* TIM 中断处理函数要在 TIM IRQ Handler 中调用 */
void MBPortTimerIRQHandler(TIM_HandleTypeDef *htim);

#endif
