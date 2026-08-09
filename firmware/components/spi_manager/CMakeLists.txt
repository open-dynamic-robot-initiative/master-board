idf_component_register(
    SRCS "spi_quad_packet.c" "spi_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_spi esp_driver_gpio soc
    PRIV_REQUIRES quad_crc esp_common
)
