idf_component_register(
    SRCS "masterboard_main.c"
    INCLUDE_DIRS "."
    REQUIRES freertos esp_driver_gpio esp_timer direct_wifi direct_ethernet quad_crc spi_manager uart_imu ws2812b_led
)
