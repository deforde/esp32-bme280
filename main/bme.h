#ifndef BME_H
#define BME_H

#include <stdbool.h>
#include <stdint.h>

bool bme_configure();

bool bme_get_temp(float *t_degC, int32_t *t_fine);

bool bme_get_pressure(int32_t t_fine, float *p_kPa);

bool bme_get_humidity(int32_t t_fine, float *humidity_pcnt);

#endif // BME_H
