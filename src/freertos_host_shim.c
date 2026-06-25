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
    size_t item_size;
    size_t length;
    size_t count;
    size_t head;
    size_t tail;
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
    /* Host validation runs each task once in registration order, not as a preemptive scheduler. */
    for (size_t index = 0; index < g_task_count; ++index)
    {
        printf("[task] %s\n", g_tasks[index].name);
        g_tasks[index].task_function(g_tasks[index].task_parameters);
    }
}

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size)
{
    struct HostQueue *host_queue = calloc(1, sizeof(*host_queue));

    if (host_queue == NULL)
    {
        return NULL;
    }

    host_queue->buffer = calloc(length, item_size);
    if (host_queue->buffer == NULL)
    {
        free(host_queue);
        return NULL;
    }

    host_queue->item_size = (size_t)item_size;
    host_queue->length = (size_t)length;
    host_queue->count = 0U;
    host_queue->head = 0U;
    host_queue->tail = 0U;
    return host_queue;
}

BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t ticks_to_wait)
{
    (void)ticks_to_wait;

    if (queue == NULL || item == NULL || queue->count >= queue->length)
    {
        return pdFAIL;
    }

    const size_t write_offset = queue->tail * queue->item_size;

    memcpy(queue->buffer + write_offset, item, queue->item_size);
    queue->tail = (queue->tail + 1U) % queue->length;
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

    const size_t read_offset = queue->head * queue->item_size;

    memcpy(buffer,
           queue->buffer + read_offset,
           queue->item_size);
    queue->head = (queue->head + 1U) % queue->length;
    --queue->count;

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
