idf_component_register(
    SRCS "direct_ethernet.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_event log esp_driver_gpio esp_eth esp_netif lan87xx
    PRIV_REQUIRES lwip
)