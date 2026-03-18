# Ejercicio 1

This project is a modified version of the ESP-IDF `hello_world` example. It has been customized to print the user's name to the serial monitor.

## How to Run

To build and run this project, ensure your environment is set up and run:

```bash
# Activate the ESP-IDF environment
get_esp32

# Build, flash to the MCU, and open the monitor
idf.py build && idf.py -p /dev/cu.usbserial-XXXX flash monitor