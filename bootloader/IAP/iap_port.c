#include "iap.h"
#include "iap_port.h"
#include "usart.h"
#include "led.h"

/* ============ 移植层: 换芯片时只需要改本文件 ============ */

#define IAP_UART        huart2           /* 移植: 改成你的 UART 句柄 */
extern UART_HandleTypeDef IAP_UART;      /* 跟随上面的宏, 无需手改 */

static uint8_t iap_rx_byte;

/* 串口收到一字节回调: 喂给 IAP 状态机并重新启动接收 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &IAP_UART)
  {
    IAP_OnRxByte(iap_rx_byte);
    HAL_UART_Receive_IT(&IAP_UART, &iap_rx_byte, 1);
  }
}

/* 串口错误回调(溢出等): 清溢出标志后重新接收 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == &IAP_UART)
  {
    __HAL_UART_CLEAR_OREFLAG(&IAP_UART);
    HAL_UART_Receive_IT(&IAP_UART, &iap_rx_byte, 1);
  }
}

/* 按键检测(进 BL 模式用): 低有效, 带 20ms 去抖 */
uint8_t IAP_Port_IsButtonPressed(void)
{
  if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET)
  {
    HAL_Delay(20);
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET)
      return 1;
  }
  return 0;
}

/* 串口发送一字节(iap.c 回复 ACK/NACK 用) */
void IAP_Port_SendByte(uint8_t byte)
{
  HAL_UART_Transmit(&IAP_UART, &byte, 1, 100);
}

/* 启动串口中断接收(每收 1 字节重新 arm 一次) */
void IAP_Port_RxStart(void)
{
  HAL_UART_Receive_IT(&IAP_UART, &iap_rx_byte, 1);
}

/* BL 模式指示灯翻转 */
void IAP_Port_LedToggle(void)
{
  static uint8_t state = 0;
  state ^= 1;
  led_write_pin(0, state);
}

/* 毫秒延时 */
void IAP_Port_DelayMs(uint32_t ms)
{
  HAL_Delay(ms);
}
