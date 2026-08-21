#include "tim_interruct.h"
#include "tim.h"
#include "key.h"
#include "my_adc.h"
#include "lcd.h"
#include "led.h"

//lcd_mode	1
extern uint8_t call_signal;
uint8_t call_signal_led_100ms;
uint8_t call_signal_flag;

//lcd_mode	2
extern uint8_t cmp_flag;
uint16_t PWM_T_cnt_1000ms;
extern uint8_t T;

extern uint8_t lcd_mode_2_lock_flag;
extern uint8_t RETRY;
extern uint8_t WAIT;
uint16_t lcd_mode_2_lock_10000ms;

extern uint8_t lcd_mode_2_lock_3000ms_flag;
uint16_t lcd_mode_2_lock_3000ms;

extern uint8_t lcd_fuc_slow,lcd_fuc_flag;

extern uint8_t lcd_mode;
extern uint8_t lcd_line_4_clear_flag;

void tim_interrupt_init(void)
{
	HAL_TIM_Base_Start_IT(&htim7);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim7)
	{
		if(cmp_flag == 1)
		{
			led_write_pin(0,1);
			if(++PWM_T_cnt_1000ms == 1000)
			{
				PWM_T_cnt_1000ms = 0;
				T--;
				if(T == 0)
				{
					lcd_mode = 0;
					led_write_pin(0,0);
					cmp_flag = 0;
				}
			}
		}
		else
		{
			HAL_TIM_PWM_Stop(&htim16,TIM_CHANNEL_1);
			HAL_TIM_PWM_Stop(&htim17,TIM_CHANNEL_1);
		}
		
		if(call_signal == 1)
		{
			if(++call_signal_led_100ms == 100)
			{
				call_signal_led_100ms = 0;
				if(call_signal_flag == 0)call_signal_flag = 1;
				else if(call_signal_flag == 1)call_signal_flag = 0;
				led_write_pin(2,call_signal_flag);
			}
		}
		else
		{
			led_write_pin(2,0);
		}
		
		if(lcd_mode_2_lock_3000ms_flag == 1 && lcd_mode_2_lock_flag != 1)
		{
			led_write_pin(1,1);
			if(++lcd_mode_2_lock_3000ms == 3000)
			{
				lcd_mode_2_lock_3000ms_flag = 0;
				lcd_mode_2_lock_3000ms = 0;
				lcd_mode = 0;
				lcd_line_4_clear_flag = 1;
				led_write_pin(1,0);
			}
		}
		
		if(lcd_mode_2_lock_flag == 1)
		{
			led_write_pin(1,1);
			if(++lcd_mode_2_lock_10000ms == 10000)
			{
				lcd_mode_2_lock_10000ms = 0;
				lcd_mode_2_lock_flag = 0;
				RETRY = 3;
				WAIT = 0;
				lcd_mode = 0;
				
				lcd_mode_2_lock_3000ms_flag = 0;
				
				lcd_line_4_clear_flag = 1;
				led_write_pin(1,0);
			}
			else if((lcd_mode_2_lock_10000ms % 1000) == 0)
			{
				WAIT--;
			}
		}
		
		if(++lcd_fuc_slow == 100)
		{
			lcd_fuc_slow = 0;
			lcd_fuc_flag = 1;
		}
		
		if(++adc_fuc_slow == 100)
		{
			adc_fuc_slow = 0;
			adc_fuc_flag = 1;
		}
		
		if(++key_fuc_slow == 10)
		{
			key_fuc_slow = 0;
			key_fuc_flag = 1;
		}
		
		if(++key_state_slow == 10)
		{
			key_state_slow = 0;
			key_state_machine();
		}
	}
}
