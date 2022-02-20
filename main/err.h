#ifndef ERR_H
#define ERR_H

#include "esp_err.h"
#include "esp_log.h"

#define CHECK_ESP_OK(r)                              \
    {                                                \
        esp_err_t e = (r);                           \
        if (e != ESP_OK)                             \
        {                                            \
            ESP_LOGI(TAG, "%s", esp_err_to_name(e)); \
            return false;                            \
        }                                            \
    }

#endif // ERR_H
