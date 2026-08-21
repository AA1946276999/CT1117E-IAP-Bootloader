#include "iap.h"
#include "iap_port.h"

#define IAP_BUF_MAX      (5 + 256 + 2)

typedef enum
{
  IAP_RX_IDLE = 0,
  IAP_RX_SOF2,
  IAP_RX_CMD,
  IAP_RX_LENL,
  IAP_RX_LENH,
  IAP_RX_DATA,
  IAP_RX_CRCL,
  IAP_RX_CRCH
} IAP_RxState_t;

typedef struct
{
  IAP_RxState_t state;
  uint8_t  buf[IAP_BUF_MAX];
  uint16_t len;
  uint16_t idx;
  volatile uint8_t frame_ready;
  uint8_t  frame_cmd;
  uint16_t frame_len;
  uint32_t write_addr;
  uint8_t  prog_buf[8];
  uint8_t  prog_len;
  uint16_t file_crc;
  uint32_t received_bytes;
  uint8_t  upgrade_ok;
} IAP_Handle_t;

static IAP_Handle_t iap;

/* CRC16-CCITT 计算: 多项式 0x1021, 初值 0xFFFF(可传入上次结果做累加校验) */
static uint16_t iap_crc16(uint16_t crc, uint8_t *data, uint16_t len)
{
  uint16_t i;
  uint8_t  b;

  for (i = 0; i < len; i++)
  {
    crc ^= (uint16_t)data[i] << 8;
    for (b = 0; b < 8; b++)
    {
      if (crc & 0x8000)
        crc = (uint16_t)((crc << 1) ^ 0x1021);
      else
        crc = (uint16_t)(crc << 1);
    }
  }
  return crc;
}

/* 通过移植层发送一字节(ACK/NACK 回复) */
static void iap_reply(uint8_t status)
{
  IAP_Port_SendByte(status);
}

/* 串口接收状态机: 逐字节解析帧 [AA 55 CMD LEN_LO LEN_HI DATA CRC_LO CRC_HI] */
static void iap_on_rx_byte(uint8_t b)
{
  switch (iap.state)
  {
    case IAP_RX_IDLE:
      if (b == IAP_SOF1)
      {
        iap.buf[0] = b;
        iap.idx = 1;
        iap.state = IAP_RX_SOF2;
      }
      break;

    case IAP_RX_SOF2:
      if (b == IAP_SOF2)
      {
        iap.buf[1] = b;
        iap.idx = 2;
        iap.state = IAP_RX_CMD;
      }
      else
        iap.state = IAP_RX_IDLE;
      break;

    case IAP_RX_CMD:
      iap.buf[2] = b;
      iap.idx = 3;
      iap.state = IAP_RX_LENL;
      break;

    case IAP_RX_LENL:
      iap.buf[3] = b;
      iap.idx = 4;
      iap.state = IAP_RX_LENH;
      break;

    case IAP_RX_LENH:
      iap.buf[4] = b;
      iap.idx = 5;
      iap.len = (uint16_t)(iap.buf[3] | ((uint16_t)iap.buf[4] << 8));
      if (iap.len > 256)
      {
        iap.state = IAP_RX_IDLE;
        iap_reply(IAP_NACK);
      }
      else if (iap.len == 0)
        iap.state = IAP_RX_CRCL;
      else
        iap.state = IAP_RX_DATA;
      break;

    case IAP_RX_DATA:
      iap.buf[iap.idx] = b;
      iap.idx++;
      if (iap.idx >= 5 + iap.len)
        iap.state = IAP_RX_CRCL;
      break;

    case IAP_RX_CRCL:
      iap.buf[iap.idx] = b;
      iap.idx++;
      iap.state = IAP_RX_CRCH;
      break;

    case IAP_RX_CRCH:
    {
      uint16_t crc, rx_crc;
      iap.buf[iap.idx] = b;
      iap.idx++;
      iap.state = IAP_RX_IDLE;

      crc = iap_crc16(0xFFFF, iap.buf, 5 + iap.len);
      rx_crc = (uint16_t)(iap.buf[5 + iap.len] | ((uint16_t)iap.buf[6 + iap.len] << 8));
      if (crc == rx_crc)
      {
        iap.frame_ready = 1;
        iap.frame_cmd = iap.buf[2];
        iap.frame_len = iap.len;
      }
      else
        iap_reply(IAP_NACK);
      break;
    }

    default:
      iap.state = IAP_RX_IDLE;
      break;
  }
}

/* 擦除 App 区: 从 App 起始页擦到芯片末尾 */
static uint8_t iap_erase_app(void)
{
  FLASH_EraseInitTypeDef erase;
  uint32_t page_error = 0;
  uint8_t ok = 0;

  HAL_FLASH_Unlock();
  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.Banks     = FLASH_BANK_1;
  erase.Page      = (IAP_APP_BASE - FLASH_BASE) / IAP_FLASH_PAGE_SIZE;   /* 起始页号 */
  erase.NbPages   = (IAP_APP_END - IAP_APP_BASE) / IAP_FLASH_PAGE_SIZE;  /* 页数 */
  if (HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK)
    ok = 1;
  HAL_FLASH_Lock();
  return ok;
}

/* 命令 0x02: 擦除成功则重置各项状态并回 ACK, 失败回 NACK */
static void iap_handle_erase(void)
{
  if (iap_erase_app())
  {
    iap.write_addr     = IAP_APP_BASE;
    iap.prog_len       = 0;
    iap.file_crc       = 0xFFFF;
    iap.received_bytes = 0;
    iap.upgrade_ok     = 0;
    iap_reply(IAP_ACK);
  }
  else
    iap_reply(IAP_NACK);
}

/* 命令 0x03: 把一帧数据按 64bit 双字写入 Flash, 不足 8B 的留到最后补写 */
static void iap_handle_data(void)
{
  uint8_t *data = &iap.buf[5];
  uint16_t len  = iap.frame_len;
  uint8_t  ok   = 1;

  /* 地址越界检查 */
  if (iap.write_addr + len > IAP_APP_END)
    ok = 0;

  HAL_FLASH_Unlock();
  /* 每 8 字节拼成一个双字写入(STM32G4 编程粒度为 64bit) */
  while (ok && len >= 8)
  {
    uint64_t v = 0;
    uint8_t i;
    for (i = 0; i < 8; i++)
      v |= (uint64_t)data[i] << (8 * i);
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, iap.write_addr, v) != HAL_OK)
    {
      ok = 0;
      break;
    }
    iap.write_addr += 8;
    data += 8;
    len -= 8;
  }
  HAL_FLASH_Lock();

  /* 不足 8 字节的余数先存缓冲, 留到结束帧一起补写 */
  if (ok && len > 0)
  {
    if (iap.prog_len + len > 8)
      ok = 0;
    else
    {
      uint16_t i;
      for (i = 0; i < len; i++)
        iap.prog_buf[iap.prog_len++] = data[i];
    }
  }

  /* 边写边累加整包 CRC 和已收字节数 */
  if (ok)
  {
    iap.file_crc       = iap_crc16(iap.file_crc, &iap.buf[5], iap.frame_len);
    iap.received_bytes += iap.frame_len;
  }
  iap_reply(ok ? IAP_ACK : IAP_NACK);
}

/* 命令 0x04: 补写余数并做整包校验(长度 + 整包CRC), 通过才允许跳转 */
static void iap_handle_end(void)
{
  uint32_t host_len;
  uint16_t host_crc;
  uint8_t  ok = 1;

  host_len = (uint32_t)iap.buf[5]
           | ((uint32_t)iap.buf[6] << 8)
           | ((uint32_t)iap.buf[7] << 16)
           | ((uint32_t)iap.buf[8] << 24);
  host_crc = (uint16_t)(iap.buf[9] | ((uint16_t)iap.buf[10] << 8));

  /* 把不足 8 字节的余数补 0xFF 后写入最后一个双字 */
  if (iap.prog_len > 0)
  {
    uint64_t v = 0;
    uint8_t i;
    while (iap.prog_len < 8)
      iap.prog_buf[iap.prog_len++] = 0xFF;
    for (i = 0; i < 8; i++)
      v |= (uint64_t)iap.prog_buf[i] << (8 * i);
    HAL_FLASH_Unlock();
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, iap.write_addr, v) != HAL_OK)
      ok = 0;
    HAL_FLASH_Lock();
  }

  /* 整包校验: 长度一致 且 整包 CRC 一致 → 升级成功 */
  if (ok && host_len == iap.received_bytes && host_crc == iap.file_crc)
  {
    iap.upgrade_ok = 1;
    iap_reply(IAP_ACK);
  }
  else
    iap_reply(IAP_NACK);
}

/* 命令 0x05: 校验通过才回 ACK, 否则拒绝; 通过后延时再跳转 App */
static void iap_handle_jump(void)
{
  if (!iap.upgrade_ok)
  {
    iap_reply(IAP_NACK);
    return;
  }
  iap_reply(IAP_ACK);
  IAP_Port_DelayMs(50);
  IAP_JumpToApplication();
}

/* 跳转到 App: 校验 SP 有效 → 关中断 → 重设 VTOR/MSP → 恢复中断 → 跳转 */
void IAP_JumpToApplication(void)
{
  typedef void (*pFunction)(void);

  /* 检查 App 起始地址第一个字(栈指针)是否在 RAM 范围 */
  if (((*(__IO uint32_t *)IAP_APP_BASE) & 0x2FFE0000) == 0x20000000)
  {
    __disable_irq();
    SCB->VTOR = IAP_APP_BASE;                                        /* 中断向量表指到 App */
    pFunction app_reset_handler = (pFunction)(*(__IO uint32_t *)(IAP_APP_BASE + 4));
    __set_MSP(*(__IO uint32_t *)IAP_APP_BASE);                       /* 切换到 App 栈 */
    __enable_irq();                                                  /* 恢复中断(复位默认开启) */
    app_reset_handler();
  }
}

/* 进入 IAP 模式: 复位状态机并发送 "BL" 欢迎串, 供上位机确认在线 */
void IAP_Init(void)
{
  static const uint8_t hello[] = "BL\r\n";
  uint8_t i;

  iap.state          = IAP_RX_IDLE;
  iap.idx            = 0;
  iap.len            = 0;
  iap.frame_ready    = 0;
  iap.write_addr     = IAP_APP_BASE;
  iap.prog_len       = 0;
  iap.file_crc       = 0xFFFF;
  iap.received_bytes = 0;
  iap.upgrade_ok     = 0;

  for (i = 0; i < sizeof(hello) - 1; i++)
    IAP_Port_SendByte(hello[i]);
}

/* 串口收到一字节时调用(由移植层的串口回调触发) */
void IAP_OnRxByte(uint8_t byte)
{
  iap_on_rx_byte(byte);
}

/* 主循环轮询: 有完整帧则按命令分发处理 */
void IAP_Process(void)
{
  if (iap.frame_ready)
  {
    iap.frame_ready = 0;
    switch (iap.frame_cmd)
    {
      case IAP_CMD_HS:    iap_reply(IAP_ACK); break;
      case IAP_CMD_ERASE: iap_handle_erase(); break;
      case IAP_CMD_DATA:  iap_handle_data();  break;
      case IAP_CMD_END:   iap_handle_end();   break;
      case IAP_CMD_JUMP:  iap_handle_jump();  break;
      default:            iap_reply(IAP_NACK); break;
    }
  }
}
