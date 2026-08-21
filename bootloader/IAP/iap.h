#ifndef __IAP_H
#define __IAP_H

#include "main.h"

/* ================= 移植配置: 换芯片时改这里 ================= */
#define IAP_APP_BASE            0x08004000UL   /* App 起始地址 */
#define IAP_APP_END             0x08020000UL   /* App 结束地址(不含) */
#define IAP_FLASH_PAGE_SIZE     2048UL         /* 芯片 Flash 页大小(字节) */
/* ========================================================== */

#define IAP_SOF1                0xAA
#define IAP_SOF2                0x55
#define IAP_CMD_HS              0x01
#define IAP_CMD_ERASE           0x02
#define IAP_CMD_DATA            0x03
#define IAP_CMD_END             0x04
#define IAP_CMD_JUMP            0x05

#define IAP_ACK                 0x06
#define IAP_NACK                0x15

void IAP_Init(void);
void IAP_OnRxByte(uint8_t byte);
void IAP_Process(void);
void IAP_JumpToApplication(void);

#endif /* __IAP_H */
