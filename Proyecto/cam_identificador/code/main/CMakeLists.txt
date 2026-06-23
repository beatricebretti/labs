
#
# Main component of TF Micro project 'person_detection'.
#

idf_component_register(
    SRCS
        "detection_responder.cc"
        "image_provider.cc"
        "main.cc"
        "main_functions.cc"
        "model_settings.cc"
        "identifier_detect_model_data.cc"
        "app_camera_esp.c"
        "esp_cli.c"

    PRIV_REQUIRES console esp32-camera esp_driver_gpio esp_driver_ledc esp_timer heap spi_flash
    INCLUDE_DIRS "")
