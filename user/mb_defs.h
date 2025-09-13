#ifndef MB_DEFS_H
#define MB_DEFS_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t UCHAR;
typedef uint16_t USHORT;
typedef int32_t eMBErrorCode;
typedef enum { MB_PAR_NONE = 0, MB_PAR_ODD, MB_PAR_EVEN } eMBParity;
typedef uint8_t BOOL;
typedef uint32_t ULONG;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* eMBErrorCode 常用值（简化） */
#define MB_ENOERR       0
#define MB_EIO         -1
#define MB_EINVAL      -2
#define MB_ETIMEDOUT   -3

#endif // MB_DEFS_H
