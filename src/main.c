#include "camera_db.h"
#include "freertos_host_shim.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    const char *mcu;
    const char *gps;
    const char *storage;
    const char *audio;
    unsigned int battery_capacity_mah;
} DeviceProfile;

typedef struct
{
    double latitude;
    double longitude;
    double speed_kmh;
    bool valid;
} GpsFix;

typedef struct
{
    char text[128];
} AudioAlert;

typedef struct
{
    QueueHandle_t gps_queue;
    QueueHandle_t storage_queue;
    QueueHandle_t audio_queue;
} AppContext;

static const DeviceProfile DEVICE_PROFILE = {
    .mcu = "nRF52840",
    .gps = "u-blox MAX-M10S",
    .storage = "SPI Flash 16 MB",
    .audio = "MAX98357A + speaker",
    .battery_capacity_mah = 1000U,
};

static void destroy_app_context(AppContext *app)
{
    if (app->gps_queue != NULL)
    {
        vQueueDelete(app->gps_queue);
        app->gps_queue = NULL;
    }

    if (app->storage_queue != NULL)
    {
        vQueueDelete(app->storage_queue);
        app->storage_queue = NULL;
    }

    if (app->audio_queue != NULL)
    {
        vQueueDelete(app->audio_queue);
        app->audio_queue = NULL;
    }
}

static void storage_task(void *context)
{
    AppContext *app = context;
    const bool storage_ready = true;

    printf("SPI flash mounted, camera entries loaded: %zu\n", camera_db_count());
    (void)xQueueSend(app->storage_queue, &storage_ready, pdMS_TO_TICKS(10));
}

static void gps_task(void *context)
{
    AppContext *app = context;
    const GpsFix fix = {
        .latitude = 60.4515,
        .longitude = 22.2670,
        .speed_kmh = 46.0,
        .valid = true,
    };

    if (xQueueSend(app->gps_queue, &fix, pdMS_TO_TICKS(10)) == pdPASS)
    {
        printf("GPS fix received from %s: %.4f, %.4f at %.1f km/h\n",
               DEVICE_PROFILE.gps,
               fix.latitude,
               fix.longitude,
               fix.speed_kmh);
    }
}

static void navigation_task(void *context)
{
    AppContext *app = context;
    bool storage_ready = false;
    GpsFix fix = {0};

    if (xQueueReceive(app->storage_queue, &storage_ready, pdMS_TO_TICKS(10)) != pdPASS || !storage_ready)
    {
        printf("Storage is not ready.\n");
        return;
    }

    if (xQueueReceive(app->gps_queue, &fix, pdMS_TO_TICKS(10)) != pdPASS || !fix.valid)
    {
        printf("No valid GPS fix available.\n");
        return;
    }

    const CameraLocation *camera = camera_db_find_nearest(fix.latitude, fix.longitude);
    if (camera == NULL)
    {
        printf("Camera database is empty.\n");
        return;
    }

    const double distance_km = camera_db_distance_km(
        fix.latitude,
        fix.longitude,
        camera->latitude,
        camera->longitude);
    AudioAlert alert = {0};

    snprintf(alert.text,
             sizeof(alert.text),
             "Seuraava nopeuskamera: %s, %.1f km päässä.",
             camera->name,
             distance_km);

    if (xQueueSend(app->audio_queue, &alert, pdMS_TO_TICKS(10)) == pdPASS)
    {
        printf("Navigation alert prepared.\n");
    }
}

static void audio_task(void *context)
{
    AppContext *app = context;
    AudioAlert alert = {0};

    if (xQueueReceive(app->audio_queue, &alert, pdMS_TO_TICKS(10)) != pdPASS)
    {
        printf("No audio alert available.\n");
        return;
    }

    printf("I2S audio output via %s: %s\n", DEVICE_PROFILE.audio, alert.text);
}

static void battery_task(void *context)
{
    (void)context;
    const unsigned int average_load_ma = 125U;
    const unsigned int runtime_hours = DEVICE_PROFILE.battery_capacity_mah / average_load_ma;
    printf("Battery profile: %u mAh LiPo, estimated runtime %u h at %u mA average load.\n",
           DEVICE_PROFILE.battery_capacity_mah,
           runtime_hours,
           average_load_ma);
}

int main(void)
{
    AppContext app = {
        .gps_queue = xQueueCreate(1U, sizeof(GpsFix)),
        .storage_queue = xQueueCreate(1U, sizeof(bool)),
        .audio_queue = xQueueCreate(1U, sizeof(AudioAlert)),
    };

    if (app.gps_queue == NULL || app.storage_queue == NULL || app.audio_queue == NULL)
    {
        fprintf(stderr, "Failed to allocate queues.\n");
        destroy_app_context(&app);
        return 1;
    }

    printf("Starting FreeRTOS application skeleton for %s\n", DEVICE_PROFILE.mcu);
    printf("GPS: %s | Storage: %s | Audio: %s\n",
           DEVICE_PROFILE.gps,
           DEVICE_PROFILE.storage,
           DEVICE_PROFILE.audio);

    if (xTaskCreate(storage_task, "storage", 512U, &app, 1U, NULL) != pdPASS ||
        xTaskCreate(gps_task, "gps", 512U, &app, 1U, NULL) != pdPASS ||
        xTaskCreate(navigation_task, "navigation", 512U, &app, 1U, NULL) != pdPASS ||
        xTaskCreate(audio_task, "audio", 512U, &app, 1U, NULL) != pdPASS ||
        xTaskCreate(battery_task, "battery", 512U, &app, 1U, NULL) != pdPASS)
    {
        fprintf(stderr, "Failed to create tasks.\n");
        destroy_app_context(&app);
        return 1;
    }

    vTaskStartScheduler();

    destroy_app_context(&app);
    return 0;
}
