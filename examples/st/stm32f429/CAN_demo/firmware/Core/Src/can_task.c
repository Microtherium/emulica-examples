/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can_task.c
  * @brief   CAN1 transmit and receive RTOS tasks.
  *
  * Overview
  * --------
  * CAN1 is initialised directly via bxCAN registers (no separate HAL CAN
  * driver file required).  Two FreeRTOS tasks operate the bus concurrently:
  *
  *   StartCANTxTask  (Normal priority, 512-word stack)
  *       Runs after CAN1_Init().  Every 500 ms it loads one TX mailbox with a
  *       standard data frame (StdID = 0x123, DLC = 8) whose first four bytes
  *       carry a 32-bit transmit counter (little-endian).  The green LED (LD3)
  *       toggles on each successful mailbox enqueue.
  *
  *   StartCANRxTask  (AboveNormal priority, 512-word stack)
  *       Blocks indefinitely on s_can_rx_sem.  CAN1_RX0_IRQHandler posts that
  *       semaphore whenever FIFO0 contains at least one message.  The task
  *       drains every pending frame from the FIFO, decodes it into
  *       s_last_rx_frame, increments s_rx_total_count, and toggles the red
  *       LED (LD4).  CAN_GetLastRxFrame() provides a thread-safe snapshot.
  *
  * Hardware
  * --------
  *   MCU   : STM32F429ZIT6 (STM32F429I-DISC1)
  *   CAN   : CAN1, APB1 = 36 MHz
  *   Pins  : PB8 = CAN1_RX (GPIO AF9)
  *            PB9 = CAN1_TX (GPIO AF9)
  *           (These pins are also the LCD B6/B7 colour bits; the GPIO
  *           reconfiguration is harmless in the Emulica simulation.)
  *
  * Bit Timing – 500 kbps @ APB1 = 36 MHz
  * ----------------------------------------
  *   Prescaler (BRP)  =  4  → BRP register field = 3
  *   Time Segment 1   = 13 TQ → TS1 register field = 12
  *   Time Segment 2   =  4 TQ → TS2 register field =  3
  *   SJW              =  1 TQ → SJW register field =  0
  *   Bit time = (1 + 13 + 4) × (4 / 36 MHz) = 18 × 111.1 ns ≈ 2 µs
  *   Baud rate = 500 kbps
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "can_task.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cmsis_os.h"

/* Private defines -----------------------------------------------------------*/

/**
  * @brief bxCAN BTR register value for 500 kbps at APB1 = 36 MHz.
  *        Bit layout (RM0090 §32.7.7):
  *          [25:24] SJW  = 0  (SJW  = 1 TQ)
  *          [22:20] TS2  = 3  (TS2  = 4 TQ)
  *          [19:16] TS1  = 12 (TS1  = 13 TQ)
  *          [9:0]   BRP  = 3  (Prescaler = 4)
  */
#define CAN1_BTR_500KBPS  ((0U  << 24U) |   /* SJW field  = 0 (1 TQ)  */ \
                           (3U  << 20U) |   /* TS2 field  = 3 (4 TQ)  */ \
                           (12U << 16U) |   /* TS1 field  = 12 (13 TQ)*/ \
                           3U)              /* BRP field  = 3 (÷4)    */

/** Standard 11-bit CAN identifier transmitted by StartCANTxTask */
#define CAN_DEMO_TX_ID    0x123U

/**
  * @brief  Timeout guard when polling CAN_MSR.INAK during init/de-init.
  *         One iteration ≈ 1 CPU cycle at 72 MHz; this gives ~1 ms margin.
  */
#define CAN_INAK_TIMEOUT  (SystemCoreClock / 1000U)

/* Private variables ---------------------------------------------------------*/

/** Binary semaphore: CAN1_RX0_IRQHandler → StartCANRxTask notification. */
static SemaphoreHandle_t s_can_rx_sem = NULL;

/** Most recently decoded CAN frame (protected by taskENTER_CRITICAL). */
static volatile can_rx_frame_t s_last_rx_frame;

/** Running total of received frames (protected by taskENTER_CRITICAL). */
static volatile uint32_t s_rx_total_count = 0U;

/* Private function prototypes -----------------------------------------------*/
static void CAN1_Init(void);

/* ---------------------------------------------------------------------------
 * CAN1_RX0_IRQHandler  –  FIFO0 message-pending ISR
 * ---------------------------------------------------------------------------
 * Fired by hardware when CAN1->RF0R.FMP0 != 0.  The handler gives the
 * binary semaphore so that StartCANRxTask can wake up and drain FIFO0 from
 * task context (keeping the ISR as short as possible).
 *
 * NVIC preemption priority = 6, which is numerically >=
 * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5), so calling
 * xSemaphoreGiveFromISR() is safe.
 * --------------------------------------------------------------------------*/
void CAN1_RX0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (s_can_rx_sem != NULL)
    {
        xSemaphoreGiveFromISR(s_can_rx_sem, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* ---------------------------------------------------------------------------
 * CAN1_Init  (internal helper)
 * ---------------------------------------------------------------------------
 * Brings up CAN1 from reset state:
 *   1. Enable APB1 clock for CAN1.
 *   2. Configure PB8 / PB9 as alternate function 9 (CAN1_RX / CAN1_TX).
 *   3. Put bxCAN into initialization mode (INRQ=1, wait for INAK=1).
 *   4. Program BTR for 500 kbps; enable automatic bus-off recovery (ABOM).
 *   5. Configure filter bank 0: 32-bit mask mode, accept-all filter, FIFO0.
 *   6. Leave initialization mode (INRQ=0, wait for INAK=0).
 *   7. Enable FIFO0 message-pending interrupt (IER.FMPIE0).
 *   8. Enable CAN1_RX0_IRQn in NVIC at priority 6.
 * --------------------------------------------------------------------------*/
static void CAN1_Init(void)
{
    uint32_t timeout;

    /* ------------------------------------------------------------------ */
    /* 1. Enable CAN1 peripheral clock on APB1                            */
    /* ------------------------------------------------------------------ */
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    __DSB();   /* ensure the clock is visible before the first register access */

    /* ------------------------------------------------------------------ */
    /* 2. GPIO: PB8 = CAN1_RX (AF9),  PB9 = CAN1_TX (AF9)                */
    /*    GPIOB clock is already enabled by MX_GPIO_Init().               */
    /* ------------------------------------------------------------------ */
    {
        GPIO_InitTypeDef gpio_init = {0};
        gpio_init.Pin       = B6_Pin | B7_Pin;   /* PB8 (B6_Pin) | PB9 (B7_Pin) */
        gpio_init.Mode      = GPIO_MODE_AF_PP;
        gpio_init.Pull      = GPIO_NOPULL;
        gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF9_CAN1;
        HAL_GPIO_Init(GPIOB, &gpio_init);
    }

    /* ------------------------------------------------------------------ */
    /* 3. Enter bxCAN initialization mode                                 */
    /* ------------------------------------------------------------------ */
    CAN1->MCR &= ~CAN_MCR_SLEEP;   /* exit sleep mode first */
    CAN1->MCR |=  CAN_MCR_INRQ;    /* request initialization */

    timeout = CAN_INAK_TIMEOUT;
    while (!(CAN1->MSR & CAN_MSR_INAK) && (timeout-- > 0U)) {}

    /* ------------------------------------------------------------------ */
    /* 4. Bit timing and operating options                                 */
    /* ------------------------------------------------------------------ */
    CAN1->MCR |= CAN_MCR_ABOM;         /* automatic bus-off recovery          */
    CAN1->BTR  = CAN1_BTR_500KBPS;     /* 500 kbps, normal mode (SILM=LBKM=0) */

    /* ------------------------------------------------------------------ */
    /* 5. Filter bank 0: accept-all, 32-bit mask mode, assigned to FIFO0  */
    /* ------------------------------------------------------------------ */
    CAN1->FMR  |=  CAN_FMR_FINIT;          /* enter filter init mode           */

    CAN1->FA1R  &= ~(1UL << 0U);           /* deactivate bank 0 while editing  */
    CAN1->FS1R  |=  (1UL << 0U);           /* 32-bit scale                     */
    CAN1->FM1R  &= ~(1UL << 0U);           /* identifier mask mode             */
    CAN1->FFA1R &= ~(1UL << 0U);           /* assign to FIFO0                  */

    CAN1->sFilterRegister[0].FR1 = 0x00000000U;  /* filter ID   = 0            */
    CAN1->sFilterRegister[0].FR2 = 0x00000000U;  /* filter mask = 0 (accept all)*/

    CAN1->FA1R  |=  (1UL << 0U);           /* activate bank 0                  */
    CAN1->FMR   &= ~CAN_FMR_FINIT;         /* leave filter init mode           */

    /* ------------------------------------------------------------------ */
    /* 6. Leave initialization mode → Normal mode                         */
    /* ------------------------------------------------------------------ */
    CAN1->MCR &= ~CAN_MCR_INRQ;

    timeout = CAN_INAK_TIMEOUT;
    while ((CAN1->MSR & CAN_MSR_INAK) && (timeout-- > 0U)) {}

    /* ------------------------------------------------------------------ */
    /* 7. Enable FIFO0 message-pending interrupt                          */
    /* ------------------------------------------------------------------ */
    CAN1->IER |= CAN_IER_FMPIE0;

    /* ------------------------------------------------------------------ */
    /* 8. NVIC: priority 6 >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY*/
    /* ------------------------------------------------------------------ */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

/* ---------------------------------------------------------------------------
 * StartCANTxTask
 * ---------------------------------------------------------------------------
 * Creates the shared RX semaphore BEFORE calling CAN1_Init (which enables
 * the NVIC), guaranteeing the semaphore handle is non-NULL when the first
 * ISR fires.  Then enters a 500 ms periodic loop that:
 *   a) Waits (up to 100 ms) for a free TX mailbox.
 *   b) Builds a frame: StdID = 0x123, DLC = 8.
 *        bytes [0..3] = 32-bit tx_counter (LE)
 *        bytes [4..7] = 0xDE 0xAD 0xBE 0xEF
 *   c) Loads the mailbox registers and sets TXRQ.
 *   d) Toggles the green LED (LD3, PG13).
 * --------------------------------------------------------------------------*/
void StartCANTxTask(void const *argument)
{
    (void)argument;

    /* Create the binary semaphore before enabling the CAN RX interrupt */
    s_can_rx_sem = xSemaphoreCreateBinary();

    /* Initialise CAN1 peripheral */
    CAN1_Init();

    uint32_t tx_counter = 0U;

    for (;;)
    {
        /* Poll until a TX mailbox is free (TME0, TME1, or TME2 set) */
        uint32_t wait_ticks = 100U;   /* max 100 × 1 ms */
        while (!(CAN1->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2))
               && (wait_ticks-- > 0U))
        {
            osDelay(1U);
        }

        if (CAN1->TSR & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2))
        {
            /* Read CODE field: index of the lowest-priority empty mailbox */
            uint8_t mb = (uint8_t)((CAN1->TSR & CAN_TSR_CODE) >> CAN_TSR_CODE_Pos);
            if (mb > 2U)
            {
                mb = 0U;   /* safety: fall back to mailbox 0 */
            }

            /* Build payload */
            uint8_t data[8U];
            data[0U] = (uint8_t)( tx_counter         & 0xFFU);
            data[1U] = (uint8_t)((tx_counter >>  8U)  & 0xFFU);
            data[2U] = (uint8_t)((tx_counter >> 16U)  & 0xFFU);
            data[3U] = (uint8_t)((tx_counter >> 24U)  & 0xFFU);
            data[4U] = 0xDEU;
            data[5U] = 0xADU;
            data[6U] = 0xBEU;
            data[7U] = 0xEFU;

            /* Load mailbox – write TIR last with TXRQ=0, set TXRQ separately */
            CAN1->sTxMailBox[mb].TIR  = (CAN_DEMO_TX_ID << CAN_TI0R_STID_Pos);  /* StdID, IDE=0, RTR=0 */
            CAN1->sTxMailBox[mb].TDTR = 8U;                                       /* DLC = 8 */
            CAN1->sTxMailBox[mb].TDLR = ((uint32_t)data[3U] << 24U) |
                                         ((uint32_t)data[2U] << 16U) |
                                         ((uint32_t)data[1U] <<  8U) |
                                          (uint32_t)data[0U];
            CAN1->sTxMailBox[mb].TDHR = ((uint32_t)data[7U] << 24U) |
                                         ((uint32_t)data[6U] << 16U) |
                                         ((uint32_t)data[5U] <<  8U) |
                                          (uint32_t)data[4U];

            /* Trigger hardware transmission */
            CAN1->sTxMailBox[mb].TIR |= CAN_TI0R_TXRQ;

            tx_counter++;

            /* Toggle green LED (LD3, PG13) on each successful enqueue */
            HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        }

        osDelay(500U);   /* 500 ms transmit period */
    }
}

/* ---------------------------------------------------------------------------
 * StartCANRxTask
 * ---------------------------------------------------------------------------
 * Spins until the TX task creates the semaphore, then blocks on it.  Each
 * time the ISR wakes the task, it drains every pending frame from FIFO0:
 *   a) Captures all four mailbox registers (RIR, RDTR, RDLR, RDHR).
 *   b) Releases the FIFO output slot (RF0R.RFOM0 = 1).
 *   c) Decodes the frame into a can_rx_frame_t.
 *   d) Stores it in s_last_rx_frame under a critical section; increments
 *      s_rx_total_count.
 *   e) Toggles the red LED (LD4, PG14).
 * --------------------------------------------------------------------------*/
void StartCANRxTask(void const *argument)
{
    (void)argument;

    /* Wait until StartCANTxTask creates the semaphore */
    while (s_can_rx_sem == NULL)
    {
        osDelay(1U);
    }

    for (;;)
    {
        /* Block until the ISR signals a new frame */
        if (xSemaphoreTake(s_can_rx_sem, portMAX_DELAY) == pdTRUE)
        {
            /* Drain all pending messages from FIFO0 */
            while (CAN1->RF0R & CAN_RF0R_FMP0)
            {
                /* Read registers before releasing the slot */
                uint32_t rir   = CAN1->sFIFOMailBox[0U].RIR;
                uint32_t rdtr  = CAN1->sFIFOMailBox[0U].RDTR;
                uint32_t rdlr  = CAN1->sFIFOMailBox[0U].RDLR;
                uint32_t rdhr  = CAN1->sFIFOMailBox[0U].RDHR;

                /* Release FIFO0 output mailbox so hardware can fill it again */
                CAN1->RF0R |= CAN_RF0R_RFOM0;

                /* Decode */
                can_rx_frame_t frame;
                frame.std_id    = (rir  & CAN_RI0R_STID_Msk) >> CAN_RI0R_STID_Pos;
                frame.is_remote = (rir  & CAN_RI0R_RTR_Msk)  ? 1U : 0U;
                frame.dlc       = (uint8_t)(rdtr & 0x0FU);
                frame.data[0U]  = (uint8_t)( rdlr         & 0xFFU);
                frame.data[1U]  = (uint8_t)((rdlr >>  8U)  & 0xFFU);
                frame.data[2U]  = (uint8_t)((rdlr >> 16U)  & 0xFFU);
                frame.data[3U]  = (uint8_t)((rdlr >> 24U)  & 0xFFU);
                frame.data[4U]  = (uint8_t)( rdhr         & 0xFFU);
                frame.data[5U]  = (uint8_t)((rdhr >>  8U)  & 0xFFU);
                frame.data[6U]  = (uint8_t)((rdhr >> 16U)  & 0xFFU);
                frame.data[7U]  = (uint8_t)((rdhr >> 24U)  & 0xFFU);
                frame.timestamp = osKernelSysTick();

                /* Update shared state under critical section */
                taskENTER_CRITICAL();
                s_last_rx_frame  = frame;
                s_rx_total_count++;
                taskEXIT_CRITICAL();

                /* Toggle red LED (LD4, PG14) on every received frame */
                HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * CAN_GetLastRxFrame
 * ---------------------------------------------------------------------------
 * Returns a thread-safe snapshot of the most recently received CAN frame and
 * the cumulative receive count.  May be called from any task context.
 * Either pointer may be NULL to skip that output.
 * --------------------------------------------------------------------------*/
void CAN_GetLastRxFrame(can_rx_frame_t *out_frame, uint32_t *out_count)
{
    taskENTER_CRITICAL();

    if (out_frame != NULL)
    {
        *out_frame = (can_rx_frame_t)s_last_rx_frame;
    }
    if (out_count != NULL)
    {
        *out_count = s_rx_total_count;
    }

    taskEXIT_CRITICAL();
}
