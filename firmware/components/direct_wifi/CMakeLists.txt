idf_component_register(SRCS "direct_wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES freertos esp_event log esp_system nvs_flash esp_netif esp_wifi)
