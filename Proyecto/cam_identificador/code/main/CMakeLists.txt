
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
    INCLUDE_DIRS ""
    EMBED_FILES
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image0"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image1"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image2"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image3"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image4"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image5"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image6"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image7"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image8"
        "../managed_components/espressif__esp-tflite-micro/examples/person_detection/static_images/sample_images/image9"
)
