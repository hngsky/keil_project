#ifndef MB_CONFIG_H
#define MB_CONFIG_H

#include "mb_defs.h"
#include "stm32f1xx_hal.h"

/* --- UART / DMA / TIM handles from CubeMX generated code --- */
/* 修改下面的 extern 名称以匹配你的 CubeMX 生成变量 */
extern UART_HandleTypeDef huart1; /* 主机: USART1 */
extern UART_HandleTypeDef huart2; /* 调试: USART2 */
extern UART_HandleTypeDef huart3; /* 从机: USART3 */

extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;

extern TIM_HandleTypeDef htim2; /* 用于 T1.5/T3.5 计时 */



/* Buffer sizes */
#define MB_SERIAL_BUF_SIZE 256

/* Timer selection */
#define MB_TIMER_HANDLE &htim2

#endif // MB_CONFIG_H
