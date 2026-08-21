#ifndef __IAP_PORT_H
#define __IAP_PORT_H

#include "main.h"

uint8_t IAP_Port_IsButtonPressed(void);
void    IAP_Port_SendByte(uint8_t byte);
void    IAP_Port_RxStart(void);
void    IAP_Port_LedToggle(void);
void    IAP_Port_DelayMs(uint32_t ms);

#endif /* __IAP_PORT_H */
