idf_component_register(
    SRCS
        "main.cc"
        "main_functions.cc"
        "motor_control.c"
        "web_server.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_server esp_driver_gpio esp_driver_ledc esp_timer
)
