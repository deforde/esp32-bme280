#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PRINT_PERIOD_MS 2000

static const char* TAG = "esp32-bme280";

void app_main(void)
{
    for (;;) {
        ESP_LOGI(TAG, "Hello, world!");
        vTaskDelay(PRINT_PERIOD_MS / portTICK_PERIOD_MS);
    }
}
