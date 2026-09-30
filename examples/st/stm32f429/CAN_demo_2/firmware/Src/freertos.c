/**
  ******************************************************************************
  * @file    freertos.c
  * @brief   FreeRTOS task bodies for the CAN / SPI / I2C RTOS demo.
  *
  * Three tasks run concurrently:
  *   CANTask  – sends a CAN frame every 500 ms (loopback, reads it back)
  *   SPITask  – transmits 4 bytes over SPI every 300 ms
  *   I2CTask  – writes 4 bytes as I2C master every 400 ms (target addr 0x50)
  *   DefaultTask – heartbeat, increments a counter every 1 s
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* External peripheral handles are in main.h */

/* -------------------------------------------------------------------------
 * Default / heartbeat task
 * -------------------------------------------------------------------------*/
void StartDefaultTask(void *argument)
{
    volatile uint32_t heartbeat = 0;
    for (;;)
    {
        heartbeat++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* -------------------------------------------------------------------------
 * CAN Task – 333 kbps loopback
 *
 * Sends a 4-byte frame with a rolling counter every 500 ms.
 * In loopback mode the frame immediately appears in RX FIFO0 – we poll
 * for it with a short delay.
 * -------------------------------------------------------------------------*/
void StartCANTask(void *argument)
{
    CAN_FilterTypeDef  canFilter;
    CAN_TxHeaderTypeDef txHdr;
    CAN_RxHeaderTypeDef rxHdr;
    uint8_t txData[4];
    uint8_t rxData[4];
    uint32_t txMailbox;
    uint8_t counter = 0;

    /* Configure a pass-all filter on FIFO0 */
    canFilter.FilterBank           = 0;
    canFilter.FilterMode           = CAN_FILTERMODE_IDMASK;
    canFilter.FilterScale          = CAN_FILTERSCALE_32BIT;
    canFilter.FilterIdHigh         = 0x0000;
    canFilter.FilterIdLow          = 0x0000;
    canFilter.FilterMaskIdHigh     = 0x0000;
    canFilter.FilterMaskIdLow      = 0x0000;
    canFilter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    canFilter.FilterActivation     = ENABLE;
    canFilter.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(&hcan1, &canFilter);

    /* Start CAN peripheral */
    HAL_CAN_Start(&hcan1);

    /* Fixed frame header */
    txHdr.StdId              = 0x123;
    txHdr.ExtId              = 0;
    txHdr.RTR                = CAN_RTR_DATA;
    txHdr.IDE                = CAN_ID_STD;
    txHdr.DLC                = 4;
    txHdr.TransmitGlobalTime = DISABLE;

    for (;;)
    {
        /* Build payload: 'C','A','N', counter */
        txData[0] = 'C';
        txData[1] = 'A';
        txData[2] = 'N';
        txData[3] = counter++;

        /* Transmit */
        HAL_CAN_AddTxMessage(&hcan1, &txHdr, txData, &txMailbox);

        /* In loopback mode the frame is echoed to RX FIFO immediately */
        vTaskDelay(pdMS_TO_TICKS(50));

        if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0)
        {
            HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHdr, rxData);
            /* rxData[3] == counter – 1, confirming round-trip */
            (void)rxData;
        }

        vTaskDelay(pdMS_TO_TICKS(495));
    }
}

/* -------------------------------------------------------------------------
 * SPI Task – 1 MHz master-only transmission
 *
 * Transmits a 4-byte sequence every 300 ms.
 * (MISO is floating – we only care about MOSI/SCK register writes here.)
 * -------------------------------------------------------------------------*/
void StartSPITask(void *argument)
{
    uint8_t txBuf[4] = { 0x53, 0x50, 0x49, 0x00 };  /* 'S','P','I', counter */
    uint8_t counter  = 0;

    for (;;)
    {
        txBuf[3] = counter++;
        /* Transmit 4 bytes; errors (BSY timeout) are ignored in this demo */
        HAL_SPI_Transmit(&hspi1, txBuf, 4, 50);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

/* -------------------------------------------------------------------------
 * I2C Task – 100 kHz master write
 *
 * Writes a 4-byte payload to device address 0x50 every 400 ms.
 * Without a real slave the transfer will time out or NACK – that's fine;
 * the register writes (START, addr, data) are still observable.
 * -------------------------------------------------------------------------*/
void StartI2CTask(void *argument)
{
    uint8_t txBuf[4] = { 0x49, 0x32, 0x43, 0x00 };  /* 'I','2','C', counter */
    uint8_t counter  = 0;

    for (;;)
    {
        txBuf[3] = counter++;
        /* 7-bit address 0x50, write, 4 bytes, 50 ms timeout (may NACK) */
        HAL_I2C_Master_Transmit(&hi2c1, 0x50 << 1, txBuf, 4, 50);
        vTaskDelay(pdMS_TO_TICKS(400));
    }
}

/* -------------------------------------------------------------------------
 * FreeRTOS application hooks
 * -------------------------------------------------------------------------*/

/** Called from xPortSysTickHandler – keeps HAL tick advancing. */
void vApplicationTickHook(void)
{
    HAL_IncTick();
}

/** Idle hook – required when configUSE_IDLE_HOOK == 1 (it's 0 here, but
 *  providing a weak-compatible stub is harmless). */
void vApplicationIdleHook(void)
{
    /* Nothing to do */
}

/** Stack overflow detection hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    __disable_irq();
    while (1) {}
}

/** Malloc failed hook */
void vApplicationMallocFailedHook(void)
{
    __disable_irq();
    while (1) {}
}
