idf_component_register(
    SRCS "uart_imu.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos log esp_driver_uart esp_hal_uart soc
)
