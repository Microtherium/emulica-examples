/*
 * FreeRTOS Kernel V10.3.1 - configuration for TI MSP432P401R
 * (Cortex-M4F). Adapted from tm4c123/freertos_demo's own
 * FreeRTOSConfig.h (same real kernel version, same real ARM_CM4F port,
 * reused verbatim - see ../../../ti/tm4c123/freertos_demo/firmware/Inc/
 * FreeRTOSConfig.h) - genuinely different from that file only in
 * configPRIO_BITS's own source: this chip's real 3-implemented-priority-
 * bit NVIC is confirmed against Zephyr's own dts/arm/ti/msp432p4xx.dtsi
 * ("&nvic { arm,num-irq-priority-bits = <3>; }"), not TM4C123's own
 * public datasheet (no equivalent MSP432 fact was available from that
 * same kind of source, but Zephyr's dts is an independently real one -
 * see renoly/hw/ti/msp432/dio.py's own docstring for the fuller citation
 * trail this whole chip rests on).
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

/* Cortex-M specific definitions. Real MSP432P401R NVIC implements 3
 * priority bits (8 levels) - confirmed against Zephyr's own
 * dts/arm/ti/msp432p4xx.dtsi (see this file's own header comment). If
 * this is wrong, xPortStartScheduler()'s own real
 * configASSERT(configPRIO_BITS) self-check hangs the entire boot
 * immediately - the same real failure mode other chips' own FreeRTOS
 * work in this plan found and fixed, so a wrong value here would not go
 * unnoticed. */
#define configPRIO_BITS         3

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   7
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 2

#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names - see every other chip's own identical convention in this
repo. Compiles the kernel's real vPortSVCHandler/xPortPendSVHandler
directly under the CMSIS-standard names, so startup.c's own SVC_Handler/
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
