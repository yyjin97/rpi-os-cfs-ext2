#include "utils.h"
#include "sched.h"
#include "peripherals/timer.h"

const unsigned int interval = 800000;
//const unsigned int interval = 7500;
unsigned int curVal = 0;
unsigned int clock = 0;

void timer_init ( void )
{
	curVal = get32(TIMER_CLO);
	curVal += interval;
	put32(TIMER_C1, curVal);
}

void handle_timer_irq( void ) 
{
	curVal += interval;
	put32(TIMER_C1, curVal);
	put32(TIMER_CS, TIMER_CS_M1);
	clock += 4;
	timer_tick();
}

u32 timer_clock( void )
{
	return clock;
}
