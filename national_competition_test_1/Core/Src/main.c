/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "key.h"
#include "led.h"
#include "lcd.h"
#include "tim_interruct.h"
#include <stdio.h>
#include "my_adc.h"
#include "tim_ic.h"
#include "my_uart.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

//lcd_mode		3
#define recn_ST		0
#define recn_FT		1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern uint8_t tx_buf[20];

//lcd_mode	0
uint8_t digit_255;
uint8_t secret_show_temp,secret_show_temp_old;
uint8_t secret_real[10] = {'A','3','5','#',' ',' ',' ',' ',' ',' '};
uint8_t secret_buf[11] = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
uint8_t secret_index;
uint8_t cmp_flag;
uint8_t call_signal;

//lcd_mode	1
uint8_t lcd_mode_1_para_select;

uint8_t F_show = 10;

uint8_t T_show = 5;

uint8_t CH_show = PWM_PA6;

uint8_t D_show = 20;

//lcd_mode	2
uint8_t F = 10;
uint8_t CH = PWM_PA6;
uint8_t T = 5;
uint8_t OD;
double OF;

double R38_convert_to_F;
double CF;

uint8_t secret_verify_flag;
uint8_t RETRY = 4;
uint8_t WAIT;
uint8_t lcd_mode_2_lock_flag;

uint8_t lcd_mode_2_lock_3000ms_flag;

//
uint8_t lcd_mode;
uint8_t lcd_fuc_slow,lcd_fuc_flag;
uint8_t lcd_buf[20];

uint8_t lcd_line_4_clear_flag;

//lcd_mode	3
uint8_t recn_h[2],recn_m[2],recn_s[2];
uint16_t recn_OF[2];
uint8_t ST_flag,FT_flag;
uint8_t SN,FN;

RTC_TimeTypeDef sTime = {0};
RTC_DateTypeDef sDate = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void key_fuc(void);
void adc_fuc(void);
void lcd_fuc(void);
void read_sercret_temp(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM7_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_TIM16_Init();
  MX_TIM17_Init();
  /* USER CODE BEGIN 2 */
  led_all_off();
LCD_Init();
HAL_Delay(50);
LCD_SetTextColor(White);
LCD_SetBackColor(Black);
LCD_Clear(Black);
HAL_Delay(50);

tim_interrupt_init();
R37_init();
R38_init();

tim_ic_init();

HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);

read_sercret_temp();
secret_show_temp_old = secret_show_temp;

sprintf((char*)tx_buf,"S:20250244");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	  key_fuc();
//	  adc_fuc();
	  lcd_fuc();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV3;
  RCC_OscInitStruct.PLL.PLLN = 20;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void lcd_fuc(void)
{
	if(lcd_fuc_flag)
	{
		lcd_fuc_flag = 0;
		
		R38_val = HAL_ADC_GetValue(&hadc1);//4015
		
		if(lcd_line_4_clear_flag == 1)
		{
			lcd_line_4_clear_flag = 0;
			LCD_Clear(Black);
		}
		
		
//		sprintf((char*)lcd_buf,"  R38:%d   ",R38_val);
//		LCD_DisplayStringLine(Line7,lcd_buf);
		
		if(cmp_flag == 1)
		{
			if(CH == PWM_PA6)
			{
				HAL_TIM_PWM_Stop(&htim17,TIM_CHANNEL_1);
				HAL_TIM_PWM_Start(&htim16,TIM_CHANNEL_1);
				__HAL_TIM_SET_COMPARE(&htim16,TIM_CHANNEL_1,OD);
				__HAL_TIM_SET_PRESCALER(&htim16,(80000000.0/(100.0*OF))-1);
			}
			else if(CH == PWM_PA7)
			{
				HAL_TIM_PWM_Stop(&htim16,TIM_CHANNEL_1);
				HAL_TIM_PWM_Start(&htim17,TIM_CHANNEL_1);
				__HAL_TIM_SET_COMPARE(&htim17,TIM_CHANNEL_1,OD);
				__HAL_TIM_SET_PRESCALER(&htim17,(80000000.0/(100.0*OF))-1);
			}
		}
		else
		{
			HAL_TIM_PWM_Stop(&htim16,TIM_CHANNEL_1);
			HAL_TIM_PWM_Stop(&htim17,TIM_CHANNEL_1);
		}
		
		
		if(lcd_mode == 0)
		{
			sprintf((char*)lcd_buf,"       IDLE   ");
			LCD_DisplayStringLine(Line1,lcd_buf);
			
			read_sercret_temp();
			
			if(secret_show_temp != secret_show_temp_old)
			{
				secret_buf[secret_index] = secret_show_temp;
			}
			
			sprintf((char*)lcd_buf,"     UN:%s ",secret_buf);
			LCD_DisplayStringLine(Line3,lcd_buf);
			
			secret_show_temp_old = secret_show_temp;
			
			HAL_RTC_GetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
		
			sprintf((char*)lcd_buf,"     %.2d-%.2d-%.2d   ",sDate.Year,sDate.Month,sDate.Date);
			LCD_DisplayStringLine(Line5,lcd_buf);
		
			sprintf((char*)lcd_buf,"     %.2d:%.2d:%.2d   ",sTime.Hours,sTime.Minutes,sTime.Seconds);
			LCD_DisplayStringLine(Line6,lcd_buf);
		}
		else if(lcd_mode == 1)
		{
			sprintf((char*)lcd_buf,"       PARA   ");
			LCD_DisplayStringLine(Line1,lcd_buf);
			
			
			sprintf((char*)lcd_buf,"     F:%dKHz   ",F_show);
			LCD_DisplayStringLine(Line3,lcd_buf);
			
			sprintf((char*)lcd_buf,"     D:%d%%   ",D_show);
			LCD_DisplayStringLine(Line4,lcd_buf);
			
			sprintf((char*)lcd_buf,"     T:%d   ",T_show);
			LCD_DisplayStringLine(Line5,lcd_buf);
			
			if(CH_show == PWM_PA6)
			{
				sprintf((char*)lcd_buf,"     CH:PA6   ");
				LCD_DisplayStringLine(Line6,lcd_buf);
			}
			else if(CH_show == PWM_PA7)
			{
				sprintf((char*)lcd_buf,"     CH:PA7   ");
				LCD_DisplayStringLine(Line6,lcd_buf);
			}
			
		}
		else if(lcd_mode == 2)
		{
			if(secret_verify_flag == 1)
			{
				sprintf((char*)lcd_buf,"       OUTP   ");
				LCD_DisplayStringLine(Line1,lcd_buf);
				
				sprintf((char*)lcd_buf,"     T:%.2d   ",T);
				LCD_DisplayStringLine(Line3,lcd_buf);
				
				if(R38_val > 4015)
					R38_val = 4015;
				
				R38_convert_to_F = ((double)R38_val/4015.0*3.3)*(1000.0/3.3)-500.0;
				
				OF = R38_convert_to_F/1000.0 + F;
				
				sprintf((char*)lcd_buf,"     OF:%.1fKHz   ",OF);
				LCD_DisplayStringLine(Line4,lcd_buf);
				
				sprintf((char*)lcd_buf,"     OD:%d%%   ",OD);
				LCD_DisplayStringLine(Line5,lcd_buf);
				
				CF = R39_val;
				
				sprintf((char*)lcd_buf,"     CF:%.1fKHz   ",CF/1000.0);
				LCD_DisplayStringLine(Line6,lcd_buf);
				
			}
			else if(secret_verify_flag == 0)
			{
				if(RETRY != 0)
				{
					sprintf((char*)lcd_buf,"       OUTP   ");
					LCD_DisplayStringLine(Line1,lcd_buf);
					
					sprintf((char*)lcd_buf,"       FAIL   ");
					LCD_DisplayStringLine(Line3,lcd_buf);
					
					sprintf((char*)lcd_buf,"      RETRY:%d   ",RETRY);
					LCD_DisplayStringLine(Line4,lcd_buf);
					
					LCD_ClearLine(Line5);
				}
				else if(RETRY == 0)
				{
					sprintf((char*)lcd_buf,"       OUTP   ");
					LCD_DisplayStringLine(Line1,lcd_buf);
					
					sprintf((char*)lcd_buf,"       LOCK   ");
					LCD_DisplayStringLine(Line3,lcd_buf);
					
					sprintf((char*)lcd_buf,"      RETRY:%d   ",RETRY);
					LCD_DisplayStringLine(Line4,lcd_buf);
					
					sprintf((char*)lcd_buf,"      WAIT:%.2d   ",WAIT);
					LCD_DisplayStringLine(Line5,lcd_buf);
				}
				
			}
			
		}
		else if(lcd_mode == 3)
		{
			sprintf((char*)lcd_buf,"       RECD   ");
			LCD_DisplayStringLine(Line1,lcd_buf);
			
			if(SN == 0)
			{
				sprintf((char*)lcd_buf,"  SN=NA   ");
				LCD_DisplayStringLine(Line3,lcd_buf);
			}
			else
			{
				sprintf((char*)lcd_buf,"  SN=%d   ",SN);
				LCD_DisplayStringLine(Line3,lcd_buf);
			}
			
			if(ST_flag == 0)
			{
				sprintf((char*)lcd_buf,"  ST=NA   ");
				LCD_DisplayStringLine(Line4,lcd_buf);
			}
			else
			{
				sprintf((char*)lcd_buf,"  ST=%.1f@%.2d:%.2d:%.2d   ",(double)recn_OF[recn_ST]/1000.0,recn_h[recn_ST],recn_m[recn_ST],recn_s[recn_ST]);
				LCD_DisplayStringLine(Line4,lcd_buf);
			}
			
			
			if(FN == 0)
			{
				sprintf((char*)lcd_buf,"  FN=NA   ");
				LCD_DisplayStringLine(Line5,lcd_buf);
			}
			else
			{
				sprintf((char*)lcd_buf,"  FN=%d   ",FN);
				LCD_DisplayStringLine(Line5,lcd_buf);
			}
			
			if(FT_flag == 0)
			{
				sprintf((char*)lcd_buf,"  FT=NA   ");
				LCD_DisplayStringLine(Line6,lcd_buf);
			}
			else
			{
				sprintf((char*)lcd_buf,"  FT=%.1f@%.2d:%.2d:%.2d   ",(double)recn_OF[recn_FT]/1000.0,recn_h[recn_FT],recn_m[recn_FT],recn_s[recn_FT]);
				LCD_DisplayStringLine(Line6,lcd_buf);
			}
			
		}
	}
}

void key_fuc(void)
{
	if(key_fuc_flag)
	{
		key_fuc_flag = 0;
		
		if(key[0].press_temp == 1)
		{
			key[0].press_temp = 0;
			
			if(lcd_mode != 2)
				LCD_Clear(Black);
			
			if(lcd_mode == 0)
			{
				lcd_mode = 1;
				lcd_mode_1_para_select = 0;
			}
			else if(lcd_mode == 1)
				lcd_mode = 3;
			else if(lcd_mode == 3)
			{
				lcd_mode = 0;
				secret_buf[secret_index] = ' ';
			}

		}
		else if(key[1].press_temp == 1)
		{
			key[1].press_temp = 0;
			
			if(lcd_mode == 0)
			{
				if(secret_buf[secret_index] != ' ')
				{
					if(++secret_index == 10)
						secret_index = 9;
				}
				else
				{
					if(secret_index == 0)
					{
						secret_buf[0] = secret_show_temp;
						secret_show_temp_old = secret_show_temp;
						secret_index++;
					}
				}
			}
			else if(lcd_mode == 1)
			{
				if(++lcd_mode_1_para_select == 4)
					lcd_mode_1_para_select = 0;
			}
		}
		else if(key[2].press_temp == 1)
		{
			key[2].press_temp = 0;
			if(lcd_mode == 0)
			{
				uint8_t i;
				cmp_flag = 1;
				for(i=0;i<10;i++)//У������
				{
					if(secret_buf[i] != secret_real[i])
					{
						cmp_flag = 0;
						break;
					}
				}
				if(cmp_flag == 1)//������ȷ
				{
					T = T_show;
					OD = D_show;
					F = F_show;
					CH = CH_show;
					
					LCD_Clear(Black);
					lcd_mode = 2;
					secret_verify_flag = 1;
					
					HAL_RTC_GetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
					HAL_RTC_GetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
					
					SN++;
					ST_flag = 1;
					
					recn_h[recn_ST] = sTime.Hours;
					recn_m[recn_ST] = sTime.Minutes;
					recn_s[recn_ST] = sTime.Seconds;
					
					recn_OF[recn_ST] = R39_val;
					
					sprintf((char*)tx_buf,"S:%.2d%.2d%.2d%.2d%.2d%.2d",sDate.Year,sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);
					HAL_UART_Transmit_IT(&huart1,tx_buf,14);
				}
				else//�������
				{
					LCD_Clear(Black);
					lcd_mode = 2;
					secret_verify_flag = 0;
					
					lcd_mode_2_lock_3000ms_flag = 1;
					
					HAL_RTC_GetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
					HAL_RTC_GetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
					
					FN++;
					FT_flag = 1;
					
					recn_h[recn_FT] = sTime.Hours;
					recn_m[recn_FT] = sTime.Minutes;
					recn_s[recn_FT] = sTime.Seconds;
					
					recn_OF[recn_FT] = R39_val;
					
					sprintf((char*)tx_buf,"F:%.2d%.2d%.2d%.2d%.2d%.2d",sDate.Year,sDate.Month,sDate.Date,sTime.Hours,sTime.Minutes,sTime.Seconds);
					HAL_UART_Transmit_IT(&huart1,tx_buf,14);
					
					RETRY--;
					if(RETRY == 0)
					{
						WAIT = 10;
						lcd_mode_2_lock_flag = 1;
					}
				}
			}
			else if(lcd_mode == 2)
			{
				if(lcd_mode_2_lock_flag == 0 && lcd_mode_2_lock_3000ms_flag == 0)
				{
					if(RETRY != 0)
					{
						lcd_mode = 0;
						LCD_Clear(Black);
					}
				}
			}
			else if(lcd_mode == 1)
			{
				if(lcd_mode_1_para_select == 0)
				{
					F_show++;
					if(F_show > 100)
						F_show = 100;
				}
				else if(lcd_mode_1_para_select == 1)
				{
					D_show += 5;
					if(D_show >= 100)
						D_show = 100;
				}
				else if(lcd_mode_1_para_select == 2)
				{
					T_show++;
				}
				else if(lcd_mode_1_para_select == 3)
				{
					if(CH_show == PWM_PA6)
						CH_show = PWM_PA7;
					else
						CH_show = PWM_PA6;
				}
			}
		}
		else if(key[3].press_temp == 1)
		{
			key[3].press_temp = 0;
			if(lcd_mode == 1)
			{
				if(lcd_mode_1_para_select == 0)
				{
					F_show--;
					if(F_show >= 200)
						F_show = 0;
				}
				else if(lcd_mode_1_para_select == 1)
				{
					D_show -= 5;
					if(D_show >= 200)
						D_show = 0;
				}
				else if(lcd_mode_1_para_select == 2)
				{
					T_show--;
					if(T_show >= 200)
						T_show = 200;
				}
				else if(lcd_mode_1_para_select == 3)
				{
					if(CH_show == PWM_PA6)
						CH_show = PWM_PA7;
					else
						CH_show = PWM_PA6;
				}
			}
		}
		
		if(key[0].press_long == 1)
		{
			key[0].press_long = 0;
			
			sprintf((char*)lcd_buf,"       Hello   ");
			LCD_DisplayStringLine(Line1,lcd_buf);
		}
		else if(key[1].press_long == 1)
		{
			key[1].press_long = 0;
			
			if(lcd_mode == 0)
			{
				secret_index = 0;
				uint8_t i;
				for(i=0;i<10;i++)
				{
					secret_buf[i] = ' ';
				}
			}
		}
		
		if(key[3].press_long == 1 && lcd_mode == 0)
		{
			key[3].press_long = 0;
			call_signal = 1;
		}
		else if(key[3].press_long == 0)
		{
			if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0) == GPIO_PIN_SET)call_signal = 0;
		}
		
	}
}

void adc_fuc(void)
{
	if(adc_fuc_flag)
	{
		adc_fuc_flag = 0;
		
		R37_val = HAL_ADC_GetValue(&hadc2);
		R38_val = HAL_ADC_GetValue(&hadc1);
		
		sprintf((char*)lcd_buf,"  R37:%d   ",R37_val);
		LCD_DisplayStringLine(Line3,lcd_buf);
		
		sprintf((char*)lcd_buf,"  R38:%d   ",R38_val);
		LCD_DisplayStringLine(Line4,lcd_buf);
		
		sprintf((char*)lcd_buf,"  R39:%d   ",R39_val);
		LCD_DisplayStringLine(Line5,lcd_buf);
		
		sprintf((char*)lcd_buf,"  R40:%d   ",R40_val);
		LCD_DisplayStringLine(Line6,lcd_buf);
		
		
	}
}

void read_sercret_temp(void)
{
	R37_val = HAL_ADC_GetValue(&hadc2);
	if(R37_val >= 4020)
	R37_val = 4020;

	digit_255 = (double)R37_val/4020.0*255.0;

	if(digit_255 <= 9)
	secret_show_temp = '0';
	else if(digit_255 >= 9 && digit_255 <= 19)
	secret_show_temp = '1';
	else if(digit_255 >= 20 && digit_255 <= 29)
	secret_show_temp = '2';
	else if(digit_255 >= 30 && digit_255 <= 39)
	secret_show_temp = '3';
	else if(digit_255 >= 40 && digit_255 <= 49)
	secret_show_temp = '4';
	else if(digit_255 >= 50 && digit_255 <= 59)
	secret_show_temp = '5';
	else if(digit_255 >= 60 && digit_255 <= 69)
	secret_show_temp = '6';
	else if(digit_255 >= 70 && digit_255 <= 79)
	secret_show_temp = '7';
	else if(digit_255 >= 80 && digit_255 <= 89)
	secret_show_temp = '8';
	else if(digit_255 >= 90 && digit_255 <= 99)
	secret_show_temp = '9';
	else if(digit_255 >= 100 && digit_255 <= 109)
	secret_show_temp = 'A';
	else if(digit_255 >= 110 && digit_255 <= 119)
	secret_show_temp = 'B';
	else if(digit_255 >= 120 && digit_255 <= 129)
	secret_show_temp = 'C';
	else if(digit_255 >= 130 && digit_255 <= 139)
	secret_show_temp = 'D';
	else if(digit_255 >= 140 && digit_255 <= 149)
	secret_show_temp = 'E';
	else if(digit_255 >= 150 && digit_255 <= 159)
	secret_show_temp = 'F';
	else if(digit_255 >= 160 && digit_255 <= 169)
	secret_show_temp = 'X';
	else if(digit_255 >= 170 && digit_255 <= 179)
	secret_show_temp = 'Y';
	else if(digit_255 >= 180 && digit_255 <= 189)
	secret_show_temp = 'Z';
	else if(digit_255 >= 190 && digit_255 <= 199)
	secret_show_temp = '@';
	else if(digit_255 >= 200 && digit_255 <= 209)
	secret_show_temp = '#';
	else if(digit_255 >= 210 && digit_255 <= 219)
	secret_show_temp = '$';
	else if(digit_255 >= 220 && digit_255 <= 229)
	secret_show_temp = '%';
	else if(digit_255 >= 230 && digit_255 <= 239)
	secret_show_temp = '&';
	else if(digit_255 >= 240 && digit_255 <= 249)
	secret_show_temp = '=';
	else if(digit_255 >= 250 && digit_255 <= 255)
	secret_show_temp = '+';
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
