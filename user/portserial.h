#ifndef PORTSERIAL_H
#define PORTSERIAL_H

#include "mb_defs.h"
#include "mb_config.h"

/* 初始化串口及 DMA (port: 1 for USART1, 3 for USART3) */
void vMBPortSerialInit(UCHAR port, ULONG ulBaudRate, eMBParity eParity);

/* 启/停接收或发送（rxEnable 或 txEnable） */
void vMBPortSerialEnable(UCHAR port, BOOL rxEnable, BOOL txEnable);

/* 发送缓冲区（DMA） - 非阻塞，发送完成由回调通知 */
void vMBPortSerialWrite(UCHAR port, const UCHAR *pucFrame, USHORT usLength);

/* 启动接收 DMA (circular) */
void vMBPortSerialStartReceiveDMA(UCHAR port);

/* 在 USART IRQ 中调用以处理 IDLE 中断 */
void MBPortSerialUSARTxIRQHandler(UART_HandleTypeDef *huart);

/* DMA TX complete & RX complete 回调入口（在 HAL 回调内调用） */
void MBPortSerialTxCpltCallback(UART_HandleTypeDef *huart);
void MBPortSerialRxHalfCpltCallback(DMA_HandleTypeDef *hdma);
void MBPortSerialRxCpltCallback(DMA_HandleTypeDef *hdma);

#endif
