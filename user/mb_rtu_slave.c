#include "mb_rtu_slave.h"
#include "mb_crc.h"
#include "portserial.h"
#include "porttimer.h"
#include "mb_config.h"
#include <string.h>

/* 使能宏 */
#ifndef MB_SLAVE_RTU_ENABLED
#define MB_SLAVE_RTU_ENABLED 1
#endif

#if MB_SLAVE_RTU_ENABLED > 0

static UCHAR slave_addr = 1;
static UCHAR slave_port = 3; /* USART3 作为从机 */
static ULONG slave_baud = 9600;
static eMBParity slave_parity = MB_PAR_NONE;

/* 接收缓冲（供上层读取） */
static UCHAR rx_frame[MB_SERIAL_BUF_SIZE];
static UCHAR tx_frame[MB_SERIAL_BUF_SIZE];

/* 帧就绪标志 */
extern volatile BOOL mb3_frame_ready;
extern volatile USHORT mb3_frame_len;
extern const UCHAR *mb3_rx_ptr;

/* 定时器到期标志（由 porttimer 设置） */
volatile BOOL mb_timer_expired_for_slave = FALSE;

/* 初始化 */
eMBErrorCode eMBRTUInit( UCHAR slaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    slave_addr = slaveAddress;
    slave_port = ucPort;
    slave_baud = ulBaudRate;
    slave_parity = eParity;

    vMBPortSerialInit(ucPort, ulBaudRate, eParity);
    vMBPortTimersInit();
    return MB_ENOERR;
}

void eMBRTUStart( void )
{
    /* 启动接收 */
    vMBPortSerialEnable(slave_port, TRUE, FALSE);
}

/* 停止 */
void eMBRTUStop( void )
{
    vMBPortSerialEnable(slave_port, FALSE, FALSE);
}

/* 应用层调用：当 eMBRTUReceive 返回 MB_ENOERR 时，*pucFrame 指向 PDU（不含地址与 CRC），len 为 PDU 长度 */
eMBErrorCode eMBRTUReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    if (!mb3_frame_ready) return MB_ETIMEDOUT;

    /* 复制到本地缓冲，方便处理 */
    USHORT len = mb3_frame_len;
    if (len < 3) { /* 最少 addr + func + crc(2) */
        mb3_frame_ready = FALSE;
        return MB_EIO;
    }
    if (len > MB_SERIAL_BUF_SIZE) len = MB_SERIAL_BUF_SIZE;
    memcpy(rx_frame, mb3_rx_ptr, len);

    /* 校验 CRC */
    USHORT crc = usMBCRC16(rx_frame, len - 2);
    USHORT rcrc = rx_frame[len - 2] | (rx_frame[len - 1] << 8);
    if (crc != rcrc) {
        mb3_frame_ready = FALSE;
        return MB_EIO;
    }

    /* 地址判断 */
    UCHAR addr = rx_frame[0];
    if (addr != slave_addr && addr != 0) { /* 非本地址且非广播 -> 忽略 */
        mb3_frame_ready = FALSE;
        return MB_EIO;
    }

    /* 返回 PDU 指针（指向 function + data 部分） */
    *pucRcvAddress = addr;
    *pucFrame = &rx_frame[1];
    *pusLength = (USHORT)(len - 3); /* PDU 不含 CRC, 不含地址, 所以 len - addr(1) - crc(2) */

    mb3_frame_ready = FALSE; /* 已取走帧 */
    return MB_ENOERR;
}

/* 发送：上层传入 PDU（不含地址与 CRC），我们要拼装地址+PDU+CRC 并发送 */
eMBErrorCode eMBRTUSend( UCHAR slaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    /* 构造帧：addr + PDU + CRC */
    int out_len = 0;
    tx_frame[out_len++] = slaveAddress;
    memcpy(&tx_frame[out_len], pucFrame, usLength);
    out_len += usLength;
    USHORT crc = usMBCRC16(tx_frame, out_len);
    tx_frame[out_len++] = crc & 0xFF;
    tx_frame[out_len++] = (crc >> 8) & 0xFF;

    /* RS485 置 TX 并发送 */
    vMBPortSerialWrite(slave_port, tx_frame, out_len);
    return MB_ENOERR;
}

/* FSM 与定时器到期处理（简化实现，主要由上面 Receive/Send 协调） */
BOOL xMBRTUReceiveFSM( void )
{
    /* 如果 mb3_frame_ready 则返回 TRUE */
    return mb3_frame_ready ? TRUE : FALSE;
}

BOOL xMBRTUTransmitFSM( void )
{
    /* 发送通过 HAL 回调完成，不在此处做复杂处理 */
    return TRUE;
}

BOOL xMBRTUTimerT15Expired( void )
{
    /* 若我们需要精确区分 T1.5, T3.5，可在 porttimer 中记录 */
    (void)mb_timer_expired_for_slave;
    return mb_timer_expired_for_slave;
}

BOOL xMBRTUTimerT35Expired( void )
{
    (void)mb_timer_expired_for_slave;
    return mb_timer_expired_for_slave;
}

#endif /* MB_SLAVE_RTU_ENABLED */
