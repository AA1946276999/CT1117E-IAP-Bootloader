#include "tim_ic.h"
#include "tim.h"

uint16_t R39_val,R40_val;

void tim_ic_init(void)
{
	HAL_TIM_IC_Start_IT(&htim2,TIM_CHANNEL_1);
	HAL_TIM_IC_Start_IT(&htim3,TIM_CHANNEL_1);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim3)
	{
		R39_val = 1000000.0/HAL_TIM_ReadCapturedValue(&htim3,TIM_CHANNEL_1);
		__HAL_TIM_SET_COUNTER(&htim3,0);
	}
	else if(htim == &htim2)
	{
		R40_val = 1000000.0/HAL_TIM_ReadCapturedValue(&htim2,TIM_CHANNEL_1);
		__HAL_TIM_SET_COUNTER(&htim2,0);
	}
}
