#include "key.h"

uint8_t i;
Key key[4];

uint8_t key_state_slow,key_fuc_slow,key_fuc_flag;

extern uint8_t lcd_mode;

void key_state_machine(void)
{
	key[0].val = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
	key[1].val = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1);
	key[2].val = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2);
	key[3].val = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
	for(i=0;i<4;i++)
	{
		if(key[i].flag == 0)
		{
			if(key[i].val == 0)
			{
				if(key[i].cnt_long == 0)key[i].flag = 1;
			}
			else
			{
				key[i].cnt_long = 0;
			}
		}
		else if(key[i].flag == 1)
		{
			if(key[i].val == 0)
			{
				key[i].flag = 2;
			}
			else
			{
				key[i].flag = 0;
			}
		}
		else if(key[i].flag == 2)
		{
			if(key[i].val == 0)
			{
				if(i == 3 && lcd_mode == 0)
				{
					key[i].flag = 0;
					key[i].press_long = 1;
					key[i].cnt_long = 0;
				}
				else if(++key[i].cnt_long == 100)
				{
					key[i].flag = 0;
					key[i].press_long = 1;
				}
			}
			else
			{
				key[i].flag = 0;
				key[i].press_temp = 1;
			}
		}
	}
}
