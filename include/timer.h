#ifndef	_TIMER_H
#define	_TIMER_H

#include "types.h"

void timer_init ( void );
void handle_timer_irq ( void );
extern u32 timer_clock( void );

#endif  /*_TIMER_H */
