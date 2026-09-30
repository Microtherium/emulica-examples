/* Quadcopter flight-control task: single-loop PID attitude control over a
 * simulated airframe, mixed into 4 motor PWM channels (GPT0..GPT3,
 * Timer A, 16-bit PWM). See quadcopter.c. */
#ifndef QUADCOPTER_H
#define QUADCOPTER_H

#include <stdint.h>

/* Real CMSIS-RTOS v1 thread entry (signature must match osThreadDef). */
void StartQuadcopterTask(void const *argument);

/* Provided by main.c - mutex-guarded UART0 write, shared by every task
 * that prints (see main.c's own comment on why the mutex is required). */
void uart0_put_string_safe(const char *s);

#endif /* QUADCOPTER_H */
