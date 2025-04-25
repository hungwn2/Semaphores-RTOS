/*
 * timebase.c
 *
 *  Created on: Apr 8, 2025
 *      Author: User
 */
#include "timebase.h"
#include "stm32f4xx.h"
#define ONE_SEC_LOAD	16000000

#define CTRL_ENABLE (1U<<0)
#define CTRL_CLCKSRC (1U<<2)
#define CTRL_TICKINT (1U<<1)
volatile uint32_t g_curr_tick;
volatile uint32_t g_curr_tick_p;
volatile uint32_t tick_freq=1;
#define CTRL_COUNTFLAG (1U<<16)
#define MAX_DELAY 0xFFFFFFFFU

void delay(uint32_t delay){
	uint32_t tickstart=get_tick();
	uint32_t wait=delay;
	if (wait<MAX_DELAY){
		wait+=(uint32_t)(tick_freq);
	}
	while((get_tick()-tickstart)<wait){}
}
void tick_increment(void)
{
	g_curr_tick+=tick_freq;

}

uint32_t get_tick(void){
	__disable_irq();
	g_curr_tick_p=g_curr_tick;
	__enable_irq();
	return g_curr_tick_p;

}

void timebase_init(void){
	SysTick->LOAD=ONE_SEC_LOAD-1;
	SysTick->VAL=0;
	SysTick->CTRL = CTRL_CLCKSRC | CTRL_TICKINT | CTRL_ENABLE;

	__enable_irq();

}

void SysTick_Handler(void){
	tick_increment();
}

