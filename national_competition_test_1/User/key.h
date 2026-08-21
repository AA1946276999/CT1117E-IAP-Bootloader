#ifndef __KEY_H
#define __KEY_H


#include "main.h"

typedef struct
{
	uint8_t flag;
	uint8_t val;
	uint8_t press_temp;
	uint8_t press_real;
	uint8_t cnt_multi;
	uint8_t press_long;
	uint8_t cnt_long;
}Key;

extern Key key[4];
extern uint8_t key_state_slow,key_fuc_slow,key_fuc_flag;

void key_state_machine(void);

#endif
