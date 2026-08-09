#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "spi_manager.h"
#include "esp_attr.h"

// type definitions
typedef struct
{
    bool is_finished;
    int demux_nb;
} spi_trans_info_t;

// global variables
static spi_device_handle_t spi = NULL;

// forward declarations
void config_demux()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = GPIO_DEMUX_PIN_SEL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE};

    gpio_config(&io_conf);
}

void IRAM_ATTR spi_pre_transfer_callback(spi_transaction_t *trans)
{
    return;
}

void IRAM_ATTR spi_post_transfer_callback(spi_transaction_t *trans)
{
    ((spi_trans_info_t *)trans->user)->is_finished = true;
}

void spi_init()
{
    config_demux();

    // initialize the spi bus
    spi_bus_config_t bus_config = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_TOTAL_LEN * 2};
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_DISABLED));

    // add device
    spi_device_interface_config_t dev_config = {
        .mode = 0,
        .clock_speed_hz = SPI_MASTER_FREQ_80M / CONFIG_SPI_DATARATE_FACTOR,
        .spics_io_num = -1,
        .queue_size = 10,
        .pre_cb = spi_pre_transfer_callback,
        .post_cb = spi_post_transfer_callback,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_config, &spi));
}

bool spi_send(int slave, uint8_t *tx_data, uint8_t *rx_data, int len)
{
    // Select the CS with DEMUX
    gpio_set_level(GPIO_DEMUX_A0, slave & 0x1);
    gpio_set_level(GPIO_DEMUX_A1, (slave >> 1) & 0x1);
    gpio_set_level(GPIO_DEMUX_A2, (slave >> 2) & 0x1);

    // Low CS
    gpio_set_level(GPIO_DEMUX_OE, 0);
    usleep(1);

    // describe transaction
    spi_trans_info_t info = {
        .is_finished = false,
        .demux_nb = slave,
    };
    spi_transaction_t trans_desc;
    memset(&trans_desc, 0, sizeof(spi_transaction_t));
    trans_desc.length = 8 * len;
    trans_desc.user = &info;
    trans_desc.tx_buffer = tx_data;
    trans_desc.rx_buffer = rx_data;

    // send transaction
    esp_err_t err = spi_device_polling_transmit(spi, &trans_desc);

    // High CS
    gpio_set_level(GPIO_DEMUX_OE, 1);

    return err == ESP_OK;
}
