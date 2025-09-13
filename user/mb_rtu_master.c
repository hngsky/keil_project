#include "mb_rtu_master.h"
#include "mb_crc.h"
#include "portserial.h"
#include "porttimer.h"
#include "mb_config.h"
#include <string.h>
#include <math.h>
/* 使能宏 */
#ifndef MB_MASTER_RTU_ENABLED
#define MB_MASTER_RTU_ENABLED 1
#endif

#if MB_MASTER_RTU_ENABLED > 0

static UCHAR master_port = 1; /* USART1 */
static ULONG master_baud = 9600;
static eMBParity master_parity = MB_PAR_NONE;

/* 发送/接收缓冲 */
static UCHAR master_tx[MB_SERIAL_BUF_SIZE];
static UCHAR master_rx[MB_SERIAL_BUF_SIZE];

/* 从 portserial 获取帧就绪（针对 USART1） */
extern volatile BOOL mb1_frame_ready;
extern volatile USHORT mb1_frame_len;
extern const UCHAR *mb1_rx_ptr;

/* 定时器到期标志 */
volatile BOOL mb_timer_expired_for_master = FALSE;

/* init */
eMBErrorCode eMBMasterRTUInit( UCHAR ucPort, ULONG ulBaudRate,eMBParity eParity )
{
    master_port = ucPort;
    master_baud = ulBaudRate;
    master_parity = eParity;
    vMBPortSerialInit(master_port, ulBaudRate, eParity);
    vMBPortTimersInit();
    return MB_ENOERR;
}

void eMBMasterRTUStart( void )
{
    vMBPortSerialEnable(master_port, TRUE, FALSE);
}

void eMBMasterRTUStop( void )
{
    vMBPortSerialEnable(master_port, FALSE, FALSE);
}

/* 主站接收：当帧准备好后复制并返回 */
eMBErrorCode eMBMasterRTUReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    if (!mb1_frame_ready) return MB_ETIMEDOUT;
    USHORT len = mb1_frame_len;
    if (len < 3) {
        mb1_frame_ready = FALSE;
        return MB_EIO;
    }
    memcpy(master_rx, mb1_rx_ptr, len);
    USHORT crc = usMBCRC16(master_rx, len - 2);
    USHORT rcrc = master_rx[len - 2] | (master_rx[len - 1] << 8);
    if (crc != rcrc) {
        mb1_frame_ready = FALSE;
        return MB_EIO;
    }
    *pucRcvAddress = master_rx[0];
    *pucFrame = &master_rx[1];
    *pusLength = (USHORT)(len - 3);
    mb1_frame_ready = FALSE;
    return MB_ENOERR;
}

/* 主站发送请求（拼装地址+PDU+CRC） */
eMBErrorCode eMBMasterRTUSend( UCHAR slaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    int out = 0;
    master_tx[out++] = slaveAddress;
    memcpy(&master_tx[out], pucFrame, usLength);
    out += usLength;
    USHORT crc = usMBCRC16(master_tx, out);
    master_tx[out++] = crc & 0xFF;
    master_tx[out++] = (crc >> 8) & 0xFF;

    vMBPortSerialWrite(master_port, master_tx, out);

    /* 启动接收等待定时器（T3.5 的若干倍作为超时） */
    /* 计算 T3.5_us */
    uint32_t charBits = 1 + 8 + 1; /* start + data + stop; parity ignored for charTime 估算 */
    uint32_t charTime_us = (charBits * 1000000UL) / master_baud;
    uint32_t t35 = (uint32_t)ceil(3.5 * charTime_us);
    /* 主站等待 3*t35 */
    vMBPortTimersEnableRxTimeout(t35 * 3);
    return MB_ENOERR;
}

BOOL xMBMasterRTUReceiveFSM( void )
{
    return mb1_frame_ready ? TRUE : FALSE;
}

BOOL xMBMasterRTUTransmitFSM( void )
{
    return TRUE;
}

BOOL xMBMasterRTUTimerExpired( void )
{
    return mb_timer_expired_for_master;
}

#endif /* MB_MASTER_RTU_ENABLED */
