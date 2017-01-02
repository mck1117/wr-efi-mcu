#ifndef __TIMERS_H
#define __TIMERS_H

void Init_Timers();
void Start_Timers();

volatile uint32_t system_timer;

void Events_1khz();

#endif
