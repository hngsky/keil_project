#ifndef MB_RTU_MASTER_H
#define MB_RTU_MASTER_H

#include "mb_defs.h"

#if MB_MASTER_RTU_ENABLED > 0

eMBErrorCode    eMBMasterRTUInit( UCHAR ucPort, ULONG ulBaudRate,eMBParity eParity );
void            eMBMasterRTUStart( void );
void            eMBMasterRTUStop( void );
eMBErrorCode    eMBMasterRTUReceive( UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength );
eMBErrorCode    eMBMasterRTUSend( UCHAR slaveAddress, const UCHAR * pucFrame, USHORT usLength );
BOOL            xMBMasterRTUReceiveFSM( void );
BOOL            xMBMasterRTUTransmitFSM( void );
BOOL            xMBMasterRTUTimerExpired( void );

#endif

#endif
