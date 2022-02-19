#include "bme.h"

#include <assert.h>
#include <stddef.h>

#include "i2c.h"

#define BME_ADDR 0x76

static bool
read_val_20u(const uint8_t *const addrs,
             size_t               n_addrs,
             int32_t *            val)
{
    assert(n_addrs == 3);

    for (size_t i = 0; i < n_addrs; ++i)
    {
        uint8_t reg_val = 0;
        if (!i2c_read_reg(BME_ADDR, addrs[i], &reg_val))
        {
            return false;
        }
        if (i == (n_addrs - 1))
        {
            reg_val &= 0xF0;
            reg_val >>= 4;
            *val <<= 4;
        }
        else
        {
            *val <<= 8;
        }
        *val |= reg_val;
    }

    return true;
}

static bool
read_val_16u(const uint8_t *const addrs,
             size_t               n_addrs,
             int32_t *            val)
{
    assert(n_addrs == 2);

    for (size_t i = 0; i < n_addrs; ++i)
    {
        uint8_t reg_val = 0;
        if (!i2c_read_reg(BME_ADDR, addrs[i], &reg_val))
        {
            return false;
        }
        *val <<= 8;
        *val |= reg_val;
    }

    return true;
}

static bool
read_val_16s(const uint8_t *const addrs,
             size_t               n_addrs,
             int32_t *            val)
{
    if (!read_val_16u(addrs, n_addrs, val))
    {
        return false;
    }
    *val = (int32_t)(*(int16_t *)val);
    return val;
}

static bool
read_val_8u(uint8_t addr, int32_t *val)
{
    uint8_t reg_val = 0;
    if (!i2c_read_reg(BME_ADDR, addr, &reg_val))
    {
        return false;
    }
    *val = reg_val;
    return true;
}

static bool
read_val_8s(uint8_t addr, int32_t *val)
{
    if (!read_val_8u(addr, val))
    {
        return false;
    }
    *val = (int32_t)(*(int8_t *)val);
    return val;
}

bool
bme_configure()
{
    // Enable humidity measurements, with oversampling x16
    if (!i2c_write_reg(BME_ADDR, 0xF2, 0x5))
    {
        return false;
    }
    // Activate polled sampling mode, with temp and pressure measurements
    // enabled, with oversampling x16
    return i2c_write_reg(BME_ADDR, 0xF4, 0xB5);
}

bool
bme_get_temp(float *t_degC, int32_t *t_fine)
{
    int32_t       adc_T         = 0;
    const uint8_t adc_T_addrs[] = { 0xFA, 0xFB, 0xFC };
    if (!read_val_20u(adc_T_addrs,
                      sizeof(adc_T_addrs) / sizeof(*adc_T_addrs),
                      &adc_T))
    {
        return false;
    }

    int32_t       dig_T1         = 0;
    const uint8_t dig_T1_addrs[] = { 0x89, 0x88 };
    if (!read_val_16u(dig_T1_addrs,
                      sizeof(dig_T1_addrs) / sizeof(*dig_T1_addrs),
                      &dig_T1))
    {
        return false;
    }

    int32_t       dig_T2         = 0;
    const uint8_t dig_T2_addrs[] = { 0x8B, 0x8A };
    if (!read_val_16s(dig_T2_addrs,
                      sizeof(dig_T2_addrs) / sizeof(*dig_T2_addrs),
                      &dig_T2))
    {
        return false;
    }

    int32_t       dig_T3         = 0;
    const uint8_t dig_T3_addrs[] = { 0x8D, 0x8C };
    if (!read_val_16s(dig_T3_addrs,
                      sizeof(dig_T3_addrs) / sizeof(*dig_T3_addrs),
                      &dig_T3))
    {
        return false;
    }

    const int32_t var1 = (((adc_T >> 3) - (dig_T1 << 1)) * dig_T2) >> 11;
    const int32_t var2
        = (((((adc_T >> 4) - dig_T1) * ((adc_T >> 4) - dig_T1)) >> 12) * dig_T3)
          >> 14;
    *t_fine = var1 + var2;
    *t_degC = (float)((*t_fine * 5 + 128) >> 8) / 100.0f;

    return true;
}

bool
bme_get_pressure(int32_t t_fine, float *p_kPa)
{
    int32_t       adc_P         = 0;
    const uint8_t adc_P_addrs[] = { 0xF7, 0xF8, 0xF9 };
    if (!read_val_20u(adc_P_addrs,
                      sizeof(adc_P_addrs) / sizeof(*adc_P_addrs),
                      &adc_P))
    {
        return false;
    }

    int64_t       dig_P1         = 0;
    const uint8_t dig_P1_addrs[] = { 0x8F, 0x8E };
    if (!read_val_16u(dig_P1_addrs,
                      sizeof(dig_P1_addrs) / sizeof(*dig_P1_addrs),
                      (int32_t *)&dig_P1))
    {
        return false;
    }

    int32_t       dig_P[8] = { 0 };
    const uint8_t dig_P_addrs[8][2]
        = { { 0x91, 0x90 }, { 0x93, 0x92 }, { 0x95, 0x94 }, { 0x97, 0x96 },
            { 0x99, 0x98 }, { 0x9B, 0x9A }, { 0x9D, 0x9C }, { 0x9F, 0x9E } };
    for (size_t i = 0; i < sizeof(dig_P_addrs) / sizeof(*dig_P_addrs); ++i)
    {
        if (!read_val_16s(dig_P_addrs[i],
                          sizeof(dig_P_addrs[i]) / sizeof(*dig_P_addrs[i]),
                          &dig_P[i]))
        {
            return false;
        }
    }

    const int64_t dig_P2 = dig_P[0];
    const int64_t dig_P3 = dig_P[1];
    const int64_t dig_P4 = dig_P[2];
    const int64_t dig_P5 = dig_P[3];
    const int64_t dig_P6 = dig_P[4];
    const int64_t dig_P7 = dig_P[5];
    const int64_t dig_P8 = dig_P[6];
    const int64_t dig_P9 = dig_P[7];

    int64_t var1;
    int64_t var2;
    int64_t p;
    var1 = (t_fine)-128000;
    var2 = var1 * var1 * dig_P6;
    var2 = var2 + ((var1 * dig_P5) << 17);
    var2 = var2 + ((dig_P4) << 35);
    var1 = ((var1 * var1 * dig_P3) >> 8) + ((var1 * dig_P2) << 12);
    var1 = ((((INT64_C(1)) << 47) + var1)) * (dig_P1) >> 33;
    p    = 1048576 - adc_P;
    if (var1 == 0)
    {
        return false;
    }
    p    = (((p << 31) - var2) * 3125) / var1;
    var1 = ((dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((dig_P8)*p) >> 19;
    p    = ((p + var1 + var2) >> 8) + ((dig_P7) << 4);

    *p_kPa = p / 256000.0f;

    return true;
}

bool
bme_get_humidity(int32_t t_fine, float *humidity_pcnt)
{
    int32_t       adc_H         = 0;
    const uint8_t adc_H_addrs[] = { 0xFD, 0xFE };
    if (!read_val_16u(adc_H_addrs,
                      sizeof(adc_H_addrs) / sizeof(*adc_H_addrs),
                      &adc_H))
    {
        return false;
    }

    int32_t dig_H1 = 0;
    if (!read_val_8u(0xA1, &dig_H1))
    {
        return false;
    }

    int32_t dig_H3 = 0;
    if (!read_val_8u(0xE3, &dig_H3))
    {
        return false;
    }

    int32_t dig_H6 = 0;
    if (!read_val_8s(0xE7, &dig_H6))
    {
        return false;
    }

    int32_t       dig_H2         = 0;
    const uint8_t dig_H2_addrs[] = { 0xE2, 0xE1 };
    if (!read_val_16s(dig_H2_addrs,
                      sizeof(dig_H2_addrs) / sizeof(*dig_H2_addrs),
                      &dig_H2))
    {
        return false;
    }

    int32_t dig_H4 = 0;
    {
        uint8_t reg_val = 0;
        if (!i2c_read_reg(BME_ADDR, 0xE4, &reg_val))
        {
            return false;
        }
        dig_H4 |= reg_val;
        dig_H4 <<= 4;
        if (!i2c_read_reg(BME_ADDR, 0xE5, &reg_val))
        {
            return false;
        }
        dig_H4 |= (reg_val | 0xF);
        dig_H4 = (int32_t)(*(int16_t *)&dig_H4);
    }

    int32_t dig_H5 = 0;
    {
        uint8_t reg_val = 0;
        if (!i2c_read_reg(BME_ADDR, 0xE6, &reg_val))
        {
            return false;
        }
        dig_H5 |= reg_val;
        dig_H5 <<= 4;
        if (!i2c_read_reg(BME_ADDR, 0xE5, &reg_val))
        {
            return false;
        }
        dig_H5 |= ((reg_val | 0xF0) >> 4);
        dig_H5 = (int32_t)(*(int16_t *)&dig_H5);
    }

    int32_t v_x1_u32r = t_fine - INT32_C(76800);
    v_x1_u32r = ((((adc_H << 14) - (dig_H4 << 20) - (dig_H5 * v_x1_u32r))
                  + INT64_C(16384))
                 >> 15)
                * (((((((v_x1_u32r * dig_H6) >> 10)
                       * (((v_x1_u32r * dig_H3) >> 11) + INT32_C(32768)))
                      >> 10)
                     + INT32_C(2097152))
                        * INT32_C(dig_H2)
                    + 8192)
                   >> 14);
    v_x1_u32r
        = v_x1_u32r
          - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * INT32_C(dig_H1))
             >> 4);
    v_x1_u32r      = v_x1_u32r < 0 ? 0 : v_x1_u32r;
    v_x1_u32r      = v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r;
    *humidity_pcnt = (UINT32_C(v_x1_u32r >> 12)) / 1024.0f;

    return true;
}
