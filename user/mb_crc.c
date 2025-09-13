#include "mb_crc.h"

USHORT usMBCRC16(const UCHAR * pucFrame, USHORT usLen)
{
    USHORT crc = 0xFFFF;
    for (USHORT pos = 0; pos < usLen; pos++) {
        crc ^= (USHORT)pucFrame[pos];
        for (int i = 0; i < 8; i++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
