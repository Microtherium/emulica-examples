/*
 * FreeRTOS Kernel V10.3.1 - configuration for TI SimpleLink CC2652R1
 * (Cortex-M4F). Adapted from cc2640r2f/freertos_demo's own
 * FreeRTOSConfig.h (same real kernel version, same real 3-priority-bit
 * SimpleLink NVIC lineage - confirmed via TI's own coresdk_cc13xx_cc26xx
 * NUM_PRIORITY_BITS constant, identical to CC2640R2F's own) - genuinely
 * different from that file only in `configENABLE_FPU`'s own reasoning:
 * CC2640R2F (Cortex-M3) has no FPU to enable at all, while this chip's
 * real Cortex-M4F core does - deliberately left at 0 anyway (matches
 * every other real M4F demo in this repo, e.g. TM4C123/STM32L476), so
 * the ARM_CM4F port's ordinary (non-lazy-FPU) manual {r4-r11, r14}
 * PendSV save path is exercised identically to every other FreeRTOS
 * demo, not a new, untested FPU-context-switching path.
 *
 * Portion Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
  void xPortSysTickHandler(void);
#endif

#define configENABLE_FPU                         0
#define configENABLE_MPU                         0

#define configUSE_PREEMPTION                     1
#define configSUPPORT_STATIC_ALLOCATION          1
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_IDLE_HOOK                      1
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       ( SystemCoreClock )
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     ( 7 )
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                    ((size_t)8192)
#define configMAX_TASK_NAME_LEN                  ( 16 )
#define configUSE_16_BIT_TICKS                   0
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configCHECK_FOR_STACK_OVERFLOW           2
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_MALLOC_FAILED_HOOK             1
#define configUSE_APPLICATION_TASK_TAG           1
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  1
#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t

#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 )

#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskCleanUpResources        0
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_vTaskDelayUntil              0
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_xTaskGetSchedulerState       1

/* Cortex-M specific definitions. Real CC2652R1 NVIC implements 3
 * priority bits - confirmed via TI's own coresdk_cc13xx_cc26xx
 * hw_ints.h NUM_PRIORITY_BITS constant, identical to CC2640R2F's own
 * confirmed value. No CMSIS device header exists for this chip to pull
 * __NVIC_PRIO_BITS from (no vendor SDK applies here - see target.toml),
 * so hardcoded directly instead of relying on that indirection. */
#define configPRIO_BITS         3

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   7
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2

#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names - see every other FreeRTOS demo's own identical convention.
Compiles the kernel's real vPortSVCHandler/xPortPendSVHandler directly
under the CMSIS-standard names, so startup.c's own SVC_Handler/
PendSV_Handler (weak, aliased to Default_Handler) get correctly overridden
by the real, non-weak kernel functions at link time - startup.c must NOT
also define real bodies for these two. */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler

/* SysTick stays a real, separate startup.c function (not renamed) that
calls osSystickHandler() explicitly - same reasoning as every other
FreeRTOS demo in this repo. */
/* #define xPortSysTickHandler SysTick_Handler */

#endif /* FREERTOS_CONFIG_H */
