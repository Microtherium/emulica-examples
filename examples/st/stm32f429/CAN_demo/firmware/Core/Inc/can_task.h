/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can_task.h
  * @brief   CAN1 transmit and receive RTOS task declarations.
  *
  * Two FreeRTOS tasks manage CAN1 traffic on PB8 (RX, AF9) / PB9 (TX, AF9):
  *
  *   StartCANTxTask  – transmits one standard data frame every 500 ms
  *                     (StdID = 0x123, DLC = 8, first four bytes = 32-bit
  *                     transmit counter in little-endian).
  *
  *   StartCANRxTask  – blocks on a binary semaphore that CAN1_RX0_IRQHandler
  *                     posts when a frame arrives in FIFO0; then drains the
  *                     FIFO and stores the most recent frame.
  *
  * Bit-rate : 500 kbps  (APB1 = 36 MHz, Prescaler = 4, TS1 = 13, TS2 = 4)
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef CAN_TASK_H
#define CAN_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "cmsis_os.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  Decoded CAN data frame, populated by StartCANRxTask.
  */
typedef struct
{
    uint32_t  std_id;       /*!< Standard 11-bit CAN identifier           */
    uint8_t   is_remote;    /*!< 1 = Remote Frame, 0 = Data Frame          */
    uint8_t   dlc;          /*!< Data Length Code (0–8)                    */
    uint8_t   data[8];      /*!< Payload bytes (only [0..dlc-1] are valid) */
    uint32_t  timestamp;    /*!< FreeRTOS kernel tick at reception          */
} can_rx_frame_t;

/* Exported function prototypes ----------------------------------------------*/

/**
  * @brief  CAN transmit RTOS task entry point.
  *         Initialises CAN1 on first run, then transmits one frame every
  *         500 ms and toggles the green LED (LD3) on each successful send.
  * @param  argument  Unused; pass NULL via osThreadCreate.
  */
void StartCANTxTask(void const *argument);

/**
  * @brief  CAN receive RTOS task entry point.
  *         Blocks on a semaphore posted by CAN1_RX0_IRQHandler, then drains
  *         FIFO0, decodes every pending frame, and toggles the red LED (LD4).
  * @param  argument  Unused; pass NULL via osThreadCreate.
  */
void StartCANRxTask(void const *argument);

/**
  * @brief  Thread-safe snapshot of the most recently received CAN frame.
  * @param  out_frame  Pointer to caller-allocated can_rx_frame_t, or NULL.
  * @param  out_count  Pointer to receive the total RX frame count, or NULL.
  */
void CAN_GetLastRxFrame(can_rx_frame_t *out_frame, uint32_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* CAN_TASK_H */
