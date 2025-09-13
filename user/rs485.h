#ifndef __RS485_H
#define __RS485_H

#include "main.h"

#define RS485_COM1_DIR_GPIO_PORT    GPIOA
#define RS485_COM1_DIR_PIN          GPIO_PIN_8

#define RS485_COM2_DIR_GPIO_PORT    GPIOB
#define RS485_COM2_DIR_PIN          GPIO_PIN_2


void RS485_COM1_SetTX(void);
void RS485_COM1_SetRX(void);
void RS485_COM2_SetTX(void);
void RS485_COM2_SetRX(void);

#endif
