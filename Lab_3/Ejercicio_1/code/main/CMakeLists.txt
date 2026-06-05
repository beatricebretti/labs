
#
# Main component of TF Micro project 'micro_speech'.
#

idf_component_register(
    SRCS
        "main.cc"
        "main_functions.cc"
        "output_handler.cc"
        "manual_model.cc"
    PRIV_REQUIRES spi_flash
    INCLUDE_DIRS ""
    PRIV_REQUIRES esp_timer)
