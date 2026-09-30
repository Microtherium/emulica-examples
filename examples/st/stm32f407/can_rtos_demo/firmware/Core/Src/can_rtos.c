/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    can_rtos.c
 * @brief   CAN-based cooperative RTOS demo implementation.
 *
 * Architecture
 * ------------
 * MiniRTOS_Run() drives a round-robin cooperative scheduler.  Tasks are
 * registered from main() before the scheduler is started:
 *
 *   Period    Task
 *   ------    ----
 *       0 ms  MX_USB_HOST_Process  – USB host maintenance (every tick)
 *      10 ms  CAN_Rx_Task          – poll CAN1 FIFO0, drain SW RX queue
 *     100 ms  CAN_Tx_Task          – send periodic heartbeat frame
 *    1000 ms  I2C_Demo             – existing I2C/TMP117 demo
 *    1000 ms  ADC_Demo             – existing ADC demo (result forwarded over I2C)
 *    1000 ms  SPI_Demo             – existing SPI flash demo
 *
 * CAN1 low-level driver
 * ---------------------
 * Uses direct register access through the CAN_TypeDef defined in stm32f407xx.h
 * (already included transitively via stm32f4xx_hal.h / main.h).
 * No HAL_CAN module is required; HAL_CAN_MODULE_ENABLED stays commented out
 * in stm32f4xx_hal_conf.h.
 *
 * Pinout (STM32F407 Discovery)
 *   PD0  CAN1_RX  AF9
 *   PD1  CAN1_TX  AF9
 *
 * Bit-timing: APB1 = 42 MHz, 500 kbps
 *   BRP = 6  (register value 5)
 *   TS1 = 11 (register value 10)
 *   TS2 = 2  (register value 1)
 *   SJW = 1  (register value 0)
 *   => TQ = 1/(42 MHz / 6) = 142.9 ns
 *   => Bit time = (1 + 11 + 2) TQ = 14 TQ = 2 µs => 500 kbps
 *
 * LED activity
 *   LD3 (orange, PD13) – toggles on every CAN TX
 *   LD4 (green,  PD12) – toggles on every CAN RX frame processed
 *   LD5 (red,    PD14) – SPI Demo activity (existing)
 *   LD6 (blue,   PD15) – I2C Demo activity (existing)
 *
 * Emulica / Renoly notes  (see also sim.resc)
 * -------------------------------------------
 * When EMULICA_IS_TESTING is true:
 *  - CAN_Demo_Init() skips all hardware initialisation.
 *  - CAN_Tx_Task() still writes frame fields to the TX mailbox registers so
 *    sim.resc AddWatchpointHook callbacks can log every transmitted frame.
 *  - One synthetic RX frame is pre-loaded into g_can_rx_queue at init time
 *    so the RX path is exercised on the very first CAN_Rx_Task() call.
 ******************************************************************************
 */
/* USER CODE END Header */

#include "can_rtos.h"
#include "main.h"   /* HAL headers, pin defines, Error_Handler */

/* =========================================================================
 * Global state
 * ====================================================================== */
CAN_Queue_t        g_can_tx_queue = { .cap = CAN_QUEUE_DEPTH };
CAN_Queue_t        g_can_rx_queue = { .cap = CAN_QUEUE_DEPTH };
volatile uint32_t  g_can_tx_count  = 0;
volatile uint32_t  g_can_rx_count  = 0;
volatile uint32_t  g_can_err_count = 0;

/* =========================================================================
 * Mini-RTOS internal state
 * ====================================================================== */
static Task_t  s_tasks[MINI_RTOS_MAX_TASKS];
static uint8_t s_task_count = 0;

/* =========================================================================
 * Queue helpers
 * ====================================================================== */

/**
 * @brief Push a CAN frame onto the tail of a queue.
 * @retval  0  success
 * @retval -1  queue full, frame dropped
 */
int CAN_Queue_Push(CAN_Queue_t *q, const CAN_Frame_t *frame)
{
    if (q->count >= q->cap) {
        return -1;   /* full – drop the frame */
    }
    q->buf[q->tail] = *frame;
    q->tail = (uint8_t)((q->tail + 1u) % q->cap);
    q->count++;
    return 0;
}

/**
 * @brief Pop a CAN frame from the head of a queue.
 * @retval  0  success – *frame filled
 * @retval -1  queue empty
 */
int CAN_Queue_Pop(CAN_Queue_t *q, CAN_Frame_t *frame)
{
    if (q->count == 0) {
        return -1;   /* empty */
    }
    *frame = q->buf[q->head];
    q->head = (uint8_t)((q->head + 1u) % q->cap);
    q->count--;
    return 0;
}

/* =========================================================================
 * CAN1 low-level helpers  (register-level, no HAL_CAN module)
 * ====================================================================== */

/**
 * @brief Enable CAN1 APB1 clock and configure PD0 (RX) / PD1 (TX) as AF9.
 *
 * Direct register manipulation is used so that this file does not pull in
 * stm32f4xx_hal_rcc_ex.h macros that may be absent when HAL_CAN is disabled.
 * The RCC and GPIO TypeDefs come from stm32f407xx.h (via main.h).
 */
static void CAN1_GPIO_Clock_Init(void)
{
    /* --- CAN1 APB1 clock (bit 25 of APB1ENR) ----------------------------- */
    RCC->APB1ENR |= (1UL << 25);
    (void)RCC->APB1ENR;  /* dummy read to flush the write through the bus    */

    /* --- GPIOD AHB1 clock (bit 3 of AHB1ENR) – safe to re-enable --------- */
    RCC->AHB1ENR |= (1UL << 3);
    (void)RCC->AHB1ENR;

    /* --- PD0 = CAN1_RX, PD1 = CAN1_TX  (both AF9) ----------------------- */

    /* MODER: alternate-function mode = 0b10 for pins 0 and 1 */
    GPIOD->MODER &= ~((3UL << (0u * 2u)) | (3UL << (1u * 2u)));
    GPIOD->MODER |=  ((2UL << (0u * 2u)) | (2UL << (1u * 2u)));

    /* OSPEEDR: high speed = 0b10 */
    GPIOD->OSPEEDR &= ~((3UL << (0u * 2u)) | (3UL << (1u * 2u)));
    GPIOD->OSPEEDR |=  ((2UL << (0u * 2u)) | (2UL << (1u * 2u)));

    /* OTYPER: push-pull = 0 */
    GPIOD->OTYPER &= ~((1UL << 0u) | (1UL << 1u));

    /* PUPDR: no pull = 0b00 */
    GPIOD->PUPDR &= ~((3UL << (0u * 2u)) | (3UL << (1u * 2u)));

    /* AFR[0]: pins 0-7.  AF9 = 0x9. */
    GPIOD->AFR[0] &= ~((0xFUL << (0u * 4u)) | (0xFUL << (1u * 4u)));
    GPIOD->AFR[0] |=  ((9UL   << (0u * 4u)) | (9UL   << (1u * 4u)));
}

/**
 * @brief Configure CAN1 baud rate (500 kbps) and set up acceptance filter.
 *
 * Filter bank 0 is configured in 32-bit mask mode with ID=0 and Mask=0
 * (accept every incoming frame) assigned to FIFO0.
 *
 * The hardware-wait loops are guarded by a timeout so that they do not spin
 * forever if called unexpectedly under simulation; EMULICA_IS_TESTING callers
 * skip this function entirely.
 */
static void CAN1_Baud_Filter_Init(void)
{
    uint32_t timeout;

    /* ---- Enter Initialisation mode (INRQ) -------------------------------- */
    CAN1->MCR |= 0x01UL;                    /* INRQ = 1                      */
    timeout = 0x0000FFFFu;
    while (!(CAN1->MSR & 0x01UL) && --timeout) {}  /* wait for INAK = 1     */

    CAN1->MCR &= ~0x02UL;                   /* clear SLEEP bit               */

    /* ---- Bit-timing: 500 kbps @ APB1 42 MHz ----------------------------- */
    /* BTR layout: [25:24]=SJW, [22:20]=TS2, [19:16]=TS1, [9:0]=BRP         */
    /* SJW=0 (1 TQ), TS2=1 (2 TQ), TS1=10 (11 TQ), BRP=5 (prescaler=6)     */
    CAN1->BTR = (0UL << 24u) | (1UL << 20u) | (10UL << 16u) | 5UL;

    /* ---- MCR options ----------------------------------------------------- */
    /* NART=0 (auto-retransmit enabled), TXFP=0 (TX priority by ID)          */
    CAN1->MCR &= ~((1UL << 4u) | (1UL << 2u));

    /* ---- Leave Initialisation mode --------------------------------------- */
    CAN1->MCR &= ~0x01UL;                   /* INRQ = 0                      */
    timeout = 0x0000FFFFu;
    while ((CAN1->MSR & 0x01UL) && --timeout) {}   /* wait for INAK = 0     */

    /* ---- Acceptance filter (bank 0, 32-bit mask, FIFO0, accept-all) ----- */
    CAN1->FMR   |=  0x01UL;         /* FINIT = 1 – enter filter init mode    */
    CAN1->FA1R  &= ~0x01UL;         /* deactivate bank 0 before reconfiguring */
    CAN1->FM1R  &= ~0x01UL;         /* bank 0: mask mode (0) not list mode (1) */
    CAN1->FS1R  |=  0x01UL;         /* bank 0: 32-bit scale                  */
    CAN1->FFA1R &= ~0x01UL;         /* bank 0: assign to FIFO0               */
    CAN1->sFilterRegister[0].FR1 = 0UL;  /* filter ID  = 0  (any)           */
    CAN1->sFilterRegister[0].FR2 = 0UL;  /* filter mask = 0 (all bits = don't care) */
    CAN1->FA1R  |=  0x01UL;         /* activate bank 0                       */
    CAN1->FMR   &= ~0x01UL;         /* FINIT = 0 – leave filter init mode    */
}

/**
 * @brief  Transmit one CAN frame via TX mailbox 0 (polling, no interrupt).
 *
 * In test mode the frame fields are written to mailbox 0 registers so that
 * sim.resc watchpoints at 0x40006580 and 0x40006588 can observe the TX.
 * Writing TIR with bit 0 set (TXRQ) is the final trigger watched by Renoly.
 *
 * @retval  0   Frame accepted (or simulation write successful).
 * @retval -1   Mailbox 0 busy on real hardware.
 */
static int CAN1_LL_Transmit(const CAN_Frame_t *frame)
{
    /* Always write the data and length registers (visible to Renoly) -------- */
    CAN1->sTxMailBox[0].TDLR =
        ((uint32_t)frame->data[0])         |
        ((uint32_t)frame->data[1] <<  8u)  |
        ((uint32_t)frame->data[2] << 16u)  |
        ((uint32_t)frame->data[3] << 24u);

    CAN1->sTxMailBox[0].TDHR =
        ((uint32_t)frame->data[4])         |
        ((uint32_t)frame->data[5] <<  8u)  |
        ((uint32_t)frame->data[6] << 16u)  |
        ((uint32_t)frame->data[7] << 24u);

    CAN1->sTxMailBox[0].TDTR = (uint32_t)(frame->len & 0x0Fu);

    if (EMULICA_IS_TESTING) {
        /*
         * Simulation path: write TIR with STID and TXRQ=1 in one shot.
         * The sim.resc watchpoint at CAN1->sTxMailBox[0].TIR (0x40006580)
         * fires on this write and prints the frame ID.
         */
        CAN1->sTxMailBox[0].TIR = ((frame->id & 0x7FFu) << 21u) | 0x01u;
        return 0;
    }

    /* Real hardware: check mailbox 0 empty (TSR bit 26 = TME0) ------------ */
    if (!(CAN1->TSR & (1UL << 26u))) {
        return -1;   /* mailbox busy */
    }

    /* Load identifier register (STID in bits 31:21, IDE=0, RTR=0) ---------- */
    CAN1->sTxMailBox[0].TIR = (frame->id & 0x7FFu) << 21u;

    /* Request transmission (TXRQ = bit 0 of TIR) – Renoly watchpoint fires  */
    CAN1->sTxMailBox[0].TIR |= 0x01u;
    return 0;
}

/* =========================================================================
 * CAN Demo – public interface
 * ====================================================================== */

void CAN_Demo_Init(void)
{
    if (EMULICA_IS_TESTING) {
        /*
         * Simulation mode: skip all hardware init.
         * Pre-populate the RX queue with one synthetic frame so that
         * CAN_Rx_Task() has something to process on its first invocation and
         * automated Emulica tests can verify the receive path end-to-end.
         */
        const CAN_Frame_t test_rx = {
            .id   = CAN_TEST_RX_STD_ID,        /* 0x456                      */
            .len  = 4,
            .data = { 'S', 'I', 'M', 0x01u }
        };
        CAN_Queue_Push(&g_can_rx_queue, &test_rx);
        return;
    }

    /* Real hardware initialisation */
    CAN1_GPIO_Clock_Init();
    CAN1_Baud_Filter_Init();
}

/**
 * @brief  CAN TX task – called every CAN_TX_PERIOD_MS by the scheduler.
 *
 * Builds a 4-byte heartbeat frame (header "CAN" + rolling sequence counter),
 * pushes a copy to the software TX queue for application-level logging, then
 * transmits directly via CAN1 TX mailbox 0.
 *
 * On success LD3 (orange LED) toggles to give a visible heartbeat indicator.
 */
void CAN_Tx_Task(void)
{
    static uint8_t seq = 0u;

    CAN_Frame_t frame = {
        .id   = CAN_TX_STD_ID,              /* 0x123                          */
        .len  = 4u,
        .data = { 'C', 'A', 'N', seq++ }
    };

    /* Keep a software copy (e.g. for DMA-based logging or replay) */
    CAN_Queue_Push(&g_can_tx_queue, &frame);

    /* Transmit via CAN1 hardware (or simulation stub) */
    if (CAN1_LL_Transmit(&frame) == 0) {
        g_can_tx_count++;
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);   /* orange LED = TX tick */
    } else {
        g_can_err_count++;
    }
}

/**
 * @brief  CAN RX task – called every CAN_RX_PERIOD_MS by the scheduler.
 *
 * Phase 1 – hardware polling (real hardware only):
 *   Reads all pending frames from CAN1 FIFO0, unpacks them, and pushes each
 *   to the software g_can_rx_queue.  Releases the mailbox after each read
 *   (RF0R.RFOM0 = 1).
 *
 * Phase 2 – software queue drain (runs in both real and test mode):
 *   Pops every frame from g_can_rx_queue and dispatches it to application
 *   logic.  Each successful reception toggles LD4 (green LED).
 *
 * Extend the "application logic" section to implement a CAN protocol stack
 * (e.g. dispatch by ID, feed a state machine, forward over UART, etc.).
 */
void CAN_Rx_Task(void)
{
    CAN_Frame_t frame;

    /* ---- Phase 1: drain hardware FIFO0 (real hardware only) ------------ */
    if (!EMULICA_IS_TESTING) {
        /*
         * RF0R bits [1:0] = FMP0: number of messages pending in FIFO0.
         * RF0R bit  [5]   = RFOM0: write 1 to release the output mailbox.
         */
        while (CAN1->RF0R & 0x03UL) {
            /* Read identifier (STID = bits 31:21 of RIR) */
            frame.id  = (uint32_t)((CAN1->sFIFOMailBox[0].RIR >> 21u) & 0x7FFu);
            /* Read DLC (bits 3:0 of RDTR) */
            frame.len = (uint8_t)(CAN1->sFIFOMailBox[0].RDTR & 0x0Fu);

            /* Unpack data bytes from two 32-bit registers */
            const uint32_t rdlr = CAN1->sFIFOMailBox[0].RDLR;
            const uint32_t rdhr = CAN1->sFIFOMailBox[0].RDHR;

            frame.data[0] = (uint8_t)( rdlr        & 0xFFu);
            frame.data[1] = (uint8_t)((rdlr >>  8u) & 0xFFu);
            frame.data[2] = (uint8_t)((rdlr >> 16u) & 0xFFu);
            frame.data[3] = (uint8_t)((rdlr >> 24u) & 0xFFu);
            frame.data[4] = (uint8_t)( rdhr        & 0xFFu);
            frame.data[5] = (uint8_t)((rdhr >>  8u) & 0xFFu);
            frame.data[6] = (uint8_t)((rdhr >> 16u) & 0xFFu);
            frame.data[7] = (uint8_t)((rdhr >> 24u) & 0xFFu);

            /* Release FIFO0 output mailbox */
            CAN1->RF0R |= (1UL << 5u);

            CAN_Queue_Push(&g_can_rx_queue, &frame);
        }
    }

    /* ---- Phase 2: process software RX queue (real + test mode) --------- */
    while (CAN_Queue_Pop(&g_can_rx_queue, &frame) == 0) {
        g_can_rx_count++;
        HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);  /* green LED = RX frame */

        /*
         * ---- Application-level dispatch ----
         * Add protocol handling here.  Example: route by CAN ID.
         *
         * switch (frame.id) {
         *     case 0x100: handle_sensor_frame(&frame);   break;
         *     case 0x200: handle_control_frame(&frame);  break;
         *     default:    break;
         * }
         */
        (void)frame;  /* suppress unused-variable warning in bare demo */
    }
}

/* =========================================================================
 * Mini-RTOS cooperative scheduler
 * ====================================================================== */

void MiniRTOS_Init(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    s_task_count = 0;
}

void MiniRTOS_RegisterTask(TaskFunc_t fn, uint32_t period_ms, const char *name)
{
    if (s_task_count >= MINI_RTOS_MAX_TASKS) {
        return;  /* silently ignore overflow; increase MINI_RTOS_MAX_TASKS */
    }
    Task_t *t    = &s_tasks[s_task_count++];
    t->fn        = fn;
    t->period_ms = period_ms;
    t->last_tick = HAL_GetTick();
    t->name      = name;
}

/**
 * @brief  Cooperative round-robin scheduler.  Never returns.
 *
 * Design notes:
 *  - A task with period_ms == 0 runs on every scheduler pass (e.g. USB host).
 *  - HAL_GetTick() uses SysTick and wraps at 2^32 ms; the unsigned subtraction
 *    (now - t->last_tick) handles the wrap correctly.
 *  - HAL_Delay(MINI_RTOS_TICK_MS) at the end of each pass keeps the CPU from
 *    busy-spinning.  Reduce MINI_RTOS_TICK_MS (or remove the delay) if
 *    sub-millisecond latency is required; a preemptive RTOS (FreeRTOS, Zephyr)
 *    would be a better fit in that case.
 */
void MiniRTOS_Run(void)
{
    for (;;) {
        const uint32_t now = HAL_GetTick();

        for (uint8_t i = 0; i < s_task_count; i++) {
            Task_t * const t = &s_tasks[i];

            /* Run if period expired (or period == 0 = run every tick) */
            if (t->period_ms == 0u || (now - t->last_tick) >= t->period_ms) {
                t->fn();
                t->last_tick = now;
            }
        }

        /* Yield CPU; use WFI or a very short delay for lower-power designs */
        HAL_Delay(MINI_RTOS_TICK_MS);
    }
}
