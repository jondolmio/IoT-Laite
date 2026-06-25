#ifndef FREERTOS_HOST_SHIM_H
#define FREERTOS_HOST_SHIM_H

#include <stdint.h>

typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;
typedef void (*TaskFunction_t)(void *);

typedef struct HostQueue *QueueHandle_t;

#define pdPASS 1
#define pdFAIL 0
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))

BaseType_t xTaskCreate(TaskFunction_t task_function,
                       const char *name,
                       uint16_t stack_depth,
                       void *task_parameters,
                       UBaseType_t priority,
                       void *task_handle);
void vTaskStartScheduler(void);

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size);
BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t ticks_to_wait);
BaseType_t xQueueReceive(QueueHandle_t queue, void *buffer, TickType_t ticks_to_wait);
void vQueueDelete(QueueHandle_t queue);

#endif
