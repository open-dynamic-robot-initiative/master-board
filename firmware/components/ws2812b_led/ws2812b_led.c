#include "ws2812b_led.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_common.h"
#include "hal/rmt_types.h"
#include "freertos/FreeRTOS.h"

// Configure these based on your project needs ********
#define LED_RMT_TX_CHANNEL RMT_CHANNEL_0
#define LED_RMT_TX_GPIO CONFIG_LED_GPIO
// ****************************************************

#define BITS_PER_LED_CMD 24
#define LED_BUFFER_ITEMS ((NUM_LEDS * BITS_PER_LED_CMD))

// These values are determined by measuring pulse timing with logic analyzer and adjusting to match datasheet.
#define T0H 14 // 0 bit high time
#define T1H 52 // 1 bit high time
#define TL 52  // low time for either bit

// This is the buffer which the hw peripheral will access while pulsing the output pin
rmt_symbol_word_t led_data_buffer[LED_BUFFER_ITEMS];

// global variables
rmt_channel_handle_t tx_channel = NULL;
rmt_encoder_handle_t rmt_encoder_handle = NULL;

static void setup_rmt_data_buffer(struct led_state new_state);

void ws2812_control_init(void)
{
  rmt_copy_encoder_config_t copy_encoder_config;
  ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &rmt_encoder_handle));

  rmt_tx_channel_config_t tx_channel_config = {
      .gpio_num = LED_RMT_TX_GPIO,
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 40000000,
      .mem_block_symbols = 3 * 64,
      .trans_queue_depth = 1,
      .flags.invert_out = false,
      .flags.with_dma = false,
  };

  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_config, &tx_channel));
  ESP_ERROR_CHECK(rmt_enable(tx_channel));
}

void ws2812_write_leds(struct led_state new_state)
{
  setup_rmt_data_buffer(new_state);

  rmt_transmit_config_t transmit_config = {
      .loop_count = 0,
      .flags.eot_level = 0,
  };
  ESP_ERROR_CHECK(rmt_transmit(tx_channel, rmt_encoder_handle, led_data_buffer, sizeof(led_data_buffer), &transmit_config));
  ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_channel, portMAX_DELAY));
}

void setup_rmt_data_buffer(struct led_state new_state)
{
  for (uint32_t led = 0; led < NUM_LEDS; led++)
  {
    uint32_t bits_to_send = new_state.leds[led];
    uint32_t mask = 1UL << (BITS_PER_LED_CMD - 1);

    for (uint32_t bit = 0; bit < BITS_PER_LED_CMD; bit++)
    {
      uint32_t bit_is_set = bits_to_send & mask;

      led_data_buffer[led * BITS_PER_LED_CMD + bit] = bit_is_set ? (rmt_symbol_word_t){
                                                                       .level0 = 1,
                                                                       .duration0 = T1H,
                                                                       .level1 = 0,
                                                                       .duration1 = TL,
                                                                   }
                                                                 : (rmt_symbol_word_t){
                                                                       .level0 = 1,
                                                                       .duration0 = T0H,
                                                                       .level1 = 0,
                                                                       .duration1 = TL,
                                                                   };

      mask >>= 1;
    }
  }
}
