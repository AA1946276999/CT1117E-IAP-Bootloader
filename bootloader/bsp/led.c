#include "led.h"

static uint8_t temp = 0x00;
static uint8_t temp_old = 0xff;

uint8_t led_fuc_slow,led_fuc_flag;

void led_write_pin(uint8_t address,uint8_t enable)
{
	if(enable)
		temp |= 0x01<<address;
	else
		temp &= ~(0x01<<address);
	if(temp_old != temp)
	{
		temp_old = temp;
		HAL_GPIO_WritePin(GPIOC,(uint16_t)(temp<<8),GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC,(uint16_t)~(temp<<8)&0xff00,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	}
}

void led_all_off(void)
{
	led_write_pin(0,0);
	led_write_pin(1,0);
	led_write_pin(2,0);
	led_write_pin(3,0);
	led_write_pin(4,0);
	led_write_pin(5,0);
	led_write_pin(6,0);
	led_write_pin(7,0);
}
