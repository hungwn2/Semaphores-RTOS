/*
 * osKernel.c
 *
 *  Created on: Apr 22, 2025
 *      Author: User
 */
#include "osKernel.h"
#define NUM_OF_THREADS	3
#define STACKSIZE 400
#define BUS_FREQ	16000000

#define CTRL_ENABLE (1U<<0)
#define CTRL_CLCKSRC (1U<<2)
#define CTRL_TICKINT (1U<<1)
#define CTRL_COUNTFLAG (1U<<16)
#define SYSTICK_RST 0
#define TIM2EN (1U<<0)
#define CR1_CEN (1U<<0)
#define DIER_UIE (1U<<0)

#define INTCTRL (*(volatile uint32_t *)0xE000ED04)
//icsr register
#define PENDSTET (1U<<26)
uint32_t period_tick;

void osSchedulerLaunch(void);
void osSchedulerRoundRobin(void);

uint32_t MILIS_PRESCALER;

//100 4 byte values
struct tcb{
	int32_t *stackPt;
	struct tcb *nextPt;
};

typedef struct tcb tcbType;
tcbType tcbs[NUM_OF_THREADS];
tcbType *currentPt;
int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];


void osKernelStackInit(int i)
{
	tcbs[i].stackPt=&TCB_STACK[i][STACKSIZE-16];
	/*Set BIT 21 in PSR to 1, to operate in thumb mode */
	TCB_STACK[i][STACKSIZE-1]=(1U<<24);
	TCB_STACK[i][STACKSIZE-3]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-4]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-5]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-6]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-7]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-8]=0xAAAAAAAA;
	//Stack frame content
	TCB_STACK[i][STACKSIZE-9]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-10]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-11]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-12]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-13]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-14]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-15]=0xAAAAAAAA;
	TCB_STACK[i][STACKSIZE-16]=0xAAAAAAAA;
//r11-r5
}

uint8_t osKernelAddThreads(void(*task0)(void), void(*task1)(void), void(*task2)(void)){
	__disable_irq();
	//disable global interupts
	tcbs[0].nextPt=&tcbs[1];
	tcbs[1].nextPt=&tcbs[2];
	tcbs[2].nextPt=&tcbs[0];
	//circular linkedlist
	//initialize stack for thread0
	osKernelStackInit(0);
	//Initialize PC
	TCB_STACK[0][STACKSIZE-2]=(uint32_t)(task0);
	osKernelStackInit(1);
	TCB_STACK[1][STACKSIZE-2]=(uint32_t)(task1);
	osKernelStackInit(2);
	TCB_STACK[2][STACKSIZE-2]=(uint32_t)(task2);
	currentPt=&tcbs[0];
	__enable_irq();
	return 1;

}


void osKernelInit(void){
	MILIS_PRESCALER=(BUS_FREQ/1000);
}


void osKernelLaunch(uint32_t quanta){
//reset systick, clear cvr, load quanta, set to low priority
//enable systick, select internal clock, thne enable interrupts
SysTick->CTRL=SYSTICK_RST;
SysTick->VAL=0;
SysTick->LOAD=(quanta*MILIS_PRESCALER)-1;
//# of clock cycle in 1ms
//kernel must have lowest priority
NVIC_SetPriority(SysTick_IRQn, 15);
SysTick->CTRL = CTRL_CLCKSRC | CTRL_ENABLE | CTRL_TICKINT;
//enable systick interrupt, and save/load content, launch scheduler
osSchedulerLaunch();
}


//when exception occurs, stack frame registers are automatically saved onto stack.
//stack should be unchanged during handler routine
__attribute__((naked)) void SysTick_Handler(void){
	//suspend the current thread, choose next thread
	__disable_irq();
	//only stack frame registers are saved, rest must be manuallly
	__asm("CPSID I");

	__asm("PUSH {R4-R11}");
	__asm("LDR R0, =currentPt");

	__asm("LDR R1, [R0]");
	//load r1
	__asm("STR SP, [R1]");
	//store sp at r1
	__asm("PUSH	{R0, LR}");
	__asm("BL osSchedulerRoundRobin");
	__asm("POP {R0, LR}");

	__asm("LDR	R1, [R0]");
	//sp=currentPt->StackPt
	__asm("LDR	SP, [R1]");

	//restore r4-11
	__asm("POP {R4-R11}");
	__asm("CPSIE   I");
	//return from exception, restore stack frame

	__asm("BX   LR");
	//branch to link address
}

void osSchedulerLaunch(){
	__asm("LDR R0, =currentPt");
	__asm("LDR R2, [R0]");
	//load r2 from address r0, r2=currentpt
	__asm("LDR SP, [R2]");
	//restore non-stack frame
	__asm("POP {R4-R11}");
	//restore r12
	__asm("POP {R12}");

	__asm("POP {R0-R3}");
	//skip lr
	__asm("ADD SP, SP, #4");
	//create new psr
	__asm("POP {LR}");
	//skip psr
	__asm("ADD SP, SP, #4");

	__asm("CPSIE    I");
	__asm("BX    LR");
	//return from subroutine
}

void osThreadYield(void){
	/* clear systick current val */

	SysTick->VAL=0;

	INTCTRL=PENDSTET;

}


void osSchedulerRoundRobin(void){
	if((++period_tick)==PERIOD){
		(*task3)();
		period_tick=0;
	}

	currentPt=currentPt->nextPt;
}


void tim2_1hz_interrupt_init(void){
	RCC->APB1ENR|=TIM2EN;
	TIM2->PSC=1600-1;
	TIM2->ARR=10000-1;
	TIM2->CNT=0;
	TIM2->CR1=CR1_CEN;
	TIM2->DIER|=DIER_UIE;
	NVIC_EnableIRQ(TIM2_IRQn);

}


void osSemaphoreInit(int32_t *semaphore, int32_t value){
	*semaphore=value;

}

void osSemaphoreSet(int32_t *semaphore){
	__disable_irq();
	*semaphore+=1;
	__enable_irq();
}
void osSemaphoreWait(int32_t * semaphore){
	__disable_irq();
	while(*semaphore<=0){
		__disable_irq();
		__enable_irq();
	}
	*semaphore-=1;
	__enable_irq();
}
