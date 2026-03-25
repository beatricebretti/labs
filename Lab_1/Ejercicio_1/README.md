# Ejercicio 1

This project is a modified version of the ESP-IDF `hello_world` example. It has been customized to print the user's name to the serial monitor.

## How to Run

To build and run this project, ensure your environment is set up and run:

```bash
# Build, flash to the MCU, and open the monitor
source ~/.zprofile && source ~/.zshrc && get_esp32 && idf.py build && idf.py flash -p /dev/<PORT> monitor
