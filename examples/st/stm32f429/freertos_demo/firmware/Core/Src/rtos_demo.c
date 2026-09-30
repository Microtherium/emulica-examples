/**
  * @file    rtos_demo.c
  * @brief   FreeRTOS feature demo for st.stm32f429.freertos_demo. Every
  *          major synchronization primitive gets its own pair of tasks so
  *          each one is independently observable as real task-switch
  *          activity (see Emulica's simulation GUI - Timeline panel):
  *
  *   - Queue:              ProducerTask  -> ConsumerTask
  *   - Binary semaphore:   PeriodicTimer -> SemaphoreTask (timer-to-task sync)
  *   - Counting semaphore: PoolTaskA/B/C contending for a 2-slot resource pool
  *   - Mutex:              MutexTaskA/B incrementing a shared counter
  *   - Software timer:     PeriodicTimer itself (runs in the Timer Service task)
  *   - Event group:        EventSetterA/B -> EventWaiterTask (wait-for-all)
  *   - Task notifications:  NotifySenderTask -> NotifyReceiverTask
  *   - Dynamic task create/delete: SpawnerTask -> OneShotTask (self-deletes)
  *
  * Task activity is made observable two ways: toggling the two onboard LEDs
  * (LD3/LD4), and a one-byte USART1 transmit per queue item in ConsumerTask -
  * genuine peripheral writes the simulation GUI can tap the same way it taps
  * any other example's sim.resc AddWatchpointHook lines.
  */
#include "main.h"
#include "rtos_demo.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"

#define EVENT_BIT_A (1UL << 0)
#define EVENT_BIT_B (1UL << 1)
#define EVENT_ALL_BITS (EVENT_BIT_A | EVENT_BIT_B)
#define POOL_SLOT_COUNT 2

extern UART_HandleTypeDef huart1;

static QueueHandle_t xDataQueue;
static SemaphoreHandle_t xBinarySemaphore;
static SemaphoreHandle_t xCountingSemaphore;
static SemaphoreHandle_t xSharedResourceMutex;
static TimerHandle_t xPeriodicTimer;
static EventGroupHandle_t xSyncEventGroup;
static TaskHandle_t xNotifyReceiverHandle;

static volatile uint32_t sharedCounter = 0;
volatile uint32_t rtosDemoCreationFailures = 0;
volatile uint32_t rtosDemoTasksCreated = 0;

static void CheckCreated(BaseType_t result)
{
  if (result == pdPASS)
  {
    rtosDemoTasksCreated++;
  }
  else
  {
    rtosDemoCreationFailures++;
  }
}

/* ---- Queue: ProducerTask feeds ConsumerTask ------------------------------ */

static void ProducerTask(void *argument)
{
  uint32_t value = 0;
  for (;;)
  {
    value++;
    xQueueSend(xDataQueue, &value, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

static void ConsumerTask(void *argument)
{
  uint32_t received;
  uint8_t txByte;
  for (;;)
  {
    if (xQueueReceive(xDataQueue, &received, portMAX_DELAY) == pdTRUE)
    {
      HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
      txByte = (uint8_t)(received & 0xFFU);
      HAL_UART_Transmit(&huart1, &txByte, 1, 50U);
    }
  }
}

/* ---- Binary semaphore: a software timer callback signals a waiting task -- */

static void SemaphoreTask(void *argument)
{
  for (;;)
  {
    if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY) == pdTRUE)
    {
      HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);
    }
  }
}

/* Runs in the Timer Service (daemon) task's context, not an ISR - but it's
 * the standard FreeRTOS pattern for periodic work decoupled from any one
 * application task. */
static void PeriodicTimerCallback(TimerHandle_t xTimer)
{
  xSemaphoreGive(xBinarySemaphore);
}

/* ---- Counting semaphore: three tasks sharing a 2-slot resource pool ------ */

static void PoolWorkerTask(void *argument)
{
  for (;;)
  {
    if (xSemaphoreTake(xCountingSemaphore, portMAX_DELAY) == pdTRUE)
    {
      vTaskDelay(pdMS_TO_TICKS(120)); /* hold the "resource" briefly */
      xSemaphoreGive(xCountingSemaphore);
    }
    vTaskDelay(pdMS_TO_TICKS(80));
  }
}

/* ---- Mutex: two tasks contending for a shared counter -------------------- */

static void MutexTaskA(void *argument)
{
  for (;;)
  {
    if (xSemaphoreTake(xSharedResourceMutex, portMAX_DELAY) == pdTRUE)
    {
      sharedCounter++;
      xSemaphoreGive(xSharedResourceMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(150));
  }
}

static void MutexTaskB(void *argument)
{
  for (;;)
  {
    if (xSemaphoreTake(xSharedResourceMutex, portMAX_DELAY) == pdTRUE)
    {
      sharedCounter += 10U;
      xSemaphoreGive(xSharedResourceMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(220));
  }
}

/* ---- Event group: wait for two independent tasks to both report in ------ */

static void EventSetterATask(void *argument)
{
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(400));
    xEventGroupSetBits(xSyncEventGroup, EVENT_BIT_A);
  }
}

static void EventSetterBTask(void *argument)
{
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(650));
    xEventGroupSetBits(xSyncEventGroup, EVENT_BIT_B);
  }
}

static void EventWaiterTask(void *argument)
{
  for (;;)
  {
    /* pdTRUE, pdTRUE = clear on exit, wait for ALL bits */
    xEventGroupWaitBits(xSyncEventGroup, EVENT_ALL_BITS, pdTRUE, pdTRUE, portMAX_DELAY);
    HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
  }
}

/* ---- Task notifications: lightweight signaling, no queue/semaphore object */

static void NotifySenderTask(void *argument)
{
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(500));
    xTaskNotifyGive(xNotifyReceiverHandle);
  }
}

static void NotifyReceiverTask(void *argument)
{
  for (;;)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);
  }
}

/* ---- Dynamic task creation/deletion -------------------------------------- */

static void OneShotTask(void *argument)
{
  vTaskDelay(pdMS_TO_TICKS(50));
  vTaskDelete(NULL);
}

static void SpawnerTask(void *argument)
{
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    xTaskCreate(OneShotTask, "OneShot", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
  }
}

void RTOS_Demo_Init(void)
{
  xDataQueue = xQueueCreate(8, sizeof(uint32_t));
  xBinarySemaphore = xSemaphoreCreateBinary();
  xCountingSemaphore = xSemaphoreCreateCounting(POOL_SLOT_COUNT, POOL_SLOT_COUNT);
  xSharedResourceMutex = xSemaphoreCreateMutex();
  xSyncEventGroup = xEventGroupCreate();

  xPeriodicTimer = xTimerCreate("PeriodicTimer", pdMS_TO_TICKS(700), pdTRUE, NULL, PeriodicTimerCallback);
  xTimerStart(xPeriodicTimer, 0);

  CheckCreated(xTaskCreate(ProducerTask, "Producer", 256, NULL, tskIDLE_PRIORITY + 3, NULL));
  CheckCreated(xTaskCreate(ConsumerTask, "Consumer", 256, NULL, tskIDLE_PRIORITY + 2, NULL));
  CheckCreated(xTaskCreate(SemaphoreTask, "SemTask", 256, NULL, tskIDLE_PRIORITY + 2, NULL));
  CheckCreated(xTaskCreate(PoolWorkerTask, "PoolA", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
  CheckCreated(xTaskCreate(PoolWorkerTask, "PoolB", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
  CheckCreated(xTaskCreate(PoolWorkerTask, "PoolC", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
  CheckCreated(xTaskCreate(MutexTaskA, "MutexA", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
  CheckCreated(xTaskCreate(MutexTaskB, "MutexB", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
  CheckCreated(xTaskCreate(EventSetterATask, "EvtSetA", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
  CheckCreated(xTaskCreate(EventSetterBTask, "EvtSetB", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
  CheckCreated(xTaskCreate(EventWaiterTask, "EvtWait", 256, NULL, tskIDLE_PRIORITY + 2, NULL));
  CheckCreated(xTaskCreate(NotifyReceiverTask, "NotifyRx", 256, NULL, tskIDLE_PRIORITY + 2, &xNotifyReceiverHandle));
  CheckCreated(xTaskCreate(NotifySenderTask, "NotifyTx", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
  CheckCreated(xTaskCreate(SpawnerTask, "Spawner", 256, NULL, tskIDLE_PRIORITY + 1, NULL));
}
