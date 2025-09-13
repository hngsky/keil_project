#include "portserial.h"
#include "mb_crc.h"
#include <string.h>
#include "rs485.h"

extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart3_rx;

/* 内部 RX 环形 buffer */
static UCHAR rxBuf1[MB_SERIAL_BUF_SIZE];
static UCHAR rxBuf3[MB_SERIAL_BUF_SIZE];

/* 当前 DMA 计数快照 */
static volatile USHORT lastPos1 = 0;
static volatile USHORT lastPos3 = 0;

/* 用于与 Modbus core 通信的帧完结标志与长度 */
volatile BOOL mb1_frame_ready = FALSE;
volatile USHORT mb1_frame_len = 0;
volatile BOOL mb3_frame_ready = FALSE;
volatile USHORT mb3_frame_len = 0;

/* 指向当前 DMA 缓冲区，用于外部读取 */
const UCHAR *mb1_rx_ptr = rxBuf1;
const UCHAR *mb3_rx_ptr = rxBuf3;

/* Helper - get DMA remaining for huart rx */
static inline uint16_t get_dma_remaining(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) return __HAL_DMA_GET_COUNTER(huart->hdmarx);
    if (huart->Instance == USART3) return __HAL_DMA_GET_COUNTER(huart->hdmarx);
    return 0;
}

/* 初始化: 这里只对 rx dma 启动及 debug uart 保持可用 */
void vMBPortSerialInit(UCHAR port, ULONG ulBaudRate, eMBParity eParity)
{
    /* UART 波特率/校验通常通过 CubeMX 设置；这里只修改 huart.Instance->BRR 或调用 HAL_UART_Init 如果需要
       为了简洁：假设 CubeMX 已为各串口设置好，或者用户在 main 里先设置波特率 */
    (void)port; (void)ulBaudRate; (void)eParity;
}

/* 启/停收发 */
void vMBPortSerialEnable(UCHAR port, BOOL rxEnable, BOOL txEnable)
{
    if (port == 1) {
        if (rxEnable) {
            RS485_COM1_SetRX();
            vMBPortSerialStartReceiveDMA(1);
        } else {
            /* 停用 DMA RX */
            HAL_UART_AbortReceive(&huart1);
        }
        if (txEnable) {
            RS485_COM1_SetTX();
        } else {
            RS485_COM1_SetRX();
        }
    } else if (port == 3) {
        if (rxEnable) {
            RS485_COM2_SetRX();
            vMBPortSerialStartReceiveDMA(3);
        } else {
            HAL_UART_AbortReceive(&huart3);
        }
        if (txEnable) {
            RS485_COM2_SetTX();
        } else {
            RS485_COM2_SetRX();
        }
    }
}

/* 启动对应 port 的 DMA RX (circular) */
void vMBPortSerialStartReceiveDMA(UCHAR port)
{
    if (port == 1) {
        /* 启动 USART1 RX DMA 到 rxBuf1 */
        HAL_UART_AbortReceive(&huart1);
        HAL_UART_Receive_DMA(&huart1, (uint8_t*)rxBuf1, MB_SERIAL_BUF_SIZE);
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
    } else if (port == 3) {
        HAL_UART_AbortReceive(&huart3);
        HAL_UART_Receive_DMA(&huart3, (uint8_t*)rxBuf3, MB_SERIAL_BUF_SIZE);
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);
    }
}

/* 写（发送）: 设置 DE = TX, 启动 DMA TX */
void vMBPortSerialWrite(UCHAR port, const UCHAR *pucFrame, USHORT usLength)
{
    if (port == 1) {
        RS485_COM1_SetTX();
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)pucFrame, usLength);
        /* 发送完成后由 HAL 的 TxCplt 回调设置 RX */
    } else if (port == 3) {
        RS485_COM2_SetTX();
        HAL_UART_Transmit_DMA(&huart3, (uint8_t*)pucFrame, usLength);
    }
}

/* 在 USART IRQ 中调用以处理 IDLE 中断 -> 把完整帧标记给 Modbus core */
void MBPortSerialUSARTxIRQHandler(UART_HandleTypeDef *huart)
{
    /* check IDLE flag */
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        /* 计算收到的新字节数 */
        if (huart->Instance == USART1) {
            uint16_t dma_cnt = __HAL_DMA_GET_COUNTER(huart->hdmarx);
            uint16_t pos = MB_SERIAL_BUF_SIZE - dma_cnt;
            if (pos != lastPos1) {
                /* 数据长度 */
                uint16_t len = (pos >= lastPos1) ? (pos - lastPos1) : (MB_SERIAL_BUF_SIZE - lastPos1 + pos);
                /* i) 若 buffer 并非循环覆盖（因为我们每次以 circular full buffer），直接把整帧长度赋值为 pos (此处简化) */
                /* 实际上为了适配 Modbus：我们认为一帧从上一次 idle 到本次 idle 的全部数据为一帧 -> 直接用 pos 作为长度 */
                mb1_frame_len = pos;
                mb1_frame_ready = TRUE;
            }
            lastPos1 = pos;
        } else if (huart->Instance == USART3) {
            uint16_t dma_cnt = __HAL_DMA_GET_COUNTER(huart->hdmarx);
            uint16_t pos = MB_SERIAL_BUF_SIZE - dma_cnt;
            if (pos != lastPos3) {
                mb3_frame_len = pos;
                mb3_frame_ready = TRUE;
            }
            lastPos3 = pos;
        }
    }
}

/* DMA / UART 回调，放在 stm32xx_hal_uart.c 的回调里或 stm32f1xx_it.c 中调用 */
void MBPortSerialTxCpltCallback(UART_HandleTypeDef *huart)
{
    /* TX finished physically? HAL 回调表示 DMA 传输完成，串口仍可能有数据在移出缓冲寄存器，
       最保险办法：等待 TC bit 设置。这里做简化：短延时 + 切回 RX */
    HAL_Delay(1); /* 若你要求更严谨，使用专门检查 TC 标志 */
    if (huart->Instance == USART1) {
        RS485_COM1_SetRX();
        vMBPortSerialStartReceiveDMA(1);
    } else if (huart->Instance == USART3) {
        RS485_COM2_SetRX();
        vMBPortSerialStartReceiveDMA(3);
    }
}

/* 可选：DMA half/complete 回调（未用） */
void MBPortSerialRxHalfCpltCallback(DMA_HandleTypeDef *hdma) { (void)hdma; }
void MBPortSerialRxCpltCallback(DMA_HandleTypeDef *hdma) { (void)hdma; }
