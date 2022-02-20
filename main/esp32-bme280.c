#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bme.h"
#include "eth.h"
#include "i2c.h"

#define WIFI_AP_SSID "Zone_H_EXT"
#define WIFI_AP_PASS "0721451834"

#define UDP_DEST_PORT         8888
#define UDP_DEST_IPV4_ADDR    "192.168.0.202"

#define MEASUREMENT_PERIOD_MS 2000

const char *TAG = "esp32-bme280";

void
app_main(void)
{
    ESP_LOGI(TAG,
             "================================= START "
             "=================================");

    eth_wifi_connect(WIFI_AP_SSID, WIFI_AP_PASS);

    int sock;
    eth_udp_init(&sock);

    if (!i2c_init())
    {
        ESP_LOGI(TAG, "I2C init error!");
        return;
    }

    char message[128] = { 0 };
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

        memset(message, 0, sizeof(message));
        const int message_len
            = snprintf(message,
                       sizeof(message),
                       "t_degC = %.2f degrees Celsius\np_kPa = %.2f "
                       "kPa\nhumidity_pcnt = %.2f %%\n",
                       t_degC,
                       p_kPa,
                       humidity_pcnt);
        eth_udp_tx(
            sock, message, message_len, UDP_DEST_IPV4_ADDR, UDP_DEST_PORT);

        vTaskDelay(MEASUREMENT_PERIOD_MS / portTICK_PERIOD_MS);
    }

    eth_udp_destroy(sock);

    ESP_LOGI(TAG,
             "================================= END "
             "=================================");
}
