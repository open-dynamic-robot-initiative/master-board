idf_component_register(SRCS "ws2812b_led.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES freertos esp_driver_rmt esp_hal_rmt)
