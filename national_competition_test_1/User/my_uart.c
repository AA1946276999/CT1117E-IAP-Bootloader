#include "my_uart.h"
#include "usart.h"
#include <string.h>
#include "rtc.h"

uint8_t tx_buf[20];
uint8_t rx_buf[50];

int8_t h,m,s;
int8_t n,y,r;


extern uint8_t T;
extern uint8_t OD;
extern double OF;
extern uint8_t CH;

uint16_t uart_control_pwm_Time;
uint8_t uart_control_pwm_Pluse;
uint16_t uart_control_pwm_Frequency;
uint8_t uart_control_pwm_nmuber;
uint8_t uart_control_pwm_temp;

extern uint8_t cmp_flag;

static volatile uint8_t number_flag;
static volatile uint8_t symbol_flag;
static volatile uint8_t big_word_flag;
static volatile uint8_t error_flag;
static volatile uint8_t u;
uint8_t secret_nmuber;

extern uint8_t secret_real[10];

extern RTC_TimeTypeDef sTime;
extern RTC_DateTypeDef sDate;

void number_check(uint8_t data);
void symbol_check(uint8_t data);
void big_word_check(uint8_t data);
void error_check(uint8_t* data_buf,uint8_t size);

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart == &huart1)
	{
		//HAL_UART_Transmit_IT(&huart1,rx_buf,Size);
		//HAL_UART_Transmit_IT(&huart1,(uint8_t*)"S:202250450244",strlen("S:202250450244"));
		
		if(rx_buf[0] == '?' && Size == 1)
		{
			while(secret_real[secret_nmuber] != ' ')
			{
				secret_nmuber++;
			}
			
			HAL_UART_Transmit_IT(&huart1,secret_real,secret_nmuber);
		}
		else if(Size-4>=4 && Size-4<=10 && rx_buf[0] == 'S' && rx_buf[1] == 'E' && rx_buf[2] == 'T' && rx_buf[3] == ':')
		{
			number_flag = 0;
			symbol_flag = 0;
			big_word_flag = 0;
			error_flag = 0;
			
			for(u=4;u<Size;u++)
			{
				if(number_flag == 0)number_check(rx_buf[u]);
				if(symbol_flag == 0)symbol_check(rx_buf[u]);
				if(big_word_flag == 0)big_word_check(rx_buf[u]);
			}
			
			error_check(rx_buf,Size-4);
			
			if(error_flag == 0)
			{
				for(u=0;u<10;u++)
				{
					secret_real[u] = ' ';
				}
				
				for(u=4;u<Size;u++)
				{
					secret_real[u-4] = rx_buf[u];
				}
				
				HAL_UART_Transmit_IT(&huart1,(uint8_t*)"OK",strlen("OK"));
			}
			else
			{
				HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
			}
		}
		else if(Size == 11 && rx_buf[4] == ':')
		{
			if(rx_buf[0] == 'T' && rx_buf[1] == 'I' && rx_buf[2] == 'M' && rx_buf[3] == 'E')
			{
				h = (rx_buf[5] - '0')*10 + (rx_buf[6] - '0');
				m = (rx_buf[7] - '0')*10 + (rx_buf[8] - '0');
				s = (rx_buf[9] - '0')*10 + (rx_buf[10] - '0');
				
				if(h >= 0 && h<= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59)
				{
					sTime.Hours = h;
					sTime.Minutes = m;
					sTime.Seconds = s;
					HAL_RTC_SetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"OK",strlen("OK"));
				}
				else
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
			}
			else if(rx_buf[0] == 'D' && rx_buf[1] == 'A' && rx_buf[2] == 'T' && rx_buf[3] == 'E')
			{
				n = (rx_buf[5] - '0')*10 + (rx_buf[6] - '0');
				y = (rx_buf[7] - '0')*10 + (rx_buf[8] - '0');
				r = (rx_buf[9] - '0')*10 + (rx_buf[10] - '0');
				
				if(n >= 0 && n<= 99 && y >= 0 && y <= 12 && r >= 0 && r <= 31)
				{
					sDate.Year = n;
					sDate.Month = y;
					sDate.Date = r;
					
					HAL_RTC_SetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"OK",strlen("OK"));
				}
				else
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
			}
		}
		else if(rx_buf[0] == 'P' && rx_buf[1] == 'A' && rx_buf[3] == ':')
		{
			u = 4;
			while(rx_buf[u] != ':')
			{
				if(rx_buf[u] >= '0' && rx_buf[u] <= '9')
					u++;
				else
				{
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
					HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
					return;
				}
			}
			
			uart_control_pwm_temp = u;//第二个':'的位置
			u -= 4;
			
			if(u == 1)
			{
				uart_control_pwm_Frequency = (rx_buf[5]-'0');
			}
			else if(u == 2)
			{
				uart_control_pwm_Frequency = (rx_buf[5]-'0')*10 + (rx_buf[6]-'0');
			}
			else if(u == 3)
			{
				uart_control_pwm_Frequency = (rx_buf[5]-'0')*100 + (rx_buf[6]-'0')*10 + (rx_buf[7]-'0');
			}
			else if(u == 4)
			{
				uart_control_pwm_Frequency = (rx_buf[5]-'0')*1000 + (rx_buf[6]-'0')*100 + (rx_buf[7]-'0')*10 + (rx_buf[8]-'0');
			}
			else
			{
				HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
				HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
				return;
			}
			
			
			u += 5;
			
			while(rx_buf[u] != '%')//u为'%'的位置s
			{
				if(rx_buf[u] >= '0' && rx_buf[u] <= '9')
					u++;
				else
				{
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
					HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
					return;
				}
			}
			
			if(rx_buf[u+1] == ':')
			{
				u -= uart_control_pwm_temp;//减完的u就是占空比的位数
				u--;//此时的u包含占空比位数和'%' 减1后为真正的占空比位数
				if(u == 1)
				{
					uart_control_pwm_Pluse = (rx_buf[uart_control_pwm_temp+1]-'0');
				}
				else if(u == 2)
				{
					uart_control_pwm_Pluse = (rx_buf[uart_control_pwm_temp+1]-'0')*10 + (rx_buf[uart_control_pwm_temp+2]-'0');
				}
				else
				{
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
					HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
					return;
				}
				
				u++;//前面减了'%' 现在需要加回来
				u += uart_control_pwm_temp;//加回到'%'的位置
				u++;//加到最后一个':'的位置
				
				u = (Size-1) - u;//Size是从1开始 减1变成和数组Index一样的从0开始 再减u得到Time的位数
				
				if(u == 1)
				{
					if(rx_buf[Size-1] > '0' && rx_buf[Size-1] <= '9')
						uart_control_pwm_Time = rx_buf[Size-1] - '0';
				}
				else if(u == 2)
				{
					if(rx_buf[Size-1] >= '0' && rx_buf[Size-1] <= '9' && rx_buf[Size-2] > '0' && rx_buf[Size-2] <= '9')
						uart_control_pwm_Time = (rx_buf[Size-2] - '0')*10 + (rx_buf[Size-1] - '0');
				}
				else if(u == 3)
				{
					if(rx_buf[Size-1] >= '0' && rx_buf[Size-1] <= '9' && rx_buf[Size-2] >= '0' && rx_buf[Size-2] <= '9' && rx_buf[Size-3] > '0' && rx_buf[Size-3] <= '9')
						uart_control_pwm_Time = (rx_buf[Size-3] - '0')*100 + (rx_buf[Size-2] - '0')*10 + (rx_buf[Size-1] - '0');
				}
				else
				{
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
					HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
					return;
				}
				
				
				if(rx_buf[2] == '6' || rx_buf[2] == '7')
				{
					if(uart_control_pwm_Time >= 200)
					{
						HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
						HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
						return;
					}
					else
					{
						T = uart_control_pwm_Time;
						OF = uart_control_pwm_Frequency;
						OD = uart_control_pwm_Pluse;
						if(rx_buf[2] == '6')
						{
							CH = PWM_PA6;
							cmp_flag = 1;
							HAL_UART_Transmit_IT(&huart1,(uint8_t*)"OK",strlen("OK"));
						}
						else if(rx_buf[2] == '7')
						{
							CH = PWM_PA7;
							cmp_flag = 1;
							HAL_UART_Transmit_IT(&huart1,(uint8_t*)"OK",strlen("OK"));
						}
						if(uart_control_pwm_Time == 0 || uart_control_pwm_Pluse == 0)
						{
							cmp_flag = 0;
						}
					}
				}
				else
				{
					HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
					HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
					return;
				}
			}
			else
			{
				HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
				HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
				return;
			}
			
			
		}
		else
			HAL_UART_Transmit_IT(&huart1,(uint8_t*)"ERROR",strlen("ERROR"));
		
		
		HAL_UARTEx_ReceiveToIdle_IT(&huart1,rx_buf,50);
	}
}

void number_check(uint8_t data)
{
	if(data == '0' || data == '1' || data == '2' || data == '3' || data == '4' || data == '5' || data == '6' || data == '7' || data == '8' || data == '9')
	{
		number_flag = 1;
	}
}

void symbol_check(uint8_t data)
{
	if(data == '@' || data == '#' || data == '$' || data == '%' || data == '&' || data == '=' || data == '+')
	{
		symbol_flag = 1;
	}
}

void big_word_check(uint8_t data)
{
	if(data == 'A' || data == 'B' || data == 'C' || data == 'D' || data == 'E' || data == 'F' || data == 'X' || data == 'Y' || data == 'Z')
	{
		big_word_flag = 1;
	}
}

void error_check(uint8_t* data_buf,uint8_t size)
{
	while(size--)
	{
		if(
		data_buf[4+size] == '0' || data_buf[4+size] == '1' || 
		data_buf[4+size] == '2' || data_buf[4+size] == '4' || 
		data_buf[4+size] == '4' || data_buf[4+size] == '5' || 
		data_buf[4+size] == '6' || data_buf[4+size] == '7' || 
		data_buf[4+size] == '8' || data_buf[4+size] == '@' ||
		data_buf[4+size] == '#' || data_buf[4+size] == '$' ||
		data_buf[4+size] == '%' || data_buf[4+size] == '&' ||
		data_buf[4+size] == '=' || data_buf[4+size] == '+' ||
		data_buf[4+size] == '#' || data_buf[4+size] == '$' ||
		data_buf[4+size] == 'A' || data_buf[4+size] == 'B' ||
		data_buf[4+size] == 'C' || data_buf[4+size] == 'D' ||
		data_buf[4+size] == 'E' || data_buf[4+size] == 'F' ||
		data_buf[4+size] == 'X' || data_buf[4+size] == 'Y' || 
		data_buf[4+size] == 'Z' || data_buf[4+size] == '9' )
		{
			error_flag = 0;
		}
		else
		{
			error_flag = 1;
		}
	}
	
	if(error_flag == 0 && (number_flag != 1 || symbol_flag != 1 || big_word_flag != 1))
	{
		error_flag = 1;
	}
}
