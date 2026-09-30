/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/**
 * @brief  CAN frame descriptor exchanged through the inter-task queue.
 */
typedef struct
{
    uint32_t id;       /**< Standard (11-bit) or extended (29-bit) CAN ID */
    uint8_t  ide;      /**< 0 = standard, 1 = extended */
    uint8_t  dlc;      /**< Data-length code (0–8 bytes) */
    uint8_t  data[8];  /**< Payload */
} CAN_Msg_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/** Number of CAN_Msg_t slots held in the receive queue */
#define CAN_QUEUE_LEN  10U
/** TX frame CAN standard ID */
#define CAN_TX_STD_ID  0x123U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

osThreadId defaultTaskHandle;
uint32_t defaultTaskBuffer[ 128 ];
osStaticThreadDef_t defaultTaskControlBlock;
osThreadId canRxTaskHandle;
uint32_t canRxTaskBuffer[ 256 ];
osStaticThreadDef_t canRxTaskControlBlock;
/* USER CODE BEGIN PV */
/* ------------ CAN RX queue (ISR → canRxTask) ----------------------------- */
static QueueHandle_t canRxQueue;
static StaticQueue_t canRxQueueStruct;
static uint8_t       canRxQueueStorage[CAN_QUEUE_LEN * sizeof(CAN_Msg_t)];

osThreadId canTxTaskHandle;
uint32_t canTxTaskBuffer[ 256 ];
osStaticThreadDef_t canTxTaskControlBlock;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
void StartDefaultTask(void const * argument);
void StartCanRxTask(void const * argument);

/* USER CODE BEGIN PFP */
void StartCanTxTask(void const * argument);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* Create the RX queue with statically-allocated storage */
  canRxQueue = xQueueCreateStatic(CAN_QUEUE_LEN,
                                   sizeof(CAN_Msg_t),
                                   canRxQueueStorage,
                                   &canRxQueueStruct);
  configASSERT(canRxQueue != NULL);

  /* Enable RX-FIFO0 interrupt now that the queue exists */
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadStaticDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128, defaultTaskBuffer, &defaultTaskControlBlock);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of canRxTask */
  osThreadStaticDef(canRxTask, StartCanRxTask, osPriorityNormal, 0, 256, canRxTaskBuffer, &canRxTaskControlBlock);
  canRxTaskHandle = osThreadCreate(osThread(canRxTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* Create canTxTask through CMSIS-RTOS, matching firmware.ioc and the other tasks. */
  osThreadStaticDef(canTxTask, StartCanTxTask, osPriorityAboveNormal, 0, 256,
                    canTxTaskBuffer, &canTxTaskControlBlock);
  canTxTaskHandle = osThreadCreate(osThread(canTxTask), NULL);
  configASSERT(canTxTaskHandle != NULL);
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 2;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */
  /* Accept-all pass-through filter → FIFO0 */
  {
    CAN_FilterTypeDef sFilter = {0};
    sFilter.FilterBank           = 0;
    sFilter.FilterMode           = CAN_FILTERMODE_IDMASK;
    sFilter.FilterScale          = CAN_FILTERSCALE_32BIT;
    sFilter.FilterIdHigh         = 0x0000U;
    sFilter.FilterIdLow          = 0x0000U;
    sFilter.FilterMaskIdHigh     = 0x0000U; /* mask 0 = accept all */
    sFilter.FilterMaskIdLow      = 0x0000U;
    sFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilter.FilterActivation     = ENABLE;
    if (HAL_CAN_ConfigFilter(&hcan1, &sFilter) != HAL_OK) { Error_Handler(); }
  }
  if (HAL_CAN_Start(&hcan1) != HAL_OK) { Error_Handler(); }
  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief  CAN RX FIFO0 message-pending callback (called from ISR context).
 *
 *         Reads the incoming frame from the hardware and posts a CAN_Msg_t to
 *         canRxQueue so that StartCanRxTask can process it at task level.
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef rxHdr;
  CAN_Msg_t           msg;
  BaseType_t          xHigherPriorityTaskWoken = pdFALSE;

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHdr, msg.data) != HAL_OK)
  {
    return; /* Frame read error — discard */
  }

  if (rxHdr.IDE == CAN_ID_EXT)
  {
    msg.id  = rxHdr.ExtId;
    msg.ide = 1U;
  }
  else
  {
    msg.id  = rxHdr.StdId;
    msg.ide = 0U;
  }
  msg.dlc = (uint8_t)rxHdr.DLC;

  /* Post to the queue from ISR; yield if a higher-priority task was woken */
  xQueueSendFromISR(canRxQueue, &msg, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief  CAN TX task body — transmits a rolling-counter frame every 500 ms.
 *
 *         Waits for a free TX mailbox, builds an 8-byte payload with a 32-bit
 *         counter in the first four bytes (big-endian) followed by a fixed
 *         0xDEADBEEF pattern, and fires the frame at CAN_TX_STD_ID.
 */
void StartCanTxTask(void const * argument)
{
  (void)argument;

  CAN_TxHeaderTypeDef txHdr;
  uint8_t             txData[8];
  uint32_t            txMailbox;
  uint32_t            txCounter = 0U;

  txHdr.StdId              = CAN_TX_STD_ID;
  txHdr.ExtId              = 0x00000000U;
  txHdr.IDE                = CAN_ID_STD;
  txHdr.RTR                = CAN_RTR_DATA;
  txHdr.DLC                = 8U;
  txHdr.TransmitGlobalTime = DISABLE;

  for (;;)
  {
    /* Payload: 4-byte big-endian counter | 0xDEADBEEF signature */
    txData[0] = (uint8_t)(txCounter >> 24U);
    txData[1] = (uint8_t)(txCounter >> 16U);
    txData[2] = (uint8_t)(txCounter >>  8U);
    txData[3] = (uint8_t)(txCounter);
    txData[4] = 0xDEU;
    txData[5] = 0xADU;
    txData[6] = 0xBEU;
    txData[7] = 0xEFU;

    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0U)
    {
      if (HAL_CAN_AddTxMessage(&hcan1, &txHdr, txData, &txMailbox) == HAL_OK)
      {
        txCounter++;
      }
    }

    osDelay(500); /* Transmit every 500 ms */
  }
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
__weak void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Heartbeat — lowest-priority supervisor task */
  for (;;)
  {
    osDelay(1000); /* Yield every 1 s; extend with LED toggle or watchdog kick */
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartCanRxTask */
/**
* @brief Function implementing the canRxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCanRxTask */
void StartCanRxTask(void const * argument)
{
  /* USER CODE BEGIN StartCanRxTask */
  CAN_Msg_t rxMsg;

  for (;;)
  {
    /* Block indefinitely until HAL_CAN_RxFifo0MsgPendingCallback posts a frame */
    if (xQueueReceive(canRxQueue, &rxMsg, portMAX_DELAY) == pdTRUE)
    {
      /*
       * Application processing point.
       *
       * rxMsg.id      — CAN ID (standard or extended)
       * rxMsg.ide     — 0 = standard frame, 1 = extended frame
       * rxMsg.dlc     — number of valid bytes in rxMsg.data
       * rxMsg.data[]  — payload bytes [0 .. dlc-1]
       *
       * Example: add loopback, protocol dispatch, logging, etc.
       */
      (void)rxMsg; /* Remove this line and add real processing below */
    }
  }
  /* USER CODE END StartCanRxTask */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
