#ifndef __LED_H
#define __LED_H

#include "main.h"

extern uint8_t led_fuc_slow,led_fuc_flag;

void led_write_pin(uint8_t address,uint8_t enable);
void led_all_off(void);

#endif
