#include "i2c.h"

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#define I2C_PORT_NUM              I2C_NUM_0
#define I2C_SDA_GPIO              21
#define I2C_SCL_GPIO              22
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_FREQ_HZ        100000
#define I2C_WRITE_TIMEOUT_MS      5000
#define I2C_ACK_CHECK_EN          1
#define I2C_ACK_CHECK_DIS         0
#define I2C_ACK                   0
#define I2C_NACK                  1

extern const char *TAG;

#define CHECK_ESP_OK(r)                              \
    {                                                \
        esp_err_t e = (r);                           \
        if (e != ESP_OK)                             \
        {                                            \
            ESP_LOGI(TAG, "%s", esp_err_to_name(e)); \
            return false;                            \
        }                                            \
    }

bool
i2c_init()
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = I2C_SDA_GPIO,
        .scl_io_num       = I2C_SCL_GPIO,
        .sda_pullup_en    = GPIO_PULLUP_DISABLE,
        .scl_pullup_en    = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        .clk_flags        = 0,
    };
    CHECK_ESP_OK(i2c_driver_install(I2C_PORT_NUM,
                                    I2C_MODE_MASTER,
                                    I2C_MASTER_RX_BUF_DISABLE,
                                    I2C_MASTER_TX_BUF_DISABLE,
                                    0));
    CHECK_ESP_OK(i2c_param_config(I2C_PORT_NUM, &conf));
    return true;
}

bool
i2c_destroy()
{
    CHECK_ESP_OK(i2c_driver_delete(I2C_PORT_NUM));
    return true;
}

bool
i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t reg_val)
{
    uint8_t data[] = {
        reg_addr,
        reg_val,
    };
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    CHECK_ESP_OK(i2c_master_start(cmd));
    CHECK_ESP_OK(i2c_master_write_byte(
        cmd, (dev_addr << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN));
    CHECK_ESP_OK(i2c_master_write(cmd, data, sizeof(data), I2C_ACK_CHECK_EN));
    CHECK_ESP_OK(i2c_master_stop(cmd));
    esp_err_t ret = i2c_master_cmd_begin(
        I2C_PORT_NUM, cmd, I2C_WRITE_TIMEOUT_MS / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    CHECK_ESP_OK(ret);
    return true;
}

bool
i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_val)
{
    i2c_cmd_handle_t cmd = NULL;
    esp_err_t        ret = ESP_OK;

    cmd = i2c_cmd_link_create();
    CHECK_ESP_OK(i2c_master_start(cmd));
    CHECK_ESP_OK(i2c_master_write_byte(
        cmd, (dev_addr << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN));
    CHECK_ESP_OK(i2c_master_write_byte(cmd, reg_addr, I2C_ACK_CHECK_EN));
    CHECK_ESP_OK(i2c_master_stop(cmd));
    ret = i2c_master_cmd_begin(
        I2C_PORT_NUM, cmd, I2C_WRITE_TIMEOUT_MS / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    CHECK_ESP_OK(ret);

    cmd = i2c_cmd_link_create();
    CHECK_ESP_OK(i2c_master_start(cmd));
    CHECK_ESP_OK(i2c_master_write_byte(
        cmd, (dev_addr << 1) | I2C_MASTER_READ, I2C_ACK_CHECK_EN));
    CHECK_ESP_OK(i2c_master_read_byte(cmd, reg_val, I2C_NACK));
    CHECK_ESP_OK(i2c_master_stop(cmd));
    ret = i2c_master_cmd_begin(
        I2C_PORT_NUM, cmd, I2C_WRITE_TIMEOUT_MS / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    CHECK_ESP_OK(ret);

    return true;
}
