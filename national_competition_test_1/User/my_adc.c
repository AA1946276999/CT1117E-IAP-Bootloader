#include "my_adc.h"
#include "adc.h"

uint16_t R37_val,R38_val;
uint8_t adc_fuc_slow,adc_fuc_flag;

void R37_init(void)
{
	HAL_ADC_Start(&hadc2);
	HAL_ADCEx_Calibration_Start(&hadc2,ADC_SINGLE_ENDED);
	R37_val = HAL_ADC_GetValue(&hadc2);
	HAL_ADC_PollForConversion(&hadc2,HAL_MAX_DELAY);
}

void R38_init(void)
{
	HAL_ADC_Start(&hadc1);
	HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED);
	R38_val = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,HAL_MAX_DELAY);
}
