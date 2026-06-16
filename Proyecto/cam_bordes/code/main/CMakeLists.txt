idf_component_register(
    SRCS
        "app_camera_esp.c"
        "edge_detector.c"
        "main.c"
    PRIV_REQUIRES driver esp32-camera esp_driver_gpio esp_driver_ledc esp_timer
    INCLUDE_DIRS ".")
