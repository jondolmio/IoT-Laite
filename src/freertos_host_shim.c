#include "freertos_host_shim.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOST_MAX_TASKS 8U

typedef struct
{
    TaskFunction_t task_function;
    const char *name;
    void *task_parameters;
} HostTask;

struct HostQueue
{
    unsigned char *buffer;
    UBaseType_t item_size;
    UBaseType_t length;
    UBaseType_t count;
};

static HostTask g_tasks[HOST_MAX_TASKS];
static size_t g_task_count;

BaseType_t xTaskCreate(TaskFunction_t task_function,
                       const char *name,
                       uint16_t stack_depth,
                       void *task_parameters,
                       UBaseType_t priority,
                       void *task_handle)
{
    (void)stack_depth;
    (void)priority;
    (void)task_handle;

    if (g_task_count >= HOST_MAX_TASKS)
    {
        return pdFAIL;
    }

    g_tasks[g_task_count].task_function = task_function;
    g_tasks[g_task_count].name = name;
    g_tasks[g_task_count].task_parameters = task_parameters;
    ++g_task_count;

    return pdPASS;
}

void vTaskStartScheduler(void)
{
    for (size_t index = 0; index < g_task_count; ++index)
    {
        printf("[task] %s\n", g_tasks[index].name);
        g_tasks[index].task_function(g_tasks[index].task_parameters);
    }
}

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size)
{
    struct HostQueue *queue = calloc(1, sizeof(*queue));

    if (queue == NULL)
    {
        return NULL;
    }

    queue->buffer = calloc(length, item_size);
    if (queue->buffer == NULL)
    {
        free(queue);
        return NULL;
    }

    queue->item_size = item_size;
    queue->length = length;
    queue->count = 0;
    return queue;
}

BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t ticks_to_wait)
{
    (void)ticks_to_wait;

    if (queue == NULL || item == NULL || queue->count >= queue->length)
    {
        return pdFAIL;
    }

    memcpy(queue->buffer + (queue->count * queue->item_size), item, queue->item_size);
    ++queue->count;
    return pdPASS;
}

BaseType_t xQueueReceive(QueueHandle_t queue, void *buffer, TickType_t ticks_to_wait)
{
    (void)ticks_to_wait;

    if (queue == NULL || buffer == NULL || queue->count == 0)
    {
        return pdFAIL;
    }

    memcpy(buffer, queue->buffer, queue->item_size);
    --queue->count;

    if (queue->count > 0U)
    {
        memmove(queue->buffer,
                queue->buffer + queue->item_size,
                (size_t)queue->count * (size_t)queue->item_size);
    }

    return pdPASS;
}

void vQueueDelete(QueueHandle_t queue)
{
    if (queue == NULL)
    {
        return;
    }

    free(queue->buffer);
    free(queue);
}
