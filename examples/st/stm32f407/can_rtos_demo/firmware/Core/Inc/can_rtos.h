/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    can_rtos.h
 * @brief   CAN-based cooperative RTOS demo for STM32F407.
 *
 * Provides:
 *  - CAN_Frame_t   : standard 11-bit CAN frame descriptor
 *  - CAN_Queue_t   : lock-free SPSC circular queue for CAN frames
 *  - Task_t        : cooperative task descriptor (period, last-run tick)
 *  - MiniRTOS API  : task registration and round-robin cooperative scheduler
 *  - CAN LL API    : low-level CAN1 driver using direct register access
 *                    (stm32f407xx.h CAN_TypeDef – no HAL_CAN module needed)
 *
 * Emulica / Renoly compatibility
 * --------------------------------
 * When (*(uint32_t*)0x10000000 == 0xCAFEBABE) the firmware is running under
 * the Emulica / Renoly simulator.  In that mode:
 *  - CAN hardware initialisation (clock gate, GPIO, BTR, filters) is skipped;
 *    the CAN1 register block is mapped as MappedMemory in platform.repl so
 *    register reads/writes succeed without stalling.
 *  - TX frames are written to the TX mailbox registers so sim.resc watchpoints
 *    can observe every transmitted frame.
 *  - One synthetic RX frame (ID = 0x456, payload "SIM\x01") is pre-loaded
 *    into the software RX queue at init time so the receive path is exercised
 *    deterministically in automated tests.
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef CAN_RTOS_H
#define CAN_RTOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Emulica test-mode sentinel  (matches convention in existing demo)
 * ---------------------------------------------------------------------- */
#ifndef EMULICA_TEST_MAGIC_ADDR
#define EMULICA_TEST_MAGIC_ADDR   0x10000000UL
#define EMULICA_TEST_MAGIC_VAL    0xCAFEBABEUL
#define EMULICA_IS_TESTING \
    (*(volatile uint32_t *)EMULICA_TEST_MAGIC_ADDR == EMULICA_TEST_MAGIC_VAL)
#endif

/* -------------------------------------------------------------------------
 * Mini-RTOS tuning
 * ---------------------------------------------------------------------- */
/** Maximum number of tasks that can be registered with MiniRTOS_RegisterTask. */
#define MINI_RTOS_MAX_TASKS     6

/** Scheduler tick period in ms.  Each pass through MiniRTOS_Run() sleeps
 *  this long so the CPU is not busy-waiting between task deadlines. */
#define MINI_RTOS_TICK_MS       5

/* -------------------------------------------------------------------------
 * CAN configuration
 * ---------------------------------------------------------------------- */
/** Depth of both the TX and the RX software frame queues. */
#define CAN_QUEUE_DEPTH         8

/** CAN_Tx_Task fires every CAN_TX_PERIOD_MS milliseconds. */
#define CAN_TX_PERIOD_MS        100

/** CAN_Rx_Task polls the hardware FIFO every CAN_RX_PERIOD_MS milliseconds. */
#define CAN_RX_PERIOD_MS        10

/** Standard 11-bit CAN ID used for periodic heartbeat TX frames. */
#define CAN_TX_STD_ID           0x123U

/** Standard 11-bit CAN ID of the synthetic RX frame injected in test mode. */
#define CAN_TEST_RX_STD_ID      0x456U

/* -------------------------------------------------------------------------
 * Data types
 * ---------------------------------------------------------------------- */

/**
 * @brief  Standard-frame CAN message (11-bit identifier, up to 8 data bytes).
 */
typedef struct {
    uint32_t id;       /**< 11-bit standard CAN identifier (bits 10:0 used). */
    uint8_t  data[8];  /**< Payload bytes.                                    */
    uint8_t  len;      /**< DLC – number of valid bytes in data[] (0-8).      */
} CAN_Frame_t;

/**
 * @brief  Single-producer / single-consumer lock-free circular queue.
 *
 * Both producer (CAN_Queue_Push) and consumer (CAN_Queue_Pop) are called
 * from task context only (no interrupt sharing), so no critical-section
 * guards are needed beyond the volatile qualifiers on head/tail/count.
 */
typedef struct {
    CAN_Frame_t      buf[CAN_QUEUE_DEPTH]; /**< Storage ring.                */
    volatile uint8_t head;  /**< Consumer read index.                         */
    volatile uint8_t tail;  /**< Producer write index.                        */
    volatile uint8_t count; /**< Number of occupied slots.                    */
    uint8_t          cap;   /**< Total capacity (= CAN_QUEUE_DEPTH).          */
} CAN_Queue_t;

/**
 * @brief  Cooperative task entry-point signature.
 *         Tasks must execute quickly and never block indefinitely.
 */
typedef void (*TaskFunc_t)(void);

/**
 * @brief  Cooperative task descriptor used internally by the scheduler.
 */
typedef struct {
    TaskFunc_t   fn;          /**< Task body.                                 */
    uint32_t     period_ms;   /**< Minimum ms between invocations; 0=every tick. */
    uint32_t     last_tick;   /**< HAL_GetTick() value at last dispatch.      */
    const char  *name;        /**< Human-readable label (aids Renoly scripts).*/
} Task_t;

/* -------------------------------------------------------------------------
 * Public API – Mini-RTOS scheduler
 * ---------------------------------------------------------------------- */

/**
 * @brief  Initialise the scheduler.  Call once before RegisterTask().
 */
void MiniRTOS_Init(void);

/**
 * @brief  Register a cooperative task with the scheduler.
 *
 * @param  fn         Task entry point.  Must return promptly; no blocking.
 * @param  period_ms  Minimum period between calls (ms).  0 = every tick.
 * @param  name       Short ASCII label (used in Renoly watchpoint messages).
 */
void MiniRTOS_RegisterTask(TaskFunc_t fn, uint32_t period_ms, const char *name);

/**
 * @brief  Enter the cooperative scheduling loop.  This function never returns.
 *
 * On each iteration:
 *  1. HAL_GetTick() is sampled.
 *  2. Every registered task whose deadline has elapsed is called in order.
 *  3. HAL_Delay(MINI_RTOS_TICK_MS) yields the CPU before the next pass.
 */
void MiniRTOS_Run(void);

/* -------------------------------------------------------------------------
 * Public API – CAN driver
 * ---------------------------------------------------------------------- */

/**
 * @brief  Initialise CAN1 GPIO (PD0=RX / PD1=TX, AF9), clock gate,
 *         baud rate (500 kbps @ APB1=42 MHz), and acceptance filter.
 *
 * In test mode (EMULICA_IS_TESTING) hardware initialisation is skipped
 * and a synthetic RX frame is pushed to g_can_rx_queue.
 */
void CAN_Demo_Init(void);

/**
 * @brief  CAN TX task – transmit one heartbeat frame per period.
 *         Register with period CAN_TX_PERIOD_MS.
 */
void CAN_Tx_Task(void);

/**
 * @brief  CAN RX task – poll hardware FIFO0, drain software RX queue.
 *         Register with period CAN_RX_PERIOD_MS.
 */
void CAN_Rx_Task(void);

/* -------------------------------------------------------------------------
 * Public API – queue helpers
 * ---------------------------------------------------------------------- */

/**
 * @brief  Push a frame to the tail of the queue.
 * @retval  0   Success.
 * @retval -1   Queue full – frame dropped.
 */
int CAN_Queue_Push(CAN_Queue_t *q, const CAN_Frame_t *frame);

/**
 * @brief  Pop a frame from the head of the queue.
 * @retval  0   Success – *frame filled.
 * @retval -1   Queue empty.
 */
int CAN_Queue_Pop(CAN_Queue_t *q, CAN_Frame_t *frame);

/* -------------------------------------------------------------------------
 * Shared state (defined in can_rtos.c, extern here)
 * ---------------------------------------------------------------------- */
extern CAN_Queue_t        g_can_tx_queue;  /**< Software TX queue.           */
extern CAN_Queue_t        g_can_rx_queue;  /**< Software RX queue.           */
extern volatile uint32_t  g_can_tx_count;  /**< Frames successfully sent.    */
extern volatile uint32_t  g_can_rx_count;  /**< Frames received and processed.*/
extern volatile uint32_t  g_can_err_count; /**< Transmit error events.       */

#ifdef __cplusplus
}
#endif
#endif /* CAN_RTOS_H */
