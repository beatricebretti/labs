idf_component_register(
    SRCS "main.c"
    PRIV_REQUIRES esp32-camera esp_driver_gpio esp_driver_ledc mbedtls
    INCLUDE_DIRS ""
)

