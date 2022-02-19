#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bme.h"
#include "i2c.h"

#define MEASUREMENT_PERIOD_MS 2000

const char *TAG = "esp32-bme280";

void
app_main(void)
{
    ESP_LOGI(TAG,
             "================================= esp32-bme280 "
             "=================================");

    if (!i2c_init())
    {
        ESP_LOGI(TAG, "I2C init error!");
        return;
    }

    for (;;)
    {
        if (!bme_configure())
        {
            ESP_LOGI(TAG, "BME configuration error!");
            return;
        }

        float   t_degC = 0.0f;
        int32_t t_fine = 0;
        if (!bme_get_temp(&t_degC, &t_fine))
        {
            ESP_LOGI(TAG, "Temp read error!");
            return;
        }
        ESP_LOGI(TAG, "t_degC = %.2f degrees Celsius", t_degC);

        float p_kPa = 0.0f;
        if (!bme_get_pressure(t_fine, &p_kPa))
        {
            ESP_LOGI(TAG, "Pressure read error!");
            return;
        }
        ESP_LOGI(TAG, "p_kPa = %.2f kPa", p_kPa);

        float humidity_pcnt = 0.0f;
        if (!bme_get_humidity(t_fine, &humidity_pcnt))
        {
            ESP_LOGI(TAG, "Humidity read error!");
            return;
        }
        ESP_LOGI(TAG, "humidity_pcnt = %.2f %%", humidity_pcnt);

        vTaskDelay(MEASUREMENT_PERIOD_MS / portTICK_PERIOD_MS);
    }
}
